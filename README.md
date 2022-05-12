# ESP32 OTA over MQTT

Repository นี้ เป็น Repo ประกอบเนื้อหาบทความ ของ blog [I AM {TEAM}](https://iamteam.me)

## Video Example
[![ESP32 OTA over MQTT](https://img.youtube.com/vi/sleYzXvy9I4/0.jpg)](https://www.youtube.com/watch?v=sleYzXvy9I4)

## Required
- python (ตอนผม test ใช้ python3)
- platformio

## How to use
file python script อยู่ใน `scripts/ota_update.py` สามารถนำ binary file ที่เป็น firmware มาทดสอบได้ และต้องตั้งชื่อ file เป็น firmware.bin

อันดับแรกควร setup ตัว mqtt broker ใน localhost ขึ้นมาก่อน สามารถใช้ตัว mosquitto ได้

จากนั้น run คำสั่ง

```python
python3 ./scripts/ota_update.py
```