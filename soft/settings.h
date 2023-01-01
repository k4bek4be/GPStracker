#pragma once
#include <avr/eeprom.h>
#include "working_modes.h"

/* u8 list - max 15 */
#define CONF_U8_GNSS_MODE	0
#define CONF_U8_SKIP_POINTS	1
#define CONF_U8_AUTO_PAUSE_TIME	2
#define CONF_U8_AUTO_PAUSE_DIST	3

#define CONF_U8_LAST		3

/* flags list - max 31 */
#define CONFFLAG_DISABLE_FILTERS	0
#define CONFFLAG_ENABLE_SBAS		1
#define CONFFLAG_LOGGING_AFTER_BOOT	2
#define CONFFLAG_AUTO_PAUSE			3

#define CONFFLAG_LAST				3

/* GNSS modes */
#define GNSS_MODE_GPS_GLONASS_GALILEO	0
#define GNSS_MODE_GPS					1
#define GNSS_MODE_GPS_GALILEO			2
#define GNSS_MODE_GALILEO				3
#define GNSS_MODE_GPS_BEIDOU			4
#define GNSS_MODE_BEIDOU				5

struct config_s {
	union {
		unsigned char conf_u8[16];
		struct {
			unsigned char gnss_mode;	// 0
			unsigned char skip_points;	// 1
			unsigned char auto_pause_time;	// 2
			unsigned char auto_pause_dist;	// 3
		};
	};
	unsigned char flags[4];
};

extern const __flash unsigned char limits_max_u8[];
extern const __flash unsigned char limits_min_u8[];
extern __flash const char *gnss_names[];
extern __flash const struct menu_struct settings_menu;

unsigned char settings_load(void); /* 0 - ok, 1 - error */
void settings_store(void);
unsigned char check_config_data(void); /* 0 - ok, 1 - error */
void settings_display_and_modify_bool(unsigned char mindex, unsigned char k);
void settings_display_and_modify_u8(unsigned char mindex, unsigned char k);
unsigned char get_flag(unsigned char index);
void set_flag(unsigned char index, unsigned char val);
void settings_bool_disp_default(unsigned char val);

