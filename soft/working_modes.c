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

const char *enter_settings_get_name(void) {
	return PSTR("> Ustawienia");
}

unsigned char tracking_pause(void) {
	System.tracking_paused = !System.tracking_paused;
	if (System.tracking_paused)
		LEDB_ON();
	else
		LEDB_OFF();
	return MODE_NO_CHANGE;
}

#define STATE_PAUSE_TRACKING_NOTPAUSED	0
#define STATE_PAUSE_TRACKING_JUSTPAUSED	1
#define STATE_PAUSE_TRACKING_PAUSED		2
#define STATE_PAUSE_TRACKING_JUSTUNPAUSED	3

const char *pause_tracking_get_name(void) {
	static unsigned char state = STATE_PAUSE_TRACKING_NOTPAUSED;
	switch (state) {
		default:
		case STATE_PAUSE_TRACKING_NOTPAUSED:
			if (System.tracking_paused) {
				set_timer(info_display, 2000);
				state = STATE_PAUSE_TRACKING_JUSTPAUSED;
			}
			return PSTR("> Wstrzymaj rej.");
		case STATE_PAUSE_TRACKING_JUSTPAUSED:
			if (timer_expired(info_display))
				state = STATE_PAUSE_TRACKING_PAUSED;
			if (!System.tracking_paused) {
				set_timer(info_display, 2000);
				state = STATE_PAUSE_TRACKING_JUSTUNPAUSED;
			}
			return PSTR("Wstrzymano!");
		case STATE_PAUSE_TRACKING_PAUSED:
			if (!System.tracking_paused) {
				set_timer(info_display, 2000);
				state = STATE_PAUSE_TRACKING_JUSTUNPAUSED;
			}
			return PSTR("> Wznow rejestr.");
		case STATE_PAUSE_TRACKING_JUSTUNPAUSED:
			if (System.tracking_paused) {
				set_timer(info_display, 2000);
				state = STATE_PAUSE_TRACKING_JUSTPAUSED;
			}
			if (timer_expired(info_display))
				state = STATE_PAUSE_TRACKING_NOTPAUSED;
			return PSTR("Wznowiono!");
	}
}

__flash const struct main_menu_pos_s main_menu[MAIN_MENU_MAXPOS+1] = {
	{
		.func = enter_settings,
		.get_name = enter_settings_get_name,
	},
	{
		.func = tracking_pause,
		.get_name = pause_tracking_get_name,
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
			if (main_menu[mp.main_menu_pos].func)
				return main_menu[mp.main_menu_pos].func();
			break;
		case K_DOWN:
			if (mp.main_menu_pos < MAIN_MENU_MAXPOS)
				mp.main_menu_pos++;
			break;
		case K_UP:
			if (mp.main_menu_pos > 0)
				mp.main_menu_pos--;
			break;
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


void display_main_menu_item(void) {
	strcpy_P(disp.line1, PSTR("  *** MENU *** "));
	strcpy_P(disp.line2, main_menu[mp.main_menu_pos].get_name());
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

unsigned char enter_settings(void) {
	display_state(DISPLAY_STATE_SETTINGS_MENU);
	return MODE_SETTINGS_MENU;
}

