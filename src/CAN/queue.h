#ifndef _CAN__QUEUE_H
#define _CAN__QUEUE_H

#include <CAN_driver.h>
#include "buffer.h"

void CANQInit(void);
INT8U CANQPost(CAN_BUF_MSG* buf);
CAN_RESULT CANQRegister(INT8U nids, INT32U* ids, OS_EVENT* queue);
CAN_RESULT CANQUnregister(OS_EVENT* queue);

#endif // _CAN__QUEUE_H
