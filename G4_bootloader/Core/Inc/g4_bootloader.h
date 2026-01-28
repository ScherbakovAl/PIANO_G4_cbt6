#pragma once

#include "stm32g4xx_hal.h"

// #include "stm32g4xx_ll_rcc.h"
// #include "stm32g4xx_ll_bus.h"
// #include "stm32g4xx_ll_crs.h"
// #include "stm32g4xx_ll_system.h"
// #include "stm32g4xx_ll_exti.h"
// #include "stm32g4xx_ll_cortex.h"
// #include "stm32g4xx_ll_utils.h"
// #include "stm32g4xx_ll_pwr.h"
// #include "stm32g4xx_ll_dma.h"
// #include "stm32g4xx_ll_usart.h"
// #include "stm32g4xx_ll_gpio.h"


#ifdef __cplusplus
extern "C" {
#endif

void Unlock();
void Lock();
void Erase();
void Read();
void Flash(uint32_t Address, uint64_t Data);

#ifdef __cplusplus
}
#endif