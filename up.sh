echo "uploading firmware......"
cd 'locker v2 rfid'
pio run -t upload --upload-port /dev/ttyUSB0
echo "monitor device"
pio device monitor