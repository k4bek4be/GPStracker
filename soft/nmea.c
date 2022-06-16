#include <avr/wdt.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <avr/sleep.h>
#include "nmea.h"
#include "main.h"
#include "uart0.h"
#include "uart1.h"
#include "xprintf.h"

/*----------------------------------------------------*/
/*  Get a line received from GPS module               */
/*----------------------------------------------------*/

UINT get_line (		/* 0:Brownout or timeout, >0: Number of bytes received. */
	char *buff,
	UINT sz_buf
)
{
	char c;
	UINT i = 0;

	set_timer(recv_timeout, 1000);

	for (;;) {
		wdt_reset();
		if (FLAGS & (F_LVD | F_POWEROFF))
			return 0;	/* A brownout is detected */
		if (timer_expired(recv_timeout))
			return 0; /* timeout; continue the main loop */
		if (!uart0_test()) {
			sleep();
			continue;
		}
		c = (char)uart0_get();
		uart1_put(c);
		if (i == 0 && c != '$')
			continue;	/* Find start of line */
		buff[i++] = c;
		if (c == '\n')
			break;	/* EOL */
		if (i >= sz_buf)
			i = 0;	/* Buffer overflow (abort this line) */
	}

	return i;
}

/* Compare sentence header string */
BYTE gp_comp (const char *str1, __flash const char *str2)
{
	char c;

	do {
		c = pgm_read_byte(str2++);
	} while (c && c == *str1++);
	return c;
}

#define FIELD_BUF_LEN	32

/* Get a column item */
static
const char* gp_col (	/* Returns pointer to the item (returns a NULL when not found) */
	const char* buf,	/* Pointer to the sentence */
	BYTE col			/* Column number (0 is the 1st item) */
) {
	BYTE c;
	static char field_buf[FIELD_BUF_LEN];
	unsigned char length = 0;

	while (col) {
		do {
			c = *buf++;
			if (c <= ' ') return NULL;
		} while (c != ',');
		col--;
	}
	while (*buf && *buf != ',' && length < FIELD_BUF_LEN-1) {
		field_buf[length++] = *buf++;
	}
	field_buf[length] = '\0';
	return field_buf;
}

unsigned int gp_val(const char *db, unsigned char count) {
	unsigned int out = 0;

	while (count--) {
		unsigned char n;
		n = *db - '0';
		if (n >= 10)
			return 0;
		out *= 10;
		out += n;
		db++;
	}
	return out;
}

static time_t gp_rmc_parse(const char *str) {
	const char *p;
	struct tm tmc;

	p = gp_col(str, 1);		/* Get h:m:s */
	if (!p)
		return 0;
	tmc.tm_hour = gp_val(p, 2);
	tmc.tm_min = gp_val(p+2, 2);
	tmc.tm_sec = gp_val(p+4, 2);

	p = gp_col(str, 9);		/* Get y:m:d */
	if (!p)
		return 0;
	tmc.tm_mday = gp_val(p, 2);
	tmc.tm_mon = gp_val(p+2, 2) - 1;
	tmc.tm_year = gp_val(p+4, 2) + 100;

	utc = mktime(&tmc);				/* Check time validity */
	if (utc == -1)
		return 0;

	p = gp_col(str, 2);		/* Get status */
	if (!p || *p != 'A') {
		System.location_valid = LOC_INVALID;
		FLAGS &= ~F_GPSOK;
		return 0;			/* Return 0 even is time is valid (comes from module's internal RTC) */
	}

	FLAGS |= F_GPSOK;
	
	return utc;
}

