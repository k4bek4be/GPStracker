/*---------------------------------------------------------*/
/* UART functions for ATmega164A/PA/324A/PA/644A/PA/1284/P */
/*---------------------------------------------------------*/

#include <avr/interrupt.h>
#include "uart0.h"

#define _PORT	PORTD
#define _DDR	DDRD
#define _RX		PD0
#define _TX		PD1
#define _UCSRA	UCSR0A
#define _UCSRB	UCSR0B
#define _UBRR	UBRR0
#define _RXEN	RXEN0
#define _RXCIE	RXCIE0
#define _TXEN	TXEN0
#define _UDRIE	UDRIE0
#define _UDRE	UDRE0
#define _UDR	UDR0
#define _USART_RX_vect	USART0_RX_vect
#define _USART_UDRE_vect	USART0_UDRE_vect

#define	UART_BAUD		9600
#define	USE_TXINT		0
#define	SZ_FIFO			512
#define RECEIVE			1
#define TRANSMIT		1

#include "uart.c"

