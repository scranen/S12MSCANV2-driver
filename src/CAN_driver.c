/** S12X derivative information */ 
/* #include "M9S12XF512.h"  */

#include "os_cpu.h" 
#include "CAN_driver.h"
#include "CAN/queue.h"
#include "CAN/buffer.h"

CAN0CTL1STR CAN_OPTIONS = { 0b10000000 };
CAN0BTR0STR CAN_TIMING1 = { 0b01000000 }; // Prescaler 1, sync jump 2
CAN0BTR1STR CAN_TIMING2 = { 0b00010100 }; // 1 sample/bit, ts1 = 5, ts2 = 2

static INT8U last_rx_error;

INT8U CANLastRxError(void) {
  INT8U result = last_rx_error; 
  last_rx_error = 0;
  return result;
}

CAN_ID CANId11(INT16U id) {
  CAN_ID result;
  result.int32 = 0;
  result.fields.id11 = id;
  return result; 
}

CAN_ID CANId29(INT32U id) {
  CAN_ID result;
  result.int32 = 0;
  result.fields.IDE = 1;
  result.fields.id292 = id & 0x7FFF;
  id >>= 15;
  result.fields.id291 = id & 0x03;
  id >>= 3;
  result.fields.id11 = id;
  return result;
}

INT32U CANId(CAN_MSG* msg) 
{
  if ((*msg).id.fields.IDE)
    return (((msg->id.fields.id11 << 3) | 
              msg->id.fields.id291) << 15) | 
              msg->id.fields.id292;
  else
    return msg->id.fields.id11;
}

CAN_RESULT CANForget(CAN_MSG* msg) { 
  return CANBufErase(msg);
}

CAN_RESULT CANRegister(INT8U nids, INT32U* ids, OS_EVENT* queue)
{
  return CANQRegister(nids, ids, queue);
}

CAN_RESULT CANUnregister(OS_EVENT* queue)
{
  return CANQUnregister(queue); 
}

/**
* \brief    Interrupt handler for RxFrame interrupt
*
* Note that the interrupt is cleared by assigning a mask value to CAN0RFLG, as
* is prescribed in Freescale Application Note AN2554/D (rev. 0).
*/
void interrupt VectorNumber_Vcan0rx i_receive_frame()
{
  CAN_BUF_MSG* msg = CANBufStore(&CAN0RXIDR0);
  CAN0RFLG = CAN0RFLG_RXF_MASK;
  
  if (msg) {
    last_rx_error = CANQPost(msg);
  } else {
    last_rx_error = 0xff; 
  }
  
  CANBufErase(&msg->message);  
}

/**
* \brief    Calculate CAN bit timings based on baudrate and CAN_OPTIONS.
*/
#define min(a, b) (((a) < (b)) ? (a) : (b))
CAN_RESULT CANConfigureBaudrate(INT32U baudrate, INT8U syncjump) {
  INT8U sum, prescaler = 64;
  INT32U clk = (CAN_OPTIONS.Bits.CLKSRC == CAN_CLOCK_OSC) ? CAN_CLOCK_OSC_SPEED : CAN_CLOCK_BUS_SPEED;
  // (tseg1 + tseg2 + 1) * baudrate * BRP = clk
  while ((prescaler > 0) &&
         ((prescaler * baudrate * 8 > clk) ||
          (clk % (prescaler * baudrate) > 0)))
    --prescaler;
    
  if (prescaler == 0)
    return CAN_USER_ERROR;

  sum = clk / (prescaler * baudrate) - 1;
  
  CAN_TIMING1.MergedBits.grpBRP = prescaler - 1;
  CAN_TIMING2.Bits.SAMP = CAN_SINGLE_SAMPLE;
  CAN_TIMING2.MergedBits.grpTSEG_20 = min(8, 4 * sum / 9) - 1;
  CAN_TIMING2.MergedBits.grpTSEG_10 = sum - CAN_TIMING2.MergedBits.grpTSEG_20 - 2;
  CAN_TIMING1.MergedBits.grpSJW = min(syncjump, CAN_TIMING2.MergedBits.grpTSEG_20);

  return CAN_OK;
}

