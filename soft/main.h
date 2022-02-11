#pragma once

#include <avr/interrupt.h>
#include "expander.h"

#define LOCALDIFF	+1			/* Time difference from UTC [hours] */
#define	IVT_SYNC	180			/* f_sync() interval (0:no periodic sync) [sec] */

#define	VI_LVL		4.2			/* Blackout threshold [volt] */
#define	VI_LVH		4.8			/* Recharge threshold [volt] */
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

#define LEDG_PORT		1 /* expander */
#define LEDG			_BV(4)

/* on/off macros */

#define	BEEP_ON()		{BUZZER_PORT |= BUZZER;}
#define	BEEP_OFF()		{BUZZER_PORT &= ~BUZZER;}
#define LEDR_ON()		{LEDR_PORT |= LEDR;}
#define LEDR_OFF()		{LEDR_PORT &= ~LEDR;}
#define	LEDG_ON()		expander_set_bit(LEDG_PORT, LEDG, 1)
#define	LEDG_OFF()		expander_set_bit(LEDG_PORT, LEDG, 0)
#define GPS_ON()		{GPS_DIS_PORT &= ~GPS_DIS;}
#define GPS_OFF()		{GPS_DIS_PORT |= GPS_DIS;}

#define FLAGS	GPIOR0		/* Status flags and bit definitions */
#define	F_POW	0x80		/* Power */
#define	F_GPSOK	0x08		/* GPS data valid */
#define	F_SYNC	0x04		/* Sync request */
#define	F_LVD	0x01		/* Low battery detect */

#define ERROR_NO	0
#define ERROR_I2C	1
#define ERROR_I2C_TIMEOUT	2

#define ms(x) (x/10)

struct timers {
	unsigned int owire;
	unsigned int beep;
	unsigned int recv_timeout;
};

struct system_s {
	struct timers timers;
	unsigned int global_error;
};

extern volatile struct system_s System;

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
