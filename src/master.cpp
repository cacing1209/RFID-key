#include <common.h>
#include <SPI.h>
#define RST_PIN 9
#define SS_PIN 10

CAR carState;
#define pin_startManual 4
#define pin_RelayStart 2
#define pin_RelayKontak 3
#define pin_RelayDinamoAmper 1
#define pin_BUZZER 5
#define pin_LED 7

systemCardProgram manageCard;
card myCard[totalCard];
MFRC522 mfrc(SS_PIN, RST_PIN);

uint8_t idCard_Admistor[totalCard][sizeCard] = {
    {51, 159, 195, 254},
    {208, 116, 56, 88},
    {5, 131, 31, 150, 139, 209, 0},
    {24, 130, 53, 166, 242, 66, 40},
};

void initmyPins()
{
    pinMode(pin_BUZZER, OUTPUT);
    pinMode(pin_LED, OUTPUT);
    pinMode(pin_startManual, OUTPUT);
    pinMode(pin_RelayStart, OUTPUT);
    pinMode(pin_RelayKontak, OUTPUT);
    pinMode(pin_RelayDinamoAmper, OUTPUT);
}
void setup()
{
    Serial.begin(9600);
    Serial.println("loading component");
    initmyPins();
    mfrc.PCD_Init();
    SPI.begin();
    carState.begin();
}

void loop()
{
    carState.startManual(pin_startManual);
    if (!mfrc.PICC_IsNewCardPresent() ||
        !mfrc.PICC_ReadCardSerial())
    {
        return;
    }
    manageCard.ReadCard(mfrc.uid, &Serial);
    if (manageCard.checkIncomingCard(&mfrc.uid, myCard, idCard_Admistor))
    {
        Serial.println("cocok");
        manageCard.access = Read;
        carState.statusEngine = ON;
    }

    else
    {
        Serial.println("ndak cocok");
    }

    mfrc.PICC_HaltA();
    mfrc.PCD_StopCrypto1();
}