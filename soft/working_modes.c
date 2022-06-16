#include "main.h"
#include "working_modes.h"
#include "display.h"
#include "HD44780-I2C.h"
#include "xprintf.h"
#include "settings.h"
#include "nmea.h"

static signed char display_mode_index;

__flash const unsigned char main_display_modes[] = {
	DISPLAY_STATE_MAIN_DEFAULT,
	DISPLAY_STATE_COORD,
	DISPLAY_STATE_ELE_SAT,
};

__flash const struct main_menu_pos_s main_menu[] = {
	{
		.func = enter_settings,
	},
};

struct menu_params_s mp;

void change_display_mode(signed char dir) {
	display_mode_index += dir;
	if (display_mode_index < 0)
		display_mode_index = sizeof(main_display_modes) - 1;
	if (display_mode_index >= (signed char)sizeof(main_display_modes))
		display_mode_index = 0;
}

unsigned char working_mode_default(unsigned char k) {
	switch (k) {
		case K_UP:
			change_display_mode(-1);
			break;
		case K_DOWN:
			change_display_mode(1);
			break;
		case K_RIGHT:
			return MODE_MAIN_MENU;
	}
	display_state(main_display_modes[display_mode_index]);
	return MODE_NO_CHANGE;
}

unsigned char working_mode_main_menu(unsigned char k) {
	switch (k) {
		case K_LEFT:
			return MODE_DEFAULT;
		case K_RIGHT:
			return main_menu_right_press();
		case K_DOWN: /* TODO next menu item */
		case K_UP: /* TODO prev menu item */
	}
	display_state(DISPLAY_STATE_MAIN_MENU);
	return MODE_NO_CHANGE;
}

unsigned char working_mode_settings_menu(unsigned char k) {
	switch (k) {
		case K_LEFT:
			if (settings_menu[mp.settings_menu_pos].type == SETTINGS_TYPE_BACK)
				return MODE_MAIN_MENU;
			/* fall through */
		case K_RIGHT:
			switch (settings_menu[mp.settings_menu_pos].type) {
				case SETTINGS_TYPE_BOOL: settings_display_and_modify_bool(mp.settings_menu_pos, k); break;
				case SETTINGS_TYPE_U8: settings_display_and_modify_u8(mp.settings_menu_pos, k); break;
			}
			break;
		case K_DOWN:
			if (mp.settings_menu_pos < SETTINGS_MENU_MAXPOS)
				mp.settings_menu_pos++;
			break;
		case K_UP:
			if (mp.settings_menu_pos > 0)
				mp.settings_menu_pos--;
			break;
	}
	return MODE_NO_CHANGE;
}

void display_settings_menu_item(void) {
	switch (settings_menu[mp.settings_menu_pos].type) {
		case SETTINGS_TYPE_BOOL:
			settings_display_and_modify_bool(mp.settings_menu_pos, 0);
			break;
		case SETTINGS_TYPE_U8:
			settings_display_and_modify_u8(mp.settings_menu_pos, 0);
			break;
		case SETTINGS_TYPE_BACK:
			strcpy_P(disp.line1, PSTR("* Ustawienia *"));
			strcpy_P(disp.line2, settings_menu[mp.settings_menu_pos].name);
			if (HAVE_NEXT_SETTING_POSITION)
				strcat_P(disp.line2, PSTR(" \x01")); /* down arrow */
			if (HAVE_PREV_SETTING_POSITION)
				strcat_P(disp.line2, PSTR(" \x02")); /* up arrow */
			break;
	};
}

unsigned char (*__flash const working_modes[])(unsigned char) = {
	working_mode_default,
	working_mode_main_menu,
	working_mode_settings_menu,
};

void key_process(void) {
	unsigned char k = getkey();
	unsigned char newmode = working_modes[System.working_mode](k);
	if (newmode != MODE_NO_CHANGE && newmode != System.working_mode) {
		LCD_Clear();
		System.working_mode = newmode;
	}
}

unsigned char main_menu_right_press(void) {
	if (main_menu[mp.main_menu_pos].func)
		return main_menu[mp.main_menu_pos].func();
	return MODE_NO_CHANGE;
}

unsigned char enter_settings(void) {
	display_state(DISPLAY_STATE_SETTINGS_MENU);
	return MODE_SETTINGS_MENU;
}

