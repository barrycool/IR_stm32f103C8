#include "IR_NEC.h"
#include "IR.h"
#include "comm.h"

void IR_send_NEC(struct IR_NEC_t *IR_NEC, uint8_t repeate_mode)
{
  IR_set_carrier_freq(NEC_FREQ);
  
  if (!repeate_mode)
  {
    //normal mode header
    IR_send_start_bit(IR_NEC->IR_header_high, NEC_START_BIT_LOW);
    
    IR_send_bit_data_PWM_PDM(NEC_TX_LSB_FIRST, NEC_BIT0_HIGH, NEC_BIT0_LOW, NEC_BIT1_HIGH, NEC_BIT1_LOW, IR_NEC->IR_address, NEC_ADDR_LEN);
    
    IR_send_bit_data_PWM_PDM(NEC_TX_LSB_FIRST, NEC_BIT0_HIGH, NEC_BIT0_LOW, NEC_BIT1_HIGH, NEC_BIT1_LOW, IR_NEC->IR_address_ext, NEC_ADDR_LEN);
    
    IR_send_bit_data_PWM_PDM(NEC_TX_LSB_FIRST, NEC_BIT0_HIGH, NEC_BIT0_LOW, NEC_BIT1_HIGH, NEC_BIT1_LOW, IR_NEC->IR_command, NEC_CMD_LEN);
    
    IR_send_bit_data_PWM_PDM(NEC_TX_LSB_FIRST, NEC_BIT0_HIGH, NEC_BIT0_LOW, NEC_BIT1_HIGH, NEC_BIT1_LOW, ~IR_NEC->IR_command, NEC_CMD_LEN);
  }
  else
  {
    //repeate mode header
    IR_send_start_bit(IR_NEC->IR_header_high, NEC_REPEATE_START_BIT_LOW);
  }
  
  //tailer
  IR_send_bit_data_PWM_PDM(NEC_TX_LSB_FIRST, NEC_BIT0_HIGH, NEC_BIT0_LOW, NEC_BIT1_HIGH, NEC_BIT1_LOW, 0, 1);
}
