| Supported Targets | ESP32 |
| ----------------- | ----- |

ESP Serial to BLE Mesh Bridge
========================

A proof of concept basing on ESP32 idf BLE Mesh onff_client example.
Made to control a simple BT-lamp (Ledvance Volkslicht).

ESP32 acts as serial to BTMesh bridge.
It opens a serial port (UART1 with RXD = pin 5 and TXD = pin 4)

Configuration
-------------
Provision and Configure Lamp and ESP with the nRF Mesh app.
Setup at least generic onOff and lighting model with the same app key.
Let the lamp publish its state periodically.

Usage
-----
Send and receive text messages on the serial port.
Message begins with ">" and ends with newline (\n).

Message structure: 

*>msgType/device/command:parameter*

msgType
  cmd : command send to device
  evt : event received from device

device
  lamps : for all devices ist mapped to BT-Mesh addresse 0xFFFF
  lampx : for a specific device (x = 1...n)

command
  state : to switch lamp (cmd) or to report state (evt)
          parameter "on" or "off"
  light : to dim lamp (cmd) or to report lightness (evt)
          parameter int value within ligtness range (0 .. 50)
  get   : to order report from device (cmd)
          parameter:  range


Controll the lamp with inputs:


*>lamp:on*    to switch lamp on

*>lamp:off*   to switch lamp off

*>lamp:light,25*  to dim the lamp values between 0..50 

First of all you need to provision and configure lamp and ESP32.

>**Notes:**
>
>1. If the client device is re-provisioned, but the server device is not, the first few get/set messages from the client will be treated as replay attacks. To avoid this, both devices should be re-provisioned prior to transmitting messages.




examples
>cmd/lamps/state:on
>evt/lamp1/state:on
>evt/lamp1/ligth:25
>cmd/lamp1/state:off
>evt/lamp1/state:off
>evt/lamp1/ligth:0
>cmd/lamp1/light:50
>cmd/lamp1/get:range
>cmd/lamp1/get:state
>cmd/lamp1/get:light

