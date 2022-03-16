/*---------------------------------------------------------------*
 * Automotive GPS logger                       (C)ChaN, 2014     *
 * Modified                                       k4be, 2022     *
 *---------------------------------------------------------------*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "main.h"
#include "ff.h"
#include "diskio.h"
#include "uart0.h"
#include "uart1.h"
#include "xprintf.h"
#include "stime.h"
#include "ds18b20.h"
#include "I2C.h"
#include "expander.h"
#include "HD44780-I2C.h"
#include "gpx.h"


/*FUSES = {0xFF, 0x11, 0xFE};*/		/* ATmega644PA fuses: Low, High, Extended.
This is the fuse settings for this project. The fuse bits will be included
in the output hex file with program code. However some old flash programmers
cannot load the fuse bits from hex file. If it is the case, remove this line
and use these values to program the fuse bits. */

volatile struct system_s System;
FATFS Fatfs;				/* File system object for each logical drive */
FIL gps_log;				/* File object */
FIL system_log;				/* System log file */
char Line[100];				/* Line buffer */
time_t utc;					/* current time */
struct location_s location;

/*---------------------------------------------------------*/
/* 100Hz timer interrupt generated by OC1A                 */
/*---------------------------------------------------------*/

ISR(TIMER1_COMPA_vect)
{
	static WORD ivt_sync;
//	static BYTE led;
	static unsigned int power_sw;
	unsigned int *volatile ctimer;
	unsigned char i;
	
	for(i=0; i<sizeof(System.timers)/sizeof(unsigned int); i++){ // decrement every variable from timers struct unless it's already zero
		ctimer = ((unsigned int *)&System.timers) + i;
		if(*ctimer)
			(*ctimer)--;
	}

	/* Sync interval */
	if (IVT_SYNC && ++ivt_sync >= IVT_SYNC * 100) {
		ivt_sync = 0;
		FLAGS |= F_SYNC;
	}

	/* Green LED drive */
/*	if (FLAGS & F_POW) {
		if ((FLAGS & F_GPSOK) || (++led & 0x20)) {
			LEDG_ON();
		} else {
			LEDG_OFF();
		}
	} else {
		LEDG_OFF();
	}*/

	/* MMC/SD timer procedure */
	disk_timerproc();
	
	/* Switch off */
	if (POWER_SW_PRESSED()) {
		if (++power_sw == POWER_SW_TIME){
			FLAGS |= F_POWEROFF;
			power_sw++;
		}
	} else {
		power_sw = 0;
	}
}


/* Power supply monitor */
ISR(ADC_vect)
{
	WORD ad;
	static BYTE lvt;

	ad = ADC * 16;
	if (FLAGS & F_LVD) {
		if (ad > (WORD)(VI_LVH * VI_MULT) * 16) {
			FLAGS &= ~F_LVD;
			lvt = 0;
		}
	} else {
		if (ad < (WORD)(VI_LVL * VI_MULT) * 16) {
			if (++lvt >= 10)
				FLAGS |= F_LVD;
		} else {
			lvt = 0;
		}
	}
}

/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/

DWORD get_fattime (void)
{
	struct tm *stm;
	time_t localtime = utc + LOCALDIFF;
	stm = gmtime(&localtime);
    return (DWORD)(stm->tm_year - 80) << 25 |
           (DWORD)(stm->tm_mon + 1) << 21 |
           (DWORD)stm->tm_mday << 16 |
           (DWORD)stm->tm_hour << 11 |
           (DWORD)stm->tm_min << 5 |
           (DWORD)stm->tm_sec >> 1;
}



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
		if (FLAGS & (F_LVD | F_POWEROFF))
			return 0;	/* A brownout is detected */
		if (timer_expired(recv_timeout))
			return 0; /* timeout; continue the main loop */
		if (!uart0_test())
			continue;
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



/*--------------------------------------------------------------------------*/
/* Controls                                                                 */

