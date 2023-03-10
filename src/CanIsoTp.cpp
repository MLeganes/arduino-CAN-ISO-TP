#include "CanIsoTp.hpp"

CanIsoTp::CanIsoTp() {}
CanIsoTp::~CanIsoTp() { CAN.end(); }

int CanIsoTp::begin(long baudRate)
{
    if (!CAN.begin(baudRate))
    {
        Serial.println("Error: start CAN Bus failed!");
        while (1)
            ;
    }
    return 0;
}

int CanIsoTp::send(pdu_t *pdu)
{
    uint8_t ret = 0;
    uint8_t bs = false;

    pdu->cantpState = CANTP_SEND;
    while (pdu->cantpState != CANTP_IDLE && pdu->cantpState != CANTP_ERROR)
    {
        bs = false;
        switch (pdu->cantpState)
        {
        case CANTP_SEND:
            // TX_DL - 1 = 7 Case of normal addressing (ISO pag. 18)
            if (pdu->len <= 7)
            {
                ret = send_SingleFrame(pdu);
                pdu->cantpState = CANTP_IDLE;
            }
            else
            {
                ret = send_FirstFrame(pdu);
                pdu->cantpState = CANTP_WAIT_FIRST_FC;
                Serial.printf("\n\t *** Can-State: CANTP_WAIT_FIRST_FC  TimeStamp: %i millis   \n", millis());
                _timerFCWait = millis(); // Start the timer to check timeout
            }
            break;

        case CANTP_WAIT_FIRST_FC: // Checking only timeouts
            if ((millis() - _timerFCWait) >= TIMEOUT_FC)
            {
                pdu->cantpState = CANTP_IDLE;
                ret = 1;
                Serial.printf("\n *** TimeOut -- WAIT_FIRST_FC  %i millis   \n", millis());
            }
            break;

        case CANTP_WAIT_FC: // Checking only timeouts
            if ((millis() - _timerFCWait) >= TIMEOUT_FC)
            {
                pdu->cantpState = CANTP_IDLE;
                ret = 1;
                Serial.printf("\n *** TimeOut -- WAIT_FC  %i millis   \n", millis());
            }
            break;

        case CANTP_SEND_CF:
            // BS = 0 send everything.
            if (pdu->blockSize == 0)
            {
                _bsCounter = 4095;

                bs = false;
                while (pdu->len > 7 && _bsCounter > 0)
                {
                    delay(pdu->separationTimeMin);
                    if (!(ret = send_ConsecutiveFrame(pdu)))
                    {
                        pdu->seqId++;
                        pdu->data += 7;
                        pdu->len -= 7;
                        if (pdu->len == 0)
                            pdu->cantpState = CANTP_IDLE;
                    }                                                                    // End if
                }                                                                        // End while
                if (pdu->len <= 7 && _bsCounter > 0 && pdu->cantpState == CANTP_SEND_CF) // Last block. /!\ bsCounter can be 0.
                {
                    delay(pdu->separationTimeMin);
                    ret = send_ConsecutiveFrame(pdu);
                    pdu->cantpState = CANTP_IDLE;
                } // End if
            }

            // BS != 0, send by blocks.
            else
            {
                bs = false;
                _bsCounter = pdu->blockSize;
                Serial.printf("\n *** Bloc Size = %i \n", _bsCounter);
                while (pdu->len > 7 && !bs)
                {
                    delay(pdu->separationTimeMin);
                    if (!(ret = send_ConsecutiveFrame(pdu)))
                    {
                        pdu->data += 7; // Update pointer.
                        pdu->len -= 7;  // 7 Bytes sended.
                        pdu->seqId++;
                        if (_bsCounter == 0 && pdu->len > 0)
                        {
                            pdu->cantpState = CANTP_WAIT_FC;
                            bs = true;
                            _timerFCWait = millis();
                        }
                        else if (pdu->len == 0)
                        {
                            pdu->cantpState = CANTP_IDLE;
                            bs = true;
                        }
                    }                     // End if
                }                         // End while
                if (pdu->len <= 7 && !bs) // Last block.
                {
                    delay(pdu->separationTimeMin);
                    ret = send_ConsecutiveFrame(pdu);
                    pdu->cantpState = CANTP_IDLE;
                } // End if
            }
            break;
        case CANTP_IDLE:
        default:
            break;
        } // End Switch.

        // Read CAN bus for FC
        if (pdu->cantpState == CANTP_WAIT_FIRST_FC || pdu->cantpState == CANTP_WAIT_FC)
        {
            _rxPacketLen = CAN.parsePacket();
            if (_rxPacketLen > 0)
            {
                if (CAN.packetId() == (long)pdu->rxId) // Response from Remote ECU
                {
                    CAN.readBytes(_rxPacketData, _rxPacketLen);
                    // check if it the flowcontrol frame
                    if ((_rxPacketData[0] & 0xF0) == N_PCItypeFC)
                    {
                        receive_FlowControlFrame(pdu);
                    }
                }
            }
        } // End Reading CAN bus.

    } // End while.
    pdu->cantpState = CANTP_IDLE;
    return ret;
}

