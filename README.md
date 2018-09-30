# Robot communication firmware

This repository contains Arduino code for the ESP8266
for communicating over MQTT/TCP/WiFi with the robothub.

It uses the ESP8266 hardware SPI in slave mode to talk to the STM32.
New data is indicated by raising the `DATA_READY` pin.

For a reference of the SPI protocol, see the `SPISlave_Master` example in the `SPISlave` library.

The status register contains the robot id,
and the first 3 data transfers set the wifi ssid/password/mqtt server.
