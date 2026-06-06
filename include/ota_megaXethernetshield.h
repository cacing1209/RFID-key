// checkpoint
// https://github.com/cacing1209/RFID-key/issues/3#issue-4412689914

#ifndef OTA_MEGAXETHERNETSHIELD_H
#define OTA_MEGAXETHERNETSHIELD_H
#include <commond.h>
#include <avr/boot.h>
#include <sdf.h>
#define FLAG_FLASH_ADDRS 0x00
#define FLAG_FLASH_PENDING 0x01
#define OTETHERNET

/*
curl -X POST http://192.168.0.8:8000/update \
  -H "Authorization: Bearer lockerqyubitL0002L0004L0008L000264L000128" \
  -H "Content-Type: application/json" \
  -d '{"download": false, "flash": true}'
test download only no flash
  */
class Updater_state
{

private:
    String url;
    int idx_flash;
    bool status_upgrade;
    bool interupt_other;
    sdf_state is_sd;
    
    bool flash_from_sd(const char *filename);
    bool write_flash_page(uint32_t address, uint8_t *data, size_t len);

public:
    bool check_firmware(const char *firmware = "firmware.hex");
    bool execute_flash(const char *filename = "firmware.hex");
    void check_sum(int idx);
    
    // SD card access wrappers (for download_firmware)
    bool sd_is_healthy();
    size_t sd_get_free_space();
    bool sd_file_exists(const char *filename);
    bool sd_remove_file(const char *filename);
    
    Updater_state();
    ~Updater_state();
};

#endif
