#include "upgrade.h"
#include "crc.h"
#include "eeprom.h"
#include "string.h"
#include "protocol.h"

struct upgrade_data_t upgrade_data;

static void set_upgrade_status(enum upgrade_status_t upgrade_status)
{
  FLASH_OBProgramInitTypeDef OBInit;
  
  HAL_FLASHEx_OBGetConfig(&OBInit);
  OBInit.OptionType |= OPTIONBYTE_DATA;
  OBInit.DATAAddress = OB_DATA_ADDRESS_DATA0;
  OBInit.DATAData = upgrade_status;
  
  HAL_FLASH_Unlock();
  HAL_FLASH_OB_Unlock();
  
  HAL_FLASHEx_OBErase();
  HAL_FLASHEx_OBProgram(&OBInit);
  
  HAL_FLASH_OB_Lock();
  HAL_FLASH_Lock();
  
  HAL_Delay(500);
  
  __set_FAULTMASK(1);
  SCB->VTOR = BOOT_LOADER_ADDR;
  HAL_FLASH_OB_Launch();
}

uint8_t upgrade_init(void)
{
  if (flash_erase(APP_BACKUP_ADDR, APP_BACKUP_SIZE) != HAL_OK)
    return 0;
  
  return 1;
}

uint8_t upgrade_recv_packet(uint16_t packet_index, uint8_t *data, uint8_t data_len)
{
  if (flash_write(APP_BACKUP_ADDR + packet_index * PACKET_MAX_SIZE, (uint16_t *)data, data_len) != HAL_OK)
    return 0;
  
  return 1;
}

uint8_t upgrade_finish(void)
{
  uint32_t crc32;
  uint32_t *app_backup_end_addr = (uint32_t *)APP_PARA_ADDR;
  uint32_t *app_backup_start_addr = (uint32_t *)APP_BACKUP_ADDR;
  
  do{
    app_backup_end_addr--;
    if (*app_backup_end_addr == UPGRADE_VALID_FLAG)
    {
      break;
    }
  }while (app_backup_end_addr > app_backup_start_addr);
  
  memcpy(&upgrade_data, app_backup_end_addr, sizeof(struct upgrade_data_t));
    
  crc32 = HAL_CRC_Calculate(&hcrc, (void*)APP_BACKUP_ADDR, upgrade_data.upgrade_fileLength / 4);
  if (crc32 != upgrade_data.upgrade_crc32)
  {
    nack_msg(latest_seq_num, UPGRADE_FINISH);
    return 0;
  }
  
  ack_msg(latest_seq_num, UPGRADE_FINISH);
  
  set_upgrade_status(UPGRADE_INIT);
  
  return 1;
}

