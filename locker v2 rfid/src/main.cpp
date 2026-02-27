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
rfid_state rfid(2000);
buzzer_state buzzer(1);
ethernet_state eth;
NTPConfig ntpCfg;

database_s data[size_mahasiswa];

char *system_t()
{
	time_t t = now();
	char buffer[20];
	sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
			year(t), month(t), day(t),
			hour(t), minute(t), second(t));
	return buffer;
}
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
	buzzer.Interval = 600;
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
void setup()
{
#if defined(DEBUG_MEM) || defined(DEBUG_ETH) || defined(DEBUG_RFID)
	delay(5000);
#endif
	Serial.begin(baudRate_PC);
	init_mypin();
	ntpCfg.Udp.begin(ntpCfg.localPort);
	unsigned long ntpTime = 0;
	while (ntpTime == 0)
	{
#ifdef DEBUG_TIME
		Serial.println("Menghubungi NTP server...");
#endif
		ntpTime = ntpCfg.getNTPTime();
	}
	setTime(ntpTime);
#ifdef DEBUG_TIME
	Serial.println("Waktu berhasil disinkronkan!");
#endif
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
		buzzer.mode = acc_mode::mode_fastloop1X;
		buzzer.act = acc_action::acc_on;
#ifdef DEBUG_RFID
		Serial.println(":bz:tone 1");
#endif
		break;

	case -4:
		buzzer.mode = acc_mode::mode_fastloop4X;
		buzzer.act = acc_action::acc_on;
#ifdef DEBUG_RFID
		Serial.println(":bz:tone 0");
#endif
		break;
	default:
		break;
	}
	static unsigned long last_sync = 0;
	if (millis() - last_sync > 3000)
	{
		Serial.print("time:");
		Serial.println(system_t());
		last_sync = millis();
	}
	// latency = millis() - last_t;
	// if (rfid.enable_debug)
	// 	Serial.println("latency processing=>" + String(latency));
}
