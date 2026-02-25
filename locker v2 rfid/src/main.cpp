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
#define PN532DEBUG
#define PN532DEBUGPRINT Serial

#include <commond.h>
#include <Wire.h>

#define baudRate_PC 9600
// #define baudRate_ESP 115200

Relay_state locker[sizeRelay];
Adafruit_PN532 nfc(-1, -1);
rfid_state rfid(350);
buzzer_state buzzer(1);
ethernet_state eth;

database_s data[size_mahasiswa];

void init_mypin()
{
	Serial.println("init my pins");
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
	buzzer.Interval = 50;
	// rfid.action = action_Card::None;
	buzzer.act = acc_action::acc_off;
	buzzer.mode = acc_mode::mode_fastloop4X;
	Wire.begin();
	SPI.begin();
	eth.begin(data);
	rfid.init_sensor(&nfc);
	pinMode(buzzer.pin, OUTPUT);
}
storage_state memory;
// void test_databased()
// {
// 	data[2].card[0] = 23;
// 	data[2].card[1] = 239;
// 	data[2].card[2] = 206;
// 	data[2].card[3] = 5;

// 	data[4].card[0] = 51;
// 	data[4].card[1] = 61;
// 	data[4].card[2] = 200;
// 	data[4].card[3] = 5;

// 	data[0].card[0] = 61;
// 	data[0].card[1] = 208;
// 	data[0].card[2] = 182;
// 	data[0].card[3] = 1;

// 	// data[15].card[0] = 2;
// 	// data[15].card[1] = 15;
// 	// data[15].card[2] = 144;
// 	// data[15].card[3] = 33;
// 	// data[15].card[4] = 151;
// 	// data[15].card[5] = 224;
// 	// data[15].card[6] = 0;

// 	memory.save_data(data);
// }
void setup()
{
#if defined(DEBUG_MEM) || defined(DEBUG_ETH) || defined(DEBUG_RFID)
	delay(5000);
#endif
	Serial.begin(baudRate_PC);

	init_mypin();

#ifdef DEBUG_RFID
	uint32_t versiondata = nfc.getFirmwareVersion();
	if (!versiondata)
	{
		Serial.println("PN532 board not found!");
	}
	else
	{
		Serial.println("rfid already use");
	}
#endif
	if (!memory.load_data(data))
	{
	}

	digitalWrite(buzzer.pin, LOW);
	delay(2000);
	digitalWrite(buzzer.pin, HIGH);
	delay(2000);
	Serial.println("Device Start");
}

void acc_main()
{
	buzzer.main();
}
void setup();
void loop()
{
	// static unsigned long latency = 0;
	// static unsigned long last_t = 0;
	// last_t = millis();
	// write to eeprom,handle relay,sync db
	acc_main();
	if (rfid.open_doors(locker))
		return;
	eth.loop(data, &memory, locker);
	signed char card = rfid.read_crd(data, &nfc, locker);
	switch (card)
	{
	case 1:
		buzzer.mode = acc_mode::mode_fastloop3X;
		buzzer.act = acc_action::acc_on;
#ifdef DEBUG_RFID
		Serial.println(":bz:tone 1");
#endif
		break;

	case -4:
		buzzer.mode = acc_mode::mode_fastloop8X;
		buzzer.act = acc_action::acc_on;
#ifdef DEBUG_RFID
		Serial.println(":bz:tone 0");
#endif
		break;

		// case -2:
		// buzzer.mode = acc_mode::denide;
		// buzzer.act = acc_action::acc_off;
		// break;
		// case 2:
		// 	buzzer.act = acc_action::acc_off;
		// 	break;
	default:
		break;
	}
	// latency = millis() - last_t;
	// if (rfid.enable_debug)
	// 	Serial.println("latency processing=>" + String(latency));
}
