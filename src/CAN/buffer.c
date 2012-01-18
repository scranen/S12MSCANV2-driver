#include <os_cpu.h>
#include "CAN_driver.h"
#include "buffer.h"

#define CAN_MSG_BUFFER_BITS_SIZE \
  (CAN_MSG_BUFFER_SIZE / 8u + ((CAN_MSG_BUFFER_SIZE % 8u) ? 1 : 0))

/** Buffer that will hold received messages */
CAN_BUF_MSG msg_buffer[CAN_MSG_BUFFER_SIZE];
/** Bit mask that tells which buffers are empty (1=empty) */
INT8U free_buffers[CAN_MSG_BUFFER_BITS_SIZE];

void CANBufClear(void) {
  INT8U index;
  for (index = 0; index < CAN_MSG_BUFFER_BITS_SIZE; ++index)
    free_buffers[index] = 0xFF;
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
  INT8U index;
  CAN_BUF_MSG* buf;
  if (!msg) 
    return CAN_STILL_IN_USE;
#if OS_ARG_CHK_EN > 0
  if (msg < msg_buffer || 
      msg >= msg_buffer + CAN_MSG_BUFFER_SIZE * sizeof(CAN_BUF_MSG) ||
      msg % sizeof(CAN_BUF_MSG) > 0)
    return CAN_USER_ERROR;
#endif
  buf = (CAN_BUF_MSG*)msg;
  --buf->usecount;          // TODO: make sure this is an atomic decrement
  if (buf->usecount)
    return CAN_STILL_IN_USE; 
  index = ((INT8U*)msg - (INT8U*)msg_buffer) / sizeof(CAN_BUF_MSG);
  free_buffers[index / 8u] |= (1 << (index % 8u));
  return CAN_OK;
}
