#include <avr/pgmspace.h>
#include "main.h"
#include "display.h"
#include "HD44780-I2C.h"
#include "xprintf.h"
#include "working_modes.h"

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

void disp_func_startup(__attribute__ ((unused)) unsigned char changed) {
	strcpy_P(disp.line1, PSTR("Uruchamianie..."));
}

void disp_func_poweroff(__attribute__ ((unused)) unsigned char changed) {
	strcpy_P(disp.line1, PSTR("Wylaczanie..."));
}

void disp_func_poweroff_lowbat(unsigned char changed) {
	disp_func_poweroff(changed);
	strcpy_P(disp.line2, PSTR("Bateria slaba!"));
}

void disp_func_start_message(__attribute__ ((unused)) unsigned char changed) {
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
}

__flash const char _gps_wait[] =		"Czekam na GPS...";
__flash const char _gps_ok[] =			"GPS OK!";
__flash const char _card_ok[] =			"Karta OK!";
__flash const char _logging_active[] =	"Zapis aktywny";

void disp_func_card_ok(__attribute__ ((unused)) unsigned char changed) {
	strcpy_P(disp.line1, _card_ok);
	strcpy_P(disp.line2, _gps_wait);
}

void disp_func_file_open(__attribute__ ((unused)) unsigned char changed) {
	strcpy_P(disp.line2, _logging_active);
}

void disp_func_file_closed(__attribute__ ((unused)) unsigned char changed) {
	strcpy_P(disp.line2, PSTR("Pliki zamkniete"));
}

void disp_func_main_default(__attribute__ ((unused)) unsigned char changed) {
	if (FLAGS & F_FILEOPEN)
		strcpy_P(disp.line1, _logging_active);
	else
		strcpy_P(disp.line1, _card_ok);

	if (FLAGS & F_GPSOK)
		strcpy_P(disp.line2, _gps_ok);
	else
		strcpy_P(disp.line2, _gps_wait);
}

void disp_func_coord(__attribute__ ((unused)) unsigned char changed) {
	if (System.location_valid == LOC_INVALID) {
		strcpy_P(disp.line1, PSTR("??? N/S"));
		strcpy_P(disp.line2, PSTR("??? E/W"));
		return;
	}
	xsprintf(disp.line1, PSTR("%2.6f%c"), (location.lat < 0)?(-location.lat):location.lat, (location.lat < 0)?'S':'N');
	xsprintf(disp.line2, PSTR("%3.6f%c"), (location.lon < 0)?(-location.lon):location.lon, (location.lon < 0)?'W':'E');
}

void disp_func_ele_sat(__attribute__ ((unused)) unsigned char changed) {
	if (System.location_valid == LOC_INVALID) {
		strcpy_P(disp.line1, PSTR("ele = ???"));
	} else {
		xsprintf(disp.line1, PSTR("ele = %.1fm"), location.alt);
	}
	xsprintf(disp.line2, PSTR("%2d satelit"), System.satellites_used);
	if (System.sbas)
		strcat_P(disp.line2, PSTR(", DGPS"));
}

void disp_func_main_menu(__attribute__ ((unused)) unsigned char changed) {
	strcpy_P(disp.line1, PSTR("  *** MENU *** "));
	strcpy_P(disp.line2, PSTR("> Ustawienia"));
}

void disp_func_settings_menu(__attribute__ ((unused)) unsigned char changed) {
	display_settings_menu_item();
}

void (*__flash const disp_funcs[])(unsigned char) = {
	[DISPLAY_STATE_STARTUP] = disp_func_startup,
	[DISPLAY_STATE_POWEROFF] = disp_func_poweroff,
	[DISPLAY_STATE_POWEROFF_LOWBAT] = disp_func_poweroff_lowbat,
	[DISPLAY_STATE_START_MESSAGE] = disp_func_start_message,
	[DISPLAY_STATE_CARD_OK] = disp_func_card_ok,
	[DISPLAY_STATE_FILE_OPEN] = disp_func_file_open,
	[DISPLAY_STATE_FILE_CLOSED] = disp_func_file_closed,
	[DISPLAY_STATE_MAIN_DEFAULT] = disp_func_main_default,
	[DISPLAY_STATE_COORD] = disp_func_coord,
	[DISPLAY_STATE_ELE_SAT] = disp_func_ele_sat,
	[DISPLAY_STATE_MAIN_MENU] = disp_func_main_menu,
	[DISPLAY_STATE_SETTINGS_MENU] = disp_func_settings_menu,
};

void display_refresh(unsigned char newstate) {
	unsigned char changed = 0;

	if (newstate)
		changed = 1;
	
	disp_funcs[System.display_state](changed);
	
	if (timer_expired(lcd)) {
		changed = 1;
		set_timer(lcd, 1000);
	}

	/* write to LCD */
	if (changed) {
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

void display_state(unsigned char newstate) {
	System.display_state = newstate;
	display_refresh(newstate);
}

