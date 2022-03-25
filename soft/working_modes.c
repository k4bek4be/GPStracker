#include "main.h"
#include "working_modes.h"
#include "display.h"
#include "HD44780-I2C.h"
#include "xprintf.h"

__flash const unsigned char main_display_modes[] = {
	DISPLAY_STATE_MAIN_DEFAULT,
	DISPLAY_STATE_COORD,
	DISPLAY_STATE_ELE_SAT,
};

void change_display_mode(signed char dir) {
	static signed char display_mode_index;
	display_mode_index += dir;
	if (display_mode_index < 0)
		display_mode_index = sizeof(main_display_modes) - 1;
	if (display_mode_index >= (signed char)sizeof(main_display_modes))
		display_mode_index = 0;
	display_state(main_display_modes[display_mode_index]);
}

unsigned char working_mode_default(unsigned char k) {
	switch (k) {
		case K_UP:
			change_display_mode(-1);
			break;
		case K_DOWN:
			change_display_mode(1);
			break;
	}
	return MODE_NO_CHANGE;
}

unsigned char (*__flash const working_modes[])(unsigned char) = {
	working_mode_default,
};

void key_process(void) {
//	static unsigned char mode_changed;
	unsigned char k = getkey();
	unsigned char newmode = working_modes[System.working_mode](k);
	if (newmode != MODE_NO_CHANGE && newmode != System.working_mode) {
		LCD_Clear();
		System.working_mode = newmode;
	}
}

