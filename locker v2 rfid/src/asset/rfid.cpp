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

rfid_state::rfid_state(const long interval_read) : interval(interval_read)
{
    if (interval_read < 25)
        interval = 27;
    if (interval_read > 3000)
        interval = 3000;
}
bool rfid_state::open_doors(Relay_state *rl)
{
    unsigned long current_t = millis();
    for (size_t i = 0; i < size_mahasiswa; i++)
    {
        if (rl[i].status == Status_RL::ON)
        {
            if (rl[i].shoow_rl && enable_debug)
            {
                rl[i].shoow_rl = false;
                Serial.println("locker on=>" + String(i));
            }

            if (current_t - rl[i].last_t >= rl[i].interval)
            {
                if (enable_debug)
                {
                    Serial.println("relay off T=>" + String(current_t - rl[i].last_t));
                }
                rl[i].status = Status_RL::OFF;
                rl[i].last_t = current_t;
                digitalWrite(rl[i].pin, HIGH);
                return true;
            }
            else
            {
                if (enable_debug)
                    Serial.println("interval=>" + String(current_t - rl[i].last_t));
                digitalWrite(rl[i].pin, LOW);
            }
        }
        else
        {
            digitalWrite(rl[i].pin, HIGH);
            rl[i].last_t = current_t;
        }
    }
    return false;
}
signed char rfid_state::registered(const database_s *db)
{
    bool match = false;
    signed char number_locker = 126;
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
                                   String(uid_incoming[xp]) + " :C_Arr:" + String(xp) + ":C_idx:" + String(xp));
                }
                match = false;
                break;
            }
        }
        if (match)
        {
            number_locker = x;
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
    return NULL; // invalid number locker
}

char rfid_state::read_crd(const database_s *data, Adafruit_PN532 *nfc, Relay_state *rl)
{
    static bool last_read_c = false;
    static unsigned long last_t = 0;
    unsigned long current_t = millis();

    bool read_c = false;
    if (current_t - last_t < interval)
        return -1;
    read_c = nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid_incoming, &size_uid_incoming, 25);
    if (read_c && !last_read_c)
    {
        if (enable_debug)
            Serial.print("Card detected => ");
        for (size_t i = 0; i < size_uid_incoming; i++)
        {
            if (enable_debug)
            {
                Serial.print(uid_incoming[i]);
                Serial.print(" ");
            }
        }
        if (enable_debug)
        {
            Serial.println();
            Serial.println("Length => " + String(size_uid_incoming));
        }
        last_read_c = true;
        last_t = current_t;
        byte number_locker = registered(data);
        if (number_locker != NULL)
        {
            rl[number_locker].status = Status_RL::ON;
            rl[number_locker].shoow_rl = true;
            rl[number_locker].last_t = millis();
            if (enable_debug)
            {
                Serial.println("created_at=>" + String(data[number_locker].created_at));
            }
        }

        return 1;
    }
    else if (!read_c && last_read_c)
    {
        if (enable_debug)
        {
            Serial.println("Card removed");
            Serial.println("uid length=>" + String(size_uid_incoming));
            Serial.print("uid after removed=>");
            for (size_t x = 0; x < size_uid_incoming; x++)
            {
                Serial.print(uid_incoming[x]);
            }
            Serial.println();
        }
        last_read_c = false;

        return -1;
    }

    return 0;
}