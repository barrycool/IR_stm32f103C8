#ifndef IR_RC6_H
#define IR_RC6_H

#include "stm32f1xx_hal.h"

#define RC6_FREQ 36000

#define RC6_BIT1_HIGH_FIRST 1

#define RC6_TX_LSB_FIRST 0

#define RC6_START_BIT_HIGH 27
#define RC6_START_BIT_LOW 9

#define RC6_DATA_LEN 5
#define RC6_DOUBEL_DATA_LEN 9
#define RC6_TRIPLE_DATA_LEN 13

#define RC6_TOGGLE_LEN RC6_DOUBEL_DATA_LEN

#define RC6_MODE_LEN 3
#define RC6_ADDR_LEN 8
#define RC6_CMD_LEN 8

struct IR_RC6_t {
  uint8_t IR_mode : 3;
  uint8_t IR_address;
  uint8_t IR_command;
  char name[16];
};

void IR_send_RC6(struct IR_RC6_t *IR_RC6);

#endif
