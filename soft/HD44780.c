//-------------------------------------------------------------------------------------------------
// Wy�wietlacz alfanumeryczny ze sterownikiem HD44780
// Sterowanie w trybie 8-bitowym z odczytem flagi zaj�to�ci
// Plik : HD44780.c	
// Mikrokontroler : Atmel AVR
// Kompilator : avr-gcc
// Autor : Rados�aw Kwiecie�
// �r�d�o : http://radzio.dxp.pl/hd44780/
// Data : 24.03.2007
//-------------------------------------------------------------------------------------------------

#include "HD44780.h"
#include <avr/pgmspace.h>


//-------------------------------------------------------------------------------------------------
//
// Funkcja zapisu bajtu do wy�wietacza (bez rozr�nienia instrukcja/dane).
//
//-------------------------------------------------------------------------------------------------
void _LCD_Write(unsigned char dataToWrite)
{
LCD_DATA_DIR = 0xFF;

LCD_RW_PORT &= ~LCD_RW;
LCD_E_PORT |= LCD_E;
LCD_DATA_PORT = dataToWrite;
LCD_E_PORT &= ~LCD_E;
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja odczytu bajtu z wy�wietacza (bez rozr�nienia instrukcja/dane).
//
//-------------------------------------------------------------------------------------------------

unsigned char _LCD_Read(void)
{
unsigned char tmp = 0;

LCD_DATA_DIR = 0x00;

LCD_RW_PORT |= LCD_RW;
LCD_E_PORT |= LCD_E;
_delay_us(1); //tu zmieni�em
tmp = LCD_DATA_PIN;
LCD_E_PORT &= ~LCD_E;
return tmp;
}

//-------------------------------------------------------------------------------------------------
//
// Funkcja zapisu rozkazu do wy�wietlacza
//
//-------------------------------------------------------------------------------------------------
void LCD_WriteCommand(unsigned char commandToWrite)
{
LCD_RS_PORT &= ~LCD_RS;
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
LCD_RS_PORT &= ~LCD_RS;
return _LCD_Read();
return 0;
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja zapisu danych do pami�ci wy�wietlacza
//
//-------------------------------------------------------------------------------------------------
void LCD_WriteData(unsigned char dataToWrite)
{
LCD_RS_PORT |= LCD_RS;
_LCD_Write(dataToWrite);
while(LCD_ReadStatus()&0x80);
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja odczytu danych z pami�ci wy�wietlacza
//
//-------------------------------------------------------------------------------------------------
unsigned char LCD_ReadData(void)
{
LCD_RS_PORT |= LCD_RS;
return _LCD_Read();
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja wy�wietlenia napisu na wyswietlaczu.
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
// Funkcja ustawienia wsp�rz�dnych ekranowych
//
//-------------------------------------------------------------------------------------------------
void LCD_GoTo(unsigned char x, unsigned char y)
{
LCD_WriteCommand(HD44780_DDRAM_SET | (x + (0x40 * y)));
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja czyszczenia ekranu wy�wietlacza.
//
//-------------------------------------------------------------------------------------------------
void LCD_Clear(void)
{
LCD_WriteCommand(HD44780_CLEAR);
_delay_ms(2);
}
//-------------------------------------------------------------------------------------------------
//
// Funkcja przywr�cenia pocz�tkowych wsp�rz�dnych wy�wietlacza.
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
LCD_DATA_DIR = 0xFF;
LCD_E_DIR 	|= LCD_E;   //
LCD_RS_DIR 	|= LCD_RS;  //
LCD_RW_DIR	|= LCD_RW;
LCD_RS_PORT &= ~LCD_RS; // wyzerowanie linii RS
LCD_E_PORT &= ~LCD_E;  // wyzerowanie linii E
LCD_RW_PORT  &= ~LCD_RW;
_delay_ms(100); // oczekiwanie na ustalibizowanie si� napiecia zasilajacego

for(i = 0; i < 3; i++) // trzykrotne powt�rzenie bloku instrukcji
  {
  LCD_E_PORT |= LCD_E;
  LCD_DATA_PORT = 0x3F;
  LCD_E_PORT &= ~LCD_E;
  _delay_ms(5); // czekaj 5ms
  }

LCD_WriteCommand(HD44780_FUNCTION_SET | HD44780_FONT5x7 | HD44780_TWO_LINE | HD44780_8_BIT); // interfejs 4-bity, 2-linie, znak 5x7
LCD_WriteCommand(HD44780_DISPLAY_ONOFF | HD44780_DISPLAY_OFF); // wy��czenie wyswietlacza
//LCD_WriteCommand(HD44780_CLEAR); // czyszczenie zawartos�i pamieci DDRAM
LCD_WriteCommand(HD44780_ENTRY_MODE | HD44780_EM_SHIFT_CURSOR | HD44780_EM_INCREMENT);// inkrementaja adresu i przesuwanie kursora
LCD_WriteCommand(HD44780_DISPLAY_ONOFF | HD44780_DISPLAY_ON | HD44780_CURSOR_OFF | HD44780_CURSOR_NOBLINK); // w��cz LCD, bez kursora i mrugania
}

//-------------------------------------------------------------------------------------------------
//
// Koniec pliku HD44780.c
//
//-------------------------------------------------------------------------------------------------
