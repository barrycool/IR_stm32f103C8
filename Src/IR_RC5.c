#include "IR_RC5.h"
#include "IR.h"
#include "comm.h"

uint8_t RC5_toggle;
void IR_send_RC5(struct IR_RC5_t *IR_RC5)
{
  IR_set_carrier_freq(RC5_FREQ);
  
  IR_send_bit_data_BI_PHASE(RC5_TX_LSB_FIRST, RC5_BIT1_HIGH_FIRST, RC5_DATA_LEN, 1, 1);
  IR_send_bit_data_BI_PHASE(RC5_TX_LSB_FIRST, RC5_BIT1_HIGH_FIRST, RC5_DATA_LEN, (IR_RC5->IR_command >> 6) & 1, 1);
  
  if (RC5_toggle)
  {
    IR_send_bit_data_BI_PHASE(RC5_TX_LSB_FIRST, RC5_BIT1_HIGH_FIRST, RC5_TOGGLE_LEN, 1, 1);
  }
  else
  {
    IR_send_bit_data_BI_PHASE(RC5_TX_LSB_FIRST, RC5_BIT1_HIGH_FIRST, RC5_TOGGLE_LEN, 0, 1);
  }
  RC5_toggle = !RC5_toggle;
  
  IR_send_bit_data_BI_PHASE(RC5_TX_LSB_FIRST, RC5_BIT1_HIGH_FIRST, RC5_DATA_LEN, IR_RC5->IR_address, RC5_ADDR_LEN);
  
  IR_send_bit_data_BI_PHASE(RC5_TX_LSB_FIRST, RC5_BIT1_HIGH_FIRST, RC5_DATA_LEN, IR_RC5->IR_command, RC5_CMD_LEN);
}
