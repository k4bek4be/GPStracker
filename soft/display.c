#include <avr/pgmspace.h>
#include "main.h"
#include "display.h"
#include "HD44780-I2C.h"
#include "xprintf.h"

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


void display_refresh(unsigned char newstate) {
	static const char *line1, *line2;
	unsigned char changed = 0;
	
	switch (System.display_state) {
		case DISPLAY_STATE_STARTUP:
			line1 = PSTR("Uruchamianie...");
			break;
		case DISPLAY_STATE_POWEROFF_LOWBAT:
			line2 = PSTR("Bateria slaba!");
			/* fall through */
		case DISPLAY_STATE_POWEROFF:
			line1 = PSTR("Wylaczanie...");
			break;
		case DISPLAY_STATE_START_MESSAGE:
			line1 = PSTR("Start");
			switch(System.status){
				case STATUS_NO_POWER: case STATUS_OK: case STATUS_NO_GPS: line2 = NULL; break;
				case STATUS_NO_DISK:			line2 = PSTR("Brak karty!"); break;
				case STATUS_DISK_ERROR:			line2 = PSTR("Blad karty!"); break;
				case STATUS_FILE_WRITE_ERROR:	line2 = PSTR("Blad zapisu!"); break;
				case STATUS_FILE_SYNC_ERROR:	line2 = PSTR("Blad zapisu FAT!"); break;
				case STATUS_FILE_CLOSE_ERROR:	line2 = PSTR("Blad zamk.pliku!"); break;
				case STATUS_FILE_OPEN_ERROR:	line2 = PSTR("Blad otw. pliku!"); break;
			}
			break;
		case DISPLAY_STATE_CARD_OK:
			line1 = PSTR("Karta OK!");
			line2 = PSTR("Czekam na GPS...");
			break;
		case DISPLAY_STATE_FILE_OPEN:
			line2 = PSTR("Zapis aktywny");
			break;
		case DISPLAY_STATE_FILE_CLOSED:
			line2 = PSTR("Pliki zamkniete");
			break;
	}
	if (newstate)
		changed = 1;
	if (timer_expired(lcd)) {
		changed = 1;
		set_timer(lcd, 1000);
	}

	/* write to LCD */
	if (changed) {
		battery_state_display();
		unsigned char len;
		LCD_GoTo(0,0);
		if (line1) {
			len = strlen_P(line1);
			if (len > 15)
				xprintf(PSTR("Warning: too long line1=%d, mem addr %4x\r\n"), (int)len, (unsigned int)line1);
			LCD_WriteTextP(line1);
			while (len<15) {
				len++;
				LCD_WriteData(' ');
			}
		} else {
			for (len=0; len<15; len++)
				LCD_WriteData(' ');
		}
		LCD_GoTo(0,1);
		if (line2) {
			len = strlen_P(line2);
			if (len > 16)
				xprintf(PSTR("Warning: too long line2=%d, mem addr %4x\r\n"), (int)len, (unsigned int)line1);
			LCD_WriteTextP(line2);
			while (len<16) {
				len++;
				LCD_WriteData(' ');
			}
		} else {
			for (len=0; len<16; len++)
				LCD_WriteData(' ');
		}
		LCD_GoTo(15, 0);
		LCD_WriteData(0); /* battery symbol */
	}
}

void display_state(unsigned char newstate) {
	System.display_state = newstate;
	display_refresh(newstate);
}
