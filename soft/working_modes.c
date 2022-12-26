#include "main.h"
#include "working_modes.h"
#include "display.h"
#include "HD44780-I2C.h"
#include "xprintf.h"
#include "settings.h"
#include "nmea.h"
#include "gpx.h"
#include "menu.h"


struct menu_params_s mp;

void tracking_pause(void) {
	System.tracking_paused = !System.tracking_paused;
	if (System.tracking_paused)
		LEDB_ON();
	else
		LEDB_OFF();
}

__flash const char *pause_tracking_get_name(void) {
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

__flash const char *save_point_get_name(void) {
	switch (mp.point_save_state) {
		default:
			return PSTR("> Zapisz punkt");
		case STATE_POINT_SAVE_NOT_DONE:
			if (timer_expired(info_display))
				mp.point_save_state = STATE_POINT_SAVE_READY;
			return PSTR("Nie zapisano!");
		case STATE_POINT_SAVE_DONE:
			if (timer_expired(info_display))
				mp.point_save_state = STATE_POINT_SAVE_READY;
			return PSTR("Zapisano!");
	}
}

void save_point(void) {
	if (timer_expired(info_display)) { /* don't save too often */
		if (System.location_valid) {
			gpx_save_single_point(&location);
			mp.point_save_state = STATE_POINT_SAVE_DONE;
		} else {
			mp.point_save_state = STATE_POINT_SAVE_NOT_DONE;
		}
	}
	set_timer(info_display, 2000);
}

void new_file(void) {
	System.open_new_file = 1;
}

__flash const char _menu_header[] = "  *** MENU *** ";
__flash const char _settings[] = "> Ustawienia";
__flash const char _new_file[] = "> Nowy plik";

__flash const struct menu_pos main_menu_list[] = {
	{
		.type = MENU_TYPE_FUNCTION,
		.display_type = MENU_DISPLAY_TYPE_NAME_CSFUNCTION,
		.name = _menu_header,
		.csdisplay = save_point_get_name,
		.func = save_point,
		.allow_back = 1,
	},
	{
		.type = MENU_TYPE_FUNCTION,
		.display_type = MENU_DISPLAY_TYPE_STRING,
		.name = _menu_header,
		.value = _settings,
		.func = enter_settings,
		.allow_back = 1,
	},
	{
		.type = MENU_TYPE_FUNCTION,
		.display_type = MENU_DISPLAY_TYPE_NAME_CSFUNCTION,
		.name = _menu_header,
		.csdisplay = pause_tracking_get_name,
		.func = tracking_pause,
		.allow_back = 1,
	},
	{
		.type = MENU_TYPE_FUNCTION,
		.display_type = MENU_DISPLAY_TYPE_STRING,
		.name = _menu_header,
		.value = _new_file,
		.func = new_file,
		.allow_back = 1,
	},
};

__flash const struct menu_struct main_menu = {
	.list = main_menu_list,
	.num = sizeof(main_menu_list) / sizeof(main_menu_list[0]),
};

__flash const struct menu_pos default_menu_list[] = {
	{
		.type = MENU_TYPE_FUNCTION,
		.display_type = MENU_DISPLAY_TYPE_FUNCTION,
		.display = disp_func_main_default,
		.func = enter_main_menu,
	},
	{
		.type = MENU_TYPE_FUNCTION,
		.display_type = MENU_DISPLAY_TYPE_FUNCTION,
		.display = disp_func_coord,
		.func = enter_main_menu,
	},
	{
		.type = MENU_TYPE_FUNCTION,
		.display_type = MENU_DISPLAY_TYPE_FUNCTION,
		.display = disp_func_ele_sat,
		.func = enter_main_menu,
	},
	{
		.type = MENU_TYPE_FUNCTION,
		.display_type = MENU_DISPLAY_TYPE_FUNCTION,
		.display = disp_distance_and_time,
		.func = enter_main_menu,
	},
	{
		.type = MENU_TYPE_FUNCTION,
		.display_type = MENU_DISPLAY_TYPE_FUNCTION,
		.display = disp_speed,
		.func = enter_main_menu,
	},
	{
		.type = MENU_TYPE_FUNCTION,
		.display_type = MENU_DISPLAY_TYPE_FUNCTION,
		.display = disp_time,
		.func = enter_main_menu,
	},
	{
		.type = MENU_TYPE_FUNCTION,
		.display_type = MENU_DISPLAY_TYPE_FUNCTION,
		.display = disp_func_temperature,
		.func = enter_main_menu,
	},
};

__flash const struct menu_struct default_menu = {
	.list = default_menu_list,
	.num = sizeof(default_menu_list) / sizeof(default_menu_list[0]),
};


void enter_settings(void) {
	xprintf(PSTR("ENTER SETTINGS MENU, %d\r\n"), (int)__menu_num);
	menu_push(settings_menu);
}

void enter_main_menu(void) {
	xprintf(PSTR("ENTER MAIN MENU, %d\r\n"), (int)__menu_num);
	menu_push(main_menu);
}

