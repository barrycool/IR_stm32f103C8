#include "IR.h"
#include "comm.h"
#include "tim.h"
//#include "uart.h"
#include <string.h>
#include "protocol.h"
#include "stm32f1xx_hal.h"
#include "IR_sircs.h"
#include "IR_NEC.h"
#include "IR_RC6.h"
#include "IR_RC5.h"
#include "IR_JVC.h"
#include "IR_learning.h"
#include "eeprom.h"

#define TX_TIMER htim3

volatile uint8_t IR_is_sending_flag;
volatile uint32_t IR_delay_ms_cnt;
volatile uint16_t IR_interval_ms_cnt;

struct IR_item_t IR_living_CMD;

//IR TX
#define IR_DATA_TEST 0
struct IR_item_t IR_CMD_list[IR_BUFFER_LEN] = {
  
#if IR_DATA_TEST == 1
  {
    .IR_CMD.IR_SIRCS = {0x10, 0x01, 0x00, 0x12, {'v', 'o', 'l', '_', '+', '\0'}},
    .IR_type = IR_SONY_SIRCS,
    .is_valid = 1,
    .delay_time = 2000,
  },
  
  {
    .IR_CMD.IR_SIRCS = {0x10, 0x01, 0x00, 0x13, {'v', 'o', 'l', '_', '-', '\0'}},
    .IR_type = IR_SONY_SIRCS,
    .is_valid = 1,
    .delay_time = 2000,
  },
#else
  0
#endif
  
};

void IR_set_carrier_freq(uint32_t freq)
{
  __HAL_TIM_SET_AUTORELOAD(&TX_TIMER, IR_TIMER_CLOCK / freq);
  __HAL_TIM_SET_COMPARE(&TX_TIMER, TIM_CHANNEL_1, TX_TIMER.Init.Period / 3);
}

void enable_ir_tx(void)
{
  HAL_TIM_PWM_Start(&TX_TIMER, TIM_CHANNEL_1);
}

void disable_ir_tx(void)
{
  HAL_TIM_PWM_Stop(&TX_TIMER, TIM_CHANNEL_1);
}

void IR_send_bit_high_first(uint8_t bit_high, uint8_t bit_low)
{
  enable_ir_tx();
  delay_n_0_1_ms(bit_high);
  
  disable_ir_tx();
  delay_n_0_1_ms(bit_low);
}

void IR_send_bit_low_first(uint8_t bit_low, uint8_t bit_high)
{
  disable_ir_tx();
  delay_n_0_1_ms(bit_low);
  
  enable_ir_tx();
  delay_n_0_1_ms(bit_high);
  
  disable_ir_tx();
}

void IR_send_start_bit(uint8_t start_bit_high, uint8_t start_bit_low)
{
  IR_send_bit_high_first(start_bit_high, start_bit_low);
}

void IR_send_bit_data_PWM_PDM(uint8_t LSB_first, uint8_t bit0_high, uint8_t bit0_low , uint8_t bit1_high, uint8_t bit1_low, uint8_t data, uint8_t data_len)
{
  uint8_t i;
  uint8_t bit;

  for (i = 0; i < data_len; i++)
  {
    if (LSB_first)
    {
      bit = data & (1 << i);
    }
    else
    {
      bit = data & (1 << (data_len - 1 - i));
    }
    
    if (bit)
    {
      IR_send_bit_high_first(bit1_high, bit1_low);
    }
    else
    {
      IR_send_bit_high_first(bit0_high, bit0_low);
    }
  }
}

void IR_send_bit_data_BI_PHASE(uint8_t LSB_first, uint8_t bit1_high_first, uint8_t bit_len, uint8_t data, uint8_t data_len)
{
  uint8_t i;
  uint8_t bit;

  for (i = 0; i < data_len; i++)
  {
    if (LSB_first)
    {
      bit = data & (1 << i);
    }
    else
    {
      bit = data & (1 << (data_len - 1 - i));
    }
    
    if (bit)
    {
      if (bit1_high_first)
      {
        IR_send_bit_high_first(bit_len, bit_len);
      }
      else
      {
        IR_send_bit_low_first(bit_len, bit_len);
      }
    }
    else
    {
      if (bit1_high_first)
      {
        IR_send_bit_low_first(bit_len, bit_len);
      }
      else
      {
        IR_send_bit_high_first(bit_len, bit_len);
      }
    }
  }
}

