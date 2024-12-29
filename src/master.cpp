#include <Arduino.h>
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
#define delay_LED 500

systemCardProgram manageCard;
cardMember Card[totalCard];
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

void cardBegin()
{
    for (size_t x = 0; x < totalCard; x++)
    {
        for (size_t xy = 0; xy < sizeCard; xy++)
        {
            manageCard.Admin[x].Administrator[xy] = idCard_Admistor[x][xy];
            Serial.print(" " + String(manageCard.Admin[x].Administrator[xy]));
            manageCard.mycard[x].RoleCard = Administrator;
        }
        Serial.println("");
    }
    manageCard.Msg = &Serial;
    manageCard.access = Equal;
}
void setup()
{
    Serial.begin(9600);
    cardBegin();
    Serial.println("\nloading component");
    mfrc.PCD_Init();
    SPI.begin();
    carState.begin(pin_startManual, pin_RelayKontak, pin_RelayStart);
    initmyPins();
}

void loop()
{
    carState.startManual();
    if (!mfrc.PICC_IsNewCardPresent() ||
        !mfrc.PICC_ReadCardSerial())
    {
        return;
    }
    manageCard.ReadCard(&mfrc.uid);
    if (manageCard.checkIncomingCard(&mfrc.uid))
    {
        manageCard.access = Lock;
        Serial.println("Acces card only Read");
        Serial.println("Engine executed ");
        carState.statusEngine = ON;
    }
    mfrc.PICC_HaltA();
    mfrc.PCD_StopCrypto1();
}