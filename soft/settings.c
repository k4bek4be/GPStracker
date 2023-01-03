#include <util/crc16.h>
#include "main.h"
#include "settings.h"
#include "display.h"
#include "xprintf.h"
#include "working_modes.h"
#include "nmea.h"
#include "menu.h"

EEMEM struct config_s config_eep;
EEMEM unsigned char config_crc;

const __flash unsigned char limits_max_u8[] = {
	[CONF_U8_GNSS_MODE] = GNSS_MODE_BEIDOU,
	[CONF_U8_SKIP_POINTS] = 120,
	[CONF_U8_AUTO_PAUSE_TIME] = 120,
	[CONF_U8_AUTO_PAUSE_DIST] = 100,
	[CONF_U8_MIN_SATS] = 12,
	[CONF_U8_AUTO_PAUSE_SPEED] = 20,
};

const __flash unsigned char limits_min_u8[] = {
	[CONF_U8_GNSS_MODE] = GNSS_MODE_GPS_GLONASS_GALILEO,
	[CONF_U8_SKIP_POINTS] = 0,
	[CONF_U8_AUTO_PAUSE_TIME] = 10,
	[CONF_U8_AUTO_PAUSE_DIST] = 2,
	[CONF_U8_MIN_SATS] = 4,
	[CONF_U8_AUTO_PAUSE_SPEED] = 0,
};

const __flash unsigned char defaults_u8[] = {
	[CONF_U8_GNSS_MODE] = GNSS_MODE_GPS_GLONASS_GALILEO,
	[CONF_U8_SKIP_POINTS] = 15,
	[CONF_U8_AUTO_PAUSE_TIME] = 30,
	[CONF_U8_AUTO_PAUSE_DIST] = 10,
	[CONF_U8_MIN_SATS] = 5,
	[CONF_U8_AUTO_PAUSE_SPEED] = 3,
};

unsigned char settings_load(void) { /* 0 - ok, 1 - error */
	unsigned char crc=0, rcrc, i;
	unsigned char *cptr = (unsigned char *)&System.conf;
	unsigned char ret;
	eeprom_read_block(cptr, &config_eep, sizeof(struct config_s));
	for (i=0; i<sizeof(struct config_s); i++) {
		crc = _crc_ibutton_update(crc, cptr[i]);
	}
	rcrc = eeprom_read_byte(&config_crc);
	crc = _crc_ibutton_update(crc, rcrc);
	ret = check_config_data();
	if (crc) {
		xputs_P(PSTR("EEPROM read: bad CRC\r\n"));
	} else if (ret) {
		xputs_P(PSTR("EEPROM read: bad data\r\n"));
	} else {
		xputs_P(PSTR("EEPROM read OK\r\n"));
	}
	ret = ret || crc;
	return ret;
}

unsigned char check_config_data(void) { /* 0 - ok, 1 - error */
	unsigned char i, ret=0;
	for (i=0; i<=CONF_U8_LAST; i++) {
		if (System.conf.conf_u8[i] > limits_max_u8[i] || System.conf.conf_u8[i] < limits_min_u8[i]) {
			ret = 1;
			System.conf.conf_u8[i] = defaults_u8[i];
		}
	}
	check_min_sat_limit();
	return ret;
}

void settings_store(void) {
	unsigned char i, crc=0;
	unsigned char *cptr = (unsigned char *)&System.conf;
	eeprom_update_block(cptr, &config_eep, sizeof(struct config_s));
	for (i=0; i<sizeof(struct config_s); i++) {
		crc = _crc_ibutton_update(crc, cptr[i]);
	}
	eeprom_update_byte(&config_crc, crc);
	xputs_P(PSTR("EEPROM write done\r\n"));
}

unsigned char get_flag(unsigned char index) {
	volatile unsigned char *sptr = &System.conf.flags[index/8];
	index %= 8;
	unsigned char val = (*sptr) & _BV(index);
	return val;
}

void set_flag(unsigned char index, unsigned char val) {
	volatile unsigned char *sptr = &System.conf.flags[index/8];
	index %= 8;
	if (val)
		*sptr |= _BV(index);
	else
		*sptr &= ~_BV(index);
}

void display_gnss_mode(unsigned char val) {
	strcpy_P(disp.line2, gnss_names[val]);
}

