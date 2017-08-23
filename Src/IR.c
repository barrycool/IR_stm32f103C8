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

#define TX_TIMER htim3

volatile uint8_t IR_is_sending_flag;
volatile uint32_t IR_delay_ms_cnt;
volatile uint16_t IR_interval_ms_cnt;

#define IR_BUFFER_LEN 20
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

uint8_t flash_flag;
uint32_t flash_timer_cnt;
#define FLASH_ADDR1 (FLASH_BASE + 60 * FLASH_PAGE_SIZE)
#define FLASH_ADDR2 (FLASH_ADDR1 + FLASH_PAGE_SIZE)

void flash_write_data(uint32_t flash_addr, uint32_t *buf, uint16_t buf_len)
{
  uint16_t i;
  
  for (i = 0; i < buf_len; i++)
  {  
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_addr + i * 4, buf[i]);
  }
}

extern void    FLASH_PageErase(uint32_t PageAddress);

void IR_save_to_flash(void)
{
  uint32_t *flash1_flag = (uint32_t *)FLASH_ADDR1;
  uint32_t *flash2_flag = (uint32_t *)FLASH_ADDR2;
  
  FLASH_EraseInitTypeDef def;
  uint32_t PageError;
  
  def.TypeErase = FLASH_TYPEERASE_PAGES;
  def.NbPages = 1;
  def.Banks = FLASH_BANK_1;
  
  __set_PRIMASK(1);
  
  __HAL_RCC_HSI_ENABLE();
  HAL_FLASH_Unlock();
  
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);
  
  if (*flash1_flag != 0xFFFFFFFF && *flash2_flag == 0xFFFFFFFF)
  {
    def.PageAddress = FLASH_ADDR1;
    HAL_FLASHEx_Erase(&def, &PageError);
    //FLASH_PageErase(FLASH_ADDR1);
    
    flash_write_data(FLASH_ADDR2, (uint32_t*)IR_CMD_list, sizeof(IR_CMD_list) / 4);
  }
  else if (*flash1_flag == 0xFFFFFFFF && *flash2_flag != 0xFFFFFFFF)
  {
    //FLASH_PageErase(FLASH_ADDR2);
    
    def.PageAddress = FLASH_ADDR2;
    flash_write_data(FLASH_ADDR1, (uint32_t*)IR_CMD_list, sizeof(IR_CMD_list) / 4);
  }
  else
  {
    def.PageAddress = FLASH_ADDR1;
    HAL_FLASHEx_Erase(&def, &PageError);
    
    def.PageAddress = FLASH_ADDR2;
    HAL_FLASHEx_Erase(&def, &PageError);
    
    //FLASH_PageErase(FLASH_ADDR1);
    //FLASH_PageErase(FLASH_ADDR2);
    
    flash_write_data(FLASH_ADDR1, (uint32_t*)IR_CMD_list, sizeof(IR_CMD_list) / 4);
  }
  
  HAL_FLASH_Lock();
  __HAL_RCC_HSI_DISABLE();
  
  __set_PRIMASK(0);
}

void IR_read_from_flash(void)
{
  uint32_t *flash1_flag = (uint32_t *)FLASH_ADDR1;
  uint32_t *flash2_flag = (uint32_t *)FLASH_ADDR2;
  
  if (*flash1_flag == 0xFFFFFFFF && *flash2_flag != 0xFFFFFFFF)
  {
    memcpy(IR_CMD_list, (uint8_t*)FLASH_ADDR2, sizeof(IR_CMD_list));
  }
  else
  {
    memcpy(IR_CMD_list, (uint8_t*)FLASH_ADDR1, sizeof(IR_CMD_list));
  }
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
  if (flash_timer_cnt)
    flash_timer_cnt--;
  
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
  IR_read_from_flash();
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

uint8_t ir_send_status_flag = 1;
uint8_t ir_index;
void IR_loop(void)
{ 
  IR_respon_CMD_list_loop();
  
  if (flash_flag && flash_timer_cnt == 0)
  {
    IR_save_to_flash();
    flash_flag = 0;
  }
  
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
  
  if (ir_send_status_flag && IR_CMD_list[ir_index].is_valid == 0x01)
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
    
    flash_flag = 1;
    flash_timer_cnt = 500;
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

void IR_pause_send(void)
{
  ir_send_status_flag = 0;
}

void IR_start_send(void)
{
  ir_send_status_flag = 1;
}


