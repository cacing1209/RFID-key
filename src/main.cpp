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
#include <Adafruit_PN532.h>

#define baudRate_PC 9600
#define baudRate_ESP 9600

Relay_state locker[sizeRelay];
// MFRC522 mfrc(SS_RFID, RSTPIN_RFID); // Hapus ini
Adafruit_PN532 nfc(-1, -1); // Tambah PN532 dengan I2C
rfid_state rfid;
SdFat32 sdcard;
Aksesoris_state buzzer;
Aksesoris_state led;
Data_state data(&sdcard, rfid);

// Variabel untuk menyimpan UID dari PN532
uint8_t uid[7];
uint8_t uidLength;

void erorCheck()
{
	//   if (digitalRead(CD_SD) == LOW)
	//   {
	//     Serial.print("memory sd card undetect : ");
	//     Serial.println(sdcard.sdErrorCode() + '\n');
	//   }

	if (sdcard.begin(CS_SD))
		Serial.println("sdcard Normal");
	else
	{
		Serial.print("sdcard Abnormal : ");
		Serial.println(sdcard.sdErrorCode() + '\n');
		sdcard.errorPrint(&Serial);
	}
	Serial.print(" ");
	
	// Ganti pengecekan MFRC522 dengan PN532
	uint32_t versiondata = nfc.getFirmwareVersion();
	if (!versiondata) {
		Serial.println("PN532 tidak ditemukan!");
		led.Status = state_ON; // Error indicator
	} else {
		Serial.print("PN532 Firmware ver: ");
		Serial.print((versiondata>>16) & 0xFF, DEC); 
		Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
		led.Status = state_OFF; // Normal
	}
}

void init_mypin()
{
	//   pinMode(CD_SD, INPUT);
	for (size_t i = 0; i < sizeRelay; i++)
	{
		locker[i].pin = pin_IO[i];
		pinMode(locker[i].pin, OUTPUT);
		digitalWrite(locker[i].pin, LOW);
		locker[i].Last_ON = millis();
	}

	led.pin = PIN_led;
	buzzer.pin = PIN_buzzer;
	led.Interval = 1000;
	buzzer.Interval = 1000;
	rfid.action = None;
	data.action = idle;
	buzzer.Status = state_OFF;
	led.Status = state_OFF;

	pinMode(led.pin, OUTPUT);
	pinMode(buzzer.pin, OUTPUT);
}

void setup()
{
	Serial.begin(baudRate_PC);
	Serial3.begin(baudRate_ESP);
	SPI.begin();
	
	// Ganti inisialisasi MFRC522 dengan PN532
	nfc.begin();
	nfc.SAMConfig(); // Konfigurasi untuk membaca ISO14443A cards
	
	init_mypin();
	erorCheck();
	data.load_data();
	// data.load_data();
	// int ex[size_rfid] = {83, 58, 129, 41};
	// for (size_t i = 0; i < size_rfid; i++)
	// {
	// 	rfid.card[3][i] = ex[i];
	// }
}

#define maxbyte 5
bool chat_open(byte *xp)
{
	const byte equal[maxbyte] = {0x15, 0x05, 0x00, 0x05, 0x15};

	for (size_t i = 0; i < maxbyte; i++)
	{
		if (i == 2)
			continue;
		if (xp[i] != equal[i])
			return false;
	}
	return true;
}

String litle_end()
{
	uint32_t val = 0;
	// Ganti mfrc.uid dengan uid dari PN532
	for (int i = 0; i < uidLength; i++)
	{
		val |= ((uint32_t)uid[i]) << (8 * i);
	}

	char buffer[11];
	sprintf(buffer, "%010lu", val);
	return String(buffer);
}

void readCard()
{
	// Ganti logika pembacaan MFRC522 dengan PN532
	if (rfid.action != None)
		return;
		
	uint8_t success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);
	
	if (!success) return; // Tidak ada kartu
	
	Serial.print("UID bytes: ");
	for (byte i = 0; i < uidLength; i++)
	{
		Serial.print(uid[i]);
		Serial.print(",");
	}
	Serial.println();
	led.Status = state_ON_fastloop;
	buzzer.Status = state_ON;
	Serial.println();
	
	rfid.action = Equal;
}

void convert_bigEndian(String input, byte *uid)
{

	uint32_t value = strtoul(input.c_str(), NULL, 10);

	// byte uid[4];
	uid[0] = (value) & 0xFF;	   // 0x53 = 83
	uid[1] = (value >> 8) & 0xFF;  // 0x3A = 58
	uid[2] = (value >> 16) & 0xFF; // 0x81 = 129
	uid[3] = (value >> 24) & 0xFF; // 0x29 = 41

	// Serial.print("UID bytes (from value): ");
	for (int i = 0; i < 4; i++)
	{
		Serial.print(uid[i]);
		if (i < 3)
			Serial.print(", ");
	}
	Serial.println();
}

String uidMahasiswa[sizeRelay];
void convert_uitbyt()
{
	static unsigned long lastProcessTime = 0;
	const unsigned long interval = 500;
	static size_t i_rfid = 0;
	unsigned long currentMillis = millis();
	if (i_rfid >= total_card_rfid)
	{
		Serial.print("i_rfid:");
		Serial.println(i_rfid);
		i_rfid = 0;
		rfid.action = None;
		
		// Ganti dump version MFRC522 dengan PN532
		uint32_t versiondata = nfc.getFirmwareVersion();
		if (versiondata) {
			Serial.print("PN532 Firmware: ");
			Serial.print((versiondata>>16) & 0xFF, DEC); 
			Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
		}
		
		data.action = sv_data;
		if (sdcard.begin(CS_SD))
			Serial.println("sdcard Normal");
		else
		{
			Serial.print("sdcard Abnormal : ");
			Serial.println(sdcard.sdErrorCode() + '\n');
		}
		return;
	}

	if (currentMillis - lastProcessTime >= interval && i_rfid < total_card_rfid)
	{
		lastProcessTime = currentMillis;

		if (uidMahasiswa[i_rfid].length() < size_rfid)
		{
			uidMahasiswa[i_rfid] = "";
		}
		else
		{
			convert_bigEndian(uidMahasiswa[i_rfid], rfid.card[i_rfid]);
		}
		i_rfid++;
	}
}

String incomingData;
bool receiving = false;
void readSerial()
{
	while (Serial3.available() && rfid.action == None)
	{
		char incomingByte = Serial3.read();
		incomingData += incomingByte;
		if (incomingByte == '\n')
		{
			incomingData.trim();
			Serial.print(receiving);
			Serial.print("<- receiving ");
			Serial.println(incomingData);

			if (incomingData == "send_1")
			{
				receiving = true;
			}
			else if (incomingData == "send_0")
			{
				if (!receiving)
					break;
				receiving = false;
				rfid.action = Register;
			}
			else if (receiving && incomingData.startsWith("A"))
			{
				long separatorIndex = incomingData.indexOf(':');
				if (separatorIndex != -1)
				{
					String nomorStr = incomingData.substring(1, separatorIndex);
					long nomorMahasiswa = nomorStr.toInt();
					if (nomorMahasiswa >= 1 && nomorMahasiswa <= sizeRelay)
					{
						String uid = incomingData.substring(separatorIndex + 1);
						uid.trim();
						uidMahasiswa[nomorMahasiswa - 1] = uid;
					}
				}
			}
			incomingData = "";
		}
	}
}

void setup();

bool checkAction()
{
	switch (rfid.action)
	{
	case Register:
		convert_uitbyt();
		return false;
		break;
	case Remove:
		return false;
		break;
	case Equal:
		rfid.action = None;
		return true;
		break;
	default:
		return false;
		break;
	}
	return false;
}

signed char getNumberlocker()
{
	if (checkAction())
	{
		for (size_t i = 0; i < total_card_rfid; i++)
		{
			bool xp = true;
			// Ganti mfrc.uid.uidByte dengan uid dari PN532
			for (size_t x = 0; x < size_rfid; x++)
			{
				if (rfid.card[i][x] != uid[x])
				{
					xp = false;
					break;
				}
			}
			if (xp)
			{
				Serial.print("cocok:");
				Serial.println(i);
				return i;
			}
		}
		return -1;
	}
	return -1;
}

void getStatusLocker()
{
	signed char nb = getNumberlocker();
	if (nb == -1)
		return;

	for (size_t i = 0; i < sizeRelay; i++)
	{
		if (nb == i)
			locker[i].status = ON;
		else
			locker[i].status = OFF;
	}
}

void openedLocker()
{
	unsigned long interval = 500;
	for (size_t i = 0; i < sizeRelay; i++)
	{
		if (locker[i].status == ON)
		{
			if (locker[i].TimeON < interval)
			{
				digitalWrite(locker[i].pin, HIGH);
			}
			else
				locker[i].status = OFF;
		}
		else
		{
			digitalWrite(locker[i].pin, LOW);
			locker[i].Last_ON = millis();
		}
		locker[i].TimeON = millis() - locker[i].Last_ON;
	}
}

void clear_byte()
{
	if (rfid.action != Equal)
		return;
	// Ganti clearing mfrc.uid dengan uid PN532
	for (size_t i = 0; i < uidLength; i++)
	{
		uid[i] = 0;
	}
	uidLength = 0;
	rfid.action = None;
}

void mainSDcard()
{
	switch (data.action)
	{
	case sv_data:
		data.save_data();
		break;
	default:
		break;
	}
}

void setup();
void loop()
{
	readSerial();
	buzzer.on(Tone01);
	led.on();
	readCard();
	getStatusLocker();
	openedLocker();
	clear_byte();
	mainSDcard();
}