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
#ifndef find_p
Relay_state locker[sizeRelay];
Adafruit_PN532 nfc(-1, -1);
rfid_state rfid(400);
buzzer_state buzzer(500);
ethernet_state eth;

database_s data[Size_Siswa];

void init_mypin()
{
#if defined(DEBUG_MEM) || defined(DEBUG_ETH) || defined(DEBUG_RFID)
	Serial.println("init my pins");
#endif
	for (size_t i = 0; i < sizeRelay; i++)
	{
		locker[i].pin = pin_IO[i];
		pinMode(locker[i].pin, OUTPUT);
		digitalWrite(locker[i].pin, HIGH);
		locker[i].interval = 100;
		locker[i].last_t = millis();
	}

	for (size_t i = 0; i < Size_Siswa; i++)
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
#if defined(DEBUG_MEM) || defined(DEBUG_ETH) || defined(DEBUG_RFID) || defined(DEBUG_SYS)
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
	// digitalWrite(buzzer.pin, LOW);
	while (1)
	{
		int freq = 3200;
		static int ritme = 25;
		Serial.print("in test=>");
		Serial.println(ritme);
		static unsigned long last_t = 0;
		static bool i = false;
		static byte custom_fl = 0, limit = 0;
		if (millis() - last_t > ritme)
		{
			if (i)
			{
				if (custom_fl > limit)
				{
					custom_fl = 0;
					continue;
				}
				tone(buzzer.pin, freq);
				custom_fl++;
			}
			else
				noTone(buzzer.pin);
			i = !i;
			last_t = millis();
		}

		if (Serial.available())
		{
			char c = Serial.read();
			int digit = 0;
			if (isdigit(c))
			{
				digit = (int)c;
				switch (digit)
				{
				case 1:
					ritme += 25;
					break;
				case 2:
					limit++;
					break;
				}
			}
		}
	}

#if defined(DEBUG_MEM) || defined(DEBUG_ETH) || defined(DEBUG_RFID) || defined(DEBUG_SYS)
	Serial.println("Device Start");
#endif
	eth.interupt_trigger = true;
}

void setup();
void loop()
{
// write to eeprom,handle relay,sync db
#ifdef DEBUG_SYS
	static unsigned long latency = 0;
	static unsigned long last_t = 0;
	last_t = millis();
	Serial.print("exec open door,");
#endif
	signed char card = 0;
	bool opened_door = rfid.open_doors(locker, eth.send_log);
#ifdef DEBUG_SYS
	Serial.print("exec acc main,");
#endif
	bool acc = buzzer.in_action();
	if (!opened_door && !acc)
	{
#ifdef DEBUG_SYS
		Serial.print("exec eth.loop,");
#endif
		eth.loop(data, &memory, locker);
#ifdef DEBUG_SYS
		Serial.print("exec rfid.loop,");
#endif
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
#ifdef DEBUG_SYS
	latency = millis() - last_t;
	Serial.println("latency processing=>" + String(latency));
#endif
}
#elif defined(find_p)
mapping_p map_p;

void setup()
{
	Serial.begin(baudRate_PC);
	map_p.begin();
	Serial.println("device start");
}
void loop()
{
	// map_p.bypass = true;
	map_p.main();
	static bool showing = true;
	if (map_p.bypass && showing)
	{
		for (size_t i = 0; i < sizeRelay; i++)
		{
			digitalWrite(pin_IO[i], LOW);
			delay(50);
			digitalWrite(pin_IO[i], HIGH);
			delay(50);
		}
		for (size_t i = 0; i < sizeRelay; i++)
		{
			digitalWrite(pin_IO[i], LOW);
			delay(50);
		}
		for (size_t i = 0; i < sizeRelay; i++)
		{
			digitalWrite(pin_IO[i], HIGH);
			delay(100);
		}
		for (size_t i = 0; i < sizeRelay; i++)
		{
			digitalWrite(pin_IO[i], LOW);
		}
		showing = false;
	}
}
#endif