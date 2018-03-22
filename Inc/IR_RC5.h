#ifndef IR_RC5_H
#define IR_RC5_H

#include "stm32f1xx_hal.h"

#define RC5_FREQ 36000

#define RC5_BIT1_HIGH_FIRST 0

#define RC5_TX_LSB_FIRST 0

#define RC5_START_BIT_HIGH 9
#define RC5_START_BIT_LOW 9

#define RC5_DATA_LEN 9
#define RC5_DOUBLE_DATA_LEN 18
#define RC5_TOGGLE_LEN RC5_DATA_LEN

#define RC5_ADDR_LEN 5
#define RC5_CMD_LEN 6

struct IR_RC5_t {
  uint8_t IR_address;
  uint8_t IR_command;
  char name[16];
};

void IR_send_RC5(struct IR_RC5_t *IR_RC5);

#endif
