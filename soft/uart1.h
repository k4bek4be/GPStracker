#ifndef UART1_DEFINED
#define UART1_DEFINED

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define _uart_init	uart1_init
#define _uart_deinit	uart1_deinit
#define _uart_get	uart1_get
#define _uart_test	uart1_test
#define _uart_put	uart1_put

void _uart_init (void);		/* Initialize UART and Flush FIFOs */
void _uart_deinit (void);		/* Deinitialize UART */
uint8_t _uart_get (void);	/* Get a byte from UART Rx FIFO */
uint8_t _uart_test (void);	/* Check number of data in UART Rx FIFO */
void _uart_put (uint8_t);	/* Put a byte into UART Tx FIFO */

#endif

