#include "IR_sircs.h"
#include "IR.h"
#include "comm.h"

uint8_t IR_TX_WF_buf[128];
uint8_t IR_TX_WF_buf_len;

uint8_t IR_decode_learning(struct IR_learning_t *IR_learning)
{
  uint8_t i, j;
  uint8_t diff_cnt = 0;
  uint8_t coding_length[IR_LEARNING_PLUSE_CNT];
  uint64_t coding_value[IR_LEARNING_PLUSE_CNT];
  uint8_t coding_mask[IR_LEARNING_PLUSE_CNT];
  uint64_t tmp;
  uint8_t bit_sum = 0;
  
  for(i = 0; i < IR_LEARNING_PLUSE_CNT; i++)
  {
    if (IR_learning->pluse_width[i] == 0xFF)
    {
      break;
    }
    
    diff_cnt++;
  }

  if (diff_cnt == 2)
  {
      coding_value[0] = 0;
      coding_length[0] = 1;
      coding_mask[0] = 1;

      coding_value[1] = 1;
      coding_length[1] = 1;
      coding_mask[1] = 1;
  }
  else if (diff_cnt == 3)
  {
      coding_value[0] = 0;
      coding_length[0] = 1;
      coding_mask[0] = 1;

      coding_value[1] = 1;
      coding_length[1] = 2;
      coding_mask[1] = 3;

      coding_value[2] = 3;
      coding_length[2] = 2;
      coding_mask[2] = 3;
  }
  else
  {
      return 0;
  }
  
  IR_TX_WF_buf_len = 0;
  IR_TX_WF_buf[IR_TX_WF_buf_len++] = IR_learning->header_high;
  IR_TX_WF_buf[IR_TX_WF_buf_len++] = IR_learning->header_low;
  
  for(i = 0; i < IR_learning->bit_number && bit_sum < 16; i++)
  {
    for(j = 0; j < diff_cnt; j++)
    {
      if ((IR_learning->bit_data & coding_mask[j]) == coding_value[j])
      {
        IR_TX_WF_buf[IR_TX_WF_buf_len++] = IR_learning->pluse_width[j];
        IR_learning->bit_data >>= coding_length[j];
        bit_sum += coding_length[j];
        break;
      }
    }

    if (j == diff_cnt)
    {
      return 0;
    }
  }

  tmp = IR_learning->bit_data_ext_16;
  IR_learning->bit_data |= tmp << (64 - bit_sum);
  
  for(;i < IR_learning->bit_number; i++)
  {
    for(j = 0; j < diff_cnt; j++)
    {
      if ((IR_learning->bit_data & coding_mask[j]) == coding_value[j])
      {
        IR_TX_WF_buf[IR_TX_WF_buf_len++] = IR_learning->pluse_width[j];
        IR_learning->bit_data >>= coding_length[j];
        bit_sum += coding_length[j];
        break;
      }
    }

    if (j == diff_cnt)
    {
      return 0;
    }
  }
  
  return 1;
}

void IR_send_learning(struct IR_learning_t *IR_learning)
{
  uint8_t i;
  
  if (!IR_decode_learning(IR_learning))
  {
    return;
  }
  
  IR_set_carrier_freq(LEARNING_FREQ);
  
  for(i = 0; i< IR_TX_WF_buf_len; i++)
  {
    if (i & 1)
    {
      disable_ir_tx();
    }
    else
    {
      enable_ir_tx();
    }
    
    delay_n_0_1_ms(IR_TX_WF_buf[i]);
  }
}
