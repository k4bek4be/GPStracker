#include <avr/pgmspace.h>
#include "main.h"
#include "display.h"
#include "HD44780-I2C.h"
#include "xprintf.h"
#include "working_modes.h"
#include "timec.h"

__flash const unsigned char battery_states[][8] = {
	{
		0b01110,
		0b11111,
		0b11111,
		0b11111,
		0b11111,
		0b11111,
		0b11111,
		0b11111,
	},
	{
		0b01110,
		0b11111,
		0b10001,
		0b11111,
		0b11111,
		0b11111,
		0b11111,
		0b11111,
	},
	{
		0b01110,
		0b11111,
		0b10001,
		0b10001,
		0b10001,
		0b11111,
		0b11111,
		0b11111,
	},
	{
		0b01110,
		0b11111,
		0b10001,
		0b10001,
		0b10001,
		0b10001,
		0b10001,
		0b11111,
	},
};

__flash const unsigned char custom_chars[] = {
	0b00000, /* 0x01 down arrow */
	0b00100,
	0b00100,
	0b00100,
	0b10101,
	0b01110,
	0b00100,
	0b00000,
	
	0b00000, /* 0x02 up arrow */
	0b00100,
	0b01110,
	0b10101,
	0b00100,
	0b00100,
	0b00100,
	0b00000,
};

struct disp_s disp;

void disp_init(void) { /* send custom characters starting with 0x01 */
	unsigned char i;
	LCD_WriteCommand(0x40 + 8); // 0x01
	for(i=0; i<sizeof(custom_chars); i++){
		LCD_WriteData(custom_chars[i]);
	};
}

void battery_state_display(void) {
	unsigned char i;
	unsigned char index;
	if (System.bat_volt > 4.0)
		index = 0;
	else if (System.bat_volt > 3.7)
		index = 1;
	else if (System.bat_volt > 3.4)
		index = 2;
	else
		index = 3;
	
	LCD_WriteCommand(0x40 + 0); // 0x00

	for(i=0; i<8; i++){
		LCD_WriteData(battery_states[index][i]);
	};
}

__flash const char _gps_wait[] =		"Czekam na GPS...";
__flash const char _gps_ok[] =			"GPS OK!";
__flash const char _card_ok[] =			"Karta OK!";
__flash const char _logging_active[] =	"Zapis aktywny";
__flash const char _logging_paused[] =	"Zapis wstrzymany";

void display_event(unsigned char event) { /* overrides display with current messages */
	switch (event) {
		case DISPLAY_EVENT_STARTUP:
			strcpy_P(disp.line1, PSTR("Uruchamianie..."));
			break;
		case DISPLAY_EVENT_LOW_BATTERY:
			strcpy_P(disp.line2, PSTR("Bateria slaba!"));
		/* fall through */
		case DISPLAY_EVENT_POWEROFF:
			strcpy_P(disp.line1, PSTR("Wylaczanie..."));
			break;
		case DISPLAY_EVENT_INITIALIZED:
			strcpy_P(disp.line1, PSTR("Start"));
			switch(System.status){
				case STATUS_NO_POWER: case STATUS_OK: case STATUS_NO_GPS: disp.line2[0] = '\0'; break;
				case STATUS_NO_DISK:			strcpy_P(disp.line2, PSTR("Brak karty!")); break;
				case STATUS_DISK_ERROR:			strcpy_P(disp.line2, PSTR("Blad karty!")); break;
				case STATUS_FILE_WRITE_ERROR:	strcpy_P(disp.line2, PSTR("Blad zapisu!")); break;
				case STATUS_FILE_SYNC_ERROR:	strcpy_P(disp.line2, PSTR("Blad zapisu FAT!")); break;
				case STATUS_FILE_CLOSE_ERROR:	strcpy_P(disp.line2, PSTR("Blad zamk.pliku!")); break;
				case STATUS_FILE_OPEN_ERROR:	strcpy_P(disp.line2, PSTR("Blad otw. pliku!")); break;
			}
			break;
		case DISPLAY_EVENT_CARD_INITIALIZED:
			strcpy_P(disp.line1, _card_ok);
			strcpy_P(disp.line2, _gps_wait);
			break;
		case DISPLAY_EVENT_FILE_CLOSED:
			strcpy_P(disp.line2, PSTR("Pliki zamkniete"));
			break;
		case DISPLAY_EVENT_FILE_OPEN:
			strcpy_P(disp.line2, PSTR("Pliki otwarte"));
			break;
	}
	display_refresh(1);
}