int CanIsoTp::receive(pdu_t *rxpdu)
{
    uint8_t ret = -1;
    uint8_t N_PCItype = -1;

    _timerSession = millis();
    rxpdu->cantpState = CANTP_IDLE;

    while (rxpdu->cantpState != CANTP_END && rxpdu->cantpState != CANTP_ERROR)
    {
        if (millis() - _timerSession >= TIMEOUT_SESSION)
        {
            Serial.printf("**** TimeOut -- receiver time: %i millis \n", millis());
            return 1;
        }

        _rxPacketLen = CAN.parsePacket();
        if (_rxPacketLen > 0)
        {
            if (CAN.packetId() == (long)rxpdu->rxId)
            {
                CAN.readBytes(_rxPacketData, _rxPacketLen);
                // Get N_PDUtype Frame from Payload-can-isotp
                N_PCItype = (_rxPacketData[0] & 0xF0);
                switch (N_PCItype)
                {
                case N_PCItypeSF:
                    ret = receive_SingleFrame(rxpdu);
                    break;
                case N_PCItypeFF:
                    ret = receive_FirstFrame(rxpdu);
                    break;
                case N_PCItypeFC:
                    ret = receive_FlowControlFrame(rxpdu);
                    break;
                case N_PCItypeCF:
                    ret = receive_ConsecutiveFrame(rxpdu);
                    break;
                default:
                    break;
                } // end switch
                memset(_rxPacketData, 0, PACKET_SIZE);
            } // end if
        }     // end if
    }         // end while
    return ret;
}

