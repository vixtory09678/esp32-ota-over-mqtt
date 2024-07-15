# ESP32 OTA over MQTT

This repository is part of the blog post [ESP32 Firmware OTA ด้วย MQTT Protocol (Concept + Code ตัวอย่าง)](https://medium.com/i-am-team/esp32-firmware-ota-%E0%B8%94%E0%B9%89%E0%B8%A7%E0%B8%A2-mqtt-protocol-c958d3b8b4d3) (currently in Thai) on the blog [I AM {TEAM}]([https://iamteam.me](https://medium.com/i-am-team))

## Video Example
[![ESP32 OTA over MQTT](https://img.youtube.com/vi/sleYzXvy9I4/0.jpg)](https://www.youtube.com/watch?v=sleYzXvy9I4)

## Required
- python 3+
- platformio

## How to use
There is a Python script in the `scripts/ota_update.py` directory. You can use your own firmware binary file for testing, and the file must be named firmware.bin.

First, set up the MQTT broker locally. You can use Mosquitto as the MQTT broker server or any other MQTT broker tools you are familiar with.

Then run the following command. This script will read the firmware.bin file and start sending the firmware to the `ota` topic. If the ESP32 or your board successfully receives the firmware, it will return the status `ok` to the `ota/feedback` topic, and then `ota_update.py` will send the next chunk of the package.

```python
python3 ./scripts/ota_update.py
```
