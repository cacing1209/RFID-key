#include <commond.h>

#define baudRate_PC 9600
#define baudRate_ESP 115200

Relay_state locker[sizeRelay];
MFRC522 mfrc(CS_RFID, RSTPIN_RFID);
SdFat32 sdcard;

void erorCheck()
{
  if (digitalRead(CD_SD) == LOW)
  {
    Serial.print("memory sd card undetect : ");
    Serial.println(sdcard.sdErrorCode() + '\n');
  }

  else if (sdcard.begin(CS_SD))
    Serial.println("sdcard Normal");
  else
  {
    Serial.print("sdcard Abnormal : ");
    Serial.println(sdcard.sdErrorCode() + '\n');
  }
}

void init_mypin()
{
  pinMode(CD_SD, INPUT);
  for (size_t i = 0; i < sizeRelay; i++)
  {
    pinMode(locker[sizeRelay].pin, OUTPUT);
    digitalWrite(locker[i].pin, LOW);
    locker[i].Last_ON = millis();
  }
}
DB_STATE database;
void loadFile()
{
  database.sd = &sdcard;
  database.load_data("/mahasiswa.json");
}
void setup()
{
  Serial.begin(baudRate_PC);
  Serial3.begin(baudRate_ESP);
  init_mypin();
  erorCheck();
}

void readCard()
{
  if (!mfrc.PICC_IsNewCardPresent() ||
      !mfrc.PICC_ReadCardSerial())
  {
    return;
  }
}

void loop()
{
}

/**
 * | Pin Arduino Mega | Fungsi | Sambung ke SD Card | Sambung ke RFID |
 * | ---------------- | ------ | ------------------ | --------------- |
 * | 50               | MISO   | MISO               | MISO            |
 * | 51               | MOSI   | MOSI               | MOSI            |
 * | 52               | SCK    | SCK                | SCK             |
 * | 2                | CS     | CS                 |                 |
 * | 3                | CS     |                    | CS              |
 * | 4                | RST    |                    | RST             |
 * | GND              | GND    | GND                | GND             |
 *
 *
 * ambil data mahasiswa dari website local ketika ada perubahan saja
 * save sdcard
 * load data mahasiswa dari sdcard ketika booting
 *
 */