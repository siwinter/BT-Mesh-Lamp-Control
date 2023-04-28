| Supported Targets | ESP32 |
| ----------------- | ----- |

ESP Serial to BLE Mesh Client
========================

A proof of concept basing on ESP32 idf BLE Mesh onff_client example.
Made to control a simple BT-lamp (Ledvance Volkslicht).

ESP32 acts as serial to BT bridge.
It opens a serial port (UART1 with RX = and TX =)

Controll the lamp with inputs:

*>lamp:on*    to switc lamp on

*>lamp:off*   to switch lamp off

*>lamp:25*  to dim the lamp values between 0..50 

First of all you neet to provision and configure lamp and ESP32.

At least generic switch on off and lightness models of server and client have to be configured with same app key.

Lamp needs to publish states (on/off ot lightness) to address or ESP (or address 0xffff for all) to provide ESP32 with its address.

>**Notes:**
>
>1. The NetKey index and AppKey index are fixed to 0x0000 in this demo.
>2. If the client device is re-provisioned, but the server device is not, the first few get/set messages from the client will be treated as replay attacks. To avoid this, both devices should be re-provisioned prior to transmitting messages.
