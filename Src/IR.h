#ifndef IR_H
#define IR_H

#include "stm32f1xx_hal.h"
#include "IR_sircs.h"
#include "IR_learning.h"
#include "IR_NEC.h"
#include "IR_RC5.h"
#include "IR_RC6.h"
#include "IR_JVC.h"

#define IR_VERSION 0x20170831

#define IR_TIMER_CLOCK 72000000

enum IR_type_t {
  IR_TYPE_SIRCS,
  IR_TYPE_NEC,
  IR_TYPE_RC6,
  IR_TYPE_RC5,
  IR_TYPE_JVC,
  IR_TYPE_LEARNING,

  IR_TYPE_MAX
};

union IR_CMD_t{
  struct IR_SIRCS_t IR_SIRCS;
  struct IR_NEC_t IR_NEC;
  struct IR_RC6_t IR_RC6;
  struct IR_RC5_t IR_RC5;
  struct IR_JVC_t IR_JVC;
  struct IR_learning_t IR_learning;
};

struct IR_item_t {
  union IR_CMD_t IR_CMD;
  uint8_t IR_type;
  
  uint8_t is_valid;
  uint32_t delay_time; //ms
};

#define IR_BUFFER_LEN 20
extern struct IR_item_t IR_CMD_list[IR_BUFFER_LEN];

void IR_set_carrier_freq(uint32_t freq);
void enable_ir_tx(void);
void disable_ir_tx(void);
void IR_send_start_bit(uint8_t start_bit_high, uint8_t start_bit_low);
void IR_send_bit_data_PWM_PDM(uint8_t LSB_first, uint8_t bit0_high, uint8_t bit0_low,
                              uint8_t bit1_high, uint8_t bit1_low, uint8_t data, uint8_t data_len);
void IR_send_bit_data_BI_PHASE(uint8_t LSB_first, uint8_t bit1_high_first, uint8_t bit_len, 
                                uint8_t data, uint8_t data_len);

void IR_init(void);
void IR_loop(void);
void IR_decrease(void);
void IR_send_living_cmd(struct IR_item_t *IR_item);
void IR_clear_CMD_list(void);
void IR_set_CMD_list(uint8_t index, struct IR_item_t *IR_item);
void IR_send_CMD_list(void);
void IR_send_next_CMD(void);
void IR_pause_send(void);
void IR_start_send(void);
void IR_stop_learning(void);
void IR_start_learning(void);

#endif