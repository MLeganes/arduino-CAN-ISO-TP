#pragma once

#include <CAN.h>

/* ************************************************************************** */
/* DEFINES																	  */
/* ************************************************************************** */
// Timeouts in ms.
#define TIMEOUT_SESSION                         1000
#define TIMEOUT_FC                              1000
#define TIMEOUT_CF                              1000
#define WFTmax                                  10      // Maximum limit number of CANTP_FC_WAIT.

// Network protocol control information type.
#define N_PCItypeSF                             0x00        //Single Frame 
#define N_PCItypeFF                             0x10        //First Frame
#define N_PCItypeCF                             0x20        //Consecutive Frame
#define N_PCItypeFC                             0x30        //Flow Control Frame

#define BUFFER_SIZE                             128        // Buffer size to Tx/Rx nanopb data.
#define PACKET_SIZE                             8          // Can Frames size

/* flow status codes */
// Flow Control autorization (ISO pag. 20)
#define CANTP_FLOWSTATUS_CTS                     (0x00u) // FC continue to send.
#define CANTP_FLOWSTATUS_WT                      (0x01u) // FC the request to continue to wait.
#define CANTP_FLOWSTATUS_OVFL                    (0x02u) // FC buffer overflow.


// // blockSize
//     0
//     No further Flow Control Frame will be sent by the receiver. The value that indicates this can be changed.
//     1—127
//     The transmitter should send this many Consecutive Frames before waiting for a Flow Control Frame.

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
