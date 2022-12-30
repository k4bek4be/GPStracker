#include "main.h"
#include "working_modes.h"
#include "display.h"
#include "HD44780-I2C.h"
#include "xprintf.h"
#include "settings.h"
#include "nmea.h"
#include "gpx.h"
#include "menu.h"

void tracking_pause(unsigned char cmd, unsigned char display) {
	switch (cmd) {
		case TRACKING_PAUSE_CMD_TOGGLE:
			System.tracking_paused = !System.tracking_paused;
			break;
		case TRACKING_PAUSE_CMD_RESUME:
			System.tracking_paused = 0;
			break;
		case TRACKING_PAUSE_CMD_PAUSE:
			System.tracking_paused = 1;
			break;
	}
	if (System.tracking_paused) {
		LEDB_ON();
		if (display)
			display_event(DISPLAY_EVENT_TRACKING_PAUSED);
	} else {
		LEDB_OFF();
		if (display)
			display_event(DISPLAY_EVENT_TRACKING_RESUMED);
	}
}

unsigned char tracking_pause_cmd(void) {
	tracking_pause(TRACKING_PAUSE_CMD_TOGGLE, 1);
	return 0;
}

__flash const char *pause_tracking_get_name(void) {
	if (System.tracking_paused)
		return PSTR("> Wznow rejestr.");
	else
		return PSTR("> Wstrzymaj rej.");
}

unsigned char save_point(void) {
	if (System.location_valid) {
		gpx_save_single_point(&location);
		display_event(DISPLAY_EVENT_POINT_SAVED);
	} else {
		display_event(DISPLAY_EVENT_POINT_NOT_SAVED);
	}
	return 0;
}

unsigned char new_file(void) {
	System.open_new_file = 1;
	return 0;
}

__flash const char _menu_header[] = "  *** MENU *** ";
__flash const char _settings[] = "> Ustawienia";
__flash const char _new_file[] = "> Nowy plik";
__flash const char _save_point[] = "> Zapisz punkt";

__flash const struct menu_pos main_menu_list[] = {
	{
		.type = MENU_TYPE_FUNCTION,
		.display_type = MENU_DISPLAY_TYPE_STRING,
		.name = _menu_header,
		.value = _save_point,
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
		.func = tracking_pause_cmd,
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


unsigned char enter_settings(void) {
	menu_push(settings_menu);
	return 1;
}

unsigned char enter_main_menu(void) {
	menu_push(main_menu);
	return 1;
}

