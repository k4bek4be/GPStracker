#pragma once

#define MENU_TYPE_SETTING_BOOL	0
#define MENU_TYPE_SETTING_U8	1
#define MENU_TYPE_DISPLAY		2
#define MENU_TYPE_FUNCTION		3

#define IS_SETTING(x) (x<3)

#define MENU_DISPLAY_TYPE_DEFAULT	0	// specific for setting type if IS_SETTING(), MENU_DISPLAY_STRING otherwise
#define MENU_DISPLAY_TYPE_STRING	1
#define MENU_DISPLAY_TYPE_FUNCTION	2
#define MENU_DISPLAY_TYPE_NAME_FUNCTION	3
#define MENU_DISPLAY_TYPE_NAME_CSFUNCTION	4
#define MENU_DISPLAY_TYPE_U8_METERS	5
#define MENU_DISPLAY_TYPE_U8_SECONDS	6

#define menu_push(x) { if(__menu_num<DATA_NUM) __menu_data[__menu_num++] = x; } // stack commands
#define menu_pop() --__menu_num
#define menu_get() &__menu_data[__menu_num-1]
#define no_menu() (__menu_num == 0)

#define DATA_NUM 3 // maximum menu depth

#define HAVE_NEXT_SETTING_POSITION	0
#define HAVE_PREV_SETTING_POSITION	0

struct menu_pos {
	unsigned char type;
	unsigned char display_type;
	__flash const char *name;		// used for line 1 when MENU_DISPLAY_TYPE_STRING or MENU_DISPLAY_TYPE_NAME_(CS)FUNCTION
	union {
		__flash const char *value;	// used for line 2 when MENU_DISPLAY_TYPE_STRING
		void (* display)(void);		// used for line 2 when MENU_DISPLAY_TYPE_NAME_FUNCTION; used for both lines when MENU_DISPLAY_TYPE_FUNCTION
		__flash const char * (* csdisplay)(void);	// used for line 2 when MENU_DISPLAY_TYPE_NAME_CSFUNCTION
	};
	unsigned char index;			// index when IS_SETTING()
	void (* changed)(void);			// what to call on changed value when IS_SETTING()
	unsigned char (* func)(void);			// what to call on MENU_DISPLAY_TYPE_NAME_FUNCTION; returns true if display refresh is needed
	unsigned char allow_back;		// left arrow will return to level up
};

struct menu_struct {
	__flash const struct menu_pos *list;	// list of elements
	unsigned char num;		// count of elements
	signed int ind:6;		// current index/position
};

extern unsigned char __menu_num;
extern struct menu_struct __menu_data[DATA_NUM];
extern void (*func_enter_table[])(struct menu_pos *); // to be defined in the application

unsigned char menu(void);

