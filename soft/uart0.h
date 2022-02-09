#ifndef UART0_DEFINED
#define UART0_DEFINED

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

void uart0_init(void);		/* Initialize UART and Flush FIFOs */
void uart0_deinit(void);		/* Deinitialize UART */
uint8_t uart0_get (void);	/* Get a byte from UART Rx FIFO */
uint8_t uart0_test(void);	/* Check number of data in UART Rx FIFO */
void uart0_put (uint8_t);	/* Put a byte into UART Tx FIFO */

#endif
