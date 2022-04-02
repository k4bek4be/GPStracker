#pragma once

#define __PROG_TYPES_COMPAT__
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "stime.h"
#include "expander.h"

#define	IVT_SYNC	180			/* f_sync() interval (0:no periodic sync) [sec] */
#define POWER_SW_TIME	300		/* power switch hold time to power off [10ms] */
#define BACKLIGHT_TIME	10000

//#define	VI_LVL		4.2			/* Blackout threshold [volt] */
//#define	VI_LVH		4.8			/* Recharge threshold [volt] */
#define	VI_LVL		3.1			/* Blackout threshold [volt] */
#define	VI_LVH		3.4			/* Recharge threshold [volt] */
#define	VI_MULT		(3.3 / 6.6 / 2.495 * 1024)

/* pin definitions */

#define BUZZER_PORT		PORTA
#define BUZZER			_BV(PA7)
#define BUZZER_DDR		DDRA

#define GPS_DIS_PORT	PORTC
#define GPS_DIS			_BV(PC6)
#define GPS_DIS_DDR		DDRC

#define LEDR_PORT		PORTA
#define LEDR			_BV(PA6)
#define LEDR_DDR		DDRA

#define SD_CS_PORT		PORTB
#define SD_CS			_BV(PB4)
#define SD_CS_DDR		DDRB

#define SD_PWROFF_PORT	PORTB
#define SD_PWROFF		_BV(PB3)
#define SD_PWROFF_DDR	DDRB
#define SD_PWROFF_PIN	PINB

#define SD_CD			_BV(PB1)
#define SD_CD_PIN		PINB

#define SD_WP			_BV(PB2)
#define SD_WP_PIN		PINB

#define POWER_ON		_BV(PA3)
#define POWER_ON_PORT	PORTA
#define POWER_ON_DDR	DDRA

#define POWER_SW		_BV(PC7)
#define POWER_SW_PIN	PINC

#define LEDG_PORT		1 /* expander */
#define LEDG			_BV(4)

#define LEDB_PORT		1 /* expander */
#define LEDB			_BV(5)

#define LEDW_PORT		1 /* expander */
#define LEDW			_BV(7)

/* on/off macros */

#define	BEEP_ON()		{BUZZER_PORT |= BUZZER;}
#define	BEEP_OFF()		{BUZZER_PORT &= ~BUZZER;}
#define LEDR_ON()		{LEDR_PORT |= LEDR;}
#define LEDR_OFF()		{LEDR_PORT &= ~LEDR;}
#define	LEDG_ON()		expander_set_bit(LEDG_PORT, LEDG, 1)
#define	LEDG_OFF()		expander_set_bit(LEDG_PORT, LEDG, 0)
#define	LEDB_ON()		expander_set_bit(LEDB_PORT, LEDB, 1)
#define	LEDB_OFF()		expander_set_bit(LEDB_PORT, LEDB, 0)
#define	LEDW_ON()		expander_set_bit(LEDW_PORT, LEDW, 1)
#define	LEDW_OFF()		expander_set_bit(LEDW_PORT, LEDW, 0)
#define GPS_ON()		{GPS_DIS_PORT &= ~GPS_DIS;}
#define GPS_OFF()		{GPS_DIS_PORT |= GPS_DIS;}
#define POWEROFF()		{POWER_ON_PORT &= ~POWER_ON;}
#define POWERON()		{POWER_ON_PORT |= POWER_ON;}

#define POWER_SW_PRESSED()	(POWER_SW_PIN & POWER_SW)

#define FLAGS	GPIOR0		/* Status flags and bit definitions */
#define	F_POW	0x80		/* Power */
#define F_POWEROFF	0x10	/* In process of switching off */
#define	F_GPSOK	0x08		/* GPS data valid */
#define	F_SYNC	0x04		/* Sync request */
#define F_FILEOPEN	0x02	/* File is open, logging in progress */
#define	F_LVD	0x01		/* Low battery detect */

/* System.global_error vals */
#define ERROR_NO	0
#define ERROR_I2C	1
#define ERROR_I2C_TIMEOUT	2

/* System.status vals */
#define STATUS_NO_POWER	0
#define STATUS_NO_DISK	1
#define STATUS_NO_GPS	2
#define STATUS_OK		3
#define STATUS_DISK_ERROR	4
#define STATUS_FILE_WRITE_ERROR	5
#define STATUS_FILE_SYNC_ERROR	6
#define STATUS_FILE_CLOSE_ERROR	7
#define STATUS_FILE_OPEN_ERROR	8

/* Keyboard */
#define K_UP	_BV(1)
#define K_DOWN	_BV(3)
#define K_LEFT	_BV(2)
#define K_RIGHT	_BV(0)
#define K_POWER	_BV(4)

/* System.location_valid values */
#define LOC_INVALID	0
#define LOC_VALID	1
#define LOC_VALID_NEW	2

#define ms(x) (x/10)

struct timers {
	unsigned int owire;
	unsigned int beep;
	unsigned int recv_timeout;
	unsigned int system_log;
	unsigned int lcd;
	unsigned int backlight;
};

struct system_s {
	struct timers timers;
	unsigned int global_error;
	unsigned char status;
	float bat_volt;
	float temperature;
	unsigned char temperature_ok;
	unsigned char satellites_used;
	unsigned char display_state;
	unsigned char location_valid;
	unsigned char keypress;
	unsigned char working_mode;
};

struct location_s {
	float lon;
	float lat;
	float alt;
	time_t time;
};

extern volatile struct system_s System;
extern struct location_s location;
extern time_t utc;

static inline void atomic_set_uint(volatile unsigned int *volatile data, unsigned int value) __attribute__((always_inline));
static inline void atomic_set_uint(volatile unsigned int *volatile data, unsigned int value){
	cli();
	*data = value;
	sei();
}
static inline unsigned int atomic_get_uint(volatile unsigned int *volatile data) __attribute__((always_inline));
static inline unsigned int atomic_get_uint(volatile unsigned int *volatile data){
	unsigned int ret;
	cli();
	ret = *data;
	sei();
	return ret;
}

#define set_timer(timer,val) set_timer_counts(timer, ms(val))
#define set_timer_counts(timer,val) atomic_set_uint(&System.timers.timer, val)
#define timer_expired(timer) !atomic_get_uint(&System.timers.timer)


void disk_timerproc (void); /* mmc.h */
char *get_iso_time(time_t time, unsigned char local);
void close_files(unsigned char flush_logs);
unsigned char getkey(void);

