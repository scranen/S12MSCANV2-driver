#ifndef _CAN__BUFFER_H
#define _CAN__BUFFER_H

#include <CAN_driver.h>

typedef struct can_buf_msg {
  CAN_MSG message; 
  INT8U usecount;
} CAN_BUF_MSG; 

CAN_BUF_MSG* CANBufAcquire(CAN_MSG* data);
void CANBufUnacquire(void);
CAN_RESULT CANBufRelease(CAN_BUF_MSG* msg, INT8U n);
CAN_RESULT CANBufInit(void);

extern CAN_BUF_MSG msg_buffer[];
extern INT8U free_buffers[];

#endif // _CAN__BUFFER_H
