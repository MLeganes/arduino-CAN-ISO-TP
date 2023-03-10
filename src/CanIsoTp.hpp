#pragma once

// While importing CAN.h, CANController.h is there an object CAN already, no needed to createa new one.
#include <CAN.h>

/* ************************************************************************** */
/* DEFINES																	  */
/* ************************************************************************** */
// Timeouts in ms. ISO1565-2 Pag37.
#define TIMEOUT_SESSION                         1000
#define TIMEOUT_FC                              1000
#define TIMEOUT_CF                              1000
#define WFTmax                                  10      // Maximum limit number of CANTP_FC_WAIT.

// Network protocol control information type. (ISO pag. 21, 25)
#define N_PCItypeSF                             0x00        //Single Frame 
#define N_PCItypeFF                             0x10        //First Frame
#define N_PCItypeCF                             0x20        //Consecutive Frame
#define N_PCItypeFC                             0x30        //Flow Control Frame

#define BUFFER_SIZE                             128        // Buffer size to Tx/Rx nanopb data. (ISO pag. 20)
#define PACKET_SIZE                             8        // Can Frames size

/* flow status codes */
// Flow Control autorization (ISO pag. 20)
#define CANTP_FLOWSTATUS_CTS                     (0x00u) // FC continue to send, the authorization to continue
#define CANTP_FLOWSTATUS_WT                      (0x01u) // FC the request to continue to wait
#define CANTP_FLOWSTATUS_OVFL                    (0x02u) // FC buffer overflow. (ISO pag.20)


// // blockSize
//     0
//     No further Flow Control Frame will be sent by the receiver. The value that indicates this can be changed.
//     1—127
//     The transmitter should send this many Consecutive Frames before waiting for a Flow Control Frame.


// No used!!!!!
/* common length values */
#define CANTP_CAN20_FRAME_LENGTH                 8u         /* CAN 2.0 frame length                                  */
#define CANTP_FF_DL_12BIT                        4095u      /* max message length for FF with 12Bit DL               */
#define CANTP_FF_DL_16BIT                        65535u     /* max message length for FF with PduLengthType = uint16 */

// No used!!!!!
/* STmin values */
// STmin
//     0
//     The transmitter may send its Consecutive Frame as fast as it can.
//     1—127
//     The transmitter has to wait at least this number of milliseconds (ms) before sending another Consecutive Frame.
//     0xF1—0xF9
//     The transmitter has to wait at least 100—900 microseconds (µs) before sending another Consecutive Frame.
//     Other
//     reserved

#define CANTP_STMIN_MILLISEC_MAX                 (0x7Fu) // 127 ms
#define CANTP_STMIN_MICROSEC_MIN                 (0xF1u) // 241 ms
#define CANTP_STMIN_MICROSEC_MAX                 (0xF9u) // 249 ms

/* ************************************************************************** */
/* STRUCT ENUMS															  	  */
/* ************************************************************************** */

typedef enum {
    CANTP_IDLE,
    CANTP_SEND,
    CANTP_WAIT_FIRST_FC,
    CANTP_WAIT_FC,
    CANTP_SEND_CF,
    CANTP_WAIT_DATA,
    CANTP_END,
    CANTP_ERROR,
} cantpStates_t;

typedef struct  pdu_s
{
    uint32_t            txId = 0;
    uint32_t            rxId = 0;
    cantpStates_t       cantpState = CANTP_SEND;
    uint8_t             fcStatus =   CANTP_FLOWSTATUS_CTS;
    uint8_t             seqId = 0;
    uint8_t             blockSize = 0;
    uint8_t             bs = false;
    uint8_t             separationTimeMin = 0;
    uint8_t             *data;
    uint16_t            len =  0;
} pdu_t;

class CanIsoTp
{
public:
    CanIsoTp();
    ~CanIsoTp();

    int  begin(long baudRate);
    void end();
    int send(pdu_t *);
    int receive(pdu_t *);

private:
    // Testing
    unsigned long timerStart = 0;
    uint8_t _packetData[PACKET_SIZE]; 
    uint8_t _packetLen = 0;

    uint8_t _rxPacketData[PACKET_SIZE];
    uint8_t _rxPacketLen = 0;
    uint8_t _receivedFCWaits = 0;   // Counter for FlowControl receiver "CANTP_FC_WAIT" frames.
    uint16_t _rxRestBytes = 0;      // Counter to check total bytes to RX
    uint16_t _receivedBytes = 0;    // Byte counter for Rx to control the size of the data.
    unsigned long _timerFCWait = 0;  //uint16_t _timerCFWait = 0;
    unsigned long _timerCFWait = 0;
    unsigned long _timerSession = 0;

    uint8_t _bsCounter = 0;

    int send_SingleFrame(pdu_t *);
    int receive_SingleFrame(pdu_t *);

    int send_FirstFrame(pdu_t *);
    int receive_FirstFrame(pdu_t *);

    int send_ConsecutiveFrame(pdu_t *);
    int receive_ConsecutiveFrame(pdu_t *);

    int send_FlowControlFrame(pdu_t *);
    int receive_FlowControlFrame(pdu_t *);

    void checker_FlowControlDelay();
};
