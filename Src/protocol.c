#include "protocol.h"
#include "crc.h"
#include "usbd_cdc_if.h"
#include <string.h>
#include "IR_RX.h"
#include "upgrade.h"

uint32_t CRC32(CRC_HandleTypeDef *hcrc, uint8_t pBuffer[], uint16_t BufferLength)
{
  uint32_t index = 0U;

  /* Process Locked */
  __HAL_LOCK(hcrc); 

  /* Change CRC peripheral state */
  hcrc->State = HAL_CRC_STATE_BUSY;

  /* Reset CRC Calculation Unit */
  __HAL_CRC_DR_RESET(hcrc);

  /* Enter Data to the CRC calculator */
  for(index = 0U; index < BufferLength; index++)
  {
    hcrc->Instance->DR = (uint32_t) pBuffer[index];
  }

  /* Change CRC peripheral state */
  hcrc->State = HAL_CRC_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hcrc);

  /* Return the CRC computed value */
  return hcrc->Instance->DR;
}

uint8_t CRC8(uint8_t *buf, uint16_t buf_len)
{
  uint8_t crc1, crc2, crc3, crc4;
  uint32_t crc32;
  
  crc32 = CRC32(&hcrc, buf, buf_len);
  
  crc1 = crc32 & 0xFF;
  crc2 = (crc32 >> 8) & 0xFF;
  crc3 = (crc32 >> 16) & 0xFF;
  crc4 = (crc32 >> 24) & 0xFF;
  
  return crc1 ^ crc2 ^ crc3 ^ crc4;
}

uint8_t msg_buf[256];
uint8_t msg_buf_len;
uint8_t msg_flag;
void receive_data_from_PC(uint8_t *buf, uint8_t buf_len)
{
  struct frame_t *frame = (struct frame_t *)buf;
  
  if (frame->header != FRAME_HEADER)
  {
    return;
  }
  
  memcpy(msg_buf + msg_buf_len, buf, buf_len);
  msg_buf_len += buf_len;
  
  if (frame->data_len + 1 > msg_buf_len)
  {
    return;
  }
  
  msg_flag = 1;
}

void send_data_to_PC(uint8_t *buf, uint8_t buf_len)
{
  #if 1
  //while(CDC_Transmit_FS(buf, buf_len) == USBD_BUSY);
  CDC_Transmit_FS(buf, buf_len);
  #else
  HAL_UART_Transmit(&huart1, buf, buf_len, 50);
  #endif
}

void IR_ack_handle(uint8_t seq_num, uint8_t msg_id)
{
  if (msg_id == SET_CMD_LIST)
  {
    IR_send_next_CMD();
  }
}

uint8_t crc8;
void cmd_handle(uint8_t *buf, uint8_t buf_len)
{
  struct frame_t *frame; 
  uint8_t index;
  struct IR_item_t *IR_item;
  
  frame= (struct frame_t *)buf;
  
  crc8 = CRC8(buf, frame->data_len);
  
  if (crc8 !=  buf[frame->data_len])
  {
    return;
  }

  if (frame->msg_id == REAL_TIME_SEND)
  {
    IR_item = (struct IR_item_t *)frame->msg_parameter;
    IR_send_living_cmd(IR_item);
  }
  else if  (frame->msg_id == CLEAR_CMD_LIST)
  {
    IR_clear_CMD_list();
  }
  else if  (frame->msg_id == SET_CMD_LIST)
  {
    index = frame->msg_parameter[0];
    IR_item = (struct IR_item_t *)(frame->msg_parameter + 1);
    IR_set_CMD_list(index, IR_item);
  }
  else if  (frame->msg_id == READ_CMD_LIST)
  {
    IR_send_CMD_list();
  }
  else if  (frame->msg_id == PAUSE_SEND)
  {
    IR_pause_send();
  }
  else if  (frame->msg_id == START_SEND)
  {
    IR_start_send();
  }
  else if  (frame->msg_id == START_LEARNING)
  {
    IR_start_learning();
    IR_RX_enable();
  }
  else if  (frame->msg_id == STOP_LEARNING)
  {
    IR_RX_disable();
    IR_stop_learning();
  }
  else if (frame->msg_id == ACK)
  {
    IR_ack_handle(frame->seq_num, frame->msg_parameter[0]);
  }
  else if (frame->msg_id == READ_MCU_VERSION)
  {
    respon_version(frame->seq_num);
  }
  else if (frame->msg_id == UPGRADE_START)
  {
    if (!upgrade_init())
    {
      nack_msg(frame->seq_num, frame->msg_id);
      return;
    }
  }
  else if (frame->msg_id == UPGRADE_PACKET)
  {
    if (!upgrade_recv_packet(*(uint16_t*)frame->msg_parameter, &frame->msg_parameter[2], PACKET_MAX_SIZE))
    {
      nack_msg(frame->seq_num, frame->msg_id);
      return;
    }
  }
  else if (frame->msg_id == UPGRADE_FINISH)
  {
    if (upgrade_finish())
    {
      nack_msg(frame->seq_num, frame->msg_id);
      return;
    }
  }
  
  if (frame->msg_id != READ_CMD_LIST && frame->msg_id != ACK &&
      frame->msg_id != NACK && frame->msg_id != READ_MCU_VERSION)
  {
    ack_msg(frame->seq_num, frame->msg_id);
  }
}

