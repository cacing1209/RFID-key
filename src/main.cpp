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
rfid_state rfid(500);
buzzer_state buzzer(75);
ethernet_state eth;

database_s data[Size_Siswa];
storage_state memory;

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
	rfid.init_sensor(&nfc);
#ifdef DEBUG_RFID
	tone(buzzer.pin, 3200);
	delay(1000);
	noTone(buzzer.pin);
#endif
	eth.begin(data, &memory);
}

void bypass_add_card()
{
	unsigned long dec_uid[Size_Siswa] = {
		3642173063,
		3642094375,
		3642092455,
		3642173047,
		3642170839,
		3642148695,
		3642146503,
		3642142119,
		3642144311,
		3642139943,
		3642096279,
		3642092407,
		3642090487,
		3642086727,
		3642088599,
		3642633751,
		3642630087,
		3642628295,
		3642624759,
		3642626519,
		3642623031,
		3642079479,
		3642077703,
		3642081271,
		3642083095,
		3642086775,
		3642084935,
		3642088647,
		3642090535,
		3642168615

	};

	database_s new_db[Size_Siswa];
	for (size_t i = 0; i < Size_Siswa; i++)
	{
		new_db[i] = data[i];
		memset(new_db[i].card, 0, size_uid);

		new_db[i].card[0] = (dec_uid[i]) & 0xFF;
		new_db[i].card[1] = (dec_uid[i] >> 8) & 0xFF;
		new_db[i].card[2] = (dec_uid[i] >> 16) & 0xFF;
		new_db[i].card[3] = (dec_uid[i] >> 24) & 0xFF;

		new_db[i].number_locker = i;
		new_db[i].statusdb = Status_db::Not_Available;
	}

	if (!memory.save_data(data, new_db))
	{
#if defined(DEBUG_MEM) || defined(DEBUG_SYS)
		Serial.println(":bypass:save_data FAILED");
#endif
		return;
	}
#if defined(DEBUG_MEM) || defined(DEBUG_SYS)
	Serial.println(":bypass:save_data OK");
#endif
}
void setup()
{
#if defined(DEBUG_MEM) || defined(DEBUG_ETH) || defined(DEBUG_RFID) || defined(DEBUG_SYS)
	Serial.begin(baudRate_PC);
	delay(6000);
#endif
	init_mypin();
	bypass_add_card();
	memory.load_data(data);
#if defined(DEBUG_MEM) || defined(DEBUG_ETH) || defined(DEBUG_RFID) || defined(DEBUG_SYS)
	Serial.println("Device Start");
#endif
	eth.interupt_trigger = true;
	for (size_t i = 0; i < 10; i++)
	{
		if (i % 2 == 0)
			tone(buzzer.pin, 3200);
		else
			noTone(buzzer.pin);
		delay(75);
	}
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
	bool opened_door = rfid.open_doors(locker, eth.send_log, &nfc);
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
		buzzer.mode = bz_mode::mode_fastloop2X;
		buzzer.act = acc_action::acc_on;
	}
	else if (card == -4)
	{
		buzzer.mode = bz_mode::mode_fastloop4X;
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
	delay(5000);
	map_p.bypass = true;
	map_p.begin();
	Serial.println("device start");
}
void loop()

// {1,2,3,4,5,13,15,20,18,17,10,7,6,9,8,29,16,28,12,25,14,11,23,19,27,21,26,22,24,	30}
{
	map_p.main();
	static bool showing = false;
	// if (map_p.bypass && showing)
	// {
	// 	for (size_t i = 0; i < sizeRelay; i++)
	// 	{
	// 		digitalWrite(pin_IO[i], LOW);
	// 		delay(50);
	// 		digitalWrite(pin_IO[i], HIGH);
	// 		delay(50);
	// 	}
	// 	for (size_t i = 0; i < sizeRelay; i++)
	// 	{
	// 		digitalWrite(pin_IO[i], LOW);
	// 		delay(50);
	// 	}
	// 	for (size_t i = 0; i < sizeRelay; i++)
	// 	{
	// 		digitalWrite(pin_IO[i], HIGH);
	// 		delay(100);
	// 	}
	// 	for (size_t i = 0; i < sizeRelay / 2; i++)
	// 	{
	// 		digitalWrite(pin_IO[i], LOW);
	// 		digitalWrite(pin_IO[sizeRelay - i], LOW);
	// 		if (i % 2 == 0)
	// 			delay(450);
	// 		else
	// 			delay(50);
	// 	}
	// 	for (size_t i = 0; i < sizeRelay; i++)
	// 	{
	// 		digitalWrite(pin_IO[i], HIGH);
	// 		delay(100);
	// 	}
	// 	for (size_t i = 0; i < sizeRelay; i++)
	// 	{
	// 		digitalWrite(pin_IO[i], LOW);
	// 		delay(800);
	// 		Serial.println(',');
	// 	}
	// 	showing = false;
	// }
	// else
	// {
	// for (size_t i = 0; i < sizeRelay; i++)
	// {
	// 	digitalWrite(pin_IO[i], LOW);
	// 	delay(800);
	// 	Serial.print(i);
	// 	Serial.print("->");
	// 	Serial.println(pin_IO[i]);
	// }
	// }
}

#endif