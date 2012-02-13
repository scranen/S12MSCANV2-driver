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
static INT8U queue_count;

void CANQInit(void) {
  INT8U q;
  for (q = 0; q < CAN_MAX_QUEUES; ++q)
    msg_queues[q].queue = (void*)0;
  queue_count = 0;
}

INT8U CANQPost(CAN_MSG* msg, INT8U* usecount)
{
  INT32U id = CANId(msg);
  INT8U error = 0, q = 0, i;
  *usecount = 0;       
  while ((msg_queues[q].queue) && (q < CAN_MAX_QUEUES) && (!error))
  {
    if (!msg_queues[q].nids)
    {   
      ++(*usecount);          
      error = OSQPost(msg_queues[q].queue, msg);
      if (error) {
        --(*usecount);
      }
    }
    else
    {
      for (i = 0; i < msg_queues[q].nids; ++i)
      {
        if (msg_queues[q].ids[i] == id)
        {              
          ++(*usecount);            
          error = OSQPost(msg_queues[q].queue, msg);
          if (error)     
            --(*usecount);
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
 * TODO: use mutex for Register.
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
  queue_count++;
  return CAN_OK;
}

INT8U CANQCount(void)
{
  return queue_count;
}
