/*---------------------------------------------------------*/
/* UART functions for ATmega164A/PA/324A/PA/644A/PA/1284/P */
/*---------------------------------------------------------*/


#include <avr/interrupt.h>
#include "uart0.h"

#define	UART0_BAUD		9600
#define	USE_TXINT		0
#define	SZ_FIFO			512


#if SZ_FIFO >= 256
typedef uint16_t	idx_t;
#else
typedef uint8_t		idx_t;
#endif


typedef struct {
	idx_t wi, ri, ct;
	uint8_t buff[SZ_FIFO];
} FIFO;
static
volatile FIFO RxFifo;
#if USE_TXINT
static
volatile FIFO TxFifo;
#endif



/* Initialize UART */

void uart0_init (void)
{
	UCSR0B = 0;

	PORTD |= _BV(PD1); DDRD |= _BV(PD1);	/* Set TXD as output */
	DDRD &= ~_BV(PD0); PORTD &= ~_BV(PD0); 	/* Set RXD as input */

	RxFifo.ct = 0; RxFifo.ri = 0; RxFifo.wi = 0;
#if USE_TXINT
	TxFifo.ct = 0; TxFifo.ri = 0; TxFifo.wi = 0;
#endif

	UBRR0L = F_CPU / UART0_BAUD / 16 - 1;
	UCSR0B = _BV(RXEN0) | _BV(RXCIE0) | _BV(TXEN0);
}


/* Deinitialize UART */

void uart0_deinit (void)
{
	UCSR0B = 0;
}


/* Get a received character */

uint8_t uart0_test (void)
{
	return RxFifo.ct;
}


uint8_t uart0_get (void)
{
	uint8_t d;
	idx_t i;


	do {
		cli(); i = RxFifo.ct; sei();
	} while (i == 0) ;
	i = RxFifo.ri;
	d = RxFifo.buff[i];
	cli();
	RxFifo.ct--;
	sei();
	RxFifo.ri = (i + 1) % sizeof RxFifo.buff;

	return d;
}


/* Put a character to transmit */

void uart0_put (uint8_t d)
{
#if USE_TXINT
	idx_t i;

	do {
		cli(); i = TxFifo.ct; sei();
	} while (i >= sizeof TxFifo.buff);
	i = TxFifo.wi;
	TxFifo.buff[i] = d;
	TxFifo.wi = (i + 1) % sizeof TxFifo.buff;
	cli();
	TxFifo.ct++;
	UCSR0B = _BV(RXEN0)|_BV(RXCIE0)|_BV(TXEN0)|_BV(UDRIE0);
	sei();
#else
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = d;
#endif
}


/* USART0 RXC interrupt */
ISR(USART0_RX_vect)
{
	uint8_t d;
	idx_t i;


	d = UDR0;
	i = RxFifo.ct;
	if (i < sizeof RxFifo.buff) {
		RxFifo.ct = ++i;
		i = RxFifo.wi;
		RxFifo.buff[i] = d;
		RxFifo.wi = (i + 1) % sizeof RxFifo.buff;
	}
}


#if USE_TXINT
ISR(USART0_UDRE_vect)
{
	idx_t n, i;


	n = TxFifo.ct;
	if (n) {
		TxFifo.ct = --n;
		i = TxFifo.ri;
		UDR0 = TxFifo.buff[i];
		TxFifo.ri = (i + 1) % sizeof TxFifo.buff;
	}
	if (n == 0)
		UCSR0B = _BV(RXEN0)|_BV(RXCIE0)|_BV(TXEN0);
}
#endif
