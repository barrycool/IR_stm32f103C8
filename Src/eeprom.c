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
#include "upgrade.h"

uint8_t eeprom_flush_flag;
uint32_t eeprom_flush_timer_cnt;

HAL_StatusTypeDef flash_erase(uint32_t addr, uint32_t size)
{
  uint32_t PageError;
  FLASH_EraseInitTypeDef def;
  HAL_StatusTypeDef status = HAL_ERROR;
  
  HAL_FLASH_Unlock();
  
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);
  
  def.TypeErase = FLASH_TYPEERASE_PAGES;
  def.PageAddress = addr;
  def.NbPages = size / FLASH_PAGE_SIZE;
  if (def.NbPages * FLASH_PAGE_SIZE < size)
  {
    def.NbPages += 1;
  }
  def.Banks = FLASH_BANK_1;
  status = HAL_FLASHEx_Erase(&def, &PageError);
  
  HAL_FLASH_Lock();
  
  return status;
}

HAL_StatusTypeDef flash_write(uint32_t addr, uint16_t * data, uint32_t data_len)
{
  uint16_t i;
  HAL_StatusTypeDef status = HAL_ERROR;
  
  HAL_FLASH_Unlock();
  
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);

  for (i = 0; i < data_len / 2; i++)
  {
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i * 2, data[i]);
    if (status != HAL_OK)
      break;
  }
  
  HAL_FLASH_Lock();
  
  return status;
}

void eeprom_save_parameter(void)
{
  uint32_t *eeprom_flag = (uint32_t *)APP_PARA_ADDR;
  uint32_t *eeprom_backup_flag = (uint32_t *)APP_PARA_BACKUP_ADDR;
  
  if (*eeprom_flag != 0xFFFFFFFF && *eeprom_backup_flag == 0xFFFFFFFF)
  {
    flash_erase(APP_PARA_ADDR, APP_PARA_SIZE);
    
    flash_write(APP_PARA_BACKUP_ADDR, (uint16_t *)IR_CMD_list, sizeof(struct IR_item_t) * IR_BUFFER_LEN);
  }
  else if (*eeprom_flag == 0xFFFFFFFF && *eeprom_backup_flag != 0xFFFFFFFF)
  {
    flash_erase(APP_PARA_BACKUP_ADDR, APP_PARA_BACKUP_SIZE);
    
    flash_write(APP_PARA_ADDR, (uint16_t *)IR_CMD_list, sizeof(struct IR_item_t) * IR_BUFFER_LEN);
  }
  else
  { 
    flash_erase(APP_PARA_ADDR, APP_PARA_SIZE);
    flash_erase(APP_PARA_BACKUP_ADDR, APP_PARA_BACKUP_SIZE);
    
    flash_write(APP_PARA_ADDR, (uint16_t *)IR_CMD_list, sizeof(struct IR_item_t) * IR_BUFFER_LEN);
  }
}

void eeprom_load_parameter(void)
{
  uint32_t *eeprom_flag = (uint32_t *)APP_PARA_ADDR;
  uint32_t *eeprom_backup_flag = (uint32_t *)APP_PARA_BACKUP_ADDR;
  
  if (*eeprom_flag == 0xFFFFFFFF && *eeprom_backup_flag != 0xFFFFFFFF)
  {
    memcpy(IR_CMD_list, eeprom_backup_flag, sizeof(struct IR_item_t) * IR_BUFFER_LEN);
  }
  else
  {
    memcpy(IR_CMD_list, eeprom_flag, sizeof(struct IR_item_t) * IR_BUFFER_LEN);
  }
}

void eeprom_decrease(void)
{
  if (eeprom_flush_timer_cnt)
    eeprom_flush_timer_cnt--;
}

void eeprom_loop(void)
{ 
  if (eeprom_flush_flag && eeprom_flush_timer_cnt == 0)
  {
    eeprom_save_parameter();
    eeprom_flush_flag = 0;
    ir_sending_index = 0;
  }
}

void eeprom_flush(void)
{
  eeprom_flush_flag = 1;
  eeprom_flush_timer_cnt = 500;
}
