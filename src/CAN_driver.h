#ifndef _CAN_DRIVER_HPP
#define _CAN_DRIVER_HPP

#include <ucos_ii.h>
#include <os_cpu.h>
#include "CAN_config.h"

// Symbolic values
enum {
  CAN_CLOCK_OSC = 0u,
  CAN_CLOCK_BUS = 1u
};
enum {
  CAN_SINGLE_SAMPLE = 0u,
  CAN_TRIPLE_SAMPLE = 1u
};

/* CAN_OPTIONS                                                               *
 * Modify to initialise the CAN bus with different options. Default          *
 * settings:                                                                 *
 *   WUPM:   0                                                               *
 *   LISTEN: 0                                                               *
 *   LOOPB:  0                                                               *
 *   CLKSRC: CAN_CLOCK_OSC                                                   *
\* All other fields are ignored.                                             */
extern CAN0CTL1STR CAN_OPTIONS;
/* CAN_TIMING1, CAN_TIMING2                                                  *
 * By default, these specify a 500K baud rate for an oscillator clock        *
 * source. Modify to manually adjust bit timings (see HCS12 manual).         *
 * Use the CAN_init(baudrate) function to work with default settings for a   *
 * specified baud rate.                                                      */ 
extern CAN0BTR0STR CAN_TIMING1;
extern CAN0BTR1STR CAN_TIMING2;

/* Typedefs */

/** 
 * The can_id union describes how a CAN identifier is mapped to the HCS12 
 * registers.
 */
typedef union can_id {
  struct {
    INT16U id291    :  3; ///< Second part of ID (if IDE is set).
    INT16U IDE      :  1; ///< Extended ID bit.
    INT16U RTR11    :  1; ///< Transmit request bit for normal IDs.
    INT16U id11     : 11; ///< The most significant.
    INT16U RTR29    :  1; ///< Transmit request bit for extended IDs.
    INT16U id292    : 15; ///< Third part of ID (if IDE is set).
  } fields; ///< Bit field decoding of the ID register type.
  INT32U int32; ///< The whole ID as a 32-bit code.
} CAN_ID;

typedef struct can_msg {
  CAN_ID id;
  INT8U data[8];     
  INT8U datalen  : 4;
  INT8U reserved : 4;
  INT8U localprio;
  INT16U timestamp;
} CAN_MSG;

/* Return values for interface functions */
typedef INT8U CAN_RESULT;
#define CAN_OK               0x00
#define CAN_TOO_MANY_QUEUES  0x01
#define CAN_SEND_BUFFER_FULL 0x02
#define CAN_USER_ERROR       0x04
#define CAN_STILL_IN_USE     0x08
#define CAN_NO_MUTEX         0x10
#define CAN_MISC_ERROR       0x20

/** Function Prototypes */

#ifdef __cplusplus
extern "C" {
#endif

CAN_ID CANId11(INT16U id);
CAN_ID CANId29(INT32U id);
INT32U CANId(CAN_MSG* msg);

INT8U CANLastRxError(void);

CAN_RESULT CANInit(void);
CAN_RESULT CANStart(void);
CAN_RESULT CANConfigureBaudrate(INT32U baudrate, INT8U syncjump);
CAN_RESULT CANSendFrame(CAN_ID u32ID, INT8U u8Prio, INT8U u8Length, INT8U *u8TxData);
CAN_RESULT CANRegister(INT8U nids, INT32U* ids, OS_EVENT* queue);
CAN_RESULT CANUnregister(OS_EVENT* queue);
CAN_RESULT CANForget(CAN_MSG* msg);

#ifdef __cplusplus
}
#endif

#endif // _CAN_DRIVER_HPP

/*******************************************************************************/
