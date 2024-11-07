#include "Engine.h"
#define RST_PIN 9
#define SS_PIN 10
MFRC522 DeviceReader(SS_PIN, RST_PIN);
RelayState Relay;
buttonState button;
EngineRunning RUN;
MFRC522::Uid cardReader;
CARD heapMycard;
void initPins()
{
    pinMode(Relay.Cotact, OUTPUT);
    pinMode(Relay.Engine_Start, OUTPUT);
    pinMode(button.start_Manual, OUTPUT);
}

void setup()
{
    RUN.begin();
    initPins();
}
bool Equal_CARDandSOURCE()
{
    for (byte i = 0; i < cardtotal; i++)
    {
        heapMycard.Equal_DBandSOURCE(heapMycard.dbCARD[i], cardReader.uidByte);
    }
}
void loop()
{
    if (!DeviceReader.PICC_IsNewCardPresent() ||
        !DeviceReader.PICC_ReadCardSerial())
    {
        return;
    }
    heapMycard.initCard(cardReader.size, cardReader.uidByte);
}