static void gp_gga_parse(const char *str) {
	const char *p;
	double tmp;

	p = gp_col(str, 7); /* satellites used */
	System.satellites_used = atoi(p);

	/* check validity */
	p = gp_col(str, 6);
	if (*p == '0') {
		System.location_valid = LOC_INVALID;
		return;
	}
	
	System.location_valid = LOC_VALID_NEW;

	/* parse location */
	p = gp_col(str, 2);		/* latitude */
	location.lat = gp_val(p, 2);	/* degrees */
	p += 2;
	xatof(&p, &tmp);	/* minutes */
	tmp /= 60;			/* convert minutes to degrees */
	location.lat += tmp;

	p = gp_col(str, 3);	/* N/S */
	if (*p != 'N')
		location.lat = -location.lat;
	
	p = gp_col(str, 4);		/* longitude */
	location.lon = gp_val(p, 3);	/* degrees */
	p += 3;
	xatof(&p, &tmp);	/* minutes */
	tmp /= 60;			/* convert minutes to degrees */
	location.lon += tmp;

	p = gp_col(str, 5); /* E/W */
	if (*p != 'E')
		location.lon = -location.lon;
	
	p = gp_col(str, 6); /* fix type */
	if (*p == '2')
		System.sbas = 1;
	else
		System.sbas = 0;
	
	p = gp_col(str, 9); /* MSL altitude */
	xatof(&p, &tmp);
	location.alt = tmp;

	location.time = utc; /* parsed from RMC */
}
/*$PMTK355*31<CR><LF>
Return $PMTK001,355,3,1,0,0*2E “$PMTK001,355,3,GLON_Enable,BEIDOU_Enable,GALILEO_Enable”
The GLONASS search mode is enabled. */
static void pmtk001_parse(const char *str) {
	const char *p;

	/* check validity */
	p = gp_col(str, 1);
	if (strcmp_P(p, PSTR("355"))) /* not the PMTK355 reply */
		return;
	p = gp_col(str, 2);
	if (*p == '0' || *p == '1') { /* invalid / unsupported */
		System.gps_only = 1;
		xputs_P(PSTR("GPS only\r\n"));
	} else {
		System.gps_only = 0;
		xputs_P(PSTR("Multi GNSS\r\n"));
	}

	gps_initialize();
}

time_t gps_parse(const char *str) {	/* Get all required data from NMEA sentences */
	if (!gp_comp(str, PSTR("$GPRMC")) || !gp_comp(str, PSTR("$GNRMC")) || !gp_comp(str, PSTR("$BDRMC")) || !gp_comp(str, PSTR("$GARMC"))) {
		return gp_rmc_parse(str);
	}
	if (!gp_comp(str, PSTR("$GPGGA")) || !gp_comp(str, PSTR("$GNGGA")) || !gp_comp(str, PSTR("$BDGGA")) || !gp_comp(str, PSTR("$GAGGA"))) {
		gp_gga_parse(str);
		return 0;
	}
	if (!System.gps_initialized && !gp_comp(str, PSTR("$PMTK011"))) {
		gps_initialize();
		return 0;
	}
	if (!gp_comp(str, PSTR("$PMTK001"))) {
		pmtk001_parse(str);
		return 0;
	}
	return 0;
}

void uart0_put_wrap(int c) {
	uart0_put((char)c);
}

void gps_initialize(void) {
	/*
	 * PMTK355: query gnss search mode (will fail if only GPS is supported)
	 * PMTK353: set gnss search mode (GPS/Galileo/Glonass/Beidou)
	 * PMTK313: enable SBAS
	 */
	if (!System.gps_initialized)
		xfprintf(uart0_put_wrap, PSTR("$PMTK355*31\r\n"));
	if (get_flag(CONFFLAG_ENABLE_SBAS))
		xfprintf(uart0_put_wrap, PSTR("$PMTK313,1*2E\r\n"));
	else
		xfprintf(uart0_put_wrap, PSTR("$PMTK313,0*2F\r\n"));
	if (!System.gps_only) {
		switch (System.conf.gnss_mode) {
			default:
			case GNSS_MODE_GPS_GLONASS_GALILEO:
				xfprintf(uart0_put_wrap, PSTR("$PMTK353,1,1,1,0,0*2A\r\n"));
				break;
			case GNSS_MODE_GPS:
				xfprintf(uart0_put_wrap, PSTR("$PMTK353,1,0,0,0,0*2A\r\n"));
				break;
			case GNSS_MODE_GPS_GALILEO:
				xfprintf(uart0_put_wrap, PSTR("$PMTK353,1,0,1,0,0*2B\r\n"));
				break;
			case GNSS_MODE_GALILEO:
				xfprintf(uart0_put_wrap, PSTR("$PMTK353,0,0,1,0,0*2A\r\n"));
				break;
			case GNSS_MODE_GPS_BEIDOU:
				xfprintf(uart0_put_wrap, PSTR("$PMTK353,1,0,0,0,1*2B\r\n"));
				break;
			case GNSS_MODE_BEIDOU:
				xfprintf(uart0_put_wrap, PSTR("$PMTK353,0,0,0,0,1*2A\r\n"));
				break;
		}
	} else {
		System.conf.gnss_mode = GNSS_MODE_GPS;
	}
	xputs_P(PSTR("GPS init sent\r\n"));
	System.gps_initialized = 1;
}

