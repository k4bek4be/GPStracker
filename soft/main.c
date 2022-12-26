/*---------------------------------------------------------------*
 * Automotive GPS logger                       (C)ChaN, 2014     *
 * Modified                                       k4be, 2022     *
 *---------------------------------------------------------------*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "ff.h"
#include "diskio.h"
#include "uart0.h"
#include "uart1.h"
#include "xprintf.h"
#include "stime.h"
#include "ds18b20.h"
#include "I2C.h"
#include "expander.h"
#include "HD44780-I2C.h"
#include "gpx.h"
#include "display.h"
#include "working_modes.h"
#include "timec.h"
#include "nmea.h"
#include "settings.h"

/*FUSES = {0xFF, 0x11, 0xFE};*/		/* ATmega644PA fuses: Low, High, Extended.
This is the fuse settings for this project. The fuse bits will be included
in the output hex file with program code. However some old flash programmers
cannot load the fuse bits from hex file. If it is the case, remove this line
and use these values to program the fuse bits. */

volatile struct system_s System;
FATFS Fatfs;				/* File system object for each logical drive */
FIL gps_log;				/* File object */
FIL system_log;				/* System log file */
char Line[100];				/* Line buffer */
time_t utc;					/* current time */
struct location_s location;

void start_bootloader(void) {
	typedef void (*do_reboot_t)(void);
	const do_reboot_t do_reboot = (do_reboot_t)((FLASHEND - 1023) >> 1);
	cli();
	uart1_deinit();
/*	close_files(0); // FIXME not working
	display_state(DISPLAY_STATE_BOOTLOADER);*/
	LCD_Clear(); /* Do not call display.c here! */
	LCD_GoTo(0,0);
	LCD_WriteTextP(PSTR("Aktualizacja"));
	LCD_GoTo(8,1);
	LCD_WriteTextP(PSTR("softu..."));
	TCCR0A = TCCR1A = TCCR2A = 0; // make sure interrupts are off and timers are reset.
	do_reboot();
	wdt_enable(WDTO_15MS);
	while(1);
}

/*---------------------------------------------------------*/
/* 100Hz timer interrupt generated by OC1A                 */
/*---------------------------------------------------------*/

ISR(TIMER1_COMPA_vect)
{
	static WORD ivt_sync;
//	static BYTE led;
	static unsigned int power_sw;
	unsigned int *volatile ctimer;
	unsigned char i;
	unsigned char k;
	static unsigned char oldk;
	
	for(i=0; i<sizeof(System.timers)/sizeof(unsigned int); i++){ // decrement every variable from timers struct unless it's already zero
		ctimer = ((unsigned int *)&System.timers) + i;
		if(*ctimer)
			(*ctimer)--;
	}

	/* Sync interval */
	if (IVT_SYNC && ++ivt_sync >= IVT_SYNC * 100) {
		ivt_sync = 0;
		FLAGS |= F_SYNC;
	}

	/* Green LED drive */
/*	if (FLAGS & F_POW) {
		if ((FLAGS & F_GPSOK) || (++led & 0x20)) {
			LEDG_ON();
		} else {
			LEDG_OFF();
		}
	} else {
		LEDG_OFF();
	}*/

	/* MMC/SD timer procedure */
	disk_timerproc();
	
	/* Switch off */
	if (POWER_SW_PRESSED()) {
		if (++power_sw == POWER_SW_TIME){
			FLAGS |= F_POWEROFF;
			power_sw++;
		}
		System.timers.backlight = ms(BACKLIGHT_TIME);
	} else {
		power_sw = 0;
	}
	
	if (uart1_test() && uart1_get() == '0' && uart1_get() == ' '){
		LEDB_ON();
		start_bootloader();
	}
	
	/* keyboard */
	k = ((~PIND) >> 4) & 0x0f;
	if (POWER_SW_PRESSED())
		k |= K_POWER;
	if (k && k != oldk) {
		System.keypress = k;
		oldk = k;
	}
	if (!k)
		oldk = 0;
}

unsigned char getkey(void) {
	unsigned char key = System.keypress;
	System.keypress = 0;
	return key;
}


/* Power supply monitor */
ISR(ADC_vect)
{
	float bat_volt;
	static unsigned int adc_buf;
	static unsigned char adc_cnt;
	static BYTE lvt;
	
	adc_buf += ADC;
	if(++adc_cnt < 15)
		return;

	bat_volt = (float)adc_buf/(float)(adc_cnt)/VI_MULT;
	adc_buf = 0;
	adc_cnt = 0;
	
	System.bat_volt = bat_volt;
	if (FLAGS & F_LVD) {
		if (bat_volt > VI_LVH) {
			FLAGS &= ~F_LVD;
			lvt = 0;
		}
	} else {
		if (bat_volt < VI_LVL) {
			if (++lvt >= 3)
				FLAGS |= F_LVD;
		} else {
			lvt = 0;
		}
	}
}

void sleep(void) {
	set_sleep_mode(SLEEP_MODE_IDLE);
	sleep_enable();
	sleep_cpu();
	sleep_disable();
}

/*--------------------------------------------------------------------------*/
/* Controls                                                                 */

void beep (UINT len, BYTE cnt)
{
	while (cnt--) {
		BEEP_ON();
		set_timer(beep, len);
		while(!timer_expired(beep)) {};
		BEEP_OFF();
		set_timer(beep, len);
		while(!timer_expired(beep)) {};
	}
}

#define LOG_SIZE	300

struct {
	char buf[LOG_SIZE+1];
	unsigned int len;
} logbuf;

void log_put(int c){
	UINT bw;
	uart1_put(c);
	logbuf.buf[logbuf.len++] = c;
	if (logbuf.len >= LOG_SIZE && (FLAGS & F_FILEOPEN)) {
		if(!f_write(&system_log, logbuf.buf, logbuf.len, &bw))
			logbuf.len = 0;
	}
	if (logbuf.len > LOG_SIZE) {
		logbuf.len--;
	}
}

static
void ioinit (void)
{
	wdt_enable(WDTO_4S);
    MCUSR = 0;
	POWER_ON_DDR |= POWER_ON;
	PORTA |= POWER_ON;
	
	BUZZER_DDR |= BUZZER;
	GPS_OFF();
	GPS_DIS_DDR |= GPS_DIS;
	LEDR_DDR |= LEDR;

	OCR1A = F_CPU/8/100-1;		/* Timer1: 100Hz interval (OC1A) */
	TCCR1B = _BV(WGM12) | _BV(CS11);
	TIMSK1 = _BV(OCIE1A);		/* Enable TC1.oca interrupt */

	/* ADC */
//	ADMUX = 0;
	ADMUX = 1; // FIXME only testing battery voltage
	ADCSRA = _BV(ADEN)|_BV(ADATE)|_BV(ADIE)|_BV(ADPS2)|_BV(ADPS1)|_BV(ADPS0);
	
	/* uart1 (debug) */	
	uart1_init();
	
	I2C_init();
	expander_init(0, 0x00, 0x00); /* all as outputs */
	LCD_Initialize();
	LCD_Clear();

	sei();
	
	ADCSRA |= _BV(ADSC);
	
	/* unused pins */
	DDRA |= _BV(PA2) | _BV(PA4) | _BV(PA5);
	PRR0 |= _BV(PRTIM2) | _BV(PRTIM0);
	ACSR = _BV(ACD);		/* Disable analog comp */
}

void close_files(unsigned char flush_logs) {
	UINT bw;
	if (FLAGS & F_FILEOPEN) {
		if (f_close(&gps_log))
			System.status = STATUS_FILE_CLOSE_ERROR;
		if (gpx_close(&gpx_file))
			System.status = STATUS_FILE_CLOSE_ERROR;
		if (flush_logs && logbuf.len && !f_write(&system_log, logbuf.buf, logbuf.len, &bw)) {
			logbuf.len = 0;
		}
		if (f_close(&system_log))
			System.status = STATUS_FILE_CLOSE_ERROR;
		xputs_P(PSTR("File closed\r\n"));
		display_state(DISPLAY_STATE_FILE_CLOSED);
	}
	FLAGS &= ~F_FILEOPEN;
	disk_ioctl(0, CTRL_POWER, 0);
}

__flash const char __open_msg[] = "Open %s\r\n";

/*-----------------------------------------------------------------------*/
/* Main                                                                  */
/*-----------------------------------------------------------------------*/

int main (void)
{
	UINT bw, len;
	static struct tm ct;
	time_t tmp_utc, localtime;
	FRESULT res;
	unsigned char prev_status;

	ioinit();
	xdev_out(log_put);
	xputs_P(PSTR("STARTUP\r\n"));
	disp_init();
	display_state(DISPLAY_STATE_STARTUP);
	settings_load();
	
	for (;;) {
		wdt_reset();
		if (FLAGS & (F_POWEROFF | F_LVD)) {
			xputs_P(PSTR("POWEROFF\r\n"));
			display_state(DISPLAY_STATE_POWEROFF);
			if (FLAGS & F_LVD) {
				display_state(DISPLAY_STATE_POWEROFF_LOWBAT);
				_delay_ms(500);
			}
			POWEROFF();
			while (POWER_SW_PRESSED());
			_delay_ms(2000); /* wait for switch off */
			FLAGS &= ~F_POWEROFF; /* restart if power is not lost */
			POWERON();
			xputs_P(PSTR("RESTART\r\n"));
		}

		display_state(DISPLAY_STATE_START_MESSAGE);
		xprintf(PSTR("LOOP err=%u\r\n"), (unsigned int)System.status);
		utc = 0;
		localtime = 0;
		prev_status = System.status;
		System.status = STATUS_NO_POWER;

		beep(250, prev_status);	/* Error beep */

		/* Wait for supply voltage stabled */
		_delay_ms(500);
		if (FLAGS & F_LVD)
			continue;

		/* report error here ( prev_status) */

		if (disk_status(0) & STA_NODISK) {
			System.status = STATUS_NO_DISK;
			continue;
		}

		res = f_mount(&Fatfs, "", 1);
		if (res != FR_OK) {
			xprintf(PSTR("FS error %u\r\n"), res);
			System.status = STATUS_DISK_ERROR;
			continue;
		}
		System.status = STATUS_NO_GPS;
		xputs(PSTR("FS Ok\r\n"));
		display_state(DISPLAY_STATE_CARD_OK);
		beep(50, 1);				/* 1 beep */

		/* Initialize GPS receiver */
		GPS_ON();		/* GPS power on */
		FLAGS |= F_POW;
		_delay_ms(1);
		uart0_init();	/* Enable UART */
		_delay_ms(300);	/* Delay */
		System.gps_initialized = 0;
		
		if (!get_flag(CONFFLAG_LOGGING_AFTER_BOOT)) {
			tracking_pause();
		}

		for (;;) { /* main loop */
			wdt_reset();
			display_refresh(DISPLAY_STATE_NO_CHANGE);
			gettemp();
			key_process();
			if (System.timers.backlight)
				LEDW_ON();
			else
				LEDW_OFF();

			if (!(FLAGS & F_GPSOK))
				xputs_P(PSTR("Waiting for GPS\r\n"));
			len = get_line(Line, sizeof Line);	/* Receive a line from GPS receiver */
			if (!len){
				if (FLAGS & (F_LVD | F_POWEROFF))
					break; /* brownout */
				continue;
			}

			tmp_utc = gps_parse(Line);
			if (tmp_utc) {
				localtime = tmp_utc + local_time_diff(tmp_utc) * 3600L;	/* Local time */
				ct = *gmtime(&localtime);
				if (timer_expired(system_log)) {
					set_timer(system_log, 5000);
					xprintf(PSTR("Time: %u.%02u.%04u %u:%02u:%02u\r\n"), ct.tm_mday, ct.tm_mon + 1, ct.tm_year+1900, ct.tm_hour, ct.tm_min, ct.tm_sec);
					xprintf(PSTR("Bat volt: %.3f\r\n"), System.bat_volt);
					if (System.temperature_ok)
						xprintf(PSTR("Temp: %.2f\r\n"), System.temperature);
					else
						xputs_P(PSTR("Temperature unknown\r\n"));
					if (System.sbas)
						xputs_P(PSTR("SBAS (DGPS) active\r\n"));
					else
						xputs_P(PSTR("SBAS inactive\r\n"));
					xputs_P(PSTR("Using GNSS: "));
					xputs_P(gnss_names[System.conf.gnss_mode]);
					xputs_P(PSTR("\r\n"));
				}
				LEDG_ON();
				_delay_ms(2);
				LEDG_OFF();
			}

			if (FLAGS & F_FILEOPEN) {
				f_write(&gps_log, Line, len, &bw);
				if (bw != len) {
					System.status = STATUS_FILE_WRITE_ERROR;
					break;
				}
				if (System.location_valid == LOC_VALID_NEW) /* a new point */
					gpx_process_point(&location, &gpx_file);
				wdt_reset();
				if (FLAGS & F_SYNC) {
					if (f_sync(&gps_log)) {
						System.status = STATUS_FILE_SYNC_ERROR;
						break;
					}
					FLAGS &= ~F_SYNC;
				}
				if (System.open_new_file) {
					close_files(1);
					System.open_new_file = 0;
					if (!System.tracking_paused)
						tracking_pause();
				}
			}
			if (System.location_valid == LOC_VALID_NEW) {
				System.location_valid = LOC_VALID;
			}

			if (localtime && !(FLAGS & F_FILEOPEN)) {
				char *time = get_iso_time(utc, 1);
				iso_time_to_filename(time);
				xsprintf(Line, PSTR("%s-NMEA.LOG"), time);
				xprintf(__open_msg, Line);
				wdt_disable();
				if (f_open(&gps_log, Line, FA_WRITE | FA_OPEN_ALWAYS)		/* Open log file */
					|| f_lseek(&gps_log, f_size(&gps_log)) 					/* Append mode */
					|| f_write(&gps_log, "\r\n", 2, &bw))					/* Put a blank line as start marker */
				{
					System.status = STATUS_FILE_OPEN_ERROR;
					break;	/* Failed to start logging */
				}

				xsprintf(Line, PSTR("%s.GPX"), time);
				xprintf(__open_msg, Line);
				if (f_open(&gpx_file, Line, FA_WRITE | FA_OPEN_ALWAYS)		/* Open log file */
					|| f_lseek(&gpx_file, f_size(&gpx_file)) 				/* Append mode */
					|| gpx_init(&gpx_file))									/* Put a blank line as start marker */
				{
					f_close(&gpx_file);
					System.status = STATUS_FILE_OPEN_ERROR;
					break;	/* Failed to start logging */
				}

				xsprintf(Line, PSTR("%s-SYSTEM.LOG"), time);
				xprintf(__open_msg, Line);
				if (f_open(&system_log, Line, FA_WRITE | FA_OPEN_ALWAYS)	/* Open log file */
					|| f_lseek(&system_log, f_size(&system_log)) 			/* Append mode */
					|| f_write(&system_log, "\r\n", 2, &bw))				/* Put a blank line as start marker */
				{
					f_close(&gpx_file);
					f_close(&gps_log);
					System.status = STATUS_FILE_OPEN_ERROR;
					break;	/* Failed to start logging */
				}
				wdt_enable(WDTO_4S);
				FLAGS |= F_FILEOPEN;
				System.status = STATUS_OK;
				beep(50, System.tracking_paused?5:2);		/* Two beeps. Start logging. */
				display_state(DISPLAY_STATE_FILE_OPEN);
				continue;
			}
		}
		
		wdt_enable(WDTO_4S);

		/* Stop GPS receiver */
		uart0_deinit();
		GPS_OFF();
		FLAGS &= ~F_POW;

		/* Close file */
		close_files(1);
		if (System.status != STATUS_FILE_CLOSE_ERROR)
			beep(500, 1);	/* Long beep on file close succeeded */
		
		settings_store(); /* save eeprom data */
	}

}


