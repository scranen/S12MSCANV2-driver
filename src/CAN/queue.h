#ifndef _CAN__QUEUE_H
#define _CAN__QUEUE_H

#include <CAN_driver.h>
#include "buffer.h"

void CANQInit(void);
INT8U CANQPost(CAN_MSG* msg, INT8U* usecount);
CAN_RESULT CANQRegister(INT8U nids, INT32U* ids, OS_EVENT* queue);
INT8U CANQCount(void);

#endif // _CAN__QUEUE_H
