# Arduino CAN ISO TP
An Arduino library for sending and receiving data using ISO 15765-2 or ISO-TP (Transport Layer) over CAN-Bus protocol. Support segmented communication point-to-point between CAN nodes via two defined CAN Identifiers. Base on 11 bit CAN identifier scheme address.

         _____________
        |             |
        |  CanIsoTp   |
        |(ISO 15765-2)|
        |_____________|
        |             |
        |    CAN      |
        |(ISO 11898-1)|
        |_____________|
               |
     |--------------------| CAN bus

## Implementation
The CAN-ISO-TP soport in transmision and reception:
* Single Frames (SF).
* First Frame (FF).
* Flow Control Frame (FC), Block Size (BS) and mimimum Separation Time (STmin).
* Consecutive Frames.
* Timeout Session.
* Timeout FlowControl.
* Timeout WFTmax (Maximum limit number of CANTP_FC_WAIT).

## Dependencies
This proyect use the library [arduino-CAN](https://github.com/adafruit/arduino-CAN) from [Adafruit](https://www.adafruit.com/) and it is compatible with the following hardware:
* [Adafruit Feather M4 CAN Express with ATSAME51](https://www.adafruit.com/)
* [SAM E5x microcontrollers](https://www.digikey.com/catalog/en/partgroup/sam-d5x-e5x/70620)
* [Microchip MCP2515](http://www.microchip.com/wwwproducts/en/en010406) based boards/shields
* [Arduino MKR CAN shield](https://store.arduino.cc/arduino-mkr-can-shield)
* [Espressif ESP32](http://espressif.com/en/products/hardware/esp32/overview)'s built-in [SJA1000](https://www.nxp.com/products/analog/interfaces/in-vehicle-network/can-transceiver-and-controllers/stand-alone-can-controller:SJA1000T) compatible CAN controller with an external 3.3V CAN transceiver

## Compatible Hardware
This protocol has been developed and tested wiht:
* [Adafruit Feather M4 CAN Express](https://www.digikey.com/catalog/en/partgroup/sam-d5x-e5x/70620)

## Installation using Git

```sh
cd ~/ProjectFolder
git clone https://github.com/MLeganes/arduino-CAN-ISO-TP.git
```
## Examples
Check the folder [/example](/example/main_example.cpp).

## License
This library is [licensed](LICENSE) under the [MIT Licence](http://en.wikipedia.org/wiki/MIT_License).