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

## Message structure: 

*>msgType/device/command:parameter*

```
msgType
    cmd : command send to device
    evt : event received from device
device
    BT_x : where x is the device's BT-Mesh address ( ex: BT_ffff for all devices)
command
    state : to switch lamp (cmd) or to report state (evt)
            parameter "on" or "off"
    light : to dim lamp (cmd) or to report lightness (evt)
            parameter int value within ligtness range (0 .. 50)
    get   : to request an event from device (cmd)
            parameter: "range" or "state" or "light"
```
## Examples
```
>cmd/BT_ffff/state:on       // switch all devices on
>evt/BT_11/state:on         // device with address 11 is switched on
>evt/BT_11/ligth:25         // lightnes of device is 25
>cmd/BT_11/state:off        // switch device off
>evt/BT_11/state:off        // device is switched off
>evt/BT_11/ligth:0          // lightnes of device is 0
>cmd/BT_11/get:range        // get range of lightness
>evt/BT_11/range:1-50       // range is 1 to 50
>cmd/BT_11/light:50         // set lightness to 50
>cmd/BT_11/get:state        // get switching state (on or off)
>cmd/BT_11/get:light        // get lightness
```
>**Notes:**
>
>1. If the client device is re-provisioned, but the server device is not, the first few get/set messages from the client will be treated as replay attacks. To avoid this, both devices should be re-provisioned prior to transmitting messages.
