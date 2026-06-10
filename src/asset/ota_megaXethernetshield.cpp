#include <ota_megaXethernetshield.h>

Updater_state::Updater_state(/* args */)
{
}

Updater_state::~Updater_state()
{
}

// === SD Card Wrapper Methods ===
bool Updater_state::sd_begin()
{
    // is_sd is a global, so its constructor may have run card.begin() before
    // SPI.begin() in setup(). Re-init here when needed (e.g. post-OTA cleanup).
    is_sd.sd_isnormal = is_sd.card.begin(CS_P_SD);
    return is_sd.sd_isnormal;
}

bool Updater_state::sd_is_healthy()
{
    return is_sd.is_healthy();
}

size_t Updater_state::sd_get_free_space()
{
    return is_sd.get_free_space();
}

bool Updater_state::sd_file_exists(const char *filename)
{
    return is_sd.file_exists(filename);
}

bool Updater_state::sd_remove_file(const char *filename)
{
    if (!is_sd.sd_isnormal)
        return false;

    return is_sd.card.remove(filename);
}

bool Updater_state::check_firmware(const char *f)
{
    bool ok = sd_file_exists(f);
#ifdef DEBUG_OTA
    Serial.println(ok ? ":ota:firmware file exists" : ":ota:firmware file not found");
#endif
    return ok;
}

// Mark the staged image and reset into avr_boot. avr_boot reads FIRMWARE.BIN
// from SD and flashes it, then jumps to the (new) application.
void Updater_state::stage_and_reboot()
{
    EEPROM.update(FLAG_FLASH_ADDRS, FLAG_FLASH_PENDING);
#ifdef DEBUG_OTA
    Serial.println(":ota:staged FIRMWARE.BIN, resetting into bootloader...");
    Serial.flush();
#endif
    // Real MCU reset: BOOTRST sends execution to avr_boot. (jmp 0 would skip it.)
    wdt_enable(WDTO_15MS);
    while (1)
    {
    }
}

// Runs once on every boot. If we previously staged an OTA, avr_boot has now
// flashed FIRMWARE.BIN; remove it so it is not reflashed on the next reset.
bool Updater_state::post_ota_cleanup()
{
    if (EEPROM.read(FLAG_FLASH_ADDRS) != FLAG_FLASH_PENDING)
        return false; // no OTA was staged

    if (!is_sd.sd_isnormal)
        sd_begin(); // SD may not have initialised at static-construction time

    if (sd_file_exists(FW_BIN_NAME) && !sd_remove_file(FW_BIN_NAME))
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:cleanup failed, will retry next boot");
#endif
        return false; // keep flag set; retry on the next boot
    }

    EEPROM.update(FLAG_FLASH_ADDRS, FLAG_FLASH_DONE);
#ifdef DEBUG_OTA
    Serial.println(":ota:update complete, FIRMWARE.BIN removed");
#endif
    return true;
}
