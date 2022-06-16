#pragma once
#include <avr/eeprom.h>
#include "working_modes.h"

/* u8 list - max 15 */
#define CONF_U8_GNSS_MODE	0

#define CONF_U8_LAST		0

/* flags list - max 31 */
#define CONFFLAG_DISABLE_FILTERS	0
#define CONFFLAG_ENABLE_SBAS		1

#define CONFFLAG_LAST				1

/* GNSS modes */
#define GNSS_MODE_GPS_GLONASS_GALILEO	0
#define GNSS_MODE_GPS					1
#define GNSS_MODE_GPS_GALILEO			2
#define GNSS_MODE_GALILEO				3
#define GNSS_MODE_GPS_BEIDOU			4
#define GNSS_MODE_BEIDOU				5

#define SETTINGS_TYPE_BACK	0
#define SETTINGS_TYPE_BOOL	1
#define SETTINGS_TYPE_U8	2

#define HAVE_NEXT_SETTING_POSITION (mp.settings_menu_pos < SETTINGS_MENU_MAXPOS)
#define HAVE_PREV_SETTING_POSITION (mp.settings_menu_pos > 0)

#define SETTINGS_MENU_MAXPOS	3

struct config_s {
	union {
		unsigned char conf_u8[16];
		struct {
			unsigned char gnss_mode; // 0
		};
	};
	unsigned char flags[4];
};

struct settings_menu_pos_s {
	unsigned char type;
	__flash const char *name;
	unsigned char index;
	void (* changed)(void);
	void (* display)(unsigned char);
};

extern const __flash unsigned char limits_max_u8[];
extern __flash const struct settings_menu_pos_s settings_menu[SETTINGS_MENU_MAXPOS+1];
extern __flash const char *gnss_names[];

unsigned char settings_load(void); /* 0 - ok, 1 - error */
void settings_store(void);
unsigned char check_config_data(void); /* 0 - ok, 1 - error */
void settings_display_and_modify_bool(unsigned char mindex, unsigned char k);
void settings_display_and_modify_u8(unsigned char mindex, unsigned char k);
unsigned char get_flag(unsigned char index);
void set_flag(unsigned char index, unsigned char val);
void settings_bool_disp_default(unsigned char val);

