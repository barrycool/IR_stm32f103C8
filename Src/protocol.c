#include "protocol.h"
#include "crc.h"
#include "usbd_cdc_if.h"
#include <string.h>
#include "IR_RX.h"
#include "upgrade.h"
#include "usart.h"

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
  CDC_Transmit_FS(buf, buf_len);
  send_data_to_pc_by_wifi(buf, buf_len);
}

void IR_ack_handle(uint8_t seq_num, uint8_t msg_id)
{
  if (msg_id == SET_CMD_LIST)
  {
    IR_send_next_CMD();
  }
}

uint8_t latest_seq_num;
static void cmd_handle(uint8_t *buf, uint8_t buf_len)
{
  struct frame_t *frame; 
  uint8_t index;
  uint8_t crc8;
  struct IR_item_t *IR_item;
  
  frame= (struct frame_t *)buf;
  
  crc8 = CRC8(buf, frame->data_len);
  
  if (crc8 !=  buf[frame->data_len])
  {
    return;
  }
  
  latest_seq_num = frame->seq_num;

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
    ir_send_status_flag = 0;
  }
  else if  (frame->msg_id == START_SEND)
  {
    ir_send_status_flag = 1;
  }
  else if  (frame->msg_id == START_LEARNING)
  {
    ir_learning_status = 0;
    IR_RX_enable();
  }
  else if  (frame->msg_id == STOP_LEARNING)
  {
    IR_RX_disable();
    ir_learning_status = 1;
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
  else if (frame->msg_id == SET_SEND_IDX)
  {
    if (frame->msg_parameter[0] < IR_BUFFER_LEN)
      ir_sending_index = frame->msg_parameter[0];
  }
  else if (frame->msg_id == SEND_CMD_TO_UART)
  {
    HAL_UART_Transmit(&huart1, &frame->msg_parameter[1], frame->msg_parameter[0], 500);
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
uint8_t TX_buf[256];

void send_frame_to_PC(uint8_t msg_id, uint8_t *seq, void *data1, uint8_t data1_len, void *data2, uint8_t data2_len)
{
  struct frame_t *frame;
  frame = (struct frame_t *)TX_buf;
  
  frame->header = FRAME_HEADER;
  frame->data_len = sizeof(struct frame_t);
  frame->seq_num = seq ? *seq : unique_seq_num++;
  frame->msg_id = msg_id;
  
  if (data1)
  {
    memcpy(&TX_buf[frame->data_len], data1, data1_len);
    frame->data_len += data1_len;
  }
  
  if (data2)
  {
    memcpy(&TX_buf[frame->data_len], data2, data2_len);
    frame->data_len += data2_len;
  }
  
  TX_buf[frame->data_len] = CRC8(TX_buf, frame->data_len);

  send_data_to_PC(TX_buf, frame->data_len + 1);
}

void respon_version(uint8_t seq)
{
  uint32_t version = IR_VERSION;
  
  send_frame_to_PC(MCU_VERSION, &seq, &version, sizeof(version), NULL, 0);
}

void respon_cmd_list(uint8_t index, struct IR_item_t * ir_item)
{
  send_frame_to_PC(SET_CMD_LIST, NULL, &index, sizeof(index), ir_item, sizeof(struct IR_item_t));
}

void nack_msg(uint8_t seq, uint8_t msg_id)
{
  send_frame_to_PC(NACK, &seq, &msg_id, sizeof(msg_id), NULL, 0);
}

void ack_msg(uint8_t seq, uint8_t msg_id)
{
  send_frame_to_PC(ACK, &seq, &msg_id, sizeof(msg_id), NULL, 0);
}

void report_receive_ir(uint8_t *ir_data, uint8_t ir_data_len)
{
  send_frame_to_PC(REAL_TIME_RECV, NULL, &ir_data_len, sizeof(ir_data_len), ir_data, ir_data_len);
}

void report_sending_cmd(uint8_t index)
{
  send_frame_to_PC(REPORT_SENDING_CMD, NULL, &index, sizeof(index), NULL, 0);
}

void send_uart_to_PC(uint8_t *data, uint8_t data_len)
{
  send_frame_to_PC(RECV_CMD_FROM_UART, NULL, data, data_len, NULL, 0);
}
