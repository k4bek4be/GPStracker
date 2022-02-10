/*
 * I2C.h
 * k4be 2022
 * License: BSD
 */ 

#ifndef I2C_H_
#define I2C_H_

#define MAX_REPEAT_I2C		3

#include <avr/io.h>
#include <util/delay.h>
#include "I2C.h"
#include "main.h"

void I2C_init(void);
unsigned int I2C_SendCommand(unsigned char address, unsigned char command, unsigned char data);
unsigned int I2C_SendCommand3byte(unsigned char address,unsigned char command, unsigned char data0, unsigned char data1);
unsigned int I2C_ReceiveCommand(unsigned char address,unsigned char command, unsigned char *data);
unsigned int I2C_ReceiveCommand3byte(unsigned char address, unsigned char command, unsigned char *data0, unsigned char *data1);
unsigned int I2C_SendOneCommandByte(unsigned char command);
unsigned int I2C_ReceiveOneCommandByte(unsigned char *command, unsigned char ack);
unsigned int I2C_WaitForTWInt(void);
unsigned int I2C_Receive_n_bytes(unsigned char address, unsigned char command, unsigned char *data, unsigned int length);
unsigned int I2C_Send_n_bytes(unsigned char address,unsigned char command, const unsigned char *data, unsigned int length);

#endif /* I2C_H_ */