void display_current_gnss_mode(void) {
	display_gnss_mode(System.conf.gnss_mode);
}

/* SETTINGS ITEMS */

__flash const char _msg_disable_filters[] = "Nie filtruj";
__flash const char _msg_enable_sbas[] = "Szukaj SBAS";
__flash const char _msg_gnss_type[] = "Rodzaj GNSS";
__flash const char _msg_skip_points[] = "Pomin punkty";
__flash const char _msg_logging_after_boot[] = "Zapis po wlacz.";
__flash const char _msg_back[] = "< Powrot";
__flash const char _msg_auto_pause[] = "Autopauza";
__flash const char _msg_auto_pause_time[] = "Autopauza czas";
__flash const char _msg_auto_pause_dist[] = "Autopauza odleg";
__flash const char _msg_auto_pause_speed[] = "Autopau. predk.";
__flash const char _msg_min_sats[] = "Minimum satelit";

__flash const struct menu_pos settings_menu_list[] = {
	{
		.type = MENU_TYPE_DISPLAY,
		.display_type = MENU_DISPLAY_TYPE_STRING,
		.name = _msg_back,
		.allow_back = 1,
	},
	{
		.type = MENU_TYPE_SETTING_BOOL,
		.name = _msg_auto_pause,
		.index = CONFFLAG_AUTO_PAUSE,
	},
	{
		.type = MENU_TYPE_SETTING_U8,
		.display_type = MENU_DISPLAY_TYPE_U8_SECONDS,
		.name = _msg_auto_pause_time,
		.index = CONF_U8_AUTO_PAUSE_TIME,
	},
	{
		.type = MENU_TYPE_SETTING_U8,
		.display_type = MENU_DISPLAY_TYPE_U8_METERS,
		.name = _msg_auto_pause_dist,
		.index = CONF_U8_AUTO_PAUSE_DIST,
	},
	{
		.type = MENU_TYPE_SETTING_U8,
		.display_type = MENU_DISPLAY_TYPE_U8_KMH,
		.name = _msg_auto_pause_speed,
		.index = CONF_U8_AUTO_PAUSE_SPEED,
	},
	{
		.type = MENU_TYPE_SETTING_BOOL,
		.name = _msg_disable_filters,
		.index = CONFFLAG_DISABLE_FILTERS,
	},
	{
		.type = MENU_TYPE_SETTING_U8,
		.name = _msg_skip_points,
		.index = CONF_U8_SKIP_POINTS,
	},
	{
		.type = MENU_TYPE_SETTING_BOOL,
		.name = _msg_enable_sbas,
		.index = CONFFLAG_ENABLE_SBAS,
		.changed = gps_initialize,
	},
	{
		.type = MENU_TYPE_SETTING_U8,
		.name = _msg_min_sats,
		.index = CONF_U8_MIN_SATS,
		.changed = check_min_sat_limit,
	},
	{
		.type = MENU_TYPE_SETTING_U8,
		.display_type = MENU_DISPLAY_TYPE_NAME_FUNCTION,
		.name = _msg_gnss_type,
		.index = CONF_U8_GNSS_MODE,
		.display = display_current_gnss_mode,
		.changed = gps_initialize,
	},
	{
		.type = MENU_TYPE_SETTING_BOOL,
		.name = _msg_logging_after_boot,
		.index = CONFFLAG_LOGGING_AFTER_BOOT,
	},
};

__flash const struct menu_struct settings_menu = {
	.list = settings_menu_list,
	.num = sizeof(settings_menu_list) / sizeof(settings_menu_list[0]),
};

__flash const char gnss_gps_glonass_galileo[] = "GPS+GL.NS+GAL.EO";
__flash const char gnss_gps[] = "GPS";
__flash const char gnss_gps_galileo[] = "GPS+GALILEO";
__flash const char gnss_galileo[] = "GALILEO";
__flash const char gnss_gps_beidou[] = "GPS+BEIDOU";
__flash const char gnss_beidou[] = "BEIDOU";

__flash const char *gnss_names[] = {
	gnss_gps_glonass_galileo,
	gnss_gps,
	gnss_gps_galileo,
	gnss_galileo,
	gnss_gps_beidou,
	gnss_beidou,
};

