/**
 * | Pin Arduino Mega | Fungsi | Sambung ke SD Card | Sambung ke PN532 |
 * | ---------------- | ------ | ------------------ | ---------------- |
 * | 50               | MISO   | MISO               |                  |
 * | 51               | MOSI   | MOSI               |                  |
 * | 52               | SCK    | SCK                |                  |
 * | 2                | CS     | CS                 |                  |
 * | 20               | SDA    |                    | SDA              |
 * | 21               | SCL    |                    | SCL              |
 * | GND              | GND    | GND                | GND              |
 * | 5V/3.3V          | VCC    |                    | VCC              |
 *
 *
 * ambil data mahasiswa dari website local ketika ada perubahan saja
 * save sdcard
 * load data mahasiswa dari sdcard ketika booting
 *
 */

#include <commond.h>
#include <Wire.h>

#define baudRate_PC 9600
#define baudRate_ESP 115200

Relay_state locker[sizeRelay];
Adafruit_PN532 nfc(-1, -1);
rfid_state rfid(3000);
acc_state buzzer;
acc_state led;
ethernet_state eth;

database_s data[size_mahasiswa];

void init_mypin()
{
	Serial.println("init my pins");
	for (size_t i = 0; i < sizeRelay; i++)
	{
		locker[i].pin = pin_IO[i];
		locker[i].interval = 150;
		pinMode(locker[i].pin, OUTPUT);
		digitalWrite(locker[i].pin, HIGH);
		locker[i].last_t = millis();
	}

	for (size_t i = 0; i < size_mahasiswa; i++)
	{
		strcpy(data[i].created_at, "2026-01-15 12:56:45");
	}

	led.pin = PIN_led;
	buzzer.pin = PIN_buzzer;
	led.Interval = 1000;
	buzzer.Interval = 250;
	rfid.action = action_Card::None;
	buzzer.act = acc_action::acc_off;
	buzzer.mode = acc_mode::mode_fastloop4X;
	led.act = acc_action::acc_off;
	led.mode = acc_mode::mode_fastloop2X;
	SPI.begin();

	if (!nfc.begin() || !nfc.SAMConfig())
	{
		if (rfid.enable_debug)
		{
			Serial.println("::NFC not already");
		}
	}
	else
	{
		if (rfid.enable_debug)
		{
			Serial.println("NFC ALREADY");
		}
	}
	pinMode(led.pin, OUTPUT);
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
void debug_sys(bool db_rfid, bool db_memory, bool db_eth)
{
	rfid.enable_debug = db_rfid;
	memory.enable_debug = db_memory;
	eth.enable_debug = db_eth;
}
void setup()
{
	delay(5000);
	Serial.begin(baudRate_PC);
	debug_sys(true, true, true);

	init_mypin();
	uint32_t versiondata = nfc.getFirmwareVersion();
	if (!versiondata)
	{

		if (rfid.enable_debug)
			Serial.println("PN532 board not found!");
	}
	else
	{
		if (rfid.enable_debug)
			Serial.println("rfid already use");
	}
	// test_databased();
	if (!memory.load_data(data))
	{
	}
	Serial.println("Device Start");
}

void acc_main()
{
	buzzer.main();
	led.main();
}
void setup();
void loop()
{
	static unsigned long latency = 0;
	static unsigned long last_t = 0;
	last_t = millis();
	// write to eeprom,handle relay,sync db
	eth.loop_ethernet(data);
	rfid.read_crd(data, &nfc, locker);
	if (rfid.open_doors(locker))
	{
		buzzer.act = acc_action::acc_on;
		led.act = acc_action::acc_on;
		Serial.println("ac is true");
	}
	acc_main();
	latency = millis() - last_t;
	// if (rfid.enable_debug)
	// 	Serial.println("latency processing=>" + String(latency));
}