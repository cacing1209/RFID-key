
#include <commond.h>
#ifndef find_p
void i2crecovery()
{
    pinMode(SCL, OUTPUT);
    pinMode(SDA, INPUT_PULLUP);
    for (size_t i = 0; i < 9; i++)
    {
        digitalWrite(SCL, HIGH);
        delayMicroseconds(5);
        digitalWrite(SCL, LOW);
        delayMicroseconds(5);
    }
    pinMode(SDA, OUTPUT);
    digitalWrite(SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(SDA, LOW);
#ifdef DEBUG_RFID
    Serial.println(":rfid:has been restart comunication");
#endif
}
rfid_state::rfid_state(const long interval_read) : interval(interval_read)
{
    if (interval_read < 25)
        interval = 27;
    if (interval_read > 3000)
        interval = 3000;
}
bool rfid_state::open_doors(Relay_state *rl, bool *send_log)
{
    unsigned long current_t = millis();
    for (size_t i = 0; i < Size_Siswa; i++)
    {
        if (rl[i].status == Status_RL::ON)
        {
            if (rl[i].reset_t)
            {
                rl[i].reset_t = false;
                send_log[i] = true;
                rl[i].last_t = millis();
#ifdef DEBUG_RFID
                Serial.println("locker on=>" + String(i));
#endif
            }

            if (current_t - rl[i].last_t >= rl[i].interval)
            {
#ifdef DEBUG_RFID
                {
                    Serial.println("relay off T=>" + String(current_t - rl[i].last_t));
                }
#endif
                rl[i].status = Status_RL::OFF;
                rl[i].last_t = current_t;
                digitalWrite(rl[i].pin, HIGH);
                delay(225);
                i2crecovery();
                return false;
            }
            else
            {
#ifdef DEBUG_RFID
                Serial.println("interval=>" + String(current_t - rl[i].last_t));
#endif
                digitalWrite(rl[i].pin, LOW);
                return true;
            }
        }
        else
        {
            digitalWrite(rl[i].pin, HIGH);
            // rl[i].last_t = current_t;
        }
    }
    return false;
}
signed char rfid_state::card_isregistered(const database_s *db)
{
    bool match = false;
    signed char number_locker = -1;
    for (size_t x = 0; x < Size_Siswa; x++)
    {
        match = true;
        // for (size_t xp = 0; xp < size_uid_incoming; xp++)
        for (size_t xp = 0; xp < 4; xp++)
        {
            if (db[x].card[xp] != uid_incoming[xp])
            {
#ifdef DEBUG_RFID
                {
                    Serial.println("::not match =>" + String(db[x].card[xp]) + ":incoming:" +
                                   String(uid_incoming[xp]) + " :C_Arr:" + String(xp) + ":C_idx:" + String(xp));
                }
#endif
                match = false;
                break;
            }
        }
        if (match)
        {
            number_locker = x;
#ifdef DEBUG_RFID
            {
                Serial.println("match=> index" + String(x));
                for (size_t i = 0; i < size_uid_incoming; i++)
                {
                    Serial.println("uid:" + String(db[x].card[i]) + ":in:" + String(uid_incoming[i]));
                }
            }
#endif
            return number_locker;
        }
    }
    return number_locker;
}
void rfid_state::init_sensor(Adafruit_PN532 *nfc)
{
    Wire.setTimeout(50);
    Wire.begin();
    Wire.setWireTimeout(3000, true);
    // Wire.setClock(50000);
#ifdef DEBUG_RFID
    bool is_normal = (nfc->begin() && nfc->SAMConfig());
    if (is_normal)
        Serial.println(":rfid:sensor already");
    else
        Serial.println(":rfid:sensor failure");
#else
    nfc->begin();
    nfc->SAMConfig();

#endif
}
bool sensor_undetect(Adafruit_PN532 *nfc)
{
    static unsigned long last_init = 0;
    static bool need_rescan = false;
    static byte counting_reset = 0;

    Wire.beginTransmission(PN532_I2C_ADDRESS);
    byte bus_error = Wire.endTransmission();
    bool bus_ok = (bus_error == 0);

    if (Wire.getWireTimeoutFlag())
    {
        Wire.clearWireTimeoutFlag();
        Wire.begin();
#ifdef DEBUG_RFID
        Serial.println(":rfid:i2c timeout, bus reset");
#endif
        need_rescan = true;
    }

    if (!bus_ok)
    {
        need_rescan = true;
#ifdef DEBUG_RFID
        Serial.println(":rfid:i2c bus error: " + String(bus_error));
#endif
    }

    bool sensor_ok = false;
    if (bus_ok)
    {
        sensor_ok = (nfc->getFirmwareVersion() != 0);
    }

    if (need_rescan)
    {
        if (counting_reset >= 10)
        {
            counting_reset = 0;
            sys.software_Resatrt();
        }

        if (millis() - last_init > 3000)
        {
            counting_reset++;
            last_init = millis();
#ifdef DEBUG_RFID
            Serial.println(":rfid:reinit pn532");
            if (need_rescan)
                Serial.println(":rfid:scan failure:" + String(counting_reset));
            else
                Serial.println(":rfid:scan succes");
#endif

            if (sensor_ok)
            {
#ifdef DEBUG_RFID
                Serial.println("board detect");
#endif
                need_rescan = false;
                counting_reset = 0;
                return true;
            }
            return true;
        }
        return false;
    }
    else if (!sensor_ok)
    {
        need_rescan = true;
#ifdef DEBUG_RFID
        Serial.println(":rfid:scan" + String(need_rescan));
#endif
    }

    return false;
}
// void rfid_state::init_sensor(Adafruit_PN532 *nfc)
// {
//     Wire.setTimeout(50);
//     Wire.begin();
//     Wire.setWireTimeout(3000, true);
// #ifdef DEBUG_RFID
//     bool is_normal = (nfc->begin() && nfc->SAMConfig());
//     if (is_normal)
//         Serial.println(":rfid:sensor already");
//     else
//         Serial.println(":rfid:sensor failure");
// #else
//     nfc->begin();
//     nfc->SAMConfig();

// #endif
// }
// bool sensor_undetect(Adafruit_PN532 *nfc)
// {
//     static unsigned long last_init = 0;
//     static bool need_rescan = false;

//     bool sensor_ok = (nfc->getFirmwareVersion() != 0);
//     if (need_rescan)
//     {
//         static byte counting_reset = 0;
//         if (counting_reset >= 15)
//         {
//             counting_reset = 0;
//             sys.software_Reset();
//         }
//         if (millis() - last_init > 4000)
//         {
//             counting_reset++;
//             last_init = millis();
// #ifdef DEBUG_RFID
//             Serial.println(":rfid:reinit pn532");
//             if (need_rescan)
//                 Serial.println(":rfid:scan failure:" + String(counting_reset));
//             else
//                 Serial.println(":rfid:scan succes");

// #endif

//             if (sensor_ok)
//             {
// #ifdef DEBUG_RFID
//                 Serial.println("board detect");
// #endif
//                 need_rescan = false;
//                 counting_reset = 0;
//                 return false;
//             }
//             return true;
//         }
//         return false;
//     }

//     else if (!sensor_ok)
//     {
//         need_rescan = !(sensor_ok);
// #ifdef DEBUG_RFID
//         Serial.println(":rfid:scan" + String(need_rescan));
// #endif
//     }
//     return false;
// }
char rfid_state::read_crd(const database_s *data, Adafruit_PN532 *nfc, Relay_state *rl)
{
    unsigned long now = millis();
    static bool last_read_c = false;
    static unsigned long last_t = 0;
    if (now - last_t < interval)
        return -1;
    if (sensor_undetect(nfc))
    {
        init_sensor(nfc);
        return -2;
    }

    bool read_c = nfc->readPassiveTargetID(
        PN532_MIFARE_ISO14443A,
        uid_incoming,
        &size_uid_incoming,
        100);

    if (read_c && !last_read_c)
    {
#ifdef DEBUG_RFID
        byte fr_read[3] = {DEC, HEX, BIN};
        const char *fr_label[3] = {"DEC", "HEX", "BIN"};
        for (size_t cf = 0; cf < 3; cf++)
        {
            Serial.print(":rfid:Card detected (");
            Serial.print(fr_label[cf]);
            Serial.print(") => ");

            for (size_t i = 0; i < size_uid_incoming; i++)
            {
                Serial.print(uid_incoming[i], fr_read[cf]);
                Serial.print(' ');
            }
            Serial.println();
        }

        if (size_uid_incoming >= 4)
        {
            unsigned long uid_decimal = 0;
            for (int i = 3; i >= 0; i--)
            {
                uid_decimal = (uid_decimal << 8) | uid_incoming[i];
            }
            Serial.print(":rfid:UID Decimal => ");
            Serial.println(uid_decimal);
        }
        else
        {
            Serial.println(":rfid:UID too short (< 4 bytes)");
        }
#endif
        last_read_c = true;
        last_t = now;

        signed char number_locker = card_isregistered(data);
        if (number_locker != -1)
        {
            rl[number_locker].status = Status_RL::ON;
            rl[number_locker].reset_t = true;
            rl[number_locker].last_t = millis();
        }
        else
        {
#ifdef DEBUG_RFID
            Serial.println(":rfid:uid not registered");
#endif
            return -4;
        }
        return 1;
    }

    if (!read_c && last_read_c)
    {
        memset(uid_incoming, 0, sizeof(uid_incoming));
        size_uid_incoming = size_uid;
        last_read_c = false;
        last_t = millis();
#ifdef DEBUG_RFID
        Serial.println(":rfid:card removed");
#endif
        return -1;
    }

    return 0;
}
#endif