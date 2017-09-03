#include "IR_RX.h"

#include "tim.h"
#include "protocol.h"
#include <stdlib.h>

#define RX_TIMER htim4

void IR_RX_enable(void)
{
  HAL_TIM_IC_Start_IT(&RX_TIMER, TIM_CHANNEL_1);
}

void IR_RX_disable(void)
{
  HAL_TIM_IC_Stop_IT(&RX_TIMER, TIM_CHANNEL_1);
}

uint8_t waveform[256];
uint8_t WF_index;
uint8_t state;

enum rx_state{
  START,
  WAIT_RISING,
  WAIT_FALLING
};

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
  {
    if (WF_index < 255)
    {
      switch(state)
      {
        case START:
          start_blink_state_led();
          state = WAIT_RISING;
          WF_index = 0;
          __HAL_TIM_SET_CAPTUREPOLARITY(&RX_TIMER, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
          __HAL_TIM_CLEAR_IT(&RX_TIMER, TIM_IT_UPDATE);
          __HAL_TIM_SET_COUNTER(&RX_TIMER, 0);
          __HAL_TIM_ENABLE_IT(&RX_TIMER, TIM_IT_UPDATE);
          break;
        case WAIT_RISING:
          state = WAIT_FALLING;
          waveform[WF_index++] = __HAL_TIM_GET_COMPARE(&RX_TIMER, TIM_CHANNEL_1);
          __HAL_TIM_SET_CAPTUREPOLARITY(&RX_TIMER, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
          __HAL_TIM_SET_COUNTER(&RX_TIMER, 0);
          break;
        case WAIT_FALLING:
          state = WAIT_RISING;
          waveform[WF_index++] = __HAL_TIM_GET_COMPARE(&RX_TIMER, TIM_CHANNEL_1);
          __HAL_TIM_SET_CAPTUREPOLARITY(&RX_TIMER, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
          __HAL_TIM_SET_COUNTER(&RX_TIMER, 0);
          break;
      }
    }
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  __HAL_TIM_DISABLE_IT(&RX_TIMER, TIM_IT_UPDATE);
  
  if (WF_index > 10)
  {
     IR_RX_disable();
    
    //IR_RX_flag = 1;
    report_receive_ir(waveform, WF_index);
    IR_stop_learning();
  }
  
  state = START;
}
