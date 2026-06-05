#include <ota_megaXethernetshield.h>
Updater_state::Updater_state(/* args */)
{
}

Updater_state::~Updater_state()
{
}

void Updater_state::check_sum(int idx) {}

// === SD Card Wrapper Methods ===
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
    if (is_sd.open_file(f))
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:firmware file exists");
#endif
        return true;
    }
#ifdef DEBUG_OTA
    Serial.println(":ota:firmware file not found");
#endif
    return false;
}

// Parse HEX file dan flash ke memory
bool Updater_state::flash_from_sd(const char *filename)
{
    SdFile fwFile;
    if (!fwFile.open(filename, O_READ))
    {
#ifdef DEBUG_OTA
        Serial.print(":ota:cannot open ");
        Serial.println(filename);
#endif
        return false;
    }

    // Validate file size - must be at least 1KB
    uint32_t file_size = fwFile.fileSize();
    if (file_size < 1000)
    {
#ifdef DEBUG_OTA
        Serial.print(":ota:invalid file size: ");
        Serial.println(file_size);
#endif
        fwFile.close();
        return false;
    }

#ifdef DEBUG_OTA
    Serial.print(":ota:parsing HEX file (size: ");
    Serial.print(file_size);
    Serial.println(" bytes)...");
#endif

    uint32_t flash_address = 0;
    uint8_t flash_buffer[SPM_PAGESIZE];
    size_t buffer_idx = 0;
    char line[128]; // Increased from 100 to prevent overflow
    int line_idx = 0;
    unsigned long bytes_flashed = 0;
    int lines_parsed = 0;
    bool parse_error = false;

    while (fwFile.available() && !parse_error)
    {
        char c = fwFile.read();

        // Parse HEX file line by line with overflow protection
        if (c == '\n' || c == '\r')
        {
            if (line_idx == 0)
                continue;

            line[line_idx] = '\0';
            line_idx = 0;
            lines_parsed++;

            // Parse Intel HEX format: :LLAAAATTDD...CC
            // Minimum valid line: :10000000DD...CC (11 chars + data)
            if (line[0] != ':' || line_idx < 11)
            {
#ifdef DEBUG_OTA
                if (line[0] != ':')
                {
                    Serial.println(":ota:invalid HEX format (no ':')");
                    parse_error = true;
                }
#endif
                continue;
            }

            // Validate hex characters (basic check)
            int byte_count = strtol(&line[1], NULL, 16);
            uint32_t addr = strtol(&line[3], NULL, 16);
            int record_type = strtol(&line[7], NULL, 16);

            // Sanity check byte count
            if (byte_count < 0 || byte_count > 32)
            {
#ifdef DEBUG_OTA
                Serial.print(":ota:invalid byte count: ");
                Serial.println(byte_count);
#endif
                continue; // Skip invalid line, don't fail entirely
            }

            if (record_type == 0x00) // Data record
            {
                for (int i = 0; i < byte_count; i++)
                {
                    // Bounds check: prevent reading past line end
                    if ((9 + i * 2 + 1) >= line_idx)
                    {
#ifdef DEBUG_OTA
                        Serial.println(":ota:line truncated, skipping");
#endif
                        break;
                    }

                    char hex_byte[3];
                    hex_byte[0] = line[9 + i * 2];
                    hex_byte[1] = line[10 + i * 2];
                    hex_byte[2] = '\0';

                    uint8_t data_byte = strtol(hex_byte, NULL, 16);
                    flash_buffer[buffer_idx++] = data_byte;

                    // Ketika buffer penuh (page size), flash ke memory
                    if (buffer_idx >= SPM_PAGESIZE)
                    {
                        if (!write_flash_page(flash_address, flash_buffer, SPM_PAGESIZE))
                        {
#ifdef DEBUG_OTA
                            Serial.print(":ota:flash write error at address ");
                            Serial.println(flash_address, HEX);
#endif
                            fwFile.close();
                            return false;
                        }
                        bytes_flashed += SPM_PAGESIZE;
                        flash_address += SPM_PAGESIZE;
                        buffer_idx = 0;

#ifdef DEBUG_OTA
                        if (bytes_flashed % (SPM_PAGESIZE * 10) == 0)
                        {
                            Serial.print(":ota:flashed ");
                            Serial.print(bytes_flashed);
                            Serial.println(" bytes");
                        }
#endif
                    }
                }
            }
            else if (record_type == 0x01) // End of file
            {
                break;
            }
            else if (record_type != 0x04 && record_type != 0x05) // Skip other types
            {
#ifdef DEBUG_OTA
                Serial.print(":ota:skipping record type ");
                Serial.println(record_type, HEX);
#endif
            }
        }
        else
        {
            // Buffer overflow protection: stop reading line if exceeds 127 chars
            if (line_idx < sizeof(line) - 1)
            {
                line[line_idx++] = c;
            }
            else
            {
#ifdef DEBUG_OTA
                Serial.println(":ota:line too long, skipping");
#endif
                line_idx = 0; // Reset line buffer
                parse_error = true;
            }
        }
    }

    // Flush remaining data jika ada
    if (buffer_idx > 0 && !parse_error)
    {
        if (!write_flash_page(flash_address, flash_buffer, buffer_idx))
        {
#ifdef DEBUG_OTA
            Serial.println(":ota:final flash write error");
#endif
            fwFile.close();
            return false;
        }
        bytes_flashed += buffer_idx;
    }

    fwFile.close();

    if (parse_error)
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:parse error detected");
#endif
        return false;
    }

#ifdef DEBUG_OTA
    Serial.print(":ota:flash complete. Lines: ");
    Serial.print(lines_parsed);
    Serial.print(", Total: ");
    Serial.print(bytes_flashed);
    Serial.println(" bytes");
#endif

    return bytes_flashed > 0;
}

// Write page ke flash memory (Arduino Mega 2560)
bool Updater_state::write_flash_page(uint32_t address, uint8_t *data, size_t len)
{
    // Validate parameters
    if (!data || len == 0)
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:invalid write parameters");
#endif
        return false;
    }

    // Check bounds - Arduino Mega 2560 has 256KB flash (0x40000)
    if (address + len > 0x40000)
    {
#ifdef DEBUG_OTA
        Serial.print(":ota:address out of range: 0x");
        Serial.print(address, HEX);
        Serial.print(" + ");
        Serial.println(len);
#endif
        return false;
    }

    // Address must be page-aligned for erase
    if (address % SPM_PAGESIZE != 0)
    {
#ifdef DEBUG_OTA
        Serial.print(":ota:address not page-aligned: 0x");
        Serial.println(address, HEX);
#endif
        return false;
    }

    // Disable interrupts selama flash write
    cli();

    // Erase page
    boot_page_erase(address);
    boot_spm_busy_wait();

    // Fill page buffer dan tulis
    for (size_t i = 0; i < len; i += 2)
    {
        // Ensure we don't read past data buffer
        uint8_t byte1 = data[i];
        uint8_t byte2 = (i + 1 < len) ? data[i + 1] : 0xFF;
        uint16_t word = byte1 | (byte2 << 8);
        boot_page_fill(address + i, word);
    }

    // Write page
    boot_page_write(address);
    boot_spm_busy_wait();

    // Re-enable RWW section
    boot_rww_enable();

    // Re-enable interrupts
    sei();

#ifdef DEBUG_OTA
    Serial.print(":ota:page written at 0x");
    Serial.println(address, HEX);
#endif

    return true;
}

bool Updater_state::execute_flash(const char *filename)
{
#ifdef DEBUG_OTA
    Serial.println(":ota:starting firmware flash...");
#endif

    if (!check_firmware(filename))
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:firmware file not found");
#endif
        return false;
    }

    if (!flash_from_sd(filename))
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:flash failed");
#endif
        return false;
    }

#ifdef DEBUG_OTA
    Serial.println(":ota:flash successful! restarting...");
#endif

    // Restart setelah flash selesai
    delay(1000);
    asm volatile("jmp 0"); // Jump ke bootloader

    return true;
}