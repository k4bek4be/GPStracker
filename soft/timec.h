#pragma once

#define TIME_OFFSET	+1 /* CET */
#define SUMMER_TIME_OFFSET	+2 /* CEST */

signed int local_time_diff(time_t time);
const char *local_time_mark(time_t time);
char *get_iso_time(time_t time, unsigned char local);

