/*
 * expander.c
 * k4be 2022
 * License: BSD
 *
 * PCA9539/TCA9539 GPIO expander driver
 */ 

#include "main.h"
#include "I2C.h"
#include "expander.h"
#include <avr/io.h>

unsigned char exp_output[EXPANDER_COUNT*2];

void expander_init(unsigned char addr, unsigned char p1in, unsigned char p2in){
	addr += EXPANDER_ADDR;
	expander_write_all();
	expander_set_dir(addr, p1in, p2in);
}

void expander_set_dir(unsigned char addr, unsigned char p1in, unsigned char p2in){
	System.global_error |= I2C_SendCommand3byte(addr, CMD_PORT0_CONFIG, p1in, p2in);
}

unsigned int expander_read(unsigned char addr){
	unsigned char low, high;
	System.global_error |= I2C_ReceiveCommand(addr, CMD_PORT0_INPUT, &low);
	System.global_error |= I2C_ReceiveCommand(addr, CMD_PORT1_INPUT, &high);
	return (unsigned int)low | (((unsigned int) high)<<8);
}

unsigned int expander_read_byte(unsigned char addr, unsigned char reg){
	unsigned char result;
	System.global_error |= I2C_ReceiveCommand(addr, reg?CMD_PORT1_INPUT:CMD_PORT0_INPUT, &result);
	return result;
}

void expander_write(unsigned char expaddr){
	unsigned char addr = EXPANDER_ADDR + expaddr;
	System.global_error |= I2C_SendCommand3byte(addr, CMD_PORT0_OUTPUT, exp_output[expaddr*2], exp_output[expaddr*2 + 1]);
}

void expander_write_all(void){
	unsigned char i;
	for(i=0; i<EXPANDER_COUNT; i++)	
		expander_write(i);
}

void expander_set_bit(unsigned char port, unsigned char val, unsigned char on){
	if(on){
		exp_output[port] |= val;
	} else {
		exp_output[port] &= ~val;
	}
/*	if(SREG & _BV(SREG_I)){
		System.expander_pending = 1;
	} else {*/
		expander_write(port / 2);
//	}
}

void expander_set_bit_no_send(unsigned char port, unsigned char val, unsigned char on){
	if(on){
		exp_output[port] |= val;
	} else {
		exp_output[port] &= ~val;
	}
}
