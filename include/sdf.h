#include <SdFat.h>
#include <ArduinoJson.h>
#define cs_pin 0x04
struct sdf_state
{

    SdFat card;
    bool sd_isnormal;
    void init();
    bool open_file(String file);
    bool save_file(String file);
    bool is_healthy();  // Check SD card status
    size_t get_free_space();  // Get free space in bytes
    bool file_exists(const char *filename);
    sdf_state::sdf_state(byte csP = 4); // constructor
};
