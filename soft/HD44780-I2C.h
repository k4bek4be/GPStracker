/*-------------------------------------------------------------------------------------------------
 * Wyświetlacz alfanumeryczny ze sterownikiem HD44780
 * Sterowanie w trybie 8-bitowym z odczytem flagi zajętości
 * Plik : HD44780-I2C.h
 * Mikrokontroler : Atmel AVR
 * Kompilator : avr-gcc
 * Autor : Radosław Kwiecień
 * Źródło : http://radzio.dxp.pl/hd44780/
 * Data : 24.03.2007
 * Modified: k4be, 2022 (I2C access mode)
 * ------------------------------------------------------------------------------------------------ */

#pragma once

#include <avr/io.h>
#include <util/delay.h>
#include "expander.h"

//-------------------------------------------------------------------------------------------------
//
// Konfiguracja sygnałów sterujących wyświetlaczem.
// Można zmieniæ stosownie do potrzeb.
//
//-------------------------------------------------------------------------------------------------
#define LCD_RS_PORT 	1
#define LCD_RS			(1 << 3)

#define LCD_RW_PORT		1
#define LCD_RW			(1 << 2)

#define LCD_E_PORT		1
#define LCD_E			(1 << 1)

#define LCD_DATA_OUTPUT() 	expander_set_dir(0, 0x00, 0x00)
#define LCD_DATA_INPUT() 	expander_set_dir(0, 0xFF, 0x00)

//-------------------------------------------------------------------------------------------------
//
// Instrukcje kontrolera Hitachi HD44780
//
//-------------------------------------------------------------------------------------------------

#define HD44780_CLEAR					0x01

#define HD44780_HOME					0x02

#define HD44780_ENTRY_MODE				0x04
	#define HD44780_EM_SHIFT_CURSOR		0
	#define HD44780_EM_SHIFT_DISPLAY	1
	#define HD44780_EM_DECREMENT		0
	#define HD44780_EM_INCREMENT		2

#define HD44780_DISPLAY_ONOFF			0x08
	#define HD44780_DISPLAY_OFF			0
	#define HD44780_DISPLAY_ON			4
	#define HD44780_CURSOR_OFF			0
	#define HD44780_CURSOR_ON			2
	#define HD44780_CURSOR_NOBLINK		0
	#define HD44780_CURSOR_BLINK		1

#define HD44780_DISPLAY_CURSOR_SHIFT	0x10
	#define HD44780_SHIFT_CURSOR		0
	#define HD44780_SHIFT_DISPLAY		8
	#define HD44780_SHIFT_LEFT			0
	#define HD44780_SHIFT_RIGHT			4

#define HD44780_FUNCTION_SET			0x20
	#define HD44780_FONT5x7				0
	#define HD44780_FONT5x10			4
	#define HD44780_ONE_LINE			0
	#define HD44780_TWO_LINE			8
	#define HD44780_4_BIT				0
	#define HD44780_8_BIT				16

#define HD44780_CGRAM_SET				0x40

#define HD44780_DDRAM_SET				0x80

//-------------------------------------------------------------------------------------------------
//
// Deklaracje funkcji
//
//-------------------------------------------------------------------------------------------------

void LCD_WriteCommand(unsigned char);
unsigned char LCD_ReadStatus(void);
void LCD_WriteData(unsigned char);
unsigned char LCD_ReadData(void);
void LCD_WriteText(const char *);
void LCD_GoTo(unsigned char, unsigned char);
void LCD_Clear(void);
void LCD_Home(void);
void LCD_Initialize(void);
void LCD_WriteTextP(const char *);

//-------------------------------------------------------------------------------------------------
//
// Koniec pliku HD44780.h
//
//-------------------------------------------------------------------------------------------------
