#pragma once

#define DISPLAY_EVENT_STARTUP		0
#define DISPLAY_EVENT_POWEROFF		1
#define DISPLAY_EVENT_LOW_BATTERY	2
#define DISPLAY_EVENT_INITIALIZED	3
#define DISPLAY_EVENT_CARD_INITIALIZED	4
#define DISPLAY_EVENT_FILE_OPEN		5
#define DISPLAY_EVENT_FILE_CLOSED	6
#define DISPLAY_EVENT_TRACKING_PAUSED	7
#define DISPLAY_EVENT_TRACKING_RESUMED	8
#define DISPLAY_EVENT_POINT_SAVED	9
#define	DISPLAY_EVENT_POINT_NOT_SAVED	10

struct disp_s {
	char line1[16];
	char line2[17];
};

extern struct disp_s disp;

void disp_init(void);
void display_event(unsigned char event);
void display_refresh(unsigned char changed);
void disp_func_main_default(void);
void disp_func_coord(void);
void disp_func_ele_sat(void);
void disp_distance_and_time(void);
void disp_speed(void);
void disp_time(void);
void disp_func_temperature(void);

