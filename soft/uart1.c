/*---------------------------------------------------------*/
/* UART functions for ATmega164A/PA/324A/PA/644A/PA/1284/P */
/*---------------------------------------------------------*/


#include <avr/interrupt.h>
#include "uart1.h"

#define	UART1_BAUD		38400
#define	USE_TXINT		1
#define	SZ_FIFO			128
#define RECEIVE			0

#if SZ_FIFO >= 256
typedef uint16_t	idx_t;
#else
typedef uint8_t		idx_t;
#endif


typedef struct {
	idx_t wi, ri, ct;
	uint8_t buff[SZ_FIFO];
} FIFO;
#if RECEIVE
static
volatile FIFO RxFifo;
#endif

#if USE_TXINT
static
volatile FIFO TxFifo;
#endif



/* Initialize UART */

void uart1_init (void)
{
	UCSR1B = 0;

	PORTD |= _BV(PD3); DDRD |= _BV(PD3);	/* Set TXD as output */
	DDRD &= ~_BV(PD2); PORTD &= ~_BV(PD2); 	/* Set RXD as input */

#if RECEIVE
	RxFifo.ct = 0; RxFifo.ri = 0; RxFifo.wi = 0;
#endif
#if USE_TXINT
	TxFifo.ct = 0; TxFifo.ri = 0; TxFifo.wi = 0;
#endif

	UBRR1 = F_CPU / UART1_BAUD / 16 - 1;
#if RECEIVE
	UCSR1B = _BV(RXEN1) | _BV(RXCIE1) | _BV(TXEN1);
#else
	UCSR1B = _BV(TXEN1);
#endif
}


/* Deinitialize UART */

void uart1_deinit (void)
{
	UCSR1B = 0;
}


/* Get a received character */
#if RECEIVE
uint8_t uart1_test (void)
{
	return RxFifo.ct;
}

uint8_t uart1_get (void)
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
#endif


/* Put a character to transmit */

void uart1_put (uint8_t d)
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
	UCSR1B |= _BV(UDRIE1);
	sei();
#else
	loop_until_bit_is_set(UCSR1A, UDRE1);
	UDR1 = d;
#endif
}

#if RECEIVE
/* USART1 RXC interrupt */
ISR(USART1_RX_vect)
{
	uint8_t d;
	idx_t i;


	d = UDR1;
	i = RxFifo.ct;
	if (i < sizeof RxFifo.buff) {
		RxFifo.ct = ++i;
		i = RxFifo.wi;
		RxFifo.buff[i] = d;
		RxFifo.wi = (i + 1) % sizeof RxFifo.buff;
	}
}
#endif

#if USE_TXINT
ISR(USART1_UDRE_vect)
{
	idx_t n, i;


	n = TxFifo.ct;
	if (n) {
		TxFifo.ct = --n;
		i = TxFifo.ri;
		UDR1 = TxFifo.buff[i];
		TxFifo.ri = (i + 1) % sizeof TxFifo.buff;
	}
	if (n == 0)
		UCSR1B &= ~_BV(UDRIE1);
}
#endif
