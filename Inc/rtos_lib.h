/*
 * rtos_lib.h
 *
 *  Created on: Apr 23, 2024
 *      Author: Yury
 *
 *      Библиотека интерфейс RTOS для переносимости кода между разными RTOS
 */

#ifndef RTOS_LIB_H_
#define RTOS_LIB_H_

#define THREDS_MAX 8
#define TIMERS_MAX 4
#define QUEUES_MAX 8
#define SEM_MAX 2

void rtos_start(void);

int rtos_thread_init(void(*thread_func)(void), int priority, int stackSize);

int rtos_queue_init(int queueLength, int itemSize);
int rtos_queue_send(int queue, const void* data, long long timeToWait);
int rtos_queue_receive(int queue, void *data, long long timeToWait);

int rtos_timer_init(int periodic, void(*timerCallBack_func)(void));
void rtos_timer_start(int timer, long long time);

int rtos_semaphore_init();
int rtos_semaphore_take(int semaphore, long long time);
int rtos_semaphore_give(int semaphore);

#endif /* RTOS_LIB_H_ */
