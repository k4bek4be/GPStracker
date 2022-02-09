#pragma once

#define LOCALDIFF	+9			/* Time difference from UTC [hours] */
#define	IVT_SYNC	180			/* f_sync() interval (0:no periodic sync) [sec] */

#define	VI_LVL		9.0			/* Blackout threshold [volt] */
#define	VI_LVH		10.0		/* Recharge threshold [volt] */
#define	VI_MULT		(10.0 / 130 / 3.0 * 1024)

#define	BEEP_ON()		0
#define	BEEP_OFF()		0
#define	LEDG_ON()		PORTB &= ~_BV(0)
#define	LEDG_OFF()		PORTB |= _BV(0)
#define GPS_ON()		PORTC |= _BV(1)
#define GPS_OFF()		PORTC &= ~_BV(1)

#define	DELAY_MS(d)		{for (Timer = (BYTE)(d / 10); Timer; ) ; }

#define FLAGS	GPIOR0		/* Status flags and bit definitions */
#define	F_POW	0x80		/* Power */
#define	F_GPSOK	0x08		/* GPS data valid */
#define	F_SYNC	0x04		/* Sync request */
#define	F_LVD	0x01		/* Low battery detect */

