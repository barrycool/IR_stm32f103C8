#ifndef IR_SIRCS_H
#define IR_SIRCS_H

#include "stm32f1xx_hal.h"

#define SIRCS_FREQ 40000

#define SIRCS_TX_LSB_FIRST 1

#define SIRCS_START_BIT_HIGH 24
#define SIRCS_START_BIT_LOW 6
#define SIRCS_BIT0_HIGH 6
#define SIRCS_BIT0_LOW 6
#define SIRCS_BIT1_HIGH 12
#define SIRCS_BIT1_LOW 6

#define SIRCS_ADDR_LEN 5
#define SIRCS_EXT3_ADDR_LEN 3
#define SIRCS_EXT5_ADDR_LEN 5
#define SIRCS_CMD_LEN 7

#define SIRCS_REPETE_INTERVAL 10 //ms
#define SIRCS_REPETE_CNT 3

enum SIRCS_dev{
    SIRCS_BDP,
    SIRCS_SOUNDBAR,

    SIRCS_DEV_MAX
};

struct IR_SIRCS_t {
  uint8_t IR_address;
  uint8_t IR_ext_3_address;
  uint8_t IR_ext_5_address;
  uint8_t IR_command;
  char name[16];
};

void IR_send_SIRCS(struct IR_SIRCS_t *IR_SIRCS);

#endif