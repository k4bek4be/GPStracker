/*---------------------------------------------------------*/
/* UART functions for ATmega164A/PA/324A/PA/644A/PA/1284/P */
/* #include-d by uart0.c and uart1.c                       */
/*---------------------------------------------------------*/

#if SZ_FIFO >= 256
typedef uint16_t	idx_t;
#else
typedef uint8_t		idx_t;
#endif


typedef struct {
	idx_t start, len;
	uint8_t buff[SZ_FIFO];
} FIFO;
#if RECEIVE
static
volatile FIFO RxFifo;
#endif

#if USE_TXINT && TRANSMIT
static
volatile FIFO TxFifo;
#endif

static inline void fifo_add(volatile FIFO *fifo, char element) __attribute__((always_inline));
static inline char fifo_get(volatile FIFO *fifo) __attribute__((always_inline));

static inline void fifo_add(volatile FIFO *fifo, char element) {
	unsigned char SREG_buf = SREG;
	idx_t len = fifo->len;
	cli();
	idx_t idx = (fifo->start + len++) % SZ_FIFO;
	fifo->buff[idx] = element;
	if (len > SZ_FIFO) {
		len--;
		fifo->start = (fifo->start + 1) % SZ_FIFO;
	}
	fifo->len = len;
	SREG = SREG_buf;
}

static inline char fifo_get(volatile FIFO *fifo) {
	char ret = 0;
	unsigned char SREG_buf = SREG;
	idx_t start;
	cli();
	if (fifo->len > 0) {
		start = fifo->start;
		ret = fifo->buff[start];
		fifo->start = (start + 1) % SZ_FIFO;
		fifo->len--;
	}
	SREG = SREG_buf;
	return ret;
}

/* Initialize UART */

void _uart_init (void)
{
	_UCSRB = 0;

	_PORT |= _BV(_TX); _DDR |= _BV(_TX);	/* Set TXD as output */
	_DDR &= ~_BV(_RX); _PORT &= ~_BV(_RX); 	/* Set RXD as input */

#if RECEIVE
	RxFifo.len = 0;
#endif
#if USE_TXINT && TRANSMIT
	TxFifo.len = 0;
#endif

	_UBRR = F_CPU / UART_BAUD / 16 - 1;
#if RECEIVE && TRANSMIT
	_UCSRB = _BV(_RXEN) | _BV(_RXCIE) | _BV(_TXEN);
#elif TRANSMIT
	_UCSRB = _BV(_TXEN);
#elif RECEIVE
	_UCSRB = _BV(_RXEN) | _BV(_RXCIE);

#endif
}


/* Deinitialize UART */

void _uart_deinit (void)
{
	_UCSRB = 0;
}


/* Get a received character */
#if RECEIVE
uint8_t _uart_test (void)
{
	return RxFifo.len > 0;
}

uint8_t _uart_get (void)
{
	return fifo_get(&RxFifo);
}
#endif

/* Put a character to transmit */

#if TRANSMIT
void _uart_put (uint8_t d)
{
#if USE_TXINT
	unsigned char SREG_buf;
	idx_t len;
	do {
		SREG_buf = SREG;
		cli();
		len = TxFifo.len;
		SREG = SREG_buf;
	} while (len >= SZ_FIFO);
	fifo_add(&TxFifo, d);
	_UCSRB |= _BV(_UDRIE);
#else
	loop_until_bit_is_set(_UCSRA, _UDRE);
	_UDR = d;
#endif
}
#endif

#if RECEIVE
/* USART1 RXC interrupt */
ISR(_USART_RX_vect)
{
	uint8_t d;

	d = _UDR;
	fifo_add(&RxFifo, d);
}
#endif


#if TRANSMIT && USE_TXINT
ISR(_USART_UDRE_vect)
{
	if (TxFifo.len)
		_UDR = fifo_get(&TxFifo);
	/* TxFifo.len is decremented by fifo_get */
	if (!TxFifo.len)
		_UCSRB &= ~_BV(_UDRIE);
}
#endif

