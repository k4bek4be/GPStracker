/*
 * I2C.c
 * k4be 2022
 * License: BSD
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include "I2C.h"
#include "main.h"

void I2C_init(void){
	TWBR = 18;
	TWSR |= 0;
}

/* two byte command (or command + value) */
unsigned int I2C_SendCommand(unsigned char address, unsigned char command, unsigned char data_byte)
{
	unsigned int error = ERROR_I2C;
	for(int index = 0 ; (index < MAX_REPEAT_I2C) && (error != ERROR_NO); index++)
	{
		error = 0;
		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // Send START condition
		error |= I2C_WaitForTWInt();
		
		error |= I2C_SendOneCommandByte(address << 1);
		error |= I2C_SendOneCommandByte(command);
		error |= I2C_SendOneCommandByte(data_byte);
		
		TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	}
	
	return error;
}

/* three byte command (or command + 2 values) */
unsigned int I2C_SendCommand3byte(unsigned char address,unsigned char command,unsigned char data0, unsigned char data1)
{
	unsigned int error = ERROR_I2C;
	for(int index = 0 ; (index < MAX_REPEAT_I2C) && (error != ERROR_NO); index++)
	{
		error = 0;
		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // Send START condition
		error |= I2C_WaitForTWInt();
		
		error |= I2C_SendOneCommandByte(address << 1);
		error |= I2C_SendOneCommandByte(command);
		error |= I2C_SendOneCommandByte(data0);
		error |= I2C_SendOneCommandByte(data1);
		
		TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	}

	return error;
}

/* read single byte */
unsigned int I2C_ReceiveCommand(unsigned char address, unsigned char command, unsigned char *data_byte)
{
	unsigned int error = ERROR_I2C;
	unsigned char _unused;
	for(int index = 0 ; (index < MAX_REPEAT_I2C) && (error != ERROR_NO); index++)
	{
		error = 0;
		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // Send START condition
		error |= I2C_WaitForTWInt();
		
		error |= I2C_SendOneCommandByte(address << 1);
		error |= I2C_SendOneCommandByte(command);
		error |= I2C_SendOneCommandByte((address << 1) | 1);

		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
		error |= I2C_WaitForTWInt();

		error |= I2C_ReceiveOneCommandByte(&_unused, 0);
		error |= I2C_ReceiveOneCommandByte(data_byte, 0);

		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	}
	
	return error;
}

/* send command + any byte count */
unsigned int I2C_Send_n_bytes(unsigned char address,unsigned char command, const unsigned char *data, unsigned int length)
{
	unsigned int error = ERROR_I2C;
	unsigned int i;
	
	for(int index = 0 ; (index < MAX_REPEAT_I2C) && (error != ERROR_NO); index++)
	{
		error = 0;
		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // Send START condition
		error |= I2C_WaitForTWInt();
		
		error |= I2C_SendOneCommandByte(address << 1);
		error |= I2C_SendOneCommandByte(command);
		for(i=0;i<length;i++){
			error |= I2C_SendOneCommandByte(data[i]);
		}
		
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	}

	return error;
}

/* read any byte count */
unsigned int I2C_Receive_n_bytes(unsigned char address, unsigned char command, unsigned char *data, unsigned int length)
{
	unsigned int i;
	unsigned int error = ERROR_I2C;
	for(int index = 0 ; (index < MAX_REPEAT_I2C) && (error != ERROR_NO); index++)
	{
		error = 0;
		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // Send START condition
		error |= I2C_WaitForTWInt();
		
		error |= I2C_SendOneCommandByte(address << 1);
		error |= I2C_SendOneCommandByte(command);
		
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
		_delay_us(1);
		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
		error |= I2C_WaitForTWInt();
		
		error |= I2C_SendOneCommandByte((address << 1) | 1);

		for(i=0; i<length; i++){
			error |= I2C_ReceiveOneCommandByte(&data[i], i != length-1);
		}

		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	}
	
	return error;
}

/* read 2 bytes */
unsigned int I2C_ReceiveCommand3byte(unsigned char address, unsigned char command, unsigned char *data0, unsigned char *data1)
{
	unsigned int error = ERROR_I2C;
	unsigned char addressTrash;
	for(int index = 0 ; (index < MAX_REPEAT_I2C) && (error != ERROR_NO); index++)
	{
		error = 0;
		TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);//Send START condition
		error |= I2C_WaitForTWInt();
		
		error |= I2C_SendOneCommandByte(address << 1);
		error |= I2C_SendOneCommandByte(command);
		error |= I2C_SendOneCommandByte((address << 1) | 1);
		
		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
		error |= I2C_WaitForTWInt();

		error |= I2C_ReceiveOneCommandByte(&addressTrash, 0);
		error |= I2C_ReceiveOneCommandByte(data0, 0);
		error |= I2C_ReceiveOneCommandByte(data1, 0);
		
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	}
	
	return error;
}

unsigned int I2C_SendOneCommandByte(unsigned char command)
{
	TWDR = command;
	TWCR = (1<<TWINT) | (1<<TWEN);
	return I2C_WaitForTWInt();
}

unsigned int I2C_ReceiveOneCommandByte(unsigned char *command, unsigned char ack)
{
	TWCR = (1<<TWINT) | (1<<TWEN) | (ack?_BV(TWEA):0);
	unsigned int error = I2C_WaitForTWInt();
	*command = TWDR;
	return error;
}

unsigned int I2C_WaitForTWInt(void){
	unsigned char count = 20;
	while (count && !(TWCR & (1<<TWINT))){
		count--;
		_delay_us(6);
	}
	if(count == 0) return ERROR_I2C_TIMEOUT;

	return ERROR_NO;
}