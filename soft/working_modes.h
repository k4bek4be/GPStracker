#pragma once

#define TRACKING_PAUSE_CMD_PAUSE	0
#define TRACKING_PAUSE_CMD_RESUME	1
#define TRACKING_PAUSE_CMD_TOGGLE	2

#define STATE_POINT_SAVE_READY	0
#define STATE_POINT_SAVE_NOT_DONE	1
#define STATE_POINT_SAVE_DONE	2

extern __flash const struct menu_struct default_menu;

void key_process(void);
unsigned char enter_settings(void);
unsigned char enter_main_menu(void);
void tracking_pause(unsigned char cmd, unsigned char display);

