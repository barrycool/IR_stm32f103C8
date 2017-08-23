#ifndef IR_JVC_H
#define IR_JVC_H

#include "stm32f1xx_hal.h"

#define JVC_FREQ 38000

#define JVC_TX_LSB_FIRST 1

#define JVC_START_BIT_HIGH  84
#define JVC_START_BIT_LOW  42

#define JVC_BIT1_HIGH  5
#define JVC_BIT1_LOW  16

#define JVC_BIT0_HIGH  5
#define JVC_BIT0_LOW  5

#define JVC_ADDR_LEN 8
#define JVC_CMD_LEN 8

struct IR_JVC_t {
  uint8_t IR_address;
  uint8_t IR_command;
  char name[16];
};

void IR_send_JVC(struct IR_JVC_t *IR_JVC);

#endif
