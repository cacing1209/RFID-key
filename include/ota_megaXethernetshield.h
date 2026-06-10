#ifndef OTA_MEGAXETHERNETSHIELD_H
#define OTA_MEGAXETHERNETSHIELD_H
#include <commond.h>
#include <EEPROM.h>
#include <sdf.h>

// OTA strategy: HTTP download -> SD (FIRMWARE.BIN) -> MCU reset -> avr_boot
// (SD-card bootloader, NRWW section) flashes FIRMWARE.BIN on the next reset.
//
// The application does NOT self-program flash: SPM instructions only execute
// from the bootloader section on the ATmega2560, so an app-side flasher is a
// silent no-op. All flashing is delegated to avr_boot.
//
// EEPROM flag (addr 0x00) is purely app-side bookkeeping: after avr_boot has
// flashed FIRMWARE.BIN, the freshly booted firmware sees the PENDING flag and
// deletes FIRMWARE.BIN so the bootloader does not reflash it on every reset
// (avr_boot is KISS: it reflashes as long as the file is present).
#define FLAG_FLASH_ADDRS 0x00
#define FLAG_FLASH_DONE 0x00
#define FLAG_FLASH_PENDING 0x01
#define FW_BIN_NAME "FIRMWARE.BIN"
#define OTETHERNET

/*
curl -X POST http://192.168.0.8:8000/update \
  -H "Authorization: Bearer <token_admin>" \
  -H "Content-Type: application/json" \
  -d '{"download": true, "flash": true}'
download/stage only (no reboot): {"download": true, "flash": false}
*/
class Updater_state
{

private:
    sdf_state is_sd;

public:
    bool check_firmware(const char *firmware = FW_BIN_NAME);

    // Set the PENDING flag and reset the MCU into avr_boot, which flashes
    // FIRMWARE.BIN from SD. Does not return (watchdog reset).
    // NOTE: a plain `jmp 0` would NOT work here -- it restarts the app
    // without entering the bootloader.
    void stage_and_reboot();

    // Call once early in setup(). If an OTA was staged, avr_boot has already
    // flashed FIRMWARE.BIN during this boot; delete it and clear the flag.
    // Returns true if a firmware update just completed.
    bool post_ota_cleanup();

    // SD card access wrappers (for download_firmware)
    bool sd_begin();
    bool sd_is_healthy();
    size_t sd_get_free_space();
    bool sd_file_exists(const char *filename);
    bool sd_remove_file(const char *filename);

    Updater_state();
    ~Updater_state();
};

#endif
