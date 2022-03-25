/*---------------------------------------------------------*/
/* UART functions for ATmega164A/PA/324A/PA/644A/PA/1284/P */
/*---------------------------------------------------------*/

#include <avr/interrupt.h>
#include "uart1.h"

#define _PORT	PORTD
#define _DDR	DDRD
#define _RX		PD2
#define _TX		PD3
#define _UCSRA	UCSR1A
#define _UCSRB	UCSR1B
#define _UBRR	UBRR1
#define _RXEN	RXEN1
#define _RXCIE	RXCIE1
#define _TXEN	TXEN1
#define _UDRIE	UDRIE1
#define _UDRE	UDRE1
#define _UDR	UDR1
#define _USART_RX_vect	USART1_RX_vect
#define _USART_UDRE_vect	USART1_UDRE_vect

#define	UART_BAUD		230400
#define	USE_TXINT		1
#define	SZ_FIFO			64
#define RECEIVE			1
#define TRANSMIT		1

#include "uart.c"

