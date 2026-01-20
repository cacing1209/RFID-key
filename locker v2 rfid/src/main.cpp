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
rfid_state rfid;
Aksesoris_state buzzer;
Aksesoris_state led;
ethernet_state eth;

database_s data[size_mahasiswa];

void init_mypin()
{
	Serial.println("init my pins");
	for (size_t i = 0; i < sizeRelay; i++)
	{
		locker[i].pin = pin_IO[i];
		locker[i].interval = 300;
		pinMode(locker[i].pin, OUTPUT);
		digitalWrite(locker[i].pin, LOW);
		locker[i].last_t = millis();
	}

	for (size_t i = 0; i < size_mahasiswa; i++)
	{
		strcpy(data[i].created_at, "2026-01-15 12:56:45");
	}

	led.pin = PIN_led;
	buzzer.pin = PIN_buzzer;
	led.Interval = 1000;
	buzzer.Interval = 2000;
	rfid.action = action_Card::None;
	buzzer.status = ac_status::state_OFF;
	led.status = ac_status::state_OFF;
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
void test_databased()
{
	// data[12].card[0] = 23;
	// data[12].card[1] = 239;
	// data[12].card[2] = 206;
	// data[12].card[3] = 5;

	// data[29].card[0] = 51;
	// data[29].card[1] = 61;
	// data[29].card[2] = 200;
	// data[29].card[3] = 5;

	// data[11].card[0] = 61;
	// data[11].card[1] = 208;
	// data[11].card[2] = 182;
	// data[11].card[3] = 1;
	// memory.save_data(data);
	// data[15].card[0] = 2;
	// data[15].card[1] = 15;
	// data[15].card[2] = 144;
	// data[15].card[3] = 33;
	// data[15].card[4] = 151;
	// data[15].card[5] = 224;
	// data[15].card[6] = 0;
}
storage_state memory;
void debug_sys()
{
	rfid.enable_debug = true;
	memory.enable_debug = true;
	eth.enable_debug = false;
}
void setup()
{
	Serial.begin(baudRate_PC);
	debug_sys();
	init_mypin();

	uint32_t versiondata = nfc.getFirmwareVersion();
	// delay(5000);
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
	if (!memory.load_data(data))
	{
	}
	test_databased();
	Serial.println("Device Start");
}

void setup();
void loop()
{
	// write to eeprom,handle relay,sync db
	// eth.loop_ethernet(data);
	rfid.read_crd(data, &nfc, locker);
	rfid.open_doors(locker);
}