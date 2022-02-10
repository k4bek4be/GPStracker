/*
 * ds18b20.h
 * k4be 2019
 * License: BSD
 */ 


#ifndef DS18B20_H_
#define DS18B20_H_

extern unsigned char temp_ok;
extern float hs_temp;

void gettemp(void);

#endif /* DS18B20_H_ */