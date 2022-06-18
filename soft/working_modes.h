#pragma once

#define MODE_NO_CHANGE	0xff
#define MODE_DEFAULT	0
#define MODE_MAIN_MENU	1
#define MODE_SETTINGS_MENU	2

#define MAIN_MENU_MAXPOS 2

#define STATE_PAUSE_TRACKING_NOTPAUSED	0
#define STATE_PAUSE_TRACKING_JUSTPAUSED	1
#define STATE_PAUSE_TRACKING_PAUSED		2
#define STATE_PAUSE_TRACKING_JUSTUNPAUSED	3

#define STATE_POINT_SAVE_READY	0
#define STATE_POINT_SAVE_NOT_DONE	1
#define STATE_POINT_SAVE_DONE	2

struct main_menu_pos_s {
	const char * (* get_name)(void);
	unsigned char (* func)(void);
};

struct menu_params_s {
	unsigned char main_menu_pos;
	unsigned char settings_menu_pos;
	unsigned char point_save_state;
};

extern struct menu_params_s mp;

void key_process(void);
unsigned char enter_settings(void);
void display_settings_menu_item(void);
void display_main_menu_item(void);