void IR_decrease(void)
{ 
  if (IR_delay_ms_cnt)
    IR_delay_ms_cnt--;
  
  if (IR_interval_ms_cnt)
    IR_interval_ms_cnt--;
}

void IR_send_command(struct IR_item_t *IR_item)
{
  IR_is_sending_flag = 1;
  
  switch (IR_item->IR_type)
  {
  case IR_TYPE_SIRCS:
    IR_send_SIRCS(&IR_item->IR_CMD.IR_SIRCS);
    break;
  case IR_TYPE_NEC:
    IR_send_NEC(&IR_item->IR_CMD.IR_NEC);
    break;
  case IR_TYPE_RC6:
    IR_send_RC6(&IR_item->IR_CMD.IR_RC6);
    break;
  case IR_TYPE_RC5:
    IR_send_RC5(&IR_item->IR_CMD.IR_RC5);
    break;
  case IR_TYPE_JVC:
    IR_send_JVC(&IR_item->IR_CMD.IR_JVC);
    break;
  case IR_TYPE_LEARNING:
    IR_send_learning(&IR_item->IR_CMD.IR_learning);
    break;
  default:
    break;
  }
  
  IR_is_sending_flag = 0;
}

void IR_init(void)
{
  eeprom_load_parameter();
}

struct  {
  uint8_t flag;
  uint8_t index;
}respon_cmd_list_data;


void IR_respon_CMD_list_loop(void)
{
  if (respon_cmd_list_data.flag)
  {
    for (;respon_cmd_list_data.index < IR_BUFFER_LEN; respon_cmd_list_data.index++)
    {
      if (IR_CMD_list[respon_cmd_list_data.index].is_valid == 0x01)
      {
        respon_cmd_list(respon_cmd_list_data.index, &IR_CMD_list[respon_cmd_list_data.index]);
        break;
      }
    }
    
    if (respon_cmd_list_data.index == IR_BUFFER_LEN)
    {
    
    }
    
    respon_cmd_list_data.flag = 0;
  }
}

volatile uint8_t ir_send_status_flag = 1;
volatile uint8_t ir_learning_status = 1;
uint8_t ir_index;
void IR_loop(void)
{ 
  IR_respon_CMD_list_loop();
  
  if (IR_interval_ms_cnt)
  {
    return;
  }
  
  if (IR_living_CMD.is_valid == 0x01)
  {
    IR_send_command(&IR_living_CMD);
    
    IR_living_CMD.is_valid = 0;
    IR_interval_ms_cnt = 45;
    return;
  }
  
  if (IR_delay_ms_cnt)
  {
    return;
  }
  
  if (ir_send_status_flag && ir_learning_status && IR_CMD_list[ir_index].is_valid == 0x01)
  { 
    IR_send_command(&IR_CMD_list[ir_index]);
    IR_delay_ms_cnt = IR_CMD_list[ir_index].delay_time;
    IR_interval_ms_cnt = 45;
  }

  if ((++ir_index) >= IR_BUFFER_LEN)
  {
    ir_index = 0;
  }
}

void IR_send_living_cmd(struct IR_item_t *IR_item)
{
  if (IR_item->is_valid)
    IR_living_CMD = *IR_item;
}

void IR_clear_CMD_list(void)
{
  uint8_t i;
  
  for (i = 0; i < IR_BUFFER_LEN; i++)
  {
    IR_CMD_list[i].is_valid = 0;
  }
}

void IR_set_CMD_list(uint8_t index, struct IR_item_t *IR_item)
{
  if (index < IR_BUFFER_LEN)
  {
    memcpy(&IR_CMD_list[index], IR_item, sizeof(struct IR_item_t));
    eeprom_flush();
  }
}

void IR_send_CMD_list(void)
{
  respon_cmd_list_data.flag = 1;
  respon_cmd_list_data.index = 0;
}

void IR_send_next_CMD(void)
{
  if (respon_cmd_list_data.index < IR_BUFFER_LEN)
  {
    respon_cmd_list_data.flag = 1;
    respon_cmd_list_data.index++;
  }
}

void IR_stop_learning(void)
{
  ir_learning_status = 1;
}

void IR_start_learning(void)
{
  ir_learning_status = 0;
}

void IR_pause_send(void)
{
  ir_send_status_flag = 0;
}

void IR_start_send(void)
{
  ir_send_status_flag = 1;
}


