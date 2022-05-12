import hashlib
import sys
import time
import paho.mqtt.client as mqtt
import threading

MQTT_HOST = 'localhost'
MQTT_PORT = 1883
MQTT_UPDATE_FIRMWARE_TOPIC = 'ota'
MQTT_UPDATE_FIRMWARE_FEEDBACK_TOPIC = 'ota/feedback'
PAYLOAD_SIZE = 1024

class OtaPacket:
	def __init__(self, payload, index):
		self.payload = payload
		self.index = index

	def getPayloadSize(self): 
		return len(self.payload)

	def getIndex(self):
		return self.index

	def getPayload(self):
		return self.payload

	def getPacketMD5(self):
		result = hashlib.md5(self.payload)
		return result.digest()

	def build(self):
		package = bytearray(b'')

		sizeIndicator = self.getPayloadSize().to_bytes(2, 'big')
		package += sizeIndicator
		package += self.getPayload()
		package += self.getPacketMD5()
		
		return package

f = open('firmware/firmware.bin', 'rb')
firmware = f.read()

totalFile = len(firmware)
countOfPacket = int(totalFile / PAYLOAD_SIZE)
lastPacketSize = totalFile % PAYLOAD_SIZE

if (lastPacketSize != 0):
	countOfPacket += 1

print('size of file {}'.format(totalFile))
print('count of packet {}'.format(countOfPacket))
print('last packet size is {}'.format(lastPacketSize))

time.sleep(1)

packetList = []
packetBuildList = []
startIndex = 0
endIndex = PAYLOAD_SIZE

print('preparing packet in a while')
for i in range(countOfPacket):
	packetList.append(OtaPacket(firmware[startIndex:endIndex], i))
	startIndex += PAYLOAD_SIZE
	endIndex += PAYLOAD_SIZE

time.sleep(3)

print('the packet already to be sent')
for packet in packetList:
	remaining = (countOfPacket - (packet.getIndex() + 1)).to_bytes(2, 'big')
	packetToBeSent = packet.build()
	packetToBeSent += remaining
	packetBuildList.append(packetToBeSent)

# MQTT PROCESS
lastIndexProcess = 0
def onDataReceive(client, userdata, msg):
	global lastIndexProcess
	if (msg.topic == MQTT_UPDATE_FIRMWARE_FEEDBACK_TOPIC):
		print('[{}] payload check => {}'.format(lastIndexProcess, msg.payload.decode('utf-8')))
		if (msg.payload.decode('utf-8') == 'ok'):
			
			if (lastIndexProcess == len(packetBuildList)):
				return

			client.publish(MQTT_UPDATE_FIRMWARE_TOPIC, packetBuildList[lastIndexProcess])
			lastIndexProcess += 1
		elif (msg.payload.decode('utf-8') == 'success'):
			print("firmware update success")
		else:
			client.publish(MQTT_UPDATE_FIRMWARE_TOPIC, packetBuildList[lastIndexProcess - 1])
		
def onConnect(client, userdata, flags, rc):
	global lastIndexProcess
	print('broker connected')
	client.subscribe(MQTT_UPDATE_FIRMWARE_FEEDBACK_TOPIC)

	sizePacket = totalFile.to_bytes(4, 'big')
	print('send size first', sizePacket)
	client.publish(MQTT_UPDATE_FIRMWARE_TOPIC, sizePacket)
	time.sleep(2)

	print('then send first packet')
	client.publish(MQTT_UPDATE_FIRMWARE_TOPIC, packetBuildList[lastIndexProcess])
	lastIndexProcess += 1

print('preparing mqtt in a while')
client = mqtt.Client()
client.on_connect = onConnect
client.on_message = onDataReceive

client.connect(MQTT_HOST, MQTT_PORT, 60)

def mqttProcess():
	print('mqtt process is started')
	client.loop_forever()


mqttBackgroundThread = threading.Thread(target=mqttProcess)
mqttBackgroundThread.start()