void beep (UINT len, BYTE cnt)
{
	while (cnt--) {
		BEEP_ON();
		set_timer(beep, len);
		while(!timer_expired(beep)) {};
		BEEP_OFF();
		set_timer(beep, len);
		while(!timer_expired(beep)) {};
	}
}


/* Make ISO time string */
const char *get_iso_time(time_t time) {
	static char output[32];
	struct tm *ct = gmtime(&time);

	xsprintf(output, PSTR("%4u-%02u-%02uT%02u:%02u:%02u.000Z"), ct->tm_year+1900, ct->tm_mon+1, ct->tm_mday, ct->tm_hour, ct->tm_min, ct->tm_sec);
	return output;
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




static
BYTE gp_val2 (
	const char *db
)
{
	BYTE n, m;


	n = db[0] - '0';
	if (n >= 10)
		return 0;
	m = db[1] - '0';
	if (m >= 10)
		return 0;

	return n * 10 + m;
}

static
UINT gp_val3 (
	const char *db
)
{
	BYTE n, m, l;


	n = db[0] - '0';
	if (n >= 10)
		return 0;
	m = db[1] - '0';
	if (m >= 10)
		return 0;
	l = db[2] - '0';
	if (l >= 10)
		return 0;

	return n * 100 + m * 10 + l;
}




static
time_t gp_rmctime (	/* Get GPS status from RMC sentence */
	const char *str
)
{
	const char *p;
	struct tm tmc;
	double tmp;

	if (gp_comp(str, PSTR("$GPRMC")))
		return 0;	/* Not the RMC */

	p = gp_col(str, 1);		/* Get h:m:s */
	if (!p)
		return 0;
	tmc.tm_hour = gp_val2(p);
	tmc.tm_min = gp_val2(p+2);
	tmc.tm_sec = gp_val2(p+4);

	p = gp_col(str, 9);		/* Get y:m:d */
	if (!p)
		return 0;
	tmc.tm_mday = gp_val2(p);
	tmc.tm_mon = gp_val2(p+2) - 1;
	tmc.tm_year = gp_val2(p+4) + 100;

	utc = mktime(&tmc);				/* Check time validity */
	if (utc == -1)
		return 0;

	p = gp_col(str, 2);		/* Get status */
	if (!p || *p != 'A') {
		FLAGS &= ~F_GPSOK;
		return 0;			/* Return 0 even is time is valid (comes from module's internal RTC) */
	}

	FLAGS |= F_GPSOK;
	
	/* parse location */
	p = gp_col(str, 3);		/* latitude */
	location.lat = gp_val2(p);	/* degrees */
	p += 2;
	xatof(&p, &tmp);	/* minutes */
	tmp /= 60;			/* convert minutes to degrees */
	location.lat += tmp;

	p = gp_col(str, 4);	/* N/S */
	if (*p != 'N')
		location.lat = -location.lat;
	
	p = gp_col(str, 5);		/* longitude */
	location.lon = gp_val3(p);	/* degrees */
	p += 3;
	xatof(&p, &tmp);	/* minutes */
	tmp /= 60;			/* convert minutes to degrees */
	location.lon += tmp;

	p = gp_col(str, 6); /* E/W */
	if (*p != 'E')
		location.lon = -location.lon;

	location.time = utc;
	
	return utc;
}


#define LOG_SIZE	300

struct {
	char buf[LOG_SIZE+1];
	unsigned int len;
} logbuf;

void log_put(char c){
	UINT bw;
	uart1_put(c);
	logbuf.buf[logbuf.len++] = c;
	if (logbuf.len >= LOG_SIZE && (FLAGS & F_FILEOPEN)) {
		if(!f_write(&system_log, logbuf.buf, logbuf.len, &bw))
			logbuf.len = 0;
	}
	if (logbuf.len > LOG_SIZE) {
		logbuf.len--;
	}
}

static
void ioinit (void)
{
	POWER_ON_DDR |= POWER_ON;
	PORTA |= POWER_ON;
	
	BUZZER_DDR |= BUZZER;
	GPS_DIS_DDR |= GPS_DIS;
	LEDR_DDR |= LEDR;

	OCR1A = F_CPU/8/100-1;		/* Timer1: 100Hz interval (OC1A) */
	TCCR1B = _BV(WGM12) | _BV(CS11);
	TIMSK1 = _BV(OCIE1A);		/* Enable TC1.oca interrupt */

	ACSR = _BV(ACD);		/* Disable analog comp */

	/* ADC */
//	ADMUX = 0;
	ADMUX = 1; // FIXME only testing battery voltage
	ADCSRA = _BV(ADEN)|_BV(ADATE)|_BV(ADIE)|_BV(ADPS2)|_BV(ADPS1)|_BV(ADPS0);
	
	/* uart1 (debug) */	
	uart1_init();
	
	I2C_init();
	expander_init(0, 0x00, 0x00); /* all as outputs */
	LCD_Initialize();
	LCD_Clear();
	LCD_WriteTextP(PSTR("TEST"));

	sei();
	
	ADCSRA |= _BV(ADSC);
}


__flash const char __open_msg[] = "Open %s\r\n";

/*-----------------------------------------------------------------------*/
/* Main                                                                  */
/*-----------------------------------------------------------------------*/

int main (void)
{
	UINT bw, len;
	struct tm *ct;
	time_t tmp_utc, localtime;
	FRESULT res;
	unsigned char prev_status;

	ioinit();
	xdev_out(log_put);
	
	xputs_P(PSTR("STARTUP\r\n"));
	
	for (;;) {
		if (FLAGS & F_POWEROFF) {
			xputs_P(PSTR("POWEROFF\r\n"));
			POWEROFF();
			while (POWER_SW_PRESSED());
			_delay_ms(2000); /* wait for switch off */
			FLAGS &= ~F_POWEROFF; /* restart if power is not lost */
			POWERON();
			xputs_P(PSTR("RESTART\r\n"));
		}

		xprintf(PSTR("LOOP err=%u\r\n"), (unsigned int)System.status);
		utc = 0;
		localtime = 0;
		prev_status = System.status;
		System.status = STATUS_NO_POWER;

		beep(250, prev_status);	/* Error beep */

		/* Wait for supply voltage stabled */
		while (FLAGS & F_LVD) {};
		_delay_ms(500);
		if (FLAGS & F_LVD)
			continue;

		/* report error here ( prev_status) */

		if (disk_status(0) & STA_NODISK) {
			System.status = STATUS_NO_DISK;
			continue;
		}

		res = f_mount(&Fatfs, "", 1);
		if (res != FR_OK) {
			xprintf(PSTR("FS error %u\r\n"), res);
			System.status = STATUS_DISK_ERROR;
			continue;
		}
		System.status = STATUS_NO_GPS;
		xputs(PSTR("FS Ok\r\n"));
		beep(50, 1);				/* 1 beep */

		/* Initialize GPS receiver */
		GPS_ON();		/* GPS power on */
		FLAGS |= F_POW;
		_delay_ms(300);	/* Delay */
		uart0_init();	/* Enable UART */
//		xfprintf(uart0_put, PSTR("$PSRF106,21*0F\r\n"));	/* Send initialization command (depends on the receiver) */

		for (;;) { /* main loop */
			gettemp();
			if (!(FLAGS & F_GPSOK))
				xputs_P(PSTR("Waiting for GPS\r\n"));
			len = get_line(Line, sizeof Line);	/* Receive a line from GPS receiver */
			if (!len){
				if (FLAGS & (F_LVD | F_POWEROFF))
					break; /* brownout */
				continue;
			}

			tmp_utc = gp_rmctime(Line);
			if (tmp_utc) {
				localtime = tmp_utc + LOCALDIFF * 3600L;	/* Local time */
				LEDG_ON();
				_delay_ms(2);
				LEDG_OFF();
			}

			if (localtime && !(FLAGS & F_FILEOPEN)) {
				ct = gmtime(&localtime);
				xsprintf(Line, PSTR("%04u-%02u-%02u_%02u-%02u.LOG"), ct->tm_year+1900, ct->tm_mon + 1, ct->tm_mday, ct->tm_hour, ct->tm_min);
				xprintf(__open_msg, Line);
				if (f_open(&gps_log, Line, FA_WRITE | FA_OPEN_ALWAYS)		/* Open log file */
					|| f_lseek(&gps_log, f_size(&gps_log)) 					/* Append mode */
					|| f_write(&gps_log, "\r\n", 2, &bw))					/* Put a blank line as start marker */
				{
					System.status = STATUS_FILE_OPEN_ERROR;
					break;	/* Failed to start logging */
				}

				xsprintf(Line, PSTR("%04u-%02u-%02u_%02u-%02u-%02u.GPX"), ct->tm_year+1900, ct->tm_mon + 1, ct->tm_mday, ct->tm_hour, ct->tm_min, ct->tm_sec);
				xprintf(__open_msg, Line);
				if (f_open(&gpx_file, Line, FA_WRITE | FA_OPEN_ALWAYS)		/* Open log file */
					|| f_lseek(&gpx_file, f_size(&gpx_file)) 				/* Append mode */
					|| gpx_init(&gpx_file))									/* Put a blank line as start marker */
				{
					f_close(&gpx_file);
					System.status = STATUS_FILE_OPEN_ERROR;
					break;	/* Failed to start logging */
				}

				xsprintf(Line, PSTR("%04u-%02u-%02u_%02u-%02u-SYSTEM.LOG"), ct->tm_year+1900, ct->tm_mon + 1, ct->tm_mday, ct->tm_hour, ct->tm_min);
				xprintf(__open_msg, Line);
				if (f_open(&system_log, Line, FA_WRITE | FA_OPEN_ALWAYS)	/* Open log file */
					|| f_lseek(&system_log, f_size(&system_log)) 			/* Append mode */
					|| f_write(&system_log, "\r\n", 2, &bw))				/* Put a blank line as start marker */
				{
					f_close(&gpx_file);
					f_close(&gps_log);
					System.status = STATUS_FILE_OPEN_ERROR;
					break;	/* Failed to start logging */
				}

				FLAGS |= F_FILEOPEN;
				System.status = STATUS_OK;
				beep(50, 2);		/* Two beeps. Start logging. */
				continue;
			}
			if (FLAGS & F_FILEOPEN) {
				f_write(&gps_log, Line, len, &bw);
				if (bw != len) {
					System.status = STATUS_FILE_WRITE_ERROR;
					break;
				}
				if(tmp_utc) /* a new point */
					gpx_process_point(&location, &gpx_file);
				if (FLAGS & F_SYNC) {
					if (f_sync(&gps_log)) {
						System.status = STATUS_FILE_SYNC_ERROR;
						break;
					}
					FLAGS &= ~F_SYNC;
				}
			}
		}

		/* Stop GPS receiver */
		uart0_deinit();
		GPS_OFF();
		FLAGS &= ~F_POW;

		/* Close file */
		if (FLAGS & F_FILEOPEN) {
			if (f_close(&gps_log))
				System.status = STATUS_FILE_CLOSE_ERROR;
			if (gpx_close(&gpx_file))
				System.status = STATUS_FILE_CLOSE_ERROR;
			if (logbuf.len && !f_write(&system_log, logbuf.buf, logbuf.len, &bw)) {
				logbuf.len = 0;
			}
			if (f_close(&system_log))
				System.status = STATUS_FILE_CLOSE_ERROR;
			xputs_P(PSTR("File closed\r\n"));
		}
		FLAGS &= ~F_FILEOPEN;
		disk_ioctl(0, CTRL_POWER, 0);
		if (System.status != STATUS_FILE_CLOSE_ERROR)
			beep(500, 1);	/* Long beep on file close succeeded */
	}

}