void protocol_loop(void)
{
  if (msg_flag)
  {
      cmd_handle(msg_buf, msg_buf_len);
      msg_flag = 0;
      msg_buf_len = 0;
  }
}

uint8_t unique_seq_num;

void respon_version(uint8_t seq)
{
  uint8_t buf[256];
  struct frame_t *frame;
  frame = (struct frame_t *)buf;
  
  frame->header = FRAME_HEADER;
  frame->data_len = sizeof(struct frame_t);
  frame->seq_num = seq;
  frame->msg_id = MCU_VERSION;
  
  frame->msg_parameter[0] = IR_VERSION & 0xFF;
  frame->msg_parameter[1] = (IR_VERSION >> 8) & 0xFF;
  frame->msg_parameter[2] = (IR_VERSION >> 16) & 0xFF;
  frame->msg_parameter[3] = (IR_VERSION >> 24) & 0xFF;
  frame->data_len += 4;
  
  buf[frame->data_len] = CRC8(buf, frame->data_len);

  send_data_to_PC(buf, frame->data_len + 1);
}

void respon_cmd_list(uint8_t index, struct IR_item_t * ir_item)
{
  uint8_t buf[256];
  struct frame_t *frame;
  frame = (struct frame_t *)buf;
  
  frame->header = FRAME_HEADER;
  frame->data_len = sizeof(struct frame_t);
  frame->seq_num = unique_seq_num++;
  frame->msg_id = SET_CMD_LIST;
  
  frame->msg_parameter[0] = index;
  frame->data_len += 1;
  
  memcpy(frame->msg_parameter + 1, ir_item, sizeof(struct IR_item_t));
  
  frame->data_len += sizeof(struct IR_item_t);
  
  buf[frame->data_len] = CRC8(buf, frame->data_len);

  send_data_to_PC(buf, frame->data_len + 1);
}

void nack_msg(uint8_t seq, uint8_t msg_id)
{
  uint8_t buf[256];
  struct frame_t *frame;
    
  frame = (struct frame_t *)buf;
  
  frame->header = FRAME_HEADER;
  frame->data_len = sizeof(struct frame_t);
  frame->seq_num = seq;
  frame->msg_id = NACK;
  
  frame->msg_parameter[0] = msg_id;
  frame->data_len += 1;
  
  buf[frame->data_len] = CRC8(buf, frame->data_len);

  send_data_to_PC(buf, frame->data_len + 1);
}

void ack_msg(uint8_t seq, uint8_t msg_id)
{
  uint8_t buf[256];
  struct frame_t *frame;
    
  frame = (struct frame_t *)buf;
  
  frame->header = FRAME_HEADER;
  frame->data_len = sizeof(struct frame_t);
  frame->seq_num = seq;
  frame->msg_id = ACK;
  
  frame->msg_parameter[0] = msg_id;
  frame->data_len += 1;
  
  buf[frame->data_len] = CRC8(buf, frame->data_len);

  send_data_to_PC(buf, frame->data_len + 1);
}

uint8_t seq_num;
  uint8_t buf[256];
void report_receive_ir(uint8_t *ir_data, uint8_t ir_data_len)
{

  struct frame_t *frame;
    
  frame = (struct frame_t *)buf;
  
  frame->header = FRAME_HEADER;
  frame->data_len = sizeof(struct frame_t);
  frame->seq_num = seq_num++;
  frame->msg_id = REAL_TIME_RECV;
  
  frame->msg_parameter[0] = ir_data_len;
  memcpy(frame->msg_parameter + 1, ir_data, ir_data_len);
  
  frame->data_len += 1 + ir_data_len;
  
  buf[frame->data_len] = CRC8(buf, frame->data_len);
  
  send_data_to_PC(buf, frame->data_len + 1);
}

void report_sending_cmd(uint8_t index)
{
  uint8_t buf[256];
  struct frame_t *frame;
    
  frame = (struct frame_t *)buf;
  
  frame->header = FRAME_HEADER;
  frame->data_len = sizeof(struct frame_t);
  frame->seq_num = seq_num++;
  frame->msg_id = REPORT_SENDING_CMD;
  
  frame->msg_parameter[0] = index;
  frame->data_len += 1;
  
  buf[frame->data_len] = CRC8(buf, frame->data_len);
  
  send_data_to_PC(buf, frame->data_len + 1);
}
