#include <ucos_ii.h>
#include "CAN_driver.h"
#include "buffer.h"

/** Buffer that will hold received messages */
CAN_BUF_MSG msg_buffer[CAN_MSG_BUFFER_SIZE];
/** Bit mask that tells which buffers are empty (1=empty) */
INT8U free_buffers[CAN_MSG_BUFFER_SIZE];
INT8U free_ptr, used_ptr;
/** Mutex for freeing buffers */
OS_EVENT* buffer_mutex;

void CANBufClear(void) {
  INT8U index;
  for (index = 0; index < CAN_MSG_BUFFER_SIZE; ++index)
    free_buffers[index] = index;
  free_ptr = 0;
  used_ptr = CAN_MSG_BUFFER_SIZE - 1;
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
* \brief    Copies the contents of buffer to a free slot
*/
CAN_BUF_MSG* CANBufAcquire(CAN_MSG* data)
{
  INT16U index;
  if (free_ptr == used_ptr)
    return ((CAN_BUF_MSG*)0);
  index = free_buffers[free_ptr];
  free_ptr = (free_ptr + 1) % CAN_MSG_BUFFER_SIZE;
  msg_buffer[index].message = *data;
  msg_buffer[index].usecount = 1;
  return &msg_buffer[index];
}

void CANBufUnacquire(void) {
  free_ptr = free_ptr ? free_ptr - 1 : (CAN_MSG_BUFFER_SIZE - 1);  
}

/**
* \brief    Erase a message obtained by CANBufStore()
*/
CAN_RESULT CANBufRelease(CAN_BUF_MSG* buffer, INT8U n)
{
  INT8U index, err;

  OSMutexPend(buffer_mutex, 0, &err);
  if (err != OS_ERR_NONE)
    return CAN_NO_MUTEX;

  buffer->usecount -= n;
  if (buffer->usecount)
  {
    if (OSMutexPost(buffer_mutex) != OS_ERR_NONE)
      return CAN_MISC_ERROR;
    return CAN_STILL_IN_USE;
  }

  index = ((INT8U*)buffer - (INT8U*)msg_buffer) / sizeof(CAN_BUF_MSG);
  used_ptr = used_ptr + 1 % CAN_MSG_BUFFER_SIZE;
  free_buffers[used_ptr] = index;

  if (OSMutexPost(buffer_mutex) != OS_ERR_NONE)
    return CAN_MISC_ERROR;
  return CAN_OK;
}
