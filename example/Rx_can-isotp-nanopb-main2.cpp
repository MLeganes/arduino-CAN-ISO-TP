#include "../proto/pb_sensor.pb.c"
#include "pb.h"
#include "pb_common.h"
#include "pb_encode.h"
#include "pb_decode.h"

#include "../lib/arduino-can-isotp/CanIsoTp.hpp"

/***
 * Global vars
 *
 */
unsigned char bytesArray[4];

// NanoPb
// uint8_t pb_buffer[32];
pb_ostream_t pb_ostream;
pb_istream_t pb_istream;
MsgSensorType1 msgsensor;

// CanIsoTp vars
CanIsoTp cantp;
pdu_t cantp_pdu;                   // Pdu to Rx
uint8_t buff[MsgSensorType1_size]; // Buffer to tx/rx data

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Starting CAN Rx");
  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false); // turn off STANDBY
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true); // turn on booster

  // CanIsoTp setup
  cantp.begin(1000000);
  cantp_pdu.txId = 0x200; // CanId this device.
  cantp_pdu.rxId = 0x100; // CanId sensorDistance.
  cantp_pdu.data = buff;  // Pointer to buffer

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

void clean_pdu()
{
    cantp_pdu.cantpState = CANTP_IDLE;
    memset(buff, 0, MsgSensorType1_size); // 24 is from nanapb.
    cantp_pdu.data = buff;
    cantp_pdu.bs = 0;
    cantp_pdu.len = 0;
    cantp_pdu.seqId = 0;
    cantp_pdu.blockSize = 0;
    cantp_pdu.separationTimeMin = 0;
    cantp_pdu.fcStatus = CANTP_FC_CTS;
}

void loop()
{

  cantp.receive(&cantp_pdu);

  // Decode NanoPb
  // Serial.printf("Start Decoding .....\n\tdata len: %i \n", cantp_pdu.len);
  // Serial.printf("Before decode cantp_pdu.data  bytes: %i %i %i %i %i %i %i %i \n", cantp_pdu.data[0], cantp_pdu.data[1], cantp_pdu.data[2], cantp_pdu.data[3], cantp_pdu.data[4], cantp_pdu.data[5], cantp_pdu.data[6], cantp_pdu.data[7]);
  // Serial.printf("Before decode buff            bytes: %i %i %i %i %i %i \n", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
  if (cantp_pdu.cantpState == CANTP_END)
  {

    // NanoProtobufffff init.
    uint32_t start_decode = micros();
    pb_istream = pb_istream_from_buffer(cantp_pdu.data, MsgSensorType1_size);
    bool pb_status = pb_decode(&pb_istream, MsgSensorType1_fields, &msgsensor);
    Serial.printf("End Decode packet. Timing: %lu microsecs. \n", (micros() - start_decode));

    if (pb_status)
    {
      Serial.printf("Decoding failed: %s\n", (pb_istream.errmsg));
    }
    Serial.printf("Decoding failed: %s\n", (pb_istream.errmsg));
    Serial.printf("Decoded msg: counter:%d distance:%f runningStamp:%i error:%i  ", msgsensor.counter, msgsensor.distance, msgsensor.runningstamp, msgsensor.error);

    clean_pdu();
    //Serial.println("LOOOOOOOOOPPP...Done!\n\n");
  }

  digitalWrite(LED_BUILTIN, LOW);

  // delay(500);
}
