#include <util/crc16.h>
#include "main.h"
#include "settings.h"
#include "display.h"
#include "xprintf.h"
#include "working_modes.h"
#include "nmea.h"

EEMEM struct config_s config_eep;
EEMEM unsigned char config_crc;

const __flash unsigned char limits_max_u8[] = {
	[CONF_U8_GNSS_MODE] = 5,
	[CONF_U8_SKIP_POINTS] = 120,
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
		if (System.conf.conf_u8[i] > limits_max_u8[i]) {
			ret = 1;
			System.conf.conf_u8[i] = 0;
		}
	}
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

void settings_display_and_modify_bool(unsigned char mindex, unsigned char k) {
	unsigned char index = settings_menu[mindex].index;
	unsigned char val = get_flag(index);
	const __flash char *name = settings_menu[mindex].name;

	if (k ==  K_LEFT || k ==  K_RIGHT) { /* change value */
		val = !val;
		set_flag(index, val);
		if (settings_menu[mindex].changed != NULL)
			settings_menu[mindex].changed();
	}

	strcpy_P(disp.line1, name);
	settings_menu[mindex].display(val);
}

void settings_display_and_modify_u8(unsigned char mindex, unsigned char k) {
	unsigned char index = settings_menu[mindex].index;
	unsigned char val = System.conf.conf_u8[index];
	const __flash char *name = settings_menu[mindex].name;

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
		if (settings_menu[mindex].changed != NULL)
			settings_menu[mindex].changed();
	}

	strcpy_P(disp.line1, name);
	settings_menu[mindex].display(val);
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

void settings_bool_disp_default(unsigned char val) {
	if (val)
		strcpy_P(disp.line2, PSTR("< Tak > "));
	else
		strcpy_P(disp.line2, PSTR("< Nie > "));
	strcat_P(disp.line2, HAVE_NEXT_SETTING_POSITION?PSTR(" \x01"):PSTR("  ")); /* down arrow */
	strcat_P(disp.line2, HAVE_PREV_SETTING_POSITION?PSTR(" \x02"):PSTR("  ")); /* up arrow */
}

void settings_u8_disp_default(unsigned char val) {
	xsprintf(disp.line2, PSTR("%d"), (int)val);
	strcat_P(disp.line2, HAVE_NEXT_SETTING_POSITION?PSTR(" \x01"):PSTR("  ")); /* down arrow */
	strcat_P(disp.line2, HAVE_PREV_SETTING_POSITION?PSTR(" \x02"):PSTR("  ")); /* up arrow */
}

void display_gnss_mode(unsigned char val) {
	strcpy_P(disp.line2, gnss_names[val]);
}

/* SETTINGS ITEMS */

__flash const char _msg_disable_filters[] = "Nie filtruj";
__flash const char _msg_enable_sbas[] = "Szukaj SBAS";
__flash const char _msg_gnss_type[] = "Rodzaj GNSS";
__flash const char _msg_skip_points[] = "Pomin punkty";
__flash const char _msg_logging_after_boot[] = "Zapis po wlacz.";
__flash const char _msg_back[] = "< Powrot";

__flash const struct settings_menu_pos_s settings_menu[SETTINGS_MENU_MAXPOS+1] = {
	{
		.type = SETTINGS_TYPE_BACK,
		.name = _msg_back,
	},
	{
		.type = SETTINGS_TYPE_BOOL,
		.name = _msg_disable_filters,
		.index = CONFFLAG_DISABLE_FILTERS,
		.display = settings_bool_disp_default,
	},
	{
		.type = SETTINGS_TYPE_U8,
		.name = _msg_skip_points,
		.index = CONF_U8_SKIP_POINTS,
		.display = settings_u8_disp_default,
	},
	{
		.type = SETTINGS_TYPE_BOOL,
		.name = _msg_enable_sbas,
		.index = CONFFLAG_ENABLE_SBAS,
		.display = settings_bool_disp_default,
		.changed = gps_initialize,
	},
	{
		.type = SETTINGS_TYPE_U8,
		.name = _msg_gnss_type,
		.index = CONF_U8_GNSS_MODE,
		.display = display_gnss_mode,
		.changed = gps_initialize,
	},
	{
		.type = SETTINGS_TYPE_BOOL,
		.name = _msg_logging_after_boot,
		.index = CONFFLAG_LOGGING_AFTER_BOOT,
		.display = settings_bool_disp_default,
	},
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

