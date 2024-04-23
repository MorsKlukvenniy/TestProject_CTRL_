/*
 * mpuinit.h
 *
 *  Created on: Apr 23, 2024
 *      Author: Yury
 */

#ifndef MPUINIT_H_
#define MPUINIT_H_

#include <stdint.h>
void mpu_init(void);
void uart_init(void (*uart_RxCallBack)(void), void (*uart_TxCallBack)(void), volatile uint8_t *Rx, volatile uint8_t *Tx);

#endif /* MPUINIT_H_ */
