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
static uint8_t messType = 0;
static int8_t temperatures[256];
/* Private function prototypes -----------------------------------------------*/
static void timerCallback();
static void UART_RxCallback();
static void UART_TxCallback();
static void UART_Thread();
static void COMMAND_Thread();
int sensorsTimer;
int uartThread, COMMANDThread;
int uartRxQueue, uartTxQueue, messageQueue;
int dataSemaphore; //!Для контроля доступа к массиву температур на чтение для отправки и запись по таймеру

//!Перчесиление команд
enum
{
	NO_COMMAND,
	TOGGLE_COMMAND,
	READ_COMMAND,
	MAX_COMMAND
}COMMAND_enum;

//!Референсные значения входных команд
static char COMMANDs[MAX_COMMAND][7] =
{
		"\0\0\0\0\0\0\0",
		"toggle\n",
		"read\n\0\0"
};

//!Типы ответных сообщений
enum
{
	MESS_BYTE,	//!Отправляю просто 256 значений температур типа int8_t
	MESS_CHAR	//!Отправляю 256 строчек по 4 символа со значениями температур типа char[4] ("-012", "+145")
}MESS_enum;

int main(void)
{

	//!All init
	mpu_init();
	//!Uart init
	uart_init(UART_RxCallback, UART_TxCallback, &UARTRx, &UARTTx);

	//!Timer init
	sensorsTimer = rtos_timer_init(1, timerCallback);
	rtos_timer_start(sensorsTimer, 1000); //!Опрос датчиков раз в секунду
	//!Threads init
	COMMANDThread = rtos_thread_init(COMMAND_Thread, 0, 128);
	uartThread 	 = rtos_thread_init(UART_Thread, 0, 128);

	//!Queues init
	uartRxQueue = rtos_queue_init(10, sizeof(uint8_t));
	uartTxQueue = rtos_queue_init(1024, sizeof(uint8_t));
	messageQueue = rtos_queue_init(5, sizeof(uint8_t));

	dataSemaphore = rtos_semaphore_init();
	/* Start scheduler */
	rtos_start();

	for (;;);

}

//! Процесс создания сообщения для отправки по UART в заданном формате
static void UART_Thread()
{
	uint8_t buff;
	while (1)
	{
		//! Использую очередь как евент(флаг), значение буфера не имеет значения
		if (rtos_queue_receive(messageQueue, &buff, 0))
		{
			if (rtos_semaphore_take(dataSemaphore, 0))
			{
				if (messType == MESS_BYTE)
				{
					int i = 0;
					for (i = 0; i < 256; i++)
					{
						rtos_queue_send(uartTxQueue, &temperatures[i], 0);
					}
				}
				else
				{
					int i = 0;
					for (i = 0; i < 256; i++)
					{
						int8_t temp_t = temperatures[i];
						char t[4];
						if (temp_t < 0)
						{
							t[0] = '-';
							temp_t = -temp_t;
						}
						else
						{
							t[0] = '+';
						}
						t[1] = (char)((temp_t / 100) + 0x30);
						temp_t -= temp_t / 100;
						t[2] = (char)((temp_t / 10) + 0x30);
						temp_t -= temp_t / 10;
						t[3] = (char)(temp_t + 0x30);
						int j = 0;
						for (j = 0; j < 4; j++)
						{
							rtos_queue_send(uartTxQueue, &t[i], 0);
						}
					}
				}
				rtos_semaphore_give(dataSemaphore);
			}
		}
	}
}

//! Процесс обработки входящих команд
static void COMMAND_Thread()
{
	uint8_t buff = 0;
	uint8_t command_status = NO_COMMAND;
	uint8_t command_counter = 0;
	while (1)
	{
		if (rtos_queue_receive(uartRxQueue, &buff, 0))
		{
			//!Чтобы прочитать команды, надо накопить входные символы
			switch (command_status)
			{
				case NO_COMMAND:
					if (COMMANDs[TOGGLE_COMMAND][0] == (char)buff)
					{
						command_counter++;
						command_status = TOGGLE_COMMAND;
					}
					else if (COMMANDs[READ_COMMAND][0] == (char)buff)
					{
						command_counter++;
						command_status = READ_COMMAND;
					}
					break;
				case TOGGLE_COMMAND:
					if (COMMANDs[TOGGLE_COMMAND][command_counter] == '\n' && (char)buff == '\n')
					{
						messType = messType == MESS_BYTE ? MESS_CHAR : MESS_BYTE; // messType = (messType + 1) & 0x1;
						command_counter = 0;
						command_status = NO_COMMAND;
					}
					else if (COMMANDs[TOGGLE_COMMAND][command_counter] == (char)buff)
					{
						command_counter++;
						command_status = TOGGLE_COMMAND;
					}
					else
					{
						command_counter = 0;
						command_status = NO_COMMAND;
					}
					break;
				case READ_COMMAND:
					if (COMMANDs[READ_COMMAND][command_counter] == '\n' && (char)buff == '\n')
					{
						command_counter = 0;
						command_status = NO_COMMAND;
						//! Использую очередь как евент(флаг), значение буфера не имеет значения
						rtos_queue_send(messageQueue, &buff, 0);
					}
					else if (COMMANDs[READ_COMMAND][command_counter] == (char)buff)
					{
						command_counter++;
						command_status = READ_COMMAND;
					}
					else
					{
						command_counter = 0;
						command_status = NO_COMMAND;
					}
					break;

			}
		}
	}
}

//! Обработчик прерывания таймера. Поулчаем значения температур от датчиков
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

//! Обработчик прерывания UART. Получаем команды
static void UART_RxCallback()
{
	uint8_t buff = UARTRx;
	rtos_queue_send(uartRxQueue, &buff, 1);
}

//! Обработчик прерывания UART. Отправляем данные
static void UART_TxCallback()
{
	uint8_t buff = 0;
	if (rtos_queue_receive(uartTxQueue, &buff, 1))
	{
		UARTTx = buff;
	}
}
