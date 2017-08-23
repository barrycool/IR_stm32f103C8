#ifndef IR_NEC_H
#define IR_NEC_H

#include "stm32f1xx_hal.h"

#define NEC_FREQ 38000

#define NEC_TX_LSB_FIRST 1

#define NEC_START_BIT_HIGH  90
#define NEC_START_BIT_LOW  45

#define NEC_BIT1_HIGH  6
#define NEC_BIT1_LOW  17

#define NEC_BIT0_HIGH  6
#define NEC_BIT0_LOW  6

#define NEC_ADDR_LEN 8
#define NEC_CMD_LEN 8

struct IR_NEC_t {
  uint8_t IR_header_high;
  uint8_t IR_address;
  uint8_t IR_address_ext;
  uint8_t IR_command;
  char name[16];
};

void IR_send_NEC(struct IR_NEC_t *IR_NEC);

#endif
