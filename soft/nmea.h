#pragma once
#include "main.h"
#include "ff.h"

time_t gps_parse(char *str);
UINT get_line(char *buff, UINT sz_buf);
void gps_initialize(void);

