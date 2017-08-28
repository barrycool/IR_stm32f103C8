#ifndef EEPROM
#define EEPROM

#include "stm32f1xx_hal.h"

HAL_StatusTypeDef flash_erase(uint32_t addr, uint32_t size);
HAL_StatusTypeDef flash_write(uint32_t addr, uint16_t * data, uint32_t data_len);
void eeprom_load_parameter(void);
void eeprom_flush(void);
void eeprom_decrease(void);
void eeprom_loop(void);

#endif