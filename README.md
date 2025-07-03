Description

The smart door lock system consists of a smart door lock device
and a gateway. The smart door lock device consists of NRF52840
microcontroller while the gateway contains ESP32.

The user can unlock/lock the door using a BLE APP or WiFi App.
If WiFi App is used, the gateway will fetch the command from
the WiFi App and will send the command to the smart lock device
through BLE.

ToDo:
 - implement TLS on MQTT
 - Add application layer out of band pairing to smart lock device
   to ease the user experience on pairing.
