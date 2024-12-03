#include <SPI.h>
#include <MFRC522.h>
#define R_start 2
#define R_kontak 3
#define Automatic_ON 3 // value
#define startManual 4
#define buzzer 5
#define Read_voltkontak 6
#define led 7
#define cardsize 8 // value
#define RST_PIN 9
#define SS_PIN 10
#define totalcard 4
#define interval_automatic 500
#define Delay_PUSH 450
#define Delay_Push_buzzer 170
#define Delay_buzzer 350
const int ENGINE_voltage = A0, tx = A1, rx = A2, Read_Retret = A3;
// identifikasi
bool otomatis = true;
bool activatedRetred = false;
int value_Retread;
bool alternativ = false;
bool cekID(byte *IDCARD, byte *source_CARD, int LENGHT_CARD);
void waktu(int count, const int value);
byte cardINPUT[8];
byte IDcard[totalcard][cardsize] = {
    {51, 159, 195, 254},
    {208, 116, 56, 88},
    {5, 131, 31, 150, 139, 209, 0},
    {24, 130, 53, 166, 242, 66, 40}
    };
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::Uid card;
bool terdaftar;
enum status
{
  Engine_ON,
  Engine_Automatic_LOCK,
  isRetret,
  Engine_OFF,
  CARD_REGISTERED,
  CARD_NOT_REGISTER,
  LED_ON,
  PUSH_DAFTAR,
  PUSH_Start,
};
enum bypas
{
  Init_LOCK,
  Init_UNLOCK
};
status mesin;
status kondisi_mesin;
bypas mycard;
status fungsi_tombol;
void setup()
{
  kondisi_mesin = CARD_NOT_REGISTER;
  Serial.begin(9600);
  pinMode(R_kontak, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(R_start, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(startManual, INPUT);
  pinMode(Read_voltkontak, INPUT);
  pinMode(tx, OUTPUT);
  pinMode(rx, INPUT);
  alternativ = false;
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println(" start READER ");
  terdaftar = false;
}
void ConfirmID()
{
  for (byte DBcard = 0; DBcard < totalcard; DBcard++)
  {
    if (cekID(IDcard[DBcard], cardINPUT, card.size))
    {
      kondisi_mesin = CARD_REGISTERED;
      terdaftar = true;
      fungsi_tombol = PUSH_Start;
      return;
    }
  }
  fungsi_tombol = PUSH_DAFTAR;
  kondisi_mesin = CARD_NOT_REGISTER;
  return;
}
void initcard()
{
  card = mfrc522.uid;
  for (byte i = 0; i < card.size; i++)
  {
    cardINPUT[i] = card.uidByte[i];
    Serial.print(" ");
    Serial.print(card.uidByte[i]);
  }
  Serial.println();
}
bool cekID(byte *IDCARD, byte *source_CARD, int LENGHT_CARD)
{
  for (byte i = 0; i < LENGHT_CARD; i++)
  {
    if (source_CARD[i] != IDCARD[i])
    {
      return false;
    }
  }
  return true;
}

void ledAnimation(int interval)
{
  if (alternativ) // bypass animasi led triger engine voltage
  {
    digitalWrite(led, HIGH);
    return;
  }

  if (analogRead(ENGINE_voltage) >= 500)
  {
    digitalWrite(led, HIGH);
  }
  else
  {
    static long int lastTime;
    if (millis() - lastTime >= interval)
    {
      digitalWrite(led, !digitalRead(led));
      lastTime = millis();
    }
  }
}
unsigned long int lastTimeActivated_Retred = 0;
unsigned long int prev = 0;
void MANUAL()
{
static int fastTime;
  if (kondisi_mesin == CARD_NOT_REGISTER)
  {
    digitalWrite(led, LOW);
    return;
  }
  ledAnimation(fastTime);
  activatedRetred = (value_Retread == 2) ? true : false;
  if (digitalRead(startManual) == HIGH && fungsi_tombol == PUSH_Start)
  {
    // Serial.println(millis() - lastTimeActivated_Retred);
    fastTime = 200;
    if (millis() - prev > Delay_PUSH)
    {
      alternativ = true;
      lastTimeActivated_Retred = millis();
      fastTime = 75;
      // Serial.println(fastTime);
      // Serial.println(" STATER ON ");
      digitalWrite(R_start, HIGH);
    }
  }
  else
  {
    fastTime = 350;
    prev = millis();
    delay(12);
    // Serial.print(0);
    digitalWrite(R_start, LOW);
  }
}

void ENGINE()
{
  if (mesin == Engine_Automatic_LOCK)
  {
    return;
    Serial.println(" ENGINE STATUS ON ");
  }
  for (int i = 0; i < Automatic_ON; i++)
  {
    Serial.print(" INTERVAL AUTOMATIC ENGINE ON : ");
    Serial.println(i);
    digitalWrite(R_start, HIGH);
    Serial.println("START ON");
    // delay(1200); // waktu untuk mesin menyala
    waktu(1, A1);
    digitalWrite(R_start, LOW);
    Serial.println("START OFF");
    waktu(5, A1);
    // delay(5000); // waktu untuk mesin mati
  }
}

long duration;
int distance, delay_Buzzer = Delay_buzzer;
static long int buzerlastTime = 0;

void Retret()
{
  // if (analogRead(Read_Retret) <= 500)
  // {
  //   digitalWrite(buzzer, LOW);
  //   return;
  // }
  if (!activatedRetred)
  {
    digitalWrite(buzzer, LOW);
    return;
  }
  digitalWrite(tx, LOW);
  delayMicroseconds(2);
  digitalWrite(tx, HIGH);
  delayMicroseconds(10);
  digitalWrite(tx, LOW);
  duration = pulseIn(rx, HIGH);
  distance = duration * 0.034 / 2;

  if (millis() - buzerlastTime >= delay_Buzzer)
  {
    // digitalWrite(led, !digitalRead(buzzer));
    digitalWrite(buzzer, !digitalRead(buzzer));
    // Serial.print("Jarak: ");
    // Serial.print(distance);
    // Serial.print(" cm");
    buzerlastTime = millis();
  }
  delay(10);

  switch (distance)
  {
  case 1 ... 100:
    delay_Buzzer = 80;
    // Serial.println(" DANGER...!!! ");
    break;
  case 150 ... 200:
    delay_Buzzer = 220;
    break;
  case 250 ... 350:
    delay_Buzzer = 300;
    break;
  default:
    delay_Buzzer = 450;
    // digitalWrite(buzzer, LOW);
    break;
  }
}
void auto_start()
{
  if (!otomatis)
  {
    return;
  }
  Serial.println(" TUNGGU BOOTING ");
  delay(5000);
  digitalWrite(R_start, HIGH);
  delay(interval_automatic);
  Serial.println("START OFF ");
  digitalWrite(R_start, LOW);
  otomatis = false;
  return;
}
void Engine_Running()
{
  if (kondisi_mesin == CARD_NOT_REGISTER)
  {
    digitalWrite(led, LOW);
    return;
  }
  MANUAL();
  Retret();
  // auto_start();
  // Serial.println(analogRead(Read_Retret));
}

void loop()
{
  Engine_Running();
  if (!mfrc522.PICC_IsNewCardPresent() ||
      !mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  initcard();
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  if (terdaftar)
  {
    return;
  }
  ConfirmID();
  if (kondisi_mesin == CARD_REGISTERED)
  {
    digitalWrite(R_kontak, HIGH);
    Serial.println(" <<==== CARD TERDAFTAR====>> ");
    // ENGINE();
  }
  else
  {
    Serial.println(" !!! CARD TIDAK TERDAFTAR !!! ");
  }
  delay(45);
}

void waktu(int count, const int value)
{
  for (int i = 0; i < count * 4; i++)
  {
    delay(250);
    if (digitalRead(value) == HIGH || analogRead(value) >= 500)
    {
      return;
    }
    delay(250);
  }
}
// KTP 5 131 31 150 139 209 0
// 24, 130, 53, 166, 242, 66, 40
//  CARD 51 159 195 254
