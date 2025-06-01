/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dac.c
  * @brief   This file provides code for the configuration
  *          of the DAC instances.
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
#include "dac.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* DAC1 init function */
void MX_DAC1_Init(void)
{

  /* USER CODE BEGIN DAC1_Init 0 */

  /* USER CODE END DAC1_Init 0 */

  LL_DAC_InitTypeDef DAC_InitStruct = {0};

  /* Peripheral clock enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_DAC1);

  /* USER CODE BEGIN DAC1_Init 1 */

  /* USER CODE END DAC1_Init 1 */

  /** DAC channel OUT1 config
  */
  LL_DAC_SetSignedFormat(DAC1, LL_DAC_CHANNEL_1, LL_DAC_SIGNED_FORMAT_DISABLE);
  DAC_InitStruct.TriggerSource = LL_DAC_TRIG_SOFTWARE;
  DAC_InitStruct.TriggerSource2 = LL_DAC_TRIG_SOFTWARE;
  DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
  DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_DISABLE;
  DAC_InitStruct.OutputConnection = LL_DAC_OUTPUT_CONNECT_INTERNAL;
  DAC_InitStruct.OutputMode = LL_DAC_OUTPUT_MODE_NORMAL;
  LL_DAC_Init(DAC1, LL_DAC_CHANNEL_1, &DAC_InitStruct);
  LL_DAC_DisableTrigger(DAC1, LL_DAC_CHANNEL_1);
  LL_DAC_DisableDMADoubleDataMode(DAC1, LL_DAC_CHANNEL_1);

  /** DAC channel OUT2 config
  */
  LL_DAC_SetSignedFormat(DAC1, LL_DAC_CHANNEL_2, LL_DAC_SIGNED_FORMAT_DISABLE);
  LL_DAC_Init(DAC1, LL_DAC_CHANNEL_2, &DAC_InitStruct);
  LL_DAC_DisableTrigger(DAC1, LL_DAC_CHANNEL_2);
  LL_DAC_DisableDMADoubleDataMode(DAC1, LL_DAC_CHANNEL_2);
  /* USER CODE BEGIN DAC1_Init 2 */

  /* USER CODE END DAC1_Init 2 */

}
/* DAC2 init function */
void MX_DAC2_Init(void)
{

  /* USER CODE BEGIN DAC2_Init 0 */

  /* USER CODE END DAC2_Init 0 */

  LL_DAC_InitTypeDef DAC_InitStruct = {0};

  /* Peripheral clock enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_DAC2);

  /* USER CODE BEGIN DAC2_Init 1 */

  /* USER CODE END DAC2_Init 1 */

  /** DAC channel OUT1 config
  */
  LL_DAC_SetSignedFormat(DAC2, LL_DAC_CHANNEL_1, LL_DAC_SIGNED_FORMAT_DISABLE);
  DAC_InitStruct.TriggerSource = LL_DAC_TRIG_SOFTWARE;
  DAC_InitStruct.TriggerSource2 = LL_DAC_TRIG_SOFTWARE;
  DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
  DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_DISABLE;
  DAC_InitStruct.OutputConnection = LL_DAC_OUTPUT_CONNECT_INTERNAL;
  DAC_InitStruct.OutputMode = LL_DAC_OUTPUT_MODE_NORMAL;
  LL_DAC_Init(DAC2, LL_DAC_CHANNEL_1, &DAC_InitStruct);
  LL_DAC_DisableTrigger(DAC2, LL_DAC_CHANNEL_1);
  LL_DAC_DisableDMADoubleDataMode(DAC2, LL_DAC_CHANNEL_1);
  /* USER CODE BEGIN DAC2_Init 2 */

  /* USER CODE END DAC2_Init 2 */

}
/* DAC3 init function */
void MX_DAC3_Init(void)
{

  /* USER CODE BEGIN DAC3_Init 0 */

  /* USER CODE END DAC3_Init 0 */

  LL_DAC_InitTypeDef DAC_InitStruct = {0};

  /* Peripheral clock enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_DAC3);

  /* USER CODE BEGIN DAC3_Init 1 */

  /* USER CODE END DAC3_Init 1 */

  /** DAC channel OUT1 config
  */
  LL_DAC_SetSignedFormat(DAC3, LL_DAC_CHANNEL_1, LL_DAC_SIGNED_FORMAT_DISABLE);
  DAC_InitStruct.TriggerSource = LL_DAC_TRIG_SOFTWARE;
  DAC_InitStruct.TriggerSource2 = LL_DAC_TRIG_SOFTWARE;
  DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
  DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_DISABLE;
  DAC_InitStruct.OutputConnection = LL_DAC_OUTPUT_CONNECT_INTERNAL;
  DAC_InitStruct.OutputMode = LL_DAC_OUTPUT_MODE_NORMAL;
  LL_DAC_Init(DAC3, LL_DAC_CHANNEL_1, &DAC_InitStruct);
  LL_DAC_DisableTrigger(DAC3, LL_DAC_CHANNEL_1);
  LL_DAC_DisableDMADoubleDataMode(DAC3, LL_DAC_CHANNEL_1);

  /** DAC channel OUT2 config
  */
  LL_DAC_SetSignedFormat(DAC3, LL_DAC_CHANNEL_2, LL_DAC_SIGNED_FORMAT_DISABLE);
  LL_DAC_Init(DAC3, LL_DAC_CHANNEL_2, &DAC_InitStruct);
  LL_DAC_DisableTrigger(DAC3, LL_DAC_CHANNEL_2);
  LL_DAC_DisableDMADoubleDataMode(DAC3, LL_DAC_CHANNEL_2);
  /* USER CODE BEGIN DAC3_Init 2 */

  /* USER CODE END DAC3_Init 2 */

}
/* DAC4 init function */
void MX_DAC4_Init(void)
{

  /* USER CODE BEGIN DAC4_Init 0 */

  /* USER CODE END DAC4_Init 0 */

  LL_DAC_InitTypeDef DAC_InitStruct = {0};

  /* Peripheral clock enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_DAC4);

  /* USER CODE BEGIN DAC4_Init 1 */

  /* USER CODE END DAC4_Init 1 */

  /** DAC channel OUT1 config
  */
  LL_DAC_SetSignedFormat(DAC4, LL_DAC_CHANNEL_1, LL_DAC_SIGNED_FORMAT_DISABLE);
  DAC_InitStruct.TriggerSource = LL_DAC_TRIG_SOFTWARE;
  DAC_InitStruct.TriggerSource2 = LL_DAC_TRIG_SOFTWARE;
  DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
  DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_DISABLE;
  DAC_InitStruct.OutputConnection = LL_DAC_OUTPUT_CONNECT_INTERNAL;
  DAC_InitStruct.OutputMode = LL_DAC_OUTPUT_MODE_NORMAL;
  LL_DAC_Init(DAC4, LL_DAC_CHANNEL_1, &DAC_InitStruct);
  LL_DAC_DisableTrigger(DAC4, LL_DAC_CHANNEL_1);
  LL_DAC_DisableDMADoubleDataMode(DAC4, LL_DAC_CHANNEL_1);

  /** DAC channel OUT2 config
  */
  LL_DAC_SetSignedFormat(DAC4, LL_DAC_CHANNEL_2, LL_DAC_SIGNED_FORMAT_DISABLE);
  LL_DAC_Init(DAC4, LL_DAC_CHANNEL_2, &DAC_InitStruct);
  LL_DAC_DisableTrigger(DAC4, LL_DAC_CHANNEL_2);
  LL_DAC_DisableDMADoubleDataMode(DAC4, LL_DAC_CHANNEL_2);
  /* USER CODE BEGIN DAC4_Init 2 */

  /* USER CODE END DAC4_Init 2 */

}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
