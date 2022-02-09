#ifndef UART1_DEFINED
#define UART1_DEFINED

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

void uart1_init(void);		/* Initialize UART and Flush FIFOs */
void uart1_deinit(void);		/* Deinitialize UART */
uint8_t uart1_get (void);	/* Get a byte from UART Rx FIFO */
uint8_t uart1_test(void);	/* Check number of data in UART Rx FIFO */
void uart1_put (uint8_t);	/* Put a byte into UART Tx FIFO */

#endif
