#ifndef SDF_H
#define SDF_H
#include <SdFat.h>
#include <ArduinoJson.h>
#define CS_P_SD 0x04

// Event log fallback ke SD card (lihat sd_log.txt). Opsi A: single file
// aktif + rotate ke .OLD saat lewat cap. Nama 8.3 di root biar gak perlu
// mkdir. Re-upload PEND = FASE lanjutan (belum diimplementasikan di sini).
#define SDLOG_ACTIVE "EVENT.LOG"
#define SDLOG_BACKUP "EVENT.OLD"
#define SDLOG_CAP_BYTES 32768UL

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

    // Append satu event (CSV) ke SDLOG_ACTIVE, rotate kalau lewat cap.
    // Format: <epoch>,<index>,<uid_decimal>,<dev_class>,<flag>\n
    // Pakai char[80] + snprintf, JANGAN String (RAM tight). Return true
    // kalau baris kesimpan utuh.
    bool log_event(unsigned long epoch, byte idx, unsigned long uid,
                   const char *dev_class, const char *flag);

    // Stream n baris terakhir SDLOG_ACTIVE ke `out` (EthernetClient/Serial).
    // Seek dari akhir file, gak load semua ke RAM. Return jumlah baris.
    size_t log_tail(Print &out, byte n = 10);

    sdf_state(byte csP = CS_P_SD); // constructor
};
#endif
