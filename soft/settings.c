#include <util/crc16.h>
#include "main.h"
#include "settings.h"
#include "display.h"
#include "xprintf.h"

EEMEM struct config_s config_eep;
EEMEM unsigned char config_crc;

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
	return 0;
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

void settings_display_and_modify_bool(unsigned char index, __flash const char *name, unsigned char k, unsigned char have_prev, unsigned char have_next) {
	unsigned char val = get_flag(index);

	if (k ==  K_LEFT || k ==  K_RIGHT) { /* change value */
		set_flag(index, !val);
	}

	strcpy_P(disp.line1, name);
	if (val)
		strcpy_P(disp.line2, PSTR("< Tak > "));
	else
		strcpy_P(disp.line2, PSTR("< Nie > "));
	strcat_P(disp.line2, have_next?PSTR(" \x01"):PSTR("  ")); /* down arrow */
	strcat_P(disp.line2, have_prev?PSTR(" \x02"):PSTR("  ")); /* up arrow */
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

