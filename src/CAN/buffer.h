#ifndef _CAN__BUFFER_H
#define _CAN__BUFFER_H

#include <CAN_driver.h>

typedef struct can_buf_msg {
  CAN_MSG message; 
  INT8U usecount;
} CAN_BUF_MSG; 

void CANBufClear(void);
CAN_BUF_MSG* CANBufStore(void* data);
CAN_RESULT CANBufErase(CAN_MSG* msg);

extern CAN_BUF_MSG msg_buffer[];
extern INT8U free_buffers[];

#endif // _CAN__BUFFER_H
