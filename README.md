| Supported Targets | ESP32 |
| ----------------- | ----- |

ESP Serial to BLE Mesh Bridge
========================

A proof of concept basing on ESP32 idf BLE Mesh onff_client example.
Made to control a simple BT-lamp (Ledvance Volkslicht).

ESP32 acts as serial to BT bridge.
It opens a serial port (UART1 with RX = and TX =)

Configuration
-------------
Provision  and Configure Lamp and ESP with the nRF Mesh app.
Setup at least generic onOff and lighting model wit the same app key.
Let the lamp publish its state periodically.

Usage
-----

Controll the lamp with inputs:

*>lamp:on*    to switc lamp on

*>lamp:off*   to switch lamp off

*>lamp:light,25*  to dim the lamp values between 0..50 

First of all you neet to provision and configure lamp and ESP32.

>**Notes:**
>
>1. The NetKey index and AppKey index are fixed to 0x0000 in this demo.
>2. If the client device is re-provisioned, but the server device is not, the first few get/set messages from the client will be treated as replay attacks. To avoid this, both devices should be re-provisioned prior to transmitting messages.
