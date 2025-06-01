/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "stm32g4xx_ll_comp.h"
#include "stm32g4xx_ll_exti.h"
#include "stm32g4xx_ll_dac.h"
#include "stm32g4xx_ll_dma.h"
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_crs.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_cortex.h"
#include "stm32g4xx_ll_utils.h"
#include "stm32g4xx_ll_pwr.h"
#include "stm32g4xx_ll_spi.h"
#include "stm32g4xx_ll_tim.h"
#include "stm32g4xx_ll_usart.h"
#include "stm32g4xx_ll_gpio.h"

#if defined(USE_FULL_ASSERT)
#include "stm32_assert.h"
#endif /* USE_FULL_ASSERT */

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
void g4(void);
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define off_Pin LL_GPIO_PIN_13
#define off_GPIO_Port GPIOC
#define offC14_Pin LL_GPIO_PIN_14
#define offC14_GPIO_Port GPIOC
#define offC15_Pin LL_GPIO_PIN_15
#define offC15_GPIO_Port GPIOC
#define offG10_Pin LL_GPIO_PIN_10
#define offG10_GPIO_Port GPIOG
#define offA2_Pin LL_GPIO_PIN_2
#define offA2_GPIO_Port GPIOA
#define offA4_Pin LL_GPIO_PIN_4
#define offA4_GPIO_Port GPIOA
#define offA6_Pin LL_GPIO_PIN_6
#define offA6_GPIO_Port GPIOA
#define offB1_Pin LL_GPIO_PIN_1
#define offB1_GPIO_Port GPIOB
#define offB2_Pin LL_GPIO_PIN_2
#define offB2_GPIO_Port GPIOB
#define offB10_Pin LL_GPIO_PIN_10
#define offB10_GPIO_Port GPIOB
#define offB12_Pin LL_GPIO_PIN_12
#define offB12_GPIO_Port GPIOB
#define offB15_Pin LL_GPIO_PIN_15
#define offB15_GPIO_Port GPIOB
#define TIM_interrupter_Pin LL_GPIO_PIN_8
#define TIM_interrupter_GPIO_Port GPIOA
#define offA11_Pin LL_GPIO_PIN_11
#define offA11_GPIO_Port GPIOA
#define offA12_Pin LL_GPIO_PIN_12
#define offA12_GPIO_Port GPIOA
#define offA15_Pin LL_GPIO_PIN_15
#define offA15_GPIO_Port GPIOA
#define offB3_Pin LL_GPIO_PIN_3
#define offB3_GPIO_Port GPIOB
#define offB4_Pin LL_GPIO_PIN_4
#define offB4_GPIO_Port GPIOB
#define offB5_Pin LL_GPIO_PIN_5
#define offB5_GPIO_Port GPIOB
#define offB7_Pin LL_GPIO_PIN_7
#define offB7_GPIO_Port GPIOB
#define offB8_Pin LL_GPIO_PIN_8
#define offB8_GPIO_Port GPIOB
#define offB9_Pin LL_GPIO_PIN_9
#define offB9_GPIO_Port GPIOB
#ifndef NVIC_PRIORITYGROUP_0
#define NVIC_PRIORITYGROUP_0         ((uint32_t)0x00000007) /*!< 0 bit  for pre-emption priority,
                                                                 4 bits for subpriority */
#define NVIC_PRIORITYGROUP_1         ((uint32_t)0x00000006) /*!< 1 bit  for pre-emption priority,
                                                                 3 bits for subpriority */
#define NVIC_PRIORITYGROUP_2         ((uint32_t)0x00000005) /*!< 2 bits for pre-emption priority,
                                                                 2 bits for subpriority */
#define NVIC_PRIORITYGROUP_3         ((uint32_t)0x00000004) /*!< 3 bits for pre-emption priority,
                                                                 1 bit  for subpriority */
#define NVIC_PRIORITYGROUP_4         ((uint32_t)0x00000003) /*!< 4 bits for pre-emption priority,
                                                                 0 bit  for subpriority */
#endif

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
