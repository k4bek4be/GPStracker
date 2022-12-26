#pragma once

#define STATE_PAUSE_TRACKING_NOTPAUSED	0
#define STATE_PAUSE_TRACKING_JUSTPAUSED	1
#define STATE_PAUSE_TRACKING_PAUSED		2
#define STATE_PAUSE_TRACKING_JUSTUNPAUSED	3

#define STATE_POINT_SAVE_READY	0
#define STATE_POINT_SAVE_NOT_DONE	1
#define STATE_POINT_SAVE_DONE	2

extern __flash const struct menu_struct default_menu;

struct menu_params_s {
	unsigned char point_save_state;
};

extern struct menu_params_s mp;


void key_process(void);
void enter_settings(void);
void enter_main_menu(void);

void tracking_pause(void);

