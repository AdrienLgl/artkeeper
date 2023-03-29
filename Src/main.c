/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <getch.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

char* CONF_PASSWORD = "password";
uint16_t brightness_ref;
uint8_t first_brightness_mesure = 0;
uint16_t pressure_ref;
uint8_t first_pressure_mesure = 0;
uint16_t humidity_ref;
uint8_t first_humidity_mesure = 0;
uint64_t tick_1000ms_elapsed;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


uint8_t system_is_disabled() {
	if (!LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_2) && !LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_8)) {
		return 1;
	} else if (LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_2)) {
		return 1;
	} else {
		return 0;
	}
}

void reset_measure() { // reset measure
	first_brightness_mesure = 0;
	first_humidity_mesure = 0;
	first_pressure_mesure = 0;
}

void enable_system() { // enable system
	reset_measure();
	// check if led are active or not
	if ((!LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_2) && !LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_8)) || LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_2)) {
		printf("Système armé\r\n"); //print system enabling
			LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_2);
			LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_8);
			// enable alarm (3 bip)
			for (int i=0; i<3; i++) {
				LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_0); // set
				LL_mDelay(200); // 500ms delay
				LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_0); // reset
				LL_mDelay(200); // 500ms delay
			}
	} else {
		printf("Votre système est déjà armé\r\n");
	}

}

void disable_system() { // disable system
	if ((!LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_2) && !LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_8)) || LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_8)) {
		printf("Désactivation du système\r\n"); //print system disabling
		LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_2);
		LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_8);
		// disable alarm (2 bip)
		for (int i=0; i<2; i++) {
			LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_0); // set
			LL_mDelay(200); // 500ms delay
			LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_0); // reset
			LL_mDelay(200); // 500ms delay
		}
	} else {
		printf("Votre système est déjà désarmé\r\n");
	}
}

void launch_alert() {
	// start alarm
	while (1) {
		printf("ALERTE!! Système compromis\r\n");
		LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_0); // set
		LL_mDelay(200); // 500ms delay
		LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_0); // reset
		LL_mDelay(200); // 500ms delay
		if (kbhit()) {
			// stop alarm
			if (getch() == 's') {
				printf("Alerte désactivée\r\n");
				disable_system();
				break;
			}
		}
	}
}

void check_values(uint16_t ref, uint16_t current_value, uint16_t threshold) {
	// compare values with references
	uint16_t check = ref - current_value;
	if (check == threshold) {
	  launch_alert();
	}
}

void check_brightness(uint16_t threshold) {
	// check brightness level
	if (system_is_disabled() == 0) {
		// launch conversion
		LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_11);
		LL_ADC_REG_StartConversion(ADC1);
		// wait
		while(!LL_ADC_IsActiveFlag_EOC(ADC1));
		int t = LL_ADC_REG_ReadConversionData12(ADC1);
		if (first_brightness_mesure == 0) {
		  brightness_ref = t;
		  first_brightness_mesure = 1;
		}
		// compare values
		check_values(brightness_ref, t, threshold);
		printf("Brightness:%i\r\n\n", t);
	}
}

void check_pressure(uint16_t threshold) {
	// check pressure level
	if (system_is_disabled() == 0) {
		LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_13);
		LL_ADC_REG_StartConversion(ADC1);
		while(!LL_ADC_IsActiveFlag_EOC(ADC1));
		int p = LL_ADC_REG_ReadConversionData12(ADC1);
		if (first_pressure_mesure == 0) {
		  pressure_ref = p;
		  first_pressure_mesure = 1;
		}
		check_values(pressure_ref, p, threshold);
		printf("Pressure:%i\r\n\n", p);
	}
}

void check_humidity(uint16_t threshold) {
	// check humidity level
	if (system_is_disabled() == 0) {
		LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_15);
		LL_ADC_REG_StartConversion(ADC1);
		while(!LL_ADC_IsActiveFlag_EOC(ADC1));
		int h = LL_ADC_REG_ReadConversionData12(ADC1);
		if (first_humidity_mesure == 0) {
		  humidity_ref = h;
		  first_humidity_mesure = 1;
		}
		check_values(humidity_ref, h, threshold);
		printf("Humidity:%i\r\n\n", h);
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC_Init();
  /* USER CODE BEGIN 2 */
  getchInit();
  LL_USART_EnableIT_RXNE(USART2);
  printf("Ready !!!!\r\n"); //print it

  LL_ADC_Enable(ADC1);
  LL_SYSTICK_EnableIT();
  LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1),
  LL_ADC_PATH_INTERNAL_TEMPSENSOR|LL_ADC_PATH_INTERNAL_VREFINT);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	if (tick_1000ms_elapsed == 1) {
		// each seconds
		check_humidity(5000);
		check_brightness(5000);
		check_pressure(5000);
		// reset tick
		tick_1000ms_elapsed = 0;
	}
	if (kbhit()) //if one char received on USART2
	{
	  switch(getch()) {
	  case '1': // enable
		  enable_system();
		  break;
	  case '0': // disable
		  disable_system();
		  break;
	  case 'a': // launch alert
		  launch_alert();
		  break;
	  }
	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_0)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {

  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {

  }

  LL_Init1msTick(16000000);

  LL_SetSystemCoreClock(16000000);
  LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);
}

/* USER CODE BEGIN 4 */
int _write(int file,char * ptr, int len)
{
  for (int i=0; i<len;i++)
  {
    while(LL_USART_IsActiveFlag_TXE(USART2)==0);
    LL_USART_TransmitData8(USART2,*ptr);
    ptr++;
  }
return len;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
