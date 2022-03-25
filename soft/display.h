#pragma once

#define DISPLAY_STATE_NO_CHANGE	0
#define DISPLAY_STATE_STARTUP	1
#define DISPLAY_STATE_POWEROFF	2
#define DISPLAY_STATE_POWEROFF_LOWBAT	3
#define DISPLAY_STATE_START_MESSAGE	4
#define DISPLAY_STATE_CARD_OK	5
#define DISPLAY_STATE_FILE_OPEN	6
#define DISPLAY_STATE_FILE_CLOSED	7

void display_refresh(unsigned char newstate);
void display_state(unsigned char newstate);

