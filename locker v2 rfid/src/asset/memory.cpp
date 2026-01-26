#include <commond.h>
#include <EEPROM.h>

#define flags_register 0x02
#define flags_sync_db 0x4
#define flags_fctry_reset 0x8
#define flags_load 0x10

EEPROMClass epr;

bool storage_state::load_data(database_s *db)
{
    int addr = flags_load;

    for (size_t i = 0; i < size_mahasiswa; i++)
    {
        epr.get(addr, db[i]);
        addr += sizeof(database_s);
        if (addr >= epr.length())
        {
            if (enable_debug)
                Serial.println(":mem:size>memory size =>" + String(addr - epr.length()));

            return false;
        }
    }
    if (enable_debug)
    {
        for (size_t x = 0; x < size_mahasiswa; x++)
        {
            Serial.print(":mem:card =>" + String(x) + "=>");
            for (size_t y = 0; y < size_uid; y++)
            {
                if (y != 0)
                    Serial.print(',');
                Serial.print(db[x].card[y]);
            }
            Serial.println();
        }

        Serial.println(":mem:load succes!!!");
    }
    return true;
}
bool storage_state::save_data(database_s *db, database_s *new_db)
{
    int addr = flags_load;
    for (size_t i = 0; i < size_mahasiswa; i++)
    {
        bool replace = false;
        for (size_t idx = 0; idx < size_uid; idx++)
        {
            if (db[i].card[idx] != new_db[i].card[idx])
            {
                db[i].card[idx] = new_db[i].card[idx];
                if (enable_debug)
                {
                    Serial.println(":mem:card replace=>" + String(i));
                }
                replace = true;
            }
        }

        if (db[i].number_locker != new_db[i].number_locker)
        {
            db[i].number_locker = new_db[i].number_locker;
            if (enable_debug)
            {
                Serial.println(":mem:number replace=>" + String(i));
            }
            replace = true;
        }

        // if (replace)
        //     epr.put(addr, db[i]);
        addr += sizeof(database_s);

        if (addr >= epr.length())
        {
            if (enable_debug)
                Serial.println(":mem:Memory need size =>" + String(addr - epr.length()));

            return false;
        }
    }

    if (enable_debug)
        Serial.println(":mem:save succes!!!");

    return true;
}
// bool storage_state::load_data(database_s *db)
// {
//     int addr = flags_load;

//     for (size_t i = 0; i < size_mahasiswa; i++)
//     {
//         uint8_t *ptr = (uint8_t *)&db[i];

//         for (size_t b = 0; b < sizeof(database_s); b++)
//         {
//             if (addr >= epr.length())
//             {
//                 if (enable_debug)
//                     Serial.println("::size>memory size => " + String(addr - epr.length()));
//                 return false;
//             }

//             ptr[b] = epr.read(addr);
//             addr++;
//         }
//     }

//     if (enable_debug)
//     {
//         for (size_t x = 0; x < size_mahasiswa; x++)
//         {
//             Serial.print("card =>" + String(x) + "=>");
//             for (size_t y = 0; y < size_uid; y++)
//             {
//                 if (y != 0)
//                     Serial.print(',');
//                 Serial.print(db[x].card[y]);
//             }
//             Serial.println();
//         }

//         Serial.println("load success!!!");
//     }

//     return true;
// }

// bool storage_state::save_data(database_s *db)
// {
//     int addr = flags_load;

//     for (size_t i = 0; i < size_mahasiswa; i++)
//     {
//         uint8_t *ptr = (uint8_t *)&db[i];

//         for (size_t b = 0; b < sizeof(database_s); b++)
//         {
//             if (addr >= EEPROM.length())
//             {
//                 if (enable_debug)
//                     Serial.println("::Memory need size => " + String(addr - EEPROM.length()));
//                 return false;
//             }

//             EEPROM.write(addr, ptr[b]);
//             addr++;
//         }
//     }

//     if (enable_debug)
//         Serial.println("save success!!!");

//     return true;
// }

void storage_state::factory_reset(database_s *db)
{
    int addr = flags_load;
    for (size_t i = 0; i < size_mahasiswa; i++)
    {
        for (size_t x = 0; x < size_uid; x++)
        {
            db[i].card[x] = 0;
        }
        // strcpy(db[i].created_at, "");
        db[i].number_locker = 0;
        epr.put(addr, db[i]);
        addr += sizeof(database_s);
    }
}