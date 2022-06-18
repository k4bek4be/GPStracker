#include <avr/pgmspace.h>
#include "stime.h"
#include "timec.h"
#include "main.h"
#include "ff.h"
#include "xprintf.h"

__flash const unsigned char mi[] = {3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};

unsigned char calc_weekday(struct tm *ct) {
	unsigned char w, x;

	x = ct->tm_year - 100;
	w = x%7;
	if(x != 0) {		/* if year is not 2000 */
		/* Count only past years, for example if current year is 2004, count only 2000-2003, so x-- */
		x--;
		w += x >> 2;	/* add number of years which are divisible by 4 4 */
		w -= x/100;		/* subtract number of years which are divisible by 100 */
		w++;			/* add the year 2000 which is divisible by 400 */
	}
			/* Add first weekday (Saturday 2000-01-01) */
	w+=4;
	
/* Months */
	if(ct->tm_mon+1 > 1) {		/* if it's not January */
		w += mi[ct->tm_mon+1 - 2];	/* add array increment */
	}
/* Days */
	w += ct->tm_mday;

/*
** If current month is March or later, and it's a leap year, this means February
** was 29 days so we have to add 1.
*/
	if(ct->tm_mon+1 > 2)
		if(!(ct->tm_year&3)) w+=1;


/* Result modulo 7 */
	return (w%7)+1; /* 1 = Monday, 7 = Sunday */
}

signed int local_time_diff(time_t time) {
	signed char offset = TIME_OFFSET;
	unsigned char t;
	struct tm *ct = gmtime(&time);
	unsigned char weekday = calc_weekday(ct);

	if (ct->tm_mon+1 == 3 || ct->tm_mon+1 == 10){
		t = 35 + weekday - ct->tm_mday;
		t += 31;
		t = t%7;
		t = 31-t;
		if (ct->tm_mon+1 == 3){ // March
			if (ct->tm_mday==t) { // last Sunday
				if (ct->tm_hour >= 2 )
					offset = SUMMER_TIME_OFFSET;
			} else if (ct->tm_mday > t) { // after last Sunday
				offset = SUMMER_TIME_OFFSET;
			}
		} else { // October
			if (ct->tm_mday == t) { // last Sunday
				if(ct->tm_hour < 2)
					offset = SUMMER_TIME_OFFSET;
			} else if (ct->tm_mday < t) { // before last Sunday
				offset = SUMMER_TIME_OFFSET;
			}
		}
	} else if (ct->tm_mon+1 > 3 && ct->tm_mon+1 < 10) {
		offset = SUMMER_TIME_OFFSET;
	}
	return offset;
}

const char *local_time_mark(time_t time) {
	static char buf[4];
	signed int time_diff = local_time_diff(time);
	if (time_diff < 0) { 
		buf[0] = '-';
		time_diff = -time_diff;
	} else {
		buf[0] = '+';
	}
	buf[1] = time_diff/10 + '0'; /* never greater than 23 */
	buf[2] = time_diff%10 + '0';
	buf[3] = '\0';
	return buf;
}

/* Make ISO time string */
char *get_iso_time(time_t time, unsigned char local) {
	static char output[32];
	if (local)
		time += local_time_diff(time) * (signed int)3600;
	struct tm *ct = gmtime(&time);

	xsprintf(output, PSTR("%4u-%02u-%02uT%02u:%02u:%02u.000%s"), ct->tm_year+1900, ct->tm_mon+1, ct->tm_mday, ct->tm_hour, ct->tm_min, ct->tm_sec, local?local_time_mark(time):"Z");
	return output;
}

/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/

DWORD get_fattime (void)
{
	struct tm *stm;
	time_t localtime = utc + local_time_diff(utc) * (signed int)3600;
	stm = gmtime(&localtime);
    return (DWORD)(stm->tm_year - 80) << 25 |
           (DWORD)(stm->tm_mon + 1) << 21 |
           (DWORD)stm->tm_mday << 16 |
           (DWORD)stm->tm_hour << 11 |
           (DWORD)stm->tm_min << 5 |
           (DWORD)stm->tm_sec >> 1;
}

void iso_time_to_filename(char *time) {
	while (*time) {
		switch (*time) {
			case ':': *time = '-'; break;
			case '+': *time = 'p'; break;
		}
		time++;
	}
}
