
#include <stdbool.h>
#include <string.h>
#include "stm32f1xx_hal.h"
#include "defines.h"
#include "setup.h"
#include "config.h"


TIM_HandleTypeDef TimHandle;
TIM_HandleTypeDef TimHandle2;
uint32_t timeoutCntGen = TIMEOUT;
uint8_t  timeoutFlgGen = 0;

uint8_t i2cBuffer[2];

extern I2C_HandleTypeDef hi2c2;
extern DMA_HandleTypeDef hdma_i2c2_rx;
extern DMA_HandleTypeDef hdma_i2c2_tx;

#if defined(CONTROL_PPM_LEFT) || defined(CONTROL_PPM_RIGHT)
uint16_t ppm_captured_value[PPM_NUM_CHANNELS + 1] = {500, 500};
uint16_t ppm_captured_value_buffer[PPM_NUM_CHANNELS+1] = {500, 500};
uint32_t ppm_timeout = 0;

bool ppm_valid = true;

void PPM_ISR_Callback(void) {
  // Dummy loop with 16 bit count wrap around
  uint16_t rc_delay = TIM2->CNT;
  TIM2->CNT = 0;

  if (rc_delay > 3000) {
    if (ppm_valid && ppm_count == PPM_NUM_CHANNELS) {
      ppm_timeout = 0;
      timeoutCntGen = 0;
      timeoutFlgGen = 0;
      memcpy(ppm_captured_value, ppm_captured_value_buffer, sizeof(ppm_captured_value));
    }
    ppm_valid = true;
    ppm_count = 0;
  }
  else if (ppm_count < PPM_NUM_CHANNELS && IN_RANGE(rc_delay, 900, 2100)){
    ppm_captured_value_buffer[ppm_count++] = CLAMP(rc_delay, 1000, 2000) - 1000;
  } else {
    ppm_valid = false;
  }
}

// SysTick executes once each ms
void PPM_SysTick_Callback(void) {
  ppm_timeout++;
  // Stop after 500 ms without PPM signal
  if(ppm_timeout > 500) {
    int i;
    for(i = 0; i < PPM_NUM_CHANNELS; i++) {
      ppm_captured_value[i] = 500;
    }
    ppm_timeout = 0;
  }
}

void PPM_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /*Configure GPIO pin : PA3 */
  GPIO_InitStruct.Pin = PPM_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(PPM_PORT, &GPIO_InitStruct);

  __HAL_RCC_TIM2_CLK_ENABLE();
  TimHandle.Instance = TIM2;
  TimHandle.Init.Period = UINT16_MAX;
  TimHandle.Init.Prescaler = (SystemCoreClock/DELAY_TIM_FREQUENCY_US)-1;;
  TimHandle.Init.ClockDivision = 0;
  TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
  HAL_TIM_Base_Init(&TimHandle);

  #if defined(CONTROL_PPM_LEFT)  
  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
  #endif

  #if defined(CONTROL_PPM_RIGHT)  
  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
  #endif

  HAL_TIM_Base_Start(&TimHandle);
}
#endif

