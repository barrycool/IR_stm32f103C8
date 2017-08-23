#include "IR_sircs.h"
#include "IR.h"
#include "comm.h"

void IR_send_SIRCS_once(struct IR_SIRCS_t *IR_SIRCS)
{
  uint8_t data_len;
  
  if (IR_SIRCS->IR_ext_5_address)
  {
    data_len = 20;
  }
  else if (IR_SIRCS->IR_ext_3_address)
  {
    data_len = 15;
  }
  else if (IR_SIRCS->IR_address)
  {
    data_len = 12;
  }
  else
  {
    return;
  }
  
  IR_send_start_bit(SIRCS_START_BIT_HIGH, SIRCS_START_BIT_LOW);
  
  IR_send_bit_data_PWM_PDM(SIRCS_TX_LSB_FIRST, SIRCS_BIT0_HIGH, SIRCS_BIT0_LOW, SIRCS_BIT1_HIGH, SIRCS_BIT1_LOW, IR_SIRCS->IR_command, SIRCS_CMD_LEN);
  
  IR_send_bit_data_PWM_PDM(SIRCS_TX_LSB_FIRST, SIRCS_BIT0_HIGH, SIRCS_BIT0_LOW, SIRCS_BIT1_HIGH, SIRCS_BIT1_LOW, IR_SIRCS->IR_address, SIRCS_ADDR_LEN);
  
  if (data_len >= 15)
  {
    IR_send_bit_data_PWM_PDM(SIRCS_TX_LSB_FIRST, SIRCS_BIT0_HIGH, SIRCS_BIT0_LOW, SIRCS_BIT1_HIGH, SIRCS_BIT1_LOW, IR_SIRCS->IR_ext_3_address, SIRCS_EXT3_ADDR_LEN);
  }
  
  if (data_len >= 20)
  {
    IR_send_bit_data_PWM_PDM(SIRCS_TX_LSB_FIRST, SIRCS_BIT0_HIGH, SIRCS_BIT0_LOW, SIRCS_BIT1_HIGH, SIRCS_BIT1_LOW, IR_SIRCS->IR_ext_5_address, SIRCS_EXT5_ADDR_LEN);
  }
}

void IR_send_SIRCS(struct IR_SIRCS_t *IR_SIRCS)
{
  uint8_t i;
  
  IR_set_carrier_freq(SIRCS_FREQ);

  for (i = 0; i < SIRCS_REPETE_CNT; i++)
  {
    IR_send_SIRCS_once(IR_SIRCS);
    delay_ms(SIRCS_REPETE_INTERVAL);
  }
}
