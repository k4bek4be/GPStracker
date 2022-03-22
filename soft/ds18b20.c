/*
 * ds18b20.c - single sensor support
 * k4be 2019
 * License: BSD
 */ 

#include <string.h>
#include <util/crc16.h>
#include <util/delay.h>
#include "main.h"
#include "ds18b20.h"
#include "1wire.h"

union {
	signed int t;
	unsigned char owbuffer[9];
} t;

#define MAX_ERRORS 5

void gettemp(void){
	unsigned char i, crc=0, tmp;
	static unsigned char error_cnt;
	unsigned char temp_ok_out = 0;
	float temp;

	if(System.timers.owire) return;

	_delay_ms(1);


	if(_1WireInit() != 1){
		return;
	}

	_1WireWriteByte(0xcc);
	_1WireWriteByte(0xbe);

	for(i=0;i<9;i++){
		tmp = _1WireReadByte();
		t.owbuffer[i] = tmp;
		crc = _crc_ibutton_update(crc, tmp);
	}
	if(!crc){
		if(t.owbuffer[0] != 0x50 || t.owbuffer[1] != 0x05 || t.owbuffer[5] != 0xff || t.owbuffer[7] != 0x10){
			temp = (float)t.t*0.0625;
			System.temperature = temp;
			temp_ok_out = 1;
		}
	}

	if(!temp_ok_out){
		if(error_cnt > MAX_ERRORS){
			System.temperature_ok = 0;
		} else {
			error_cnt++;
		}
	} else {
		error_cnt = 0;
		System.temperature_ok = 1;
	}
	
	_1WireInit();
	_1WireWriteByte(0xcc);
	_1WireWriteByte(0x44);
	atomic_set_uint(&System.timers.owire, ms(4000));
}
