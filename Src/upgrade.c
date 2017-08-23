#include "upgrade.h"
#include "crc.h"

struct upgrade_data_t upgrade_data;

static HAL_StatusTypeDef flash_erase(uint32_t addr, uint32_t size)
{
  uint32_t PageError;
  FLASH_EraseInitTypeDef def;
  HAL_StatusTypeDef status = HAL_ERROR;
  
  __disable_interrupt();
  HAL_FLASH_Unlock();
  
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);
  
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
  __enable_interrupt();
  
  return status;
}

static HAL_StatusTypeDef flash_write(uint32_t addr, uint16_t * data, uint32_t data_len)
{
  uint16_t i;
  HAL_StatusTypeDef status = HAL_ERROR;
  
  __disable_interrupt();
  HAL_FLASH_Unlock();
  
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);
  
  for (i = 0; i < data_len / 2; i++)
  {
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i * 2, data[i]);
    if (HAL_OK != status)
      break;
  }
  
  HAL_FLASH_Lock();
  __enable_interrupt();
  
  return status;
}

static HAL_StatusTypeDef set_upgrade_status(enum upgrade_status_t upgrade_status)
{
  HAL_StatusTypeDef status = HAL_ERROR;
  
  upgrade_data.upgrade_status = upgrade_status;
  
  status = flash_erase(BOOT_LOADER_PARA_ADDR, BOOT_LOADER_PARA_SIZE);
  status = flash_write(BOOT_LOADER_PARA_ADDR, (uint16_t *)&upgrade_data, sizeof(struct upgrade_data_t));
  
  return status;
}

uint8_t upgrade_init(void)
{
  if (flash_erase(APP_BACKUP_ADDR, APP_BACKUP_SIZE) != HAL_OK)
    return 0;
  
  return 1;
}

uint8_t upgrade_recv_packet(uint8_t packet_index, uint8_t *data, uint8_t data_len)
{
  if (flash_write(APP_BACKUP_ADDR + packet_index * PACKET_MAX_SIZE, (uint16_t *)data, data_len) != HAL_OK)
    return 0;
  
  return 1;
}


uint8_t upgrade_finish(uint32_t file_length, uint32_t CRC32)
{
  uint32_t crc32;
  
  crc32 = HAL_CRC_Calculate(&hcrc, (void*)APP_BACKUP_ADDR, file_length / 4);
  if (crc32 != CRC32)
    return 0;
    
  upgrade_data.upgrade_flag = UPGRADE_VALID_FLAG;
  upgrade_data.file_length = file_length;
  upgrade_data.CRC32 = CRC32;
  
  if (set_upgrade_status(UPGRADE_INIT) != HAL_OK)
    return 0;
  
  return 1;
}

