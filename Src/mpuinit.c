/*
 * mpuinit.c
 *
 *  Created on: Apr 23, 2024
 *      Author: Yury
 */

#include "mpuinit.h"
#include <stdlib.h>

static void(*uart_RxCallBack_func)(void);
static void(*uart_TxCallBack_func)(void);

#ifdef STM32_BUILD
#include "stm32f7xx_hal.h"
#include "stm32f723e_discovery.h"

static void MPU_Config(void);
static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);

UART_HandleTypeDef UartHandle;
uint8_t *UARTTx;
uint8_t *UARTRx;
#else
//!Different init functions
#endif


void mpu_init()
{
#ifdef STM32_BUILD
	  /* Configure the MPU attributes */
	  MPU_Config();

	  /* Enable the CPU Cache */
	  CPU_CACHE_Enable();

	  /* STM32F7xx HAL library initialization:
	       - Configure the Flash prefetch
	       - Systick timer is configured by default as source of time base, but user
	         can eventually implement his proper time base source (a general purpose
	         timer for example or other time source), keeping in mind that Time base
	         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
	         handled in milliseconds basis.
	       - Set NVIC Group Priority to 4
	       - Low Level Initialization
	     */
	  HAL_Init();

	  /* Configure the System clock to 216 MHz */
	  SystemClock_Config();
#else
	  //!Different init functions
#endif
}

void uart_init(void (*uart_RxCallBack)(void), void (*uart_TxCallBack)(void), uint8_t *Rx, uint8_t *Tx)
{

#ifdef STM32_BUILD
	  UartHandle.Instance				= UART4;
	  UartHandle.Init.BaudRate			= 115200;
	  UartHandle.Init.Mode 				= UART_MODE_TX_RX;
	  UartHandle.Init.Parity 			= UART_PARITY_NONE;
	  UartHandle.Init.StopBits 			= UART_STOPBITS_1;
	  UartHandle.Init.WordLength 		= UART_WORDLENGTH_8B;
	  UARTTx = Tx;
	  UARTRx = Rx;

	  if(HAL_UART_Transmit_IT(&UartHandle, UARTTx, sizeof(uint8_t)) != HAL_OK)
	  {
		  exit(1);
	  }
	  if(HAL_UART_Receive_IT(&UartHandle, UARTRx, sizeof(uint8_t)) != HAL_OK)
	  {
		  exit(1);
	  }
#else
	  //!Different init functions
#endif
	  uart_RxCallBack_func = uart_RxCallBack;
	  uart_TxCallBack_func = uart_TxCallBack;
}

#ifdef STM32_BUILD

static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef  ret = HAL_OK;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 432;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;

  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }

  /* Activate the OverDrive to reach the 216 MHz Frequency */
  ret = HAL_PWREx_EnableOverDrive();
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
}

static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU as Strongly ordered for not defined regions */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x00;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uart_RxCallBack_func();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	uart_TxCallBack_func();
}

#else
	//!Different init functions
#endif
