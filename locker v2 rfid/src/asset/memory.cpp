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
#ifdef DEBUG_MEM
            Serial.println(":mem:size>memory size =>" + String(addr - epr.length()));
#endif
            return false;
        }
    }
#ifdef DEBUG_MEM
    {
        Serial.println(":mem:load");
        for (size_t x = 0; x < size_mahasiswa; x++)
        {
            Serial.print(":mem:card =>" + String(x) + "=>");
            for (size_t y = 0; y < size_uid; y++)
            {
                if (y != 0)
                    Serial.print(',');
                Serial.print(db[x].card[y]);
            }
            if (db[x].statusdb == Status_db::Available)
                Serial.println(" avaiable");
            else
                Serial.println(" unavaiable");
        }

        Serial.println(":mem:load succes!!!");
    }
#endif
    return true;
}
bool storage_state::save_data(database_s *db, database_s *new_db)
{
    int addr = flags_load;
    bool saved = false;
    for (size_t i = 0; i < size_mahasiswa; i++)
    {
        Status_db st = Status_db::Available;
        bool replace = false;
        for (size_t idx = 0; idx < size_uid; idx++)
        {
            // if (i == 10 || i == 25)
            // {
            //     new_db[i].card[idx] = 0;
            // }
            if (db[i].card[idx] != new_db[i].card[idx])
            {
                db[i].card[idx] = new_db[i].card[idx];
#ifdef DEBUG_MEM
                Serial.println(":mem:card uid=>" + String(db[i].card[idx]));
#endif
                replace = true;
            }
            if (new_db[i].card[idx] != 0)
            {
                st = Status_db::Not_Available;
            }
        }

        if (db[i].number_locker != new_db[i].number_locker)
        {
            db[i].number_locker = new_db[i].number_locker;
#ifdef DEBUG_MEM
            {
                Serial.println(":mem:number replace=>" + String(i));
            }
#endif
            replace = true;
        }
        if (st != db[i].statusdb)
        {
            db[i].statusdb = st;
            replace = true;
        }
#ifdef DEBUG_MEM
        Serial.println("status " + String(db[i].statusdb == Status_db::Available ? "avaiable" : "not avaiable"));
#endif
        if (replace)
        {
#ifdef DEBUG_MEM
            Serial.println(":mem:card replace=>" + String(i));
#endif
            // epr.put(addr, db[i]);
            saved = true;
        }
        addr += sizeof(database_s);

        if (addr >= epr.length())
        {
#ifdef DEBUG_MEM
            Serial.println(":mem:Memory need size =>" + String(addr - epr.length()));
#endif
            return false;
        }
    }

#ifdef DEBUG_MEM
    {
        if (saved)
            Serial.println("\n:mem:saved new data!!!");
        else
            Serial.println("\n:mem:data alredy registered!!!");
        Serial.println("\n:mem:sync succes!!!");
    }
#endif

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
//                 #ifdef DEBUG_MEM
//                     Serial.println("::size>memory size => " + String(addr - epr.length()));
//                 return false;
//             }

//             ptr[b] = epr.read(addr);
//             addr++;
//         }
//     }

//     #ifdef DEBUG_MEM
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
//                 #ifdef DEBUG_MEM
//                     Serial.println("::Memory need size => " + String(addr - EEPROM.length()));
//                 return false;
//             }

//             EEPROM.write(addr, ptr[b]);
//             addr++;
//         }
//     }

//     #ifdef DEBUG_MEM
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