#ifndef IR_LEARNING_H
#define IR_LEARNING_H

#include "stm32f1xx_hal.h"

#define LEARNING_FREQ 38000
#define IR_LEARNING_PLUSE_CNT 3

struct IR_learning_t{
  uint64_t bit_data;
  uint16_t bit_data_ext_16;

  uint8_t bit_number;

  uint8_t header_high;
  uint8_t header_low;

  uint8_t pluse_width[IR_LEARNING_PLUSE_CNT];
  char name[16];
};

void IR_send_learning(struct IR_learning_t *IR_learning);

#endif