void disp_func_main_default(void) {
	if (FLAGS & F_FILEOPEN) {
		if (System.tracking_paused)
			strcpy_P(disp.line1, _logging_paused);
		else
			strcpy_P(disp.line1, _logging_active);
	} else
		strcpy_P(disp.line1, _card_ok);

	if (FLAGS & F_GPSOK)
		strcpy_P(disp.line2, _gps_ok);
	else
		strcpy_P(disp.line2, _gps_wait);
}

void disp_func_coord(void) {
	if (System.location_valid == LOC_INVALID) {
		strcpy_P(disp.line1, PSTR("??? N/S"));
		strcpy_P(disp.line2, PSTR("??? E/W"));
		return;
	}
	xsprintf(disp.line1, PSTR("%2.6f%c"), (location.lat < 0)?(-location.lat):location.lat, (location.lat < 0)?'S':'N');
	xsprintf(disp.line2, PSTR("%3.6f%c"), (location.lon < 0)?(-location.lon):location.lon, (location.lon < 0)?'W':'E');
}

void disp_func_ele_sat(void) {
	if (System.location_valid == LOC_INVALID) {
		strcpy_P(disp.line1, PSTR("ele = ???"));
	} else {
		xsprintf(disp.line1, PSTR("ele = %.1fm"), location.alt);
	}
	xsprintf(disp.line2, PSTR("%2d satelit"), System.satellites_used);
	if (System.sbas)
		strcat_P(disp.line2, PSTR(", DGPS"));
}

void disp_distance_and_time(void) {
	xsprintf(disp.line1, PSTR("%.2f km"), (float)System.distance / 100000.0);
	if (utc > 0 && System.time_start > 0) {
		xsprintf(disp.line2, PSTR("t=%u s"), (unsigned int)(utc - System.time_start));
	} else {
		strcpy_P(disp.line2, PSTR("Czas nieznany"));
	}
}

void disp_speed(void) {
	strcpy_P(disp.line1, PSTR("Predkosc:"));
	if (utc > 0 && System.time_start > 0) {
		xsprintf(disp.line2, PSTR("%.2f km/h"), (float)System.distance / (float)(utc - System.time_start) * 0.036);
	} else {
		strcpy_P(disp.line2, PSTR("nieznana"));
	}
}

void disp_time(void) {
	if (utc == 0) {
		strcpy_P(disp.line1, PSTR("?"));
		strcpy_P(disp.line2, PSTR("?"));
		return;
	}
	time_t time = utc;
	time += local_time_diff(time) * (signed int)3600;
	struct tm *ct = gmtime(&time);

	xsprintf(disp.line1, PSTR("%d.%02d.%4d"), ct->tm_mday, ct->tm_mon+1, ct->tm_year+1900);
	xsprintf(disp.line2, PSTR("%d:%02d:%02d"), ct->tm_hour, ct->tm_min, ct->tm_sec);
}

void disp_func_temperature(void) {
	strcpy_P(disp.line1, PSTR("Temperatura"));
	if (System.temperature_ok) {
		xsprintf(disp.line2, PSTR("%.1f stC"), System.temperature);
	} else {
		strcpy_P(disp.line2, PSTR("Blad!"));
	}
}

void display_refresh(unsigned char changed) {
	if (timer_expired(lcd)) {
		changed = 1;
	}

	/* write to LCD */
	if (changed) {
		set_timer(lcd, 1000);
		battery_state_display();
		unsigned char len;
		LCD_GoTo(0,0);
		len = strlen(disp.line1);
		LCD_WriteText(disp.line1);
		while (len<15) {
			len++;
			LCD_WriteData(' ');
		}
		LCD_WriteData(0); /* battery symbol */
		LCD_GoTo(0,1);
		len = strlen(disp.line2);
		LCD_WriteText(disp.line2);
		while (len<16) {
			len++;
			LCD_WriteData(' ');
		}
	}
}

