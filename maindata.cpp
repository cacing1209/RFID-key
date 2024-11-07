#include "Engine.h"

bool CARD::Equal_DBandSOURCE(uint8_t *DB, uint8_t *SC)
{
    for (byte i = 0; i < sizeByte; i++)
    {
        if (DB != SC)
        {
            return false;
        }
    }
    return true;
}
void CARD::initCard(uint8_t size, uint8_t *carduid)
{
    for (byte i = 0; i < size; i++)
    {
        Serial.print(" ");
        Serial.print(carduid[i]);
    }
    Serial.println();
}