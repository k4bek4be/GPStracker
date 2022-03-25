#ifndef UART0_DEFINED
#define UART0_DEFINED

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define _uart_init	uart0_init
#define _uart_deinit	uart0_deinit
#define _uart_get	uart0_get
#define _uart_test	uart0_test
#define _uart_put	uart0_put

void _uart_init (void);		/* Initialize UART and Flush FIFOs */
void _uart_deinit (void);		/* Deinitialize UART */
uint8_t _uart_get (void);	/* Get a byte from UART Rx FIFO */
uint8_t _uart_test (void);	/* Check number of data in UART Rx FIFO */
void _uart_put (uint8_t);	/* Put a byte into UART Tx FIFO */

#endif

