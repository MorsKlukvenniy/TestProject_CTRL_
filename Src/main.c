/**
  ******************************************************************************
  * @file    FreeRTOS\FreeRTOS_Timers\Src\main.c
  * @author  MCD Application Team
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#define STM32_BUILD
#define FREERTOS_BUILD
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "mpuinit.h"
#include "rtos_lib.h"
#include "sensors.h"
#include "cmsis_os.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
volatile uint8_t UARTTx;
volatile uint8_t UARTRx;
/* Private function prototypes -----------------------------------------------*/
static void timerCallback();
static void UART_RxCallback();
static void UART_TxCallback();
static void UART_Thread();
static void Comand_Thread();
int sensorsTimer;
int uartThread, comandThread;
int uartRxQueue, uartTxQueue;
int dataSemaphore;

static uint8_t temperatures[256];

int main(void)
{

	//!All init
	mpu_init();
	//!Uart init
	uart_init(UART_RxCallback, UART_TxCallback, &UARTRx, &UARTTx);

	//!Timer init
	sensorsTimer = rtos_timer_init(1, timerCallback);
	rtos_timer_start(sensorsTimer, 2000);
	//!Threads init
	comandThread = rtos_thread_init(Comand_Thread, 0, 128);
	uartThread 	 = rtos_thread_init(UART_Thread, 0, 128);

	//!Queues init
	uartRxQueue = rtos_queue_init(10, sizeof(uint8_t));
	uartTxQueue = rtos_queue_init(1024, sizeof(uint8_t));

	dataSemaphore = rtos_semaphore_init();
	/* Start scheduler */
	rtos_start();

	/* We should never get here as control is now taken by the scheduler */
	for (;;);

}

static void UART_Thread()
{

}

static void Comand_Thread()
{

}

static void timerCallback()
{
	if (rtos_semaphore_take(dataSemaphore, 0))
	{
		int i = 0;
		for (i = 0; i < 256; i ++)
		{
			temperatures[i] = get_temperature(i);
		}
		rtos_semaphore_give(dataSemaphore);
	}
}

static void UART_RxCallback()
{
	uint8_t buff = UARTRx;
	rtos_queue_send(uartRxQueue, &buff, 1);
}

static void UART_TxCallback()
{
	uint8_t buff;
	if (rtos_queue_receive(uartTxQueue, &buff, 1))
	{
		UARTTx = buff;
	}
}
