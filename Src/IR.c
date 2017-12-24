#include "IR.h"
#include "comm.h"
#include "tim.h"
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
uint32_t IR_delay_ms_cnt;

volatile uint32_t jiffies;

struct IR_item_t IR_living_CMD;
struct IR_item_t IR_CMD_list[IR_BUFFER_LEN];

uint8_t blink_state_led_flag;
uint32_t blink_state_led_timeout;
void start_blink_state_led(void)
{
  if (blink_state_led_flag == 0)
  {
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    blink_state_led_flag = 1;
  }
  blink_state_led_timeout = 600;
}

void stop_blink_state_led(void)
{
  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
}

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
  ++jiffies;
  
  if (IR_delay_ms_cnt)
    IR_delay_ms_cnt--;
  
  if (blink_state_led_timeout)
    blink_state_led_timeout--;
}

void IR_send_command(struct IR_item_t *IR_item, uint8_t repeate_mode)
{
  IR_is_sending_flag = 1;
  
  start_blink_state_led();
  
  switch (IR_item->IR_type)
  {
  case IR_TYPE_SIRCS:
    IR_send_SIRCS(&IR_item->IR_CMD.IR_SIRCS);
    break;
  case IR_TYPE_NEC:
    IR_send_NEC(&IR_item->IR_CMD.IR_NEC, repeate_mode);
    break;
  case IR_TYPE_RC6:
    IR_send_RC6(&IR_item->IR_CMD.IR_RC6);
    break;
  case IR_TYPE_RC5:
    IR_send_RC5(&IR_item->IR_CMD.IR_RC5);
    break;
  /*case IR_TYPE_JVC:
    IR_send_JVC(&IR_item->IR_CMD.IR_JVC);
    break;*/
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
uint8_t match_flag;
void IR_respon_CMD_list_loop(void)
{
  if (respon_cmd_list_data.flag)
  {
    match_flag = 0;
    
    for (;respon_cmd_list_data.index < IR_BUFFER_LEN; respon_cmd_list_data.index++)
    {
      if (IR_CMD_list[respon_cmd_list_data.index].is_valid == 0x01)
      {
        respon_cmd_list(respon_cmd_list_data.index, &IR_CMD_list[respon_cmd_list_data.index]);
        match_flag = 1;
        break;
      }
    }
    
    if (!match_flag)
    {
      nack_msg(latest_seq_num, READ_CMD_LIST);
    }
    
    respon_cmd_list_data.flag = 0;
  }
}

extern uint8_t eeprom_flush_flag;

volatile uint8_t ir_send_status_flag = 1;
volatile uint8_t ir_learning_status = 1;
uint8_t ir_sending_index;
uint8_t ir_last_sending_index = 0xFF;
uint8_t ir_last_sending_jiffies;

void IR_loop(void)
{
  IR_respon_CMD_list_loop();
  
  if (blink_state_led_flag && blink_state_led_timeout == 0)
  {
    blink_state_led_flag = 0;
    stop_blink_state_led();
  }

  if (IR_living_CMD.is_valid == 0x01)
  {
    IR_send_command(&IR_living_CMD, 0);
    
    IR_living_CMD.is_valid = 0;
    ir_last_sending_index = 0xFF;
    return;
  }
  
  if (IR_delay_ms_cnt)
  {
    return;
  }
  
  if (ir_send_status_flag && ir_learning_status && eeprom_flush_flag == 0 && IR_CMD_list[ir_sending_index].is_valid == 0x01)
  {
    report_sending_cmd(ir_sending_index);
    IR_send_command(&IR_CMD_list[ir_sending_index], (ir_sending_index == ir_last_sending_index) && (jiffies - ir_last_sending_jiffies < 120));
    IR_delay_ms_cnt = IR_CMD_list[ir_sending_index].delay_time;
    
    ir_last_sending_index = ir_sending_index;
    ir_last_sending_jiffies = jiffies;
  }

  if ((++ir_sending_index) >= IR_BUFFER_LEN)
  {
    ir_sending_index = 0;
  }
}

void IR_send_living_cmd(struct IR_item_t *IR_item)
{
  IR_living_CMD = *IR_item;
  IR_living_CMD.is_valid = 1;
}

void IR_clear_CMD_list(void)
{
  uint8_t i;
  
  for (i = 0; i < IR_BUFFER_LEN; i++)
  {
    IR_CMD_list[i].is_valid = 0;
  }
  
  eeprom_flush();
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
