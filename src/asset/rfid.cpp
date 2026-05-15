
#include <commond.h>
#ifndef find_p
void i2crecovery_ultra_reset()
{
    Wire.end();
    pinMode(SCL, INPUT_PULLUP);
    pinMode(SDA, INPUT_PULLUP);
    delayMicroseconds(10);

    pinMode(SCL, OUTPUT);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(10);

    for (size_t i = 0; i < 9; i++)
    {
        if (digitalRead(SDA) == HIGH)
            break;
        digitalWrite(SCL, LOW);
        delayMicroseconds(10);
        digitalWrite(SCL, HIGH);
        delayMicroseconds(10);
    }

    // generate STOP: SDA LOW -> HIGH while SCL is HIGH
    pinMode(SDA, OUTPUT);
    digitalWrite(SDA, LOW);
    delayMicroseconds(10);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(10);
    digitalWrite(SDA, HIGH);
    delayMicroseconds(10);

    pinMode(SDA, INPUT_PULLUP);
    pinMode(SCL, INPUT_PULLUP);

    Wire.begin();
    Wire.setWireTimeout(25000, true);
#ifdef DEBUG_RFID
    Serial.println(":rfid:bus recovered, sda=" + String(digitalRead(SDA)));
#endif
}
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
rfid_state::rfid_state(const long interval_executed, const int interval_current_read) : interval_reExecute(interval_executed), interval_current_Read_C(interval_current_read)
{
    if (interval_executed < 25)
        interval_reExecute = 27;
    if (interval_executed > 3000)
        interval_reExecute = 3000;

    if (interval_current_read < 50)
        interval_current_Read_C = 75;
    if (interval_current_read > 250)
        interval_reExecute = 200;
}
bool rfid_state::open_doors(Relay_state *rl, bool *send_log, Adafruit_PN532 *nfc)
{
    unsigned long current_t = millis();
    for (size_t i = 0; i < Size_Siswa; i++)
    {
        if (rl[i].status == Status_RL::ON)
        {
            if (rl[i].open_loker)
            {
                rl[i].open_loker = false;
                send_log[i] = true;
                rl[i].last_t = millis();

                Wire.end();
                pinMode(SDA, INPUT_PULLUP);
                pinMode(SCL, INPUT_PULLUP);
#ifdef DEBUG_RFID
                Serial.println("locker on=>" + String(i) + " :i2c held");
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
                i2crecovery();
                init_sensor(nfc);
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
    Wire.setWireTimeout(25000, true);
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
    static unsigned long last_probe = 0;
    static bool need_rescan = false;
    static byte counting_reset = 0;
    static byte fail_streak = 0;

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
        fail_streak = 0;
    }

    if (!bus_ok)
    {
#ifdef DEBUG_RFID
        Serial.println(":rfid:i2c bus error: " + String(bus_error));
#endif
        if (++fail_streak >= 2)
            need_rescan = true;
    }

    bool sensor_ok = false;
    bool did_probe = false;
    unsigned long probe_interval = need_rescan ? 3000UL : 5000UL;
    if (bus_ok && millis() - last_probe >= probe_interval)
    {
        last_probe = millis();
        sensor_ok = (nfc->getFirmwareVersion() != 0);
        did_probe = true;
        if (sensor_ok)
            fail_streak = 0;
        else if (++fail_streak >= 2)
            need_rescan = true;
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
            if (did_probe && sensor_ok)
                Serial.println(":rfid:scan succes");
            else
                Serial.println(":rfid:scan failure:" + String(counting_reset));
#endif

            if (did_probe && sensor_ok)
            {
#ifdef DEBUG_RFID
                Serial.println("board detect");
#endif
                need_rescan = false;
                counting_reset = 0;
                fail_streak = 0;
                return true;
            }
            return true;
        }
        return false;
    }

    return false;
}

char rfid_state::read_crd(const database_s *data, Adafruit_PN532 *nfc, Relay_state *rl)
{
    unsigned long now = millis();
    static bool last_read_c = false;
    static unsigned long last_t = 0;
    if (now - last_t < interval_reExecute)
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
        interval_current_Read_C);

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
        // last_t = now;

        signed char number_locker = card_isregistered(data);
        if (number_locker != -1)
        {
            rl[number_locker].status = Status_RL::ON;
            rl[number_locker].open_loker = true;
            rl[number_locker].last_t = millis();

            // bypass
            // if (number_locker == 3)
            // {

            //     Wire.end();
            //     pinMode(SDA, INPUT_PULLUP);
            //     pinMode(SCL, INPUT_PULLUP);

            //     for (size_t i = 0; i < sizeRelay; i++)
            //     {
            //         digitalWrite(rl[i].pin, LOW);
            //         delay(50);
            //         digitalWrite(rl[i].pin, HIGH);
            //         delay(200);
            //     }
            //     i2crecovery();
            //     init_sensor(nfc);
            // }
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
#endif
