/*-------------------------------------------------------------------------------------------------
 * Wyúwietlacz alfanumeryczny ze sterownikiem HD44780
 * Sterowanie w trybie 8-bitowym z odczytem flagi zajÍtoúci
 * Plik : HD44780.c	
 * Mikrokontroler : Atmel AVR
 * Kompilator : avr-gcc
 * Autor : Rados≥aw KwiecieÒ
 * èrÛd≥o : http://radzio.dxp.pl/hd44780/
 * Data : 24.03.2007
 * Modified: k4be, 2022 (I2C access mode)
 * ------------------------------------------------------------------------------------------------ */

#include "HD44780.h"
#include <avr/pgmspace.h>


//-------------------------------------------------------------------------------------------------
//
// Funkcja zapisu bajtu do wyúwietacza (bez rozrÛønienia instrukcja/dane).
//
//-------------------------------------------------------------------------------------------------
void _LCD_Write(unsigned char dataToWrite)
{
	LCD_DATA_OUTPUT();

	expander_set_bit_no_send(LCD_RW_PORT, LCD_RW, 0);
	expander_set_bit_no_send(LCD_E_PORT, LCD_E, 1);
	exp_output[0] = dataToWrite;
	expander_write(0);
	expander_set_bit(LCD_E_PORT, LCD_E, 0);
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja odczytu bajtu z wyúwietacza (bez rozrÛønienia instrukcja/dane).
//
//-------------------------------------------------------------------------------------------------

unsigned char _LCD_Read(void)
{
	unsigned char tmp = 0;

	LCD_DATA_INPUT();

	expander_set_bit_no_send(LCD_RW_PORT, LCD_RW, 1);
	expander_set_bit(LCD_E_PORT, LCD_E, 1);
	tmp = expander_read_byte(0, 0);
	expander_set_bit(LCD_E_PORT, LCD_E, 0);
	return tmp;
}

//-------------------------------------------------------------------------------------------------
//
// Funkcja zapisu rozkazu do wyúwietlacza
//
//-------------------------------------------------------------------------------------------------
void LCD_WriteCommand(unsigned char commandToWrite)
{
	expander_set_bit(LCD_RS_PORT, LCD_RS, 0);
	_LCD_Write(commandToWrite);
	while(LCD_ReadStatus()&0x80);
}

//-------------------------------------------------------------------------------------------------
//
// Funkcja odczytu bajtu statusowego
//
//-------------------------------------------------------------------------------------------------
unsigned char LCD_ReadStatus(void)
{
	expander_set_bit(LCD_RS_PORT, LCD_RS, 0);
	return _LCD_Read();
	return 0;
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja zapisu danych do pamiÍci wyúwietlacza
//
//-------------------------------------------------------------------------------------------------
void LCD_WriteData(unsigned char dataToWrite)
{
	expander_set_bit(LCD_RS_PORT, LCD_RS, 1);
	_LCD_Write(dataToWrite);
	while(LCD_ReadStatus()&0x80);
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja odczytu danych z pamiÍci wyúwietlacza
//
//-------------------------------------------------------------------------------------------------
unsigned char LCD_ReadData(void)
{
	expander_set_bit(LCD_RS_PORT, LCD_RS, 1);
	return _LCD_Read();
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja wyúwietlenia napisu na wyswietlaczu.
//
//-------------------------------------------------------------------------------------------------
void LCD_WriteText(const char * text)
{
	while(*text)
	  LCD_WriteData(*text++);
}

void LCD_WriteTextP(const char *text)
{
	char a;
	while((a = pgm_read_byte(text++)))
	  LCD_WriteData(a);
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja ustawienia wspÛ≥rzÍdnych ekranowych
//
//-------------------------------------------------------------------------------------------------
void LCD_GoTo(unsigned char x, unsigned char y)
{
	LCD_WriteCommand(HD44780_DDRAM_SET | (x + (0x40 * y)));
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja czyszczenia ekranu wyúwietlacza.
//
//-------------------------------------------------------------------------------------------------
void LCD_Clear(void)
{
	LCD_WriteCommand(HD44780_CLEAR);
	_delay_ms(2);
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja przywrÛcenia poczπtkowych wspÛ≥rzÍdnych wyúwietlacza.
//
//-------------------------------------------------------------------------------------------------
void LCD_Home(void)
{
	LCD_WriteCommand(HD44780_HOME);
	_delay_ms(2);
}
//-------------------------------------------------------------------------------------------------
//
// Procedura inicjalizacji kontrolera HD44780.
//
//-------------------------------------------------------------------------------------------------
void LCD_Initalize(void)
{
	unsigned char i;
	LCD_DATA_OUTPUT();

	expander_set_bit_no_send(LCD_RS_PORT, LCD_RS, 0); // wyzerowanie linii RS
	expander_set_bit_no_send(LCD_E_PORT, LCD_E, 0);  // wyzerowanie linii E
	expander_set_bit(LCD_RW_PORT, LCD_RW, 0);
	_delay_ms(100); // oczekiwanie na ustalibizowanie siÍ napiecia zasilajacego

	exp_output[0] = 0x3F;
	expander_write(0);
	for(i = 0; i < 3; i++) // trzykrotne powtÛrzenie bloku instrukcji
	{
	  expander_set_bit(LCD_E, LCD_E, 1);
	  expander_set_bit(LCD_E, LCD_E, 0);
	  _delay_ms(5); // czekaj 5ms
	}

	LCD_WriteCommand(HD44780_FUNCTION_SET | HD44780_FONT5x7 | HD44780_TWO_LINE | HD44780_8_BIT); // interfejs 4-bity, 2-linie, znak 5x7
	LCD_WriteCommand(HD44780_DISPLAY_ONOFF | HD44780_DISPLAY_OFF); // wy≥πczenie wyswietlacza
	//LCD_WriteCommand(HD44780_CLEAR); // czyszczenie zawartosÊi pamieci DDRAM
	LCD_WriteCommand(HD44780_ENTRY_MODE | HD44780_EM_SHIFT_CURSOR | HD44780_EM_INCREMENT);// inkrementaja adresu i przesuwanie kursora
	LCD_WriteCommand(HD44780_DISPLAY_ONOFF | HD44780_DISPLAY_ON | HD44780_CURSOR_OFF | HD44780_CURSOR_NOBLINK); // w≥πcz LCD, bez kursora i mrugania
}

//-------------------------------------------------------------------------------------------------
//
// Koniec pliku HD44780.c
//
//-------------------------------------------------------------------------------------------------
