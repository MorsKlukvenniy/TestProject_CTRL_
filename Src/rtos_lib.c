/*
 * rtos_lib.c
 *
 *  Created on: Apr 23, 2024
 *      Author: Yury
 */

#include "rtos_lib.h"

#ifdef FREERTOS_BUILD
#include "cmsis_os.h"
osThreadId 		threads_id[THREDS_MAX];
osTimerDef_t	timers_def[TIMERS_MAX];
osTimerId		timers_id[TIMERS_MAX];
QueueHandle_t 	queues_id[QUEUES_MAX];
SemaphoreHandle_t sem_id[SEM_MAX];
#else
struct k_thread threads_id[THREDS_MAX];
struct k_msgq	queues_id[QUEUES_MAX];
struct k_timer	timers_id[TIMERS_MAX];
struct  k_mutex sem_id[SEM_MAX];
//!Different init functions
#endif

static int threads_count = 0;
static int timers_count = 0;
static int queues_count = 0;
static int sem_count = 0;

void rtos_start(void)
{
#ifdef FREERTOS_BUILD
	osKernelStart();
#else
	int i = 0;
	for (i = 0; i < threads_count; i++)
	{
		k_thread_start(&threads_id[i]);
	}
#endif
}

int rtos_thread_init(void(*thread_func)(const void*), int priority, int stackSize)
{
	if (threads_count < THREDS_MAX)
	{
#ifdef FREERTOS_BUILD
		osThreadDef(threads_count, thread_func, priority, 0, stackSize);
		threads_id[threads_count] = osThreadCreate(osThread(threads_count), NULL);
#else
		k_thread_create(&threads_id[threads_count], k_thread_stack_alloc(stackSize), stackSize, thread_func, NULL, NULL, NULL, priority, 0, K_NO_WAIT);
#endif
	return threads_count++;
	}
	else
	{
		return 0;
	}
}


int rtos_queue_init(int queueLength, int itemSize)
{
	if (queues_count < QUEUES_MAX)
	{
#ifdef FREERTOS_BUILD
		queues_id[queues_count] = xQueueCreate(queueLength, itemSize);
#else
		k_msgq_alloc_init(&queues_id[queues_count], itemSize, queueLength);
#endif
	return queues_count++;
	}
	else
	{
		return 0;
	}
}
int rtos_queue_send(int queue, const void* data, long long timeToWait)
{
	if (queue <= queues_count)
	{
#ifdef FREERTOS_BUILD
		if (xQueueSend(queues_id[queue], data, timeToWait < 0 ? portMAX_DELAY : portTICK_PERIOD_MS * timeToWait) == pdTRUE)
			return 1;
#else
		if (!k_msgq_put(&queues_id[queue], data, timeToWait < 0 ? K_FOREVER : K_MSEC(timeToWait)))
			return 1;
#endif
		return 0;
	}
	else
	{
		return 0;
	}
}
int rtos_queue_receive(int queue, void *data, long long timeToWait)
{
	if (queue <= queues_count)
	{
#ifdef FREERTOS_BUILD
		if (xQueueReceive(queues_id[queue], data, timeToWait < 0 ? portMAX_DELAY : portTICK_PERIOD_MS * timeToWait) == pdTRUE)
			return 1;
#else
		if (!k_msgq_get(&queues_id[queue], data, timeToWait < 0 ? K_FOREVER : K_MSEC(timeToWait)))
			return 1;
#endif
		return 0;
	}
	else
	{
		return 0;
	}
}
int rtos_timer_init(int periodic, void(*timerCallBack_func)(const void*))
{
	if (timers_count < TIMERS_MAX)
	{
#ifdef FREERTOS_BUILD
	timers_def[timers_count].ptimer = timerCallBack_func;
	timers_id[timers_count] = osTimerCreate(&timers_def[timers_count], periodic, NULL);
#else
	k_timer_init(&timers_id[timers_count], timerCallBack_func, NULL);
#endif
	return timers_count++;
	}
	else
	{
		return 0;
	}
}
void rtos_timer_start(int timer, long long time)
{
	if (timer <= timers_count)
	{
#ifdef FREERTOS_BUILD
	osTimerStart(timers_id[timer], portTICK_PERIOD_MS * time);
#else
	k_timer_start(&timers_id[timer], K_MSEC(timeToWait), 0);
#endif
	}
}

int rtos_semaphore_init()
{
	if (sem_count < SEM_MAX)
	{
#ifdef FREERTOS_BUILD
		sem_id[sem_count] = xSemaphoreCreateMutex();
#else
		if (k_mutex_init(&sem_id[sem_count]))
			return 0;
#endif
	return sem_count++;
	}
	else
	{
		return 0;
	}
}
int rtos_semaphore_take(int semaphore, long long time)
{
	if (semaphore < sem_count)
	{
#ifdef FREERTOS_BUILD
		if (xSemaphoreTake(sem_id[semaphore], time < 0 ? portMAX_DELAY : portTICK_PERIOD_MS * time) == pdTRUE)
			return 1;
#else
		if (!k_mutex_lock(&sem_id[semaphore]), time < 0 ? K_FOREVER : K_MSEC(time))
			return 1;
#endif
		return 0;
	}
	else
	{
		return 0;
	}
}
int rtos_semaphore_give(int semaphore)
{
	if (semaphore < sem_count)
	{
#ifdef FREERTOS_BUILD
		if (xSemaphoreGive(sem_id[semaphore]) == pdTRUE)
			return 1;
#else
		if (!k_mutex_unlock(&sem_id[semaphore]))
			return 1;
#endif
		return 0;
	}
	else
	{
		return 0;
	}
}

