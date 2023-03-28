#include <Arduino.h>
#include <CAN.h>        // https://github.com/adafruit/arduino-CAN
#include <CanIsoTp.hpp> // https://github.com/MLeganes/arduino-can-iso-tp

// CanIsoTp
CanIsoTp cantp;  // CAN IsoTp protocol to send and receive the pdus
pdu_t cantp_pdu; // Pdu used in CAN IsoTp to transmit the data.

uint8_t buff[128]; // Buffer used in the pdu.data.
uint8_t msgSF[] = { 0x00, 0x01, 0x02, 0x03, 0x04};
uint8_t msgCF[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15};

void setup()
{
    // Serial communitacion setup
    Serial.begin(115200);
    while (!Serial)
        ;

    // Feather CAN setup
    Serial.print("\nStarting Feather Setup...");
    pinMode(PIN_CAN_STANDBY, OUTPUT);
    digitalWrite(PIN_CAN_STANDBY, false); // turn off STANDBY
    pinMode(PIN_CAN_BOOSTEN, OUTPUT);
    digitalWrite(PIN_CAN_BOOSTEN, true); // turn on booster

    // CanIsoTp setup
    cantp.begin(100000);    // Baud Rate
    cantp_pdu.rxId = 0x200; // CanId rx.
    cantp_pdu.txId = 0x100; // CanId tx.

    Serial.println("...Done!");
}

void loop()
{
    // Single Frame
    Serial.println("Sending Single Frame");
    cantp_pdu.data = msgSF;
    cantp_pdu.len = 5;
    cantp.send(&cantp_pdu);
    delay(500);

    // Multiframe
    Serial.println("Sending Multiframe");
    cantp_pdu.data = msgCF;
    cantp_pdu.len = 22;
    cantp.send(&cantp_pdu);

    // Receiver
    Serial.println("Receiving Frames");
    cantp_pdu.data = buff;
    cantp.receive(&cantp_pdu);
    delay(500);
}
