#include <common.h>
bool systemCardProgram::checkIncomingCard(MFRC522::Uid *IncomingCard, card *myCard,
                                          uint8_t Admin[totalCardAdministor][sizeCard])
{
    for (size_t y = 0; y < totalCard; y++)
    {
        for (size_t x = 0; x < sizeCard; x++)
        {
            if (IncomingCard->uidByte[x] != mycard.IdCard[y] || IncomingCard->uidByte[x] != Admin[y][x])
            {
                break;
            }

            access == Read;
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
        serial->print((index < 0) ? ' ' : ',' + String(IncomingCard.uidByte[index]));
        totalbitCard++;
    }
    serial->print(" total bit: " + String(totalbitCard));
}

void LED::animationLED(uint8_t indexbutton_LED, actionLED action)
{
    static long int priviouseTime = 0;

    switch (action)
    {
    case flip_flop:
        delayLED = 500;
        if (millis() - priviouseTime >= delayLED)
        {
            digitalWrite(indexbutton_LED, !digitalRead(indexbutton_LED));
            priviouseTime = millis();
        }

        break;
    case ON_2x:
        delayLED = 500;
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
        delayLED = 500;
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
    }
}

void CAR::startManual(uint8_t indexbutton_Start)
{
    static uint8_t timePriviouse;
    if (cardSystem.access != Lock || statusEngine == OFF)
    {
        return;
    }

    else if (millis() - timePriviouse >= delayPress_Start && digitalRead(indexbutton_Start) == HIGH)
    {
        digitalWrite(indexbutton_Start, HIGH);
    }

    else
    {
        timePriviouse = millis();
        digitalWrite(indexbutton_Start, LOW);
    }
}

void CAR::begin()
{
    statusEngine = OFF;
    delayPress_Start = 450;
    RelayStart, RelayDinamoAmper, RelayKontak = false;
}