#pragma once
#include <avr/eeprom.h>

struct config_s {
	union {
		unsigned char conf_u8[16];
		struct {
			unsigned char __empty1;
			unsigned char __empty2;
		} u8;
	};
	unsigned char flags[4];
};

/* flags list */
#define CONFFLAG_DISABLE_FILTERS	0

unsigned char settings_load(void); /* 0 - ok, 1 - error */
void settings_store(void);
unsigned char check_config_data(void); /* 0 - ok, 1 - error */
void settings_display_and_modify_bool(unsigned char index, __flash const char *name, unsigned char k, unsigned char have_prev, unsigned char have_next);
unsigned char get_flag(unsigned char index);
void set_flag(unsigned char index, unsigned char val);

