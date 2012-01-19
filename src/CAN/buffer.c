#include <ucos_ii.h>
#include "CAN_driver.h"
#include "buffer.h"

#define CAN_MSG_BUFFER_BITS_SIZE \
  (CAN_MSG_BUFFER_SIZE / 8u + ((CAN_MSG_BUFFER_SIZE % 8u) ? 1 : 0))

/** Buffer that will hold received messages */
CAN_BUF_MSG msg_buffer[CAN_MSG_BUFFER_SIZE];
/** Bit mask that tells which buffers are empty (1=empty) */
INT8U free_buffers[CAN_MSG_BUFFER_BITS_SIZE];
/** Mutex for freeing buffers */
OS_EVENT* buffer_mutex;

void CANBufClear(void) {
  INT8U index;
  for (index = 0; index < CAN_MSG_BUFFER_BITS_SIZE; ++index)
    free_buffers[index] = 0xFF;
}

CAN_RESULT CANBufInit(void)
{
  INT8U err;
  CANBufClear();
  buffer_mutex = OSMutexCreate(CAN_MUTEX_PRIO, &err);
  if (err != OS_ERR_NONE)
    return CAN_NO_MUTEX;
  return CAN_OK;
}
/**
* \brief    Find empty buffer slot
*/
INT16U free_buffer_slot() 
{
  INT8U result = 0;
  INT8U index = 0;
  INT8U mask;
  while (!result && !free_buffers[index] && 
         index < CAN_MSG_BUFFER_BITS_SIZE)
    ++index;
  if (index < CAN_MSG_BUFFER_BITS_SIZE) 
  {
    mask = free_buffers[index];
    while (!(mask & 1)) 
    {      
      mask >>= 1;
      ++result;
    }
    mask = 1 << result;
    result += 8 * index; 
    free_buffers[index] ^= mask;
    return result;
  }
  return CAN_MSG_BUFFER_SIZE;
}

/**
* \brief    Copies the contents of buffer to a free slot
*/
CAN_BUF_MSG* CANBufStore(void* data)
{
  INT16U index = free_buffer_slot();  
  if (index < CAN_MSG_BUFFER_SIZE)
  {                             
    msg_buffer[index].message = *((CAN_MSG*)data);
    msg_buffer[index].usecount = 1;
    return &msg_buffer[index];
  }
  else
    return (CAN_BUF_MSG*)0;
}

/**
* \brief    Erase a message obtained by CANBufStore()
*/
CAN_RESULT CANBufErase(CAN_MSG* msg)
{
  INT8U index, err;
  CAN_BUF_MSG* buf;

  // Deleting a null pointer is treated as a no-op.
  if (!msg) 
    return CAN_OK;

#if OS_ARG_CHK_EN > 0
  if (msg < msg_buffer || 
      msg >= msg_buffer + CAN_MSG_BUFFER_SIZE * sizeof(CAN_BUF_MSG) ||
      msg % sizeof(CAN_BUF_MSG) > 0)
    return CAN_USER_ERROR;
#endif
  buf = (CAN_BUF_MSG*)msg;

  // Acquire the mutex. This is necessary, because we could otherwise
  // have a time-of-check-to-time-of-use error, causing us to free the
  // buffer twice (and the second time, it might have been reallocated
  // by another task).
  OSMutexPend(buffer_mutex, 0, &err);
  if (err != OS_ERR_NONE)
    return CAN_NO_MUTEX;

  --buf->usecount;
  if (buf->usecount)
  {
    if (OSMutexPost(buffer_mutex) != OS_ERR_NONE)
      return CAN_MISC_ERROR;
    return CAN_STILL_IN_USE;
  }
  index = ((INT8U*)msg - (INT8U*)msg_buffer) / sizeof(CAN_BUF_MSG);
  free_buffers[index / 8u] |= (1 << (index % 8u));
  if (OSMutexPost(buffer_mutex) != OS_ERR_NONE)
    return CAN_MISC_ERROR;
  return CAN_OK;
}
