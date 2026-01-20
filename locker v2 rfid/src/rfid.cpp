#include <commond.h>

// String litle_end()
// {
//     uint32_t val = 0;
//     for (int i = 0; i < uidLength; i++)
//     {
//         val |= ((uint32_t)uid[i]) << (8 * i);
//     }

//     char buffer[11];
//     sprintf(buffer, "%010lu", val);
//     return String(buffer);
// }

// bool sensor_bussy()
// {
//     static unsigned long lt = 0;
//     bool sensor_isbussy = (buzzer.Status != state_OFF);

//     if (sensor_isbussy)
//     {
//         lt = millis();
//         return true;
//     }

//     if (millis() - lt > 3000)
//         return false;

//     return true;
// }
void rfid_state::open_doors(Relay_state *rl)
{
    for (size_t i = 0; i < size_mahasiswa; i++)
    {
        if (rl[i].status == Status_RL::ON)
        {
            if (millis() - rl[i].last_t >= rl[i].interval)
            {
                rl[i].status = Status_RL::OFF;
                digitalWrite(rl[i].pin, LOW);
            }
            else
            {
                digitalWrite(rl[i].pin, HIGH);
            }
        }
        else
        {
            digitalWrite(rl[i].pin, LOW);
        }
    }
}
signed char rfid_state::registered(const database_s *db)
{
    bool match = false;
    signed char number_locker = -1;
    for (size_t x = 0; x < size_mahasiswa; x++)
    {
        match = true;
        for (size_t xp = 0; xp < size_uid_incoming; xp++)
        {
            if (db[x].card[xp] != uid_incoming[xp])
            {
                if (enable_debug)
                {
                    Serial.println("::not match =>" + String(db[x].card[xp]) + ":incoming:" +
                                   String(uid_incoming[xp]) + " :i:" + String(xp));
                }
                match = false;
                break;
            }
            number_locker = xp;
        }
        if (match)
        {
            if (enable_debug)
            {
                Serial.println("match=> index" + String(x));
                for (size_t i = 0; i < size_uid_incoming; i++)
                {
                    Serial.println("uid:" + String(db[x].card[i]) + ":in:" + String(uid_incoming[i]));
                }
            }
            return number_locker;
        }
    }
    return 126;
}

char rfid_state::read_crd(const database_s *data, Adafruit_PN532 *nfc, Relay_state *rl)
{
    static bool last_read_c = false;

    bool read_c = nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid_incoming, &size_uid_incoming, 1000);
    if (read_c && !last_read_c)
    {
        Serial.print("Card detected => ");
        for (size_t i = 0; i < size_uid_incoming; i++)
        {
            if (enable_debug)
            {
                // if (uid[i] < 0x10)
                //     Serial.print("0");
                Serial.print(uid_incoming[i], HEX);
                Serial.print(" ");
            }
        }
        if (enable_debug)
        {
            Serial.println();
            Serial.println("Length => " + String(size_uid_incoming));
        }
        last_read_c = true;
        byte number_locker = registered(data);
        if (number_locker != 126)
            rl[number_locker].status = Status_RL::ON;

        return 1;
    }
    else if (!read_c && last_read_c)
    {
        if (enable_debug)
            Serial.println("Card removed");
        last_read_c = false;
        return -1;
    }

    return 0;
}