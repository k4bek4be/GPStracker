/*
 * expander.h
 * k4be 2022
 * License: BSD
 *
 * PCA9539/TCA9539 GPIO expander driver
 */ 


#ifndef EXPANDER_H_
#define EXPANDER_H_

#define P00	0
#define P01	1
#define P02	2
#define P03	3
#define P04	4
#define P05 5
#define P06 6
#define P07 7

#define P10 0
#define P11 1
#define P12 2
#define P13 3
#define P14 4
#define P15 5
#define P16 6
#define P17 7

#define EXP_PORT0	(128|0)
#define EXP_PORT1	(128|1)

#define CMD_PORT0_INPUT 0
#define CMD_PORT1_INPUT 1
#define CMD_PORT0_OUTPUT 2
#define CMD_PORT1_OUTPUT 3
#define CMD_PORT0_INV 4
#define CMD_PORT1_INV 5
#define CMD_PORT0_CONFIG 6
#define CMD_PORT1_CONFIG 7

#define EXPANDER_ADDR 0b1110100 // first address

#define EXPANDER_COUNT 1 // number of expanders (contiguous addresses)

void expander_init(unsigned char addr, unsigned char p1in, unsigned char p2in);
unsigned int expander_read(unsigned char addr);
void expander_write(unsigned char addr);
void expander_write_all(void);
void expander_set_bit(unsigned char port, unsigned char val, unsigned char on);

#endif /* EXPANDER_H_ */