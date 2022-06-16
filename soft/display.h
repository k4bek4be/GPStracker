#pragma once

#define DISPLAY_STATE_NO_CHANGE	0
#define DISPLAY_STATE_STARTUP	1
#define DISPLAY_STATE_POWEROFF	2
#define DISPLAY_STATE_POWEROFF_LOWBAT	3
#define DISPLAY_STATE_START_MESSAGE	4
#define DISPLAY_STATE_CARD_OK	5
#define DISPLAY_STATE_FILE_OPEN	6
#define DISPLAY_STATE_FILE_CLOSED	7
#define DISPLAY_STATE_MAIN_DEFAULT	8
#define DISPLAY_STATE_COORD	9
#define DISPLAY_STATE_ELE_SAT	10
#define DISPLAY_STATE_MAIN_MENU	11
#define DISPLAY_STATE_SETTINGS_MENU	12

struct disp_s {
	char line1[16];
	char line2[17];
};

extern struct disp_s disp;

void disp_init(void);
void display_refresh(unsigned char newstate);
void display_state(unsigned char newstate);

