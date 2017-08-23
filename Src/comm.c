#include "comm.h"
#include "tim.h"

/*
int fputc(int ch, FILE *f)
{
  while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
  
  USART_SendData(USART1, (uint16_t)ch);
  
  return ch;
}


void delay_init(void)
{
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
}*/

void delay_0_1_ms(void)
{ 
  __HAL_TIM_SET_COUNTER(&htim1, 0);
  
  __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
  
  __HAL_TIM_ENABLE(&htim1);
  
  while(!__HAL_TIM_GET_FLAG(&htim1, TIM_FLAG_UPDATE));
  
  __HAL_TIM_DISABLE(&htim1);
}

void delay_n_0_1_ms(uint8_t time)
{
  uint8_t i;
  
  for (i = 0; i < time; i++)
  {
    delay_0_1_ms();
  }
}

void delay_1ms(void)
{
  delay_n_0_1_ms(10);
}

void delay_ms(uint32_t time)
{
  uint32_t i;
  
  for (i = 0; i< time; i++)
  {
    delay_1ms();
  }
}



