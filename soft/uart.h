#ifndef UART_DEFINED
#define UART_DEFINED

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

void uart_init(void);		/* Initialize UART and Flush FIFOs */
void uart_deinit(void);		/* Deinitialize UART */
uint8_t uart_get (void);	/* Get a byte from UART Rx FIFO */
uint8_t uart_test(void);	/* Check number of data in UART Rx FIFO */
void uart_put (uint8_t);	/* Put a byte into UART Tx FIFO */

#endif
