#include <common.h>
bool systemCardProgram::checkIncomingCard(MFRC522::Uid *IncomingCard)
{
    if (access != Equal)
    {
        Serial.println(" access is not Equal ");
        return false;
    }

    for (size_t y = 0; y < totalCard; y++)
    {
        for (size_t x = 0; x < sizeCard; x++)
        {
            if (IncomingCard->uidByte[x] != mycard[y].Administrator[x] ||
                IncomingCard->uidByte[x] != mycard[y].IdCard[x])
            {
                break;
            }
            return true;
        }
    }
    return false;
}
void systemCardProgram::ReadCard(MFRC522::Uid IncomingCard, HardwareSerial *serial)
{
    uint8_t totalbitCard = 0;
    serial->print("Read Card :");
    for (size_t index = 0; index < IncomingCard.size; index++)
    {
        serial->print(" " + String(IncomingCard.uidByte[index]));
        totalbitCard++;
    }
    serial->println(" total bit: " + String(totalbitCard));
}

void LED::animationLED(uint8_t indexbutton_LED, actionLED action, uint8_t delayLED)
{
    static long int priviouseTime = 0;

    switch (action)
    {
    case flip_flop:

        if (millis() - priviouseTime >= delayLED)
        {
            digitalWrite(indexbutton_LED, !digitalRead(indexbutton_LED));
            priviouseTime = millis();
        }

        break;
    case ON_2x:

        static uint8_t on2x = 0;
        if (millis() - priviouseTime >= delayLED)
        {
            on2x++;
            if (on2x > 4)
            {
                priviouseTime = millis();
                break;
            }
            digitalWrite(indexbutton_LED, !digitalRead(indexbutton_LED));
            priviouseTime = millis();
        }
        else if (on2x > 8)
        {
            on2x = 0;
        }
        break;
    case ON_4X:

        static uint8_t togglepPin = 0;
        if (millis() - priviouseTime >= delayLED)
        {
            togglepPin++;
            if (togglepPin > 8)
            {
                priviouseTime = millis();
                break;
            }
            digitalWrite(indexbutton_LED, !digitalRead(indexbutton_LED));
            priviouseTime = millis();
        }
        else if (togglepPin > 16)
        {
            togglepPin = 0;
        }
        break;
    }
}

void CAR::startManual()
{
    static long int timePriviouse = 0;
    if (cardSystem.access != Read || statusEngine == OFF)
    {
        return;
    }

    else if (millis() - timePriviouse >= delayPress_Start && digitalRead(indexbutton_Start) == HIGH)
    {
        digitalWrite(indexRelay_Start, HIGH);
    }

    else
    {
        timePriviouse = millis();
        digitalWrite(indexRelay_Start, LOW);
    }
    digitalWrite(indexRelay_Kontak, HIGH);
}

void CAR::begin(uint8_t pin_Start, uint8_t pinRelay_Kontak, uint8_t pinRelay_Start)
{
    indexbutton_Start = pin_Start, indexRelay_Kontak = pinRelay_Kontak, indexRelay_Start = pinRelay_Start;
    statusEngine = OFF;
    delayPress_Start = 450;
    RelayStart = false;
    RelayDinamoAmper = false;
    RelayKontak = false;
    digitalWrite(indexRelay_Start, LOW);
    digitalWrite(indexRelay_Kontak, LOW);
}