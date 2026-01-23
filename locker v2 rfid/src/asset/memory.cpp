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
                Serial.println("::size>memory size =>" + String(addr - epr.length()));

            return false;
        }
    }
    if (enable_debug)
    {
        for (size_t x = 0; x < size_mahasiswa; x++)
        {
            Serial.print("card =>" + String(x) + "=>");
            for (size_t y = 0; y < size_uid; y++)
            {
                if (y != 0)
                    Serial.print(',');
                Serial.print(db[x].card[y]);
            }
            Serial.println();
        }

        Serial.println("load succes!!!");
    }
    return true;
}
bool storage_state::save_data(database_s *db)
{
    // int addr = flags_load;

    // for (size_t i = 0; i < size_mahasiswa; i++)
    // {
    //     epr.put(addr, db[i]);
    //     addr += sizeof(database_s);
    //     if (addr >= epr.length())
    //     {
    //         if (enable_debug)
    //             Serial.println("::Memory need size =>" + String(addr - epr.length()));

    //         return false;
    //     }
    // }

    if (enable_debug)
        Serial.println("save succes!!!");

    return true;
}
void storage_state::factory_reset(database_s *db)
{
    int addr = flags_load;
    for (size_t i = 0; i < size_mahasiswa; i++)
    {
        for (size_t x = 0; x < size_uid; x++)
        {
            db[i].card[x] = 0;
        }
        strcpy(db[i].created_at, "");
        db[i].number_locker = 0;
        epr.put(addr, db[i]);
        addr += sizeof(database_s);
    }
}