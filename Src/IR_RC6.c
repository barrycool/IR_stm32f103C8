#include "IR_RC6.h"
#include "IR.h"
#include "comm.h"

uint8_t RC6_toggle;
void IR_send_RC6(struct IR_RC6_t *IR_RC6)
{
  IR_set_carrier_freq(RC6_FREQ);
  
  IR_send_start_bit(RC6_START_BIT_HIGH, RC6_START_BIT_LOW);
  
  IR_send_bit_data_BI_PHASE(RC6_TX_LSB_FIRST, RC6_BIT1_HIGH_FIRST, RC6_DATA_LEN, 1, 1);
  
  IR_send_bit_data_BI_PHASE(RC6_TX_LSB_FIRST, RC6_BIT1_HIGH_FIRST, RC6_DATA_LEN, IR_RC6->IR_mode, RC6_MODE_LEN);
  
  if (RC6_toggle)
  {
    IR_send_bit_data_BI_PHASE(RC6_TX_LSB_FIRST, RC6_BIT1_HIGH_FIRST, RC6_TOGGLE_LEN, 1, 1);
  }
  else
  {
    IR_send_bit_data_BI_PHASE(RC6_TX_LSB_FIRST, RC6_BIT1_HIGH_FIRST, RC6_TOGGLE_LEN, 0, 1);
  }
  RC6_toggle = !RC6_toggle;
  
  IR_send_bit_data_BI_PHASE(RC6_TX_LSB_FIRST, RC6_BIT1_HIGH_FIRST, RC6_DATA_LEN, IR_RC6->IR_address, RC6_ADDR_LEN);
  
  IR_send_bit_data_BI_PHASE(RC6_TX_LSB_FIRST, RC6_BIT1_HIGH_FIRST, RC6_DATA_LEN, IR_RC6->IR_command, RC6_CMD_LEN);
}
