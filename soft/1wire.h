/*
 * 1wire.h
 * k4be 2019
 * License: BSD
 */ 


#ifndef OWIRE_H_
#define OWIRE_H_

#undef SEARCHROM
#undef INITST

#ifdef INITST
extern unsigned char initst;
#endif

#include <avr/io.h>

#define _1WPORT PORTB
#define _1WDDR DDRB
#define _1WPIN PINB
#define _1WL _BV(PB0)

unsigned char _1WireInit(void);
void _1WireWriteSlot(unsigned char bit, unsigned char pr);
unsigned char _1WireReadSlot(void);
void _1WireWriteByte(unsigned char dana);
void _1WireWriteBytePR(unsigned char dana);
unsigned char _1WireReadByte(void);
void _1WirePoweroff(void);
void _1WireSendRom(unsigned char *rom);
unsigned char _1WireSearch(unsigned char rv, unsigned char *buf);

#define _1WLOW() _1WDDR |= _1WL;
#define _1WHIGH() _1WDDR &= ~_1WL;
#define _1WISHIGH() (_1WPIN & _1WL)

#define _1WirePoweroff() { _1WDDR &= ~_1WL; _1WPORT &= ~_1WL; }



#endif /* OWIRE_H_ */