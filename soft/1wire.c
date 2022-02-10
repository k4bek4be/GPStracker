/*
 * 1wire.c
 * k4be 2019
 * License: BSD
 */ 

#include "1wire.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#ifdef SEARCHROM
#ifdef INITST
unsigned char initst;
#endif
#endif

unsigned char _1WireInit(void){
	unsigned char out=0;
	_1WPORT &= ~_1WL;
	_1WLOW();
	_delay_us(480);
	cli();
	_1WHIGH();
	_delay_us(70);
	if(!_1WISHIGH()) out++;
	sei();
	_delay_us(410);
	if(!_1WISHIGH()) out++;
	return out; // 0 - short; 1 - ok; 2 - open/no slaves
}

void _1WireWriteByte(unsigned char data){
	register unsigned char i;
	for(i=0;i<8;i++){
		_1WireWriteSlot(data&1,0);
		data >>= 1;
	}
}

void _1WireWriteBytePR(unsigned char data){ // enable strong pullup right after the command
	register unsigned char i;
	for(i=0;i<7;i++){
		_1WireWriteSlot(data&1,0);
		data >>= 1;
	}
	_1WireWriteSlot(data,1);
	_1WPORT |= _1WL;
	_1WDDR |= _1WL;
	sei();
	if(!_1WISHIGH()) _1WirePoweroff();
}


void _1WireWriteSlot(unsigned char x, unsigned char pr){
	cli();
	_1WLOW();
	if(x) _delay_us(6); else _delay_us(60);
	_1WHIGH();
	if(x) _delay_us(64); else _delay_us(10);
	if(!pr) sei();
	_delay_us(1);
}

unsigned char _1WireReadByte(void){
	register unsigned char i=1;
	unsigned char data=0;
	while(i){
		if(_1WireReadSlot()) data |= i;
		i <<= 1;
	}
	return data;
}

unsigned char _1WireReadSlot(void){
	unsigned char out=0;
	cli();
	_1WLOW();
	_delay_us(6);
	_1WHIGH();
	_delay_us(9);
	if(_1WISHIGH()) out=1;
	sei();
	_delay_us(55);
	return out;
}
/*
void _1WirePoweroff(void){ // turn off power
	_1WDDR &= ~_1WL;
	_1WPORT &= ~_1WL;
}*/

#ifdef SEARCHROM

unsigned char _1WireSearch(unsigned char rv, unsigned char *buf){
	unsigned char i,a,b,mask=1,lz=0;
#ifdef INITST
	if((initst = _1WireInit())!=1) return 0xff;
#else
	if(_1WireInit()!=1) return 0xff;
#endif
	_1WireWriteByte(0xf0);
	for(i=1;i<65;i++){
		a = _1WireReadSlot();
		b = _1WireReadSlot();
		if(a && b) return 0xff;
		if(a==b) {
			if(i>rv) a = 0;
			else if(i==rv) a = 1;
			else a = !!((*buf)&mask);
			if(!a) lz=i;
		}
		_1WireWriteSlot(a,0);
		if(a) (*buf)|=mask; else (*buf)&=~mask;
		mask<<=1;
		if(!mask){
			mask = 1;
			buf++;
		}
	}
	rv=lz;
	return rv;
}

void _1WireSendRom(unsigned char *rom){
	register unsigned char i;
	for(i=0;i<8;i++){
		_1WireWriteByte(rom[i]);
	}
}
#endif
