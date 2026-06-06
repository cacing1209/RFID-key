#include <sdf.h>
#include <commond.h>
sdf_state::sdf_state(byte csP)
{
    sd_isnormal = card.begin(csP);
}
void sdf_state::init()
{
    if (!sd_isnormal)
    {
#ifdef DEBUG_SD
        Serial.println(":sd:abnormal");
#endif
        return;
    }
}

bool sdf_state::is_healthy()
{
    if (!sd_isnormal)
        return false;
    
    // Try to access root directory
    SdFile root;
    if (!root.open("/", O_READ))
    {
#ifdef DEBUG_SD
        Serial.println(":sd:health check failed");
#endif
        return false;
    }
    root.close();
    return true;
}

bool sdf_state::file_exists(const char *filename)
{
    if (!sd_isnormal)
        return false;
    
    SdFile f;
    bool exists = f.open(filename, O_READ);
    if (exists)
        f.close();
    return exists;
}

size_t sdf_state::get_free_space()
{
    if (!sd_isnormal)
        return 0;
    
    // Check total size (simplified - depends on SdFat version)
    uint64_t vol_sectors = card.card()->sectorCount();
    return (vol_sectors * 512UL);
}

bool sdf_state::open_file(String f)
{
    if (!sd_isnormal)
    {
#ifdef DEBUG_SD
        Serial.println(":sd:card not initialized");
#endif
        return false;
    }
    
    bool f_opened = card.open(f.c_str(), O_READ);
    if (!f_opened)
    {
#ifdef DEBUG_SD
        Serial.println(":sd:cannot open file " + String(f));
#endif
        return false;
    }
    return true;
}

bool sdf_state::save_file(String f)
{
    if (!sd_isnormal)
    {
#ifdef DEBUG_SD
        Serial.println(":sd:card not initialized");
#endif
        return false;
    }
    
    bool f_created = card.open(f.c_str(), O_CREAT | O_TRUNC | O_WRITE);
    if (!f_created)
    {
#ifdef DEBUG_SD
        Serial.println(":sd:cannot create file " + String(f));
#endif
        return false;
    }
    // Caller harus manual close file via card.close()
    return true;
}