/**
* \brief    Set CAN acceptance filters
*           Currently just accepts all messages.
*/
void setFilters(INT32U nfilters, INT32U* filters) {
  CAN0IDAC_IDAM = 0x01;               /* Four 16-bit acceptance filters */                                                          
  
  CAN0IDAR0 = 0x00;    /* 16 bit Filter 0 */
  CAN0IDMR0 = 0xFF;   
  CAN0IDAR1 = 0x00;     
  CAN0IDMR1 = 0xFF;    
  
  CAN0IDAR2 = 0x00;                   /* 16 bit Filter 1 */
  CAN0IDMR2 = 0x07;   
  CAN0IDAR3 = 0x00;                   
  CAN0IDMR3 = 0x07;    
  
  CAN0IDAR4 = 0x00;                   /* 16 bit Filter 2 */
  CAN0IDMR4 = 0x00;   
  CAN0IDAR5 = 0x00;                   
  CAN0IDMR5 = 0x07;    
  
  CAN0IDAR6 = 0x00;                   /* 16 bit Filter 3 */
  CAN0IDMR6 = 0x00;   
  CAN0IDAR7 = 0x00;                   
  CAN0IDMR7 = 0x07;  
}

CAN_RESULT CANInit(void) {
  CANBufClear();            // Initialise SW receive buffer   
  CANQInit();               // Initialise queueing system
  return CAN_OK;
}

/**
* \brief    Initial CAN setup.
*/
CAN_RESULT CANStart(void) 
{
  // Set controller to initialisation mode.
  CAN0CTL0 = 0b00000001;
  while (!(CAN0CTL1_INITAK));
  
  // Enable CAN controller, copying CLKSRC, LOOPB, LISTEN and WUPM from 
  // CAN_OPTIONS.
  CAN0CTL1 = CAN0CTL1_CANE_MASK | (CAN_OPTIONS.Byte & 
               (CAN0CTL1_CLKSRC_MASK | CAN0CTL1_LOOPB_MASK |
                CAN0CTL1_LISTEN_MASK | CAN0CTL1_WUPM_MASK));
  CAN0BTR0 = CAN_TIMING1.Byte;
  CAN0BTR1 = CAN_TIMING2.Byte;
  
  setFilters(0, (void*)0);

  CAN0CTL0 = 0b00000000;    // Exit initialisation mode  
  while (CAN0CTL1_INITAK);  
  while (!CAN0CTL0_SYNCH);  // Wait for CAN bus synchronisation 
  CAN0RIER = 0b01111101;    // Enable interrupts: all status changes and rxfull.
  CAN0RFLG = 0b11000011;    // Clear receiver flags

  return CAN_OK;                       
}

/**
* \brief    Send CAN frame
*/
INT8U CANSendFrame(CAN_ID id, INT8U prio, INT8U len, INT8U *data)
{   
    INT8U buf;              // Holds the selected transmission buffer 
    INT8U index;            // Iterates through the data

    CAN0TBSEL = CAN0TFLG;   // Select lowest empty buffer 
    buf = CAN0TBSEL;		    // Backup selected buffer   
    if (!CAN0TBSEL){        // Return if no buffer was selected
        return CAN_SEND_BUFFER_FULL;
    }
    
    // Load ID and data into registers
    *((INT32U*)((INT32U)(&CAN0TXIDR0))) = id.int32;
    for (index = 0; index < len; ++index) 
      (&CAN0TXDSR0)[index] = data[index];

    CAN0TXDLR = len;        // Set Data Length Code
    CAN0TXTBPR = prio;      // Set Priority
    CAN0TFLG = buf;         // Start transmission
                         
    return CAN_OK;
 }
