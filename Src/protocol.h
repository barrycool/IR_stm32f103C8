#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "IR.h"

#define FRAME_HEADER 0x55

enum msg_t {
    NACK,
    ACK,
    REAL_TIME_SEND,
    CLEAR_CMD_LIST,
    SET_CMD_LIST,
    READ_CMD_LIST,
    REAL_TIME_RECV,
    READ_MCU_VERSION,
    MCU_VERSION,
    UPGRADE_START,
    UPGRADE_PACKET,
    UPGRADE_FINISH,
    PAUSE_SEND,
    START_SEND,
    STOP_LEARNING,
    START_LEARNING,
    REPORT_SENDING_CMD,
    SET_SEND_IDX,
};

struct frame_t {
  uint8_t header;
  uint8_t data_len;
  uint8_t seq_num;
  uint8_t msg_id;
  uint8_t msg_parameter[0];
};

extern uint8_t latest_seq_num;

void cmd_handle(uint8_t *buf, uint8_t buf_len);
void nack_msg(uint8_t seq, uint8_t msg_id);
void ack_msg(uint8_t seq, uint8_t msg_id);
void report_receive_ir(uint8_t *ir_data, uint8_t ir_data_len);
void respon_cmd_list(uint8_t index, struct IR_item_t * ir_item);
void receive_data_from_PC(uint8_t *buf, uint8_t buf_len);
void protocol_loop(void);
void respon_version(uint8_t seq);
void report_sending_cmd(uint8_t index);

#endif