int CanIsoTp::send_ConsecutiveFrame(pdu_t *pdu)
{
    uint8_t payload[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t sizeToCopy = 7;

    payload[0] = (N_PCItypeCF | (pdu->seqId & 0X0F));

    if (pdu->len > 7)
        sizeToCopy = 7;
    else
        sizeToCopy = pdu->len;
    memcpy(&payload[1], pdu->data, sizeToCopy);

    // To delete!
    Serial.printf("\n********* send ConsecutiveFrame PCI type FC: %i Seq Id: %i \n", N_PCItypeFC, pdu->seqId + 1);
    Serial.print(" Bytes: ");
    for (int i = 0; i < sizeToCopy; i++)
        Serial.printf("%i ", payload[i]);
    Serial.println(" ");

    // Send can frame
    CAN.beginPacket(pdu->txId);
    CAN.write(payload, 8);
    CAN.endPacket();
    _bsCounter--;
    return 0;
}
int CanIsoTp::receive_ConsecutiveFrame(pdu_t *pdu)
{
    Serial.printf("****[1] receive ConsecutiveFrame status: %d: \n", pdu->cantpState);

    uint32_t timeDiff = millis() - _timerCFWait;
    _timerCFWait = millis();

    if (pdu->cantpState != CANTP_WAIT_DATA)
        return 0;
    if (_rxRestBytes <= 7)
    {
        memcpy(pdu->data + 6 + 7 * (pdu->seqId - 1), _rxPacketData + 1, _rxRestBytes);
        pdu->cantpState = CANTP_END;
        Serial.printf("****[4] receive_ConsecutiveFrame status: %d: seqId: %d \n", pdu->cantpState, pdu->seqId);
    }
    else
    {
        memcpy(pdu->data + 6 + 7 * (pdu->seqId - 1), _rxPacketData + 1, 7);
        Serial.printf("****[5] last ConsecutiveFrame status: %d: seqId: %d \n", pdu->cantpState, pdu->seqId);
        _rxRestBytes -= 7;
    }

    pdu->seqId++;
    return 0;
}

int CanIsoTp::send_FlowControlFrame(pdu_t *pdu)
{
    // To send can-isotp pdu
    uint8_t payload[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    // N_PCI:
    // Byte 0 = Type(bits 7-4) + FlowControlStatus(bits 3-0)
    payload[0] = (N_PCItypeFC | pdu->fcStatus);
    // Byte 1 = BlockSize(bits 7-0)
    payload[1] = pdu->blockSize;
    payload[2] = pdu->separationTimeMin;

    Serial.printf("\n********* send_FlowControlFrame PCI type FC: %d Total-len: %d \n", N_PCItypeFC, sizeof(payload));

    // Send can frame
    CAN.beginPacket(pdu->txId);
    CAN.write(payload, 8);
    CAN.endPacket();

    Serial.printf("\n Time to receive FF and Send FC frames: %i micros.\n", (micros() - timerStart));
    return 0;
}

int CanIsoTp::receive_FlowControlFrame(pdu_t *pdu)
{
    Serial.print("**** Received FlowControlFrame ");
    Serial.printf(" len: %i  Bytes: ", _rxPacketLen);
    for (int i = 0; i < (int)_rxPacketLen; i++)
        Serial.printf("%i ", _rxPacketData[i]);
    Serial.println(" ");

    uint8_t ret = 0;

    if (pdu->cantpState != CANTP_WAIT_DATA && pdu->cantpState != CANTP_WAIT_FIRST_FC && pdu->cantpState != CANTP_WAIT_FC)
        return 0;
    if (pdu->cantpState == CANTP_WAIT_FIRST_FC)
    {
        pdu->blockSize = _rxPacketData[1];
        pdu->separationTimeMin = _rxPacketData[2]; // 0x7F, by defaul 127ms
        if ((pdu->separationTimeMin > 0x7F && pdu->separationTimeMin < 0xF1) || (pdu->separationTimeMin > 0xF9))
            pdu->separationTimeMin = 0x7F;
    }

    // Get FlowControl state
    switch (_rxPacketData[0] & 0x0F)
    {
    case CANTP_FLOWSTATUS_CTS: // continue to send
        pdu->cantpState = CANTP_SEND_CF;
        Serial.println(" FC-Status: CANTP_FC_CTS ");
        break;

    case CANTP_FLOWSTATUS_WT:
        _receivedFCWaits++;
        if (_receivedFCWaits >= WFTmax)
        {
            _receivedFCWaits = 0;
            pdu->cantpState = CANTP_IDLE;
            Serial.println(" FC-Status: CANTP_FC_WAIT ");
            ret = 1;
        }
        break;

    case CANTP_FLOWSTATUS_OVFL:
    default:
        pdu->cantpState = CANTP_IDLE;
        Serial.println(" FC-Status: CANTP_FC_OVFLW ");
        ret = 1;
        break;
    }
    Serial.printf("\t Can-State: %i, Block Size: %i, TSmin: %i\n", pdu->cantpState, pdu->blockSize, pdu->separationTimeMin);
    return ret;
}

int CanIsoTp::send_FirstFrame(pdu_t *pdu)
{
    // To send can-isotp pdu
    uint8_t payload[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    // N_PCI:
    // Byte 0 = Type(bits 7-4) + DL(bits 3-0 data-length)
    // Byte 1 = DL(bits 7-0 rest of data-length)
    payload[0] = (N_PCItypeFF | ((pdu->len & 0x0F00) >> 8));
    payload[1] = (pdu->len & 0x00FF);
    memcpy(&payload[2], pdu->data, 6); // Only copy 6Bytes to transfer.

    Serial.printf("*** Send FirstFrame PCI type SF: %d Total-len: %d ", N_PCItypeFF, sizeof(payload));
    Serial.print(" Bytes: ");
    for (int i = 0; i < 8; i++)
        Serial.printf("%i ", payload[i]);
    Serial.println(" ");

    // Send can frame
    CAN.beginPacket(pdu->txId);
    CAN.write(payload, 8);
    CAN.endPacket();

    pdu->len -= 6;  // Len is -6 now, already sended the first 6.
    pdu->data += 6; //!\\ Increasing pointer!!!
    pdu->seqId = 1; // SequenceNumber = 0 for FF.
    _receivedFCWaits = 0; // Reset counter
    return 0;
}

int CanIsoTp::receive_FirstFrame(pdu_t *pdu)
{
    Serial.println("****rcv switch ***** receive_FirstFrame");

    timerStart = micros(); // time from start to end send FlowControl Frame.
    pdu->seqId = 1;
    pdu->len = (_rxPacketData[0] & 0x0F << 8); // len is 16bits: 0000 XXXX. 0000 0000.
    pdu->len += _rxPacketData[1];              // len is 16bits: 0000 0000. XXXX XXXX.
    _rxRestBytes = pdu->len;

    memcpy(pdu->data, &_rxPacketData[2], 6);
    _rxRestBytes -= 6;
    pdu->cantpState = CANTP_WAIT_DATA;

    // Send 1° FlowControlFrame
    pdu_t pdu_fc;
    pdu_fc.txId = pdu->txId;
    pdu_fc.rxId = pdu->rxId;
    pdu_fc.fcStatus = CANTP_FLOWSTATUS_CTS; // FlowControl state
    pdu_fc.blockSize = 0;
    pdu_fc.separationTimeMin = 0x7F;
    return send_FlowControlFrame(&pdu_fc);
}

int CanIsoTp::send_SingleFrame(pdu_t *pdu)
{
    // To send can-isotp pdu
    uint8_t payload[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    // Byte 0: PCI(bits 7-4 Frame-Type) + DL(bist 3-0 datalen)
    payload[0] = (N_PCItypeSF | pdu->len);
    // Bytes 1-7: insert data
    memcpy(&payload[1], pdu->data, pdu->len);

    // Send can pdu
    CAN.beginPacket(pdu->txId);
    CAN.write(payload, pdu->len + 1);
    CAN.endPacket();
    pdu->cantpState = CANTP_END;
    return 0;
}

int CanIsoTp::receive_SingleFrame(pdu_t *pdu)
{
    pdu->len = _rxPacketData[0] & 0x0F;
    memcpy(pdu->data, &_rxPacketData[1], pdu->len);
    pdu->cantpState = CANTP_END;
    return 0;
}

void CanIsoTp::end() { CAN.end(); }
