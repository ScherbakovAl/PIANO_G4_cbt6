/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOF);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOG);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

  /**/
  LL_GPIO_ResetOutputPin(off_GPIO_Port, off_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offC14_GPIO_Port, offC14_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offC15_GPIO_Port, offC15_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offG10_GPIO_Port, offG10_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offA2_GPIO_Port, offA2_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offA4_GPIO_Port, offA4_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offA6_GPIO_Port, offA6_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offB1_GPIO_Port, offB1_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offB2_GPIO_Port, offB2_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offB10_GPIO_Port, offB10_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offB12_GPIO_Port, offB12_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offB15_GPIO_Port, offB15_Pin);

  /**/
  LL_GPIO_ResetOutputPin(TIM_interrupter_GPIO_Port, TIM_interrupter_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offA11_GPIO_Port, offA11_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offA12_GPIO_Port, offA12_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offA15_GPIO_Port, offA15_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offB3_GPIO_Port, offB3_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offB4_GPIO_Port, offB4_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offB5_GPIO_Port, offB5_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offB7_GPIO_Port, offB7_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offB8_GPIO_Port, offB8_Pin);

  /**/
  LL_GPIO_ResetOutputPin(offB9_GPIO_Port, offB9_Pin);

  /**/
  GPIO_InitStruct.Pin = off_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(off_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offC14_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offC14_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offC15_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offC15_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offG10_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offG10_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offA2_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offA2_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offA4_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offA4_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offA6_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offA6_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offB1_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offB1_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offB2_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offB2_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offB10_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offB10_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offB12_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offB12_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offB15_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offB15_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = TIM_interrupter_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(TIM_interrupter_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offA11_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offA11_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offA12_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offA12_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offA15_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offA15_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offB3_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offB3_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offB4_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offB4_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offB5_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offB5_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offB7_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offB7_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offB8_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offB8_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = offB9_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(offB9_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
