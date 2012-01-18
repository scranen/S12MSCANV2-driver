#include "queue.h"
#include "buffer.h"

/**
* \brief   An implementation of a queueing system for CAN messages using µCOS
*          queues. 
* TODO: make thread-safe
*/
typedef struct can_queue
{
  INT8U nids;
  INT32U* ids;
  OS_EVENT* queue;
} CAN_QUEUE;

static CAN_QUEUE msg_queues[CAN_MAX_QUEUES];

void CANQInit(void) {
  INT8U q;
  for (q = 0; q < CAN_MAX_QUEUES; ++q)
    msg_queues[q].queue = (void*)0;
}

INT8U CANQPost(CAN_BUF_MSG* buf)
{
  INT32U id = CANId(&buf->message);
  INT8U error = 0, q = 0, i;              
  while ((msg_queues[q].queue) && (q < CAN_MAX_QUEUES) && (!error))
  {
    if (!msg_queues[q].nids)
    {   
      ++(buf->usecount);          
      error = OSQPost(msg_queues[q].queue, &buf->message);
      if (error) {
        --(buf->usecount);
      }
    }
    else
    {
      for (i = 0; i < msg_queues[q].nids; ++i)
      {
        if (msg_queues[q].ids[i] == id)
        {              
          ++(buf->usecount);            
          error = OSQPost(msg_queues[q].queue, &buf->message);
          if (error)     
            --(buf->usecount);
          break;
        }
      }
    }
    ++q;
    if (error)
      return error;
  }
  return CAN_OK;
}

/*
 * TODO: use mutex for Register/Unregister.
 */

CAN_RESULT CANQRegister(INT8U nids, INT32U* ids, OS_EVENT* queue)
{
  INT8U q = 0;
  while (msg_queues[q].queue && q < CAN_MAX_QUEUES)
    ++q;
  if (q == CAN_MAX_QUEUES)
    return CAN_TOO_MANY_QUEUES;
  msg_queues[q].queue = queue;
  msg_queues[q].ids = ids;
  msg_queues[q].nids = nids;
  return CAN_OK;
}

CAN_RESULT CANQUnregister(OS_EVENT* queue)
{
  INT8U qi, qe;
  while (msg_queues[qe].queue && qe < CAN_MAX_QUEUES)
  {
    if (msg_queues[qe].queue == queue)
      qi = qe;
    ++qe;
  }
  --qe;
  msg_queues[qi] = msg_queues[qe];
  msg_queues[qe].queue = (void*)0;
  return CAN_OK;
}
