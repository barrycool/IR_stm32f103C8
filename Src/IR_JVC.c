#include "IR_JVC.h"
#include "IR.h"
#include "comm.h"

void IR_send_JVC(struct IR_JVC_t *IR_JVC)
{
  IR_set_carrier_freq(JVC_FREQ);
  
  IR_send_start_bit(JVC_START_BIT_HIGH, JVC_START_BIT_LOW);
  
  IR_send_bit_data_PWM_PDM(JVC_TX_LSB_FIRST, JVC_BIT0_HIGH, JVC_BIT0_LOW, JVC_BIT1_HIGH, JVC_BIT1_LOW, IR_JVC->IR_address, JVC_ADDR_LEN);
  
  IR_send_bit_data_PWM_PDM(JVC_TX_LSB_FIRST, JVC_BIT0_HIGH, JVC_BIT0_LOW, JVC_BIT1_HIGH, JVC_BIT1_LOW, IR_JVC->IR_command, JVC_CMD_LEN);
  
  IR_send_bit_data_PWM_PDM(JVC_TX_LSB_FIRST, JVC_BIT0_HIGH, JVC_BIT0_LOW, JVC_BIT1_HIGH, JVC_BIT1_LOW, 0, 1);
}
