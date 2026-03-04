/**
 * | Pin Arduino Mega | Fungsi | Sambung ke PN532 |
 * | ---------------- | ------ | ---------------- |
 * | 2                | CS     |                  |
 * | 20               | SDA    | SDA              |
 * | 21               | SCL    | SCL              |
 * | GND              | GND    | GND              |
 * | 5V/3.3V          | VCC    | VCC              |
 *
 *
 */
// #define PN532DEBUG
// #define PN532DEBUGPRINT Serial

#include <commond.h>
#include <Wire.h>

#define baudRate_PC 9600

Relay_state locker[sizeRelay];
Adafruit_PN532 nfc(-1, -1);
rfid_state rfid(400);
buzzer_state buzzer(500);
ethernet_state eth;

database_s data[size_mahasiswa];

void init_mypin()
{
#if defined(DEBUG_MEM) || defined(DEBUG_ETH) || defined(DEBUG_RFID)
	Serial.println("init my pins");
#endif
	for (size_t i = 0; i < sizeRelay; i++)
	{
		locker[i].pin = pin_IO[i];
		pinMode(locker[i].pin, OUTPUT);
		locker[i].interval = 100;
		digitalWrite(locker[i].pin, HIGH);
		locker[i].last_t = millis();
	}

	for (size_t i = 0; i < size_mahasiswa; i++)
	{
		for (size_t xp = 0; xp < size_uid; xp++)
		{
			data[i].card[xp] = 0;
		}
		// strcpy(data[i].created_at, "");
		data[i].number_locker = 0;
		data[i].statusdb = Status_db::Available;
	}

	buzzer.pin = PIN_buzzer;
	// rfid.action = action_Card::None;
	buzzer.act = acc_action::acc_off;
	buzzer.mode = bz_mode::mode_fastloop4X;
	pinMode(buzzer.pin, OUTPUT);
	Wire.begin();
	SPI.begin();
	eth.begin(data);
	rfid.init_sensor(&nfc);
}
storage_state memory;
void setup()
{
	Serial.begin(baudRate_PC);
#if defined(DEBUG_MEM) || defined(DEBUG_ETH) || defined(DEBUG_RFID)
	Serial.begin(baudRate_PC);
	delay(2000);
#endif
	init_mypin();

	// #ifdef DEBUG_RFID
	// Serial.println("get board rfid");
	// uint32_t versiondata = nfc.getFirmwareVersion();
	// if (!versiondata)
	// {
	// 	Serial.println("PN532 board not found!");
	// }
	// else
	// {
	// 	Serial.println("rfid already use");
	// }
	// #endif
	memory.load_data(data);
	digitalWrite(buzzer.pin, LOW);
	delay(100);
	digitalWrite(buzzer.pin, HIGH);
	delay(100);
#if defined(DEBUG_MEM) || defined(DEBUG_ETH) || defined(DEBUG_RFID)
	Serial.println("Device Start");
#endif
	Serial.println("Device Start");
	eth.interupt_trigger = true;
}

void setup();
void loop()
{
	// write to eeprom,handle relay,sync db
	static unsigned long latency = 0;
	static unsigned long last_t = 0;
	signed char card = 0;
	last_t = millis();
	Serial.print("exec open door,");
	bool opened_door = rfid.open_doors(locker, eth.send_log);
	Serial.print("exec acc main,");
	bool acc = buzzer.in_action();
	if (!opened_door && !acc)
	{
		Serial.print("exec eth.loop,");
		eth.loop(data, &memory, locker);
		Serial.print("exec rfid.loop,");
		card = rfid.read_crd(data, &nfc, locker);
	}

	if (card == 1)
	{
		buzzer.mode = bz_mode::mode_fastloop1X;
		buzzer.act = acc_action::acc_on;
	}
	if (card == 1 || card == -4)
	{
		eth.interupt_trigger = true;
	}
	latency = millis() - last_t;
	Serial.println("latency processing=>" + String(latency));
}
