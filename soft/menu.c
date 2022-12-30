#include "main.h"
#include "menu.h"
#include "settings.h"
#include "display.h"
#include "xprintf.h"

unsigned char __menu_num; // Menu stack variables
struct menu_struct __menu_data[DATA_NUM];
__flash const char _NULL_STRING[] = "(NULL)";

void settings_display_bool(unsigned char index) {
	unsigned char val = get_flag(index);
	if (val)
		strcpy_P(disp.line2, PSTR("< Tak > "));
	else
		strcpy_P(disp.line2, PSTR("< Nie > "));
	strcat_P(disp.line2, HAVE_NEXT_SETTING_POSITION?PSTR(" \x01"):PSTR("  ")); /* down arrow */
	strcat_P(disp.line2, HAVE_PREV_SETTING_POSITION?PSTR(" \x02"):PSTR("  ")); /* up arrow */
}

void settings_display_u8(unsigned char index) {
	unsigned char val = System.conf.conf_u8[index];
	
	xsprintf(disp.line2, PSTR("%d"), (int)val);
	strcat_P(disp.line2, HAVE_NEXT_SETTING_POSITION?PSTR(" \x01"):PSTR("  ")); /* down arrow */
	strcat_P(disp.line2, HAVE_PREV_SETTING_POSITION?PSTR(" \x02"):PSTR("  ")); /* up arrow */
}

void settings_change_bool(struct menu_pos pos, unsigned char k) {
	unsigned char index = pos.index;
	unsigned char val = get_flag(index);

	if (k ==  K_LEFT || k ==  K_RIGHT) { /* change value */
		val = !val;
		set_flag(index, val);
		if (pos.changed != NULL)
			pos.changed();
	}
}

void settings_change_u8(struct menu_pos pos, unsigned char k) {
	unsigned char index = pos.index;
	unsigned char val = System.conf.conf_u8[index];

	if (k == K_LEFT) {
		if (val)
			val--;
	}

	if (k == K_RIGHT) {
		if (val < limits_max_u8[index])
			val++;
	}

	if (k ==  K_LEFT || k ==  K_RIGHT) {
		System.conf.conf_u8[index] = val;
		if (pos.changed != NULL)
			pos.changed();
	}
}

unsigned char menu(void) {
	struct menu_struct *curr;
	struct menu_pos pos;
	unsigned char display_line1_as_string = 0;
	unsigned char display_changed = 0;
	
	unsigned char k = getkey();
	
	curr = menu_get();
	pos = curr->list[curr->ind];

	switch (k) {
		case K_UP:
			if (curr->ind > 0) {
				curr->ind--;
				pos = curr->list[curr->ind];
				display_changed = 1;
			}
			break;
		case K_DOWN:
			if (curr->ind < curr->num-1) {
				curr->ind++;
				pos = curr->list[curr->ind];
				display_changed = 1;
			}
			break;
	}
	
	switch (pos.display_type) {
		case MENU_DISPLAY_TYPE_DEFAULT:
			if (IS_SETTING(pos.type))
				break;
		/* fall through */
		case MENU_DISPLAY_TYPE_STRING:
			display_line1_as_string = 1;
			if (pos.value) {
				strcpy_P(disp.line2, pos.value);
			} else {
				strcpy_P(disp.line2, _NULL_STRING);
			}
			break;
		case MENU_DISPLAY_TYPE_NAME_FUNCTION:
			display_line1_as_string = 1;
		/* fall through */
		case MENU_DISPLAY_TYPE_FUNCTION:
			pos.display();
			break;
		case MENU_DISPLAY_TYPE_NAME_CSFUNCTION:
			display_line1_as_string = 1;
			strcpy_P(disp.line2, pos.csdisplay());
			break;
		default:	/* bad data */
			break;
	}
	
	switch (pos.type) {
		case MENU_TYPE_SETTING_BOOL:
			if (pos.display_type == MENU_DISPLAY_TYPE_DEFAULT) {
				settings_display_bool(pos.index);
				display_line1_as_string = 1;
			}
			if (k) {
				settings_change_bool(pos, k);
				display_changed = 1;
			}
			break;
		case MENU_TYPE_SETTING_U8:
			if (pos.display_type == MENU_DISPLAY_TYPE_DEFAULT) {
				settings_display_u8(pos.index);
				display_line1_as_string = 1;
			}
			if (k) {
				settings_change_u8(pos, k);
				display_changed = 1;
			}
			break;
		case MENU_TYPE_DISPLAY:
			/* nothing special to do */
			break;
		case MENU_TYPE_FUNCTION:
			if (k == K_RIGHT) {
				display_changed = pos.func();
			}
			break;
	}
	
	if (display_line1_as_string) {
		if (pos.name) {
			strcpy_P(disp.line1, pos.name);
		} else {
			strcpy_P(disp.line1, _NULL_STRING);
		}
	}

	if (pos.allow_back && k == K_LEFT) {
		menu_pop();
		display_changed = 1;
	}
	
	if (display_changed) {
		display_refresh(1);
		set_timer(lcd, 50); /* ensure update on next iteration */
	}
	return 1;
}

