#ifndef UPGRADE_H
#define UPGRADE_H

#include "stm32f1xx_hal.h"

#define FLASH_SIZE (64 * 1024) //(FLASH_BANK1_END - FLASH_BASE + 1)

#define BOOT_LOADER_ADDR FLASH_BASE
#define BOOT_LOADER_SIZE (4 * 1024)
#define APP_ADDR (BOOT_LOADER_ADDR + BOOT_LOADER_SIZE)
#define APP_SIZE ((FLASH_SIZE - BOOT_LOADER_SIZE - APP_PARA_SIZE - APP_PARA_BACKUP_SIZE) / 2)
#define APP_BACKUP_ADDR (APP_ADDR + APP_SIZE)
#define APP_BACKUP_SIZE APP_SIZE
#define APP_PARA_ADDR (APP_BACKUP_ADDR + APP_BACKUP_SIZE)
#define APP_PARA_SIZE (2 * 1024) //(FLASH_PAGE_SIZE * 2)
#define APP_PARA_BACKUP_ADDR (APP_PARA_ADDR + APP_PARA_SIZE)
#define APP_PARA_BACKUP_SIZE  APP_PARA_SIZE

#define UPGRADE_VALID_FLAG 0xA55AA55A

#define PACKET_MAX_SIZE 52

enum upgrade_status_t{
  UPGRADE_INIT = 0x08,
  UPGRADE_CHECK_SUM_ERROR,
  UPGRADE_LENGTH_OUT_OF_RANGE,
  UPGRADE_SUCCESS
};

struct upgrade_data_t {
    uint32_t upgrade_flag;
    uint32_t upgrade_version;
    uint32_t upgrade_fileLength;
    uint32_t upgrade_crc32;
};

uint8_t upgrade_init(void);
uint8_t upgrade_recv_packet(uint16_t packet_index, uint8_t *data, uint8_t data_len);
uint8_t upgrade_finish(void);

#endif