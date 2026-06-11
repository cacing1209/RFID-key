#include <sdf.h>
#include <commond.h>
sdf_state::sdf_state(byte csP)
{
    // JANGAN card.begin() di sini: constructor jalan saat static-init, SEBELUM
    // SPI.begin()/Serial.begin() di setup(). Poke SPI di tahap itu bisa nge-hang
    // board sebelum Serial nyala (gak ada output sama sekali). begin() yg
    // sebenernya dilakuin di runtime: ethernet_state::begin() (sd_card) atau
    // Updater_state::sd_begin() (OTA). csP diabaikan di sini.
    (void)csP;
    sd_isnormal = false;
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

// Append satu baris event ke SDLOG_ACTIVE. Rotate (active -> backup) kalau
// ukuran file >= cap. Single open-write-sync-close, pakai char[80] (no String).
bool sdf_state::log_event(unsigned long epoch, byte idx, unsigned long uid,
                          const char *dev_class, const char *flag)
{
    if (!sd_isnormal)
    {
#ifdef DEBUG_SD
        Serial.println(":sd:log skip (card disabled)");
#endif
        return false;
    }

    SdFile f;
    if (!f.open(SDLOG_ACTIVE, O_WRITE | O_CREAT))
    {
        // Open utk WRITE|CREAT gagal = card dicabut/rusak (bukan sekadar file
        // gak ada). Disable logging biar op berikut gak nyangkut di card mati;
        // loop() yg nyoba re-detect & recover (lihat BUG SD Card sd_log.txt).
        sd_isnormal = false;
#ifdef DEBUG_SD
        Serial.println(F(":sd:log open FAIL " SDLOG_ACTIVE " -> disable, tunggu re-detect"));
#endif
        return false;
    }

    // Rotasi: file aktif penuh -> rename ke .OLD (buang .OLD lama dulu),
    // lalu bikin file aktif baru yg kosong.
    if (f.fileSize() >= SDLOG_CAP_BYTES)
    {
        f.close();
        card.remove(SDLOG_BACKUP);             // buang backup lama (abaikan hasil)
        card.rename(SDLOG_ACTIVE, SDLOG_BACKUP); // event.log -> event.old
#ifdef DEBUG_SD
        Serial.println(":sd:rotate " SDLOG_ACTIVE " -> " SDLOG_BACKUP);
#endif
        if (!f.open(SDLOG_ACTIVE, O_WRITE | O_CREAT))
        {
            sd_isnormal = false; // card hilang pas rotate -> disable, biar re-detect recover
#ifdef DEBUG_SD
            Serial.println(F(":sd:log reopen FAIL -> disable, tunggu re-detect"));
#endif
            return false;
        }
    }

    f.seekSet(f.fileSize()); // posisikan di akhir (append)

    char line[80];
    int n = snprintf(line, sizeof(line), "%lu,%u,%lu,%s,%s\n",
                     epoch, (unsigned)idx, uid, dev_class, flag);
    if (n <= 0)
    {
        f.close();
        return false;
    }
    if (n >= (int)sizeof(line)) // snprintf ke-truncate -> tulis yg muat aja
        n = sizeof(line) - 1;

    size_t w = f.write((const uint8_t *)line, n);
    f.sync();
    uint32_t total = f.fileSize();
    f.close();

    bool ok = (w == (size_t)n);
#ifdef DEBUG_SD
    // line udah diakhiri '\n', jadi Serial.print(line) langsung pindah baris.
    Serial.print(ok ? ":sd:saved " : ":sd:save FAIL ");
    Serial.print(line);
    Serial.print(":sd:file ");
    Serial.print(SDLOG_ACTIVE);
    Serial.print(" = ");
    Serial.print(total);
    Serial.println(" bytes");
#endif
    return ok;
}

// Stream n baris terakhir SDLOG_ACTIVE ke `out`. Scan mundur dari akhir file
// per-chunk buat nemu offset awal n baris terakhir, baru stream maju. Gak
// pernah load seluruh file ke RAM. Return jumlah baris yg ke-print.
size_t sdf_state::log_tail(Print &out, byte n)
{
    if (!sd_isnormal || n == 0)
        return 0;

    SdFile f;
    if (!f.open(SDLOG_ACTIVE, O_READ))
        return 0;

    const uint16_t CHUNK = 64;
    char buf[CHUNK];

    uint32_t pos = f.fileSize();
    uint32_t start = 0;       // default: dari awal kalau baris < n
    uint16_t newlines = 0;    // file diakhiri '\n', jadi butuh n+1 utk skip n baris
    while (pos > 0)
    {
        uint16_t want = (pos < CHUNK) ? (uint16_t)pos : CHUNK;
        pos -= want;
        f.seekSet(pos);
        if (f.read(buf, want) != (int)want)
            break;
        for (int i = (int)want - 1; i >= 0; i--)
        {
            if (buf[i] == '\n' && ++newlines == (uint16_t)n + 1)
            {
                start = pos + (uint32_t)i + 1;
                pos = 0; // hentikan loop luar
                break;
            }
        }
    }

    f.seekSet(start);
    size_t lines = 0;
    int got;
    while ((got = f.read(buf, CHUNK)) > 0)
    {
        out.write((const uint8_t *)buf, (size_t)got);
        for (int i = 0; i < got; i++)
            if (buf[i] == '\n')
                lines++;
    }
    f.close();
    return lines;
}
