# Laporan Project — Locker Sekolah RFID

> Dokumen status & progres project. Update terakhir mengikuti commit `36e8d75` (branch `test`).

---

## 1. Ringkasan

Sistem locker sekolah berbasis kartu RFID. Setiap controller mengelola sampai **30 locker** (1 unit Mega), bisa di-deploy dalam beberapa varian model (`L0002` … `L0512`) yang membedakan mapping pin relay sesuai layout PCB.

Operasi inti:

- Siswa tap kartu RFID → controller cek UID terhadap database lokal di EEPROM → buka relay locker yang sesuai → kirim event log ke server pusat lewat HTTP.
- Controller juga expose **HTTP API** (port `8000`) untuk pendaftaran kartu, query status locker, reset, dan restart dari sisi backend.

---

## 2. Hardware

| Komponen        | Detail                                                    |
| --------------- | --------------------------------------------------------- |
| MCU             | ATmega2560 (Arduino Mega) — `megaatmega2560`              |
| Network         | Ethernet shield W5100 / W5500 (CS = D10)                  |
| RFID            | PN532 via I²C (SDA = D20, SCL = D21)                      |
| Relay locker    | 32 pin output, mapping per-model (`pin_IO[]`)             |
| Indikator       | LED (D68), Buzzer (D69)                                   |
| Storage on-MCU  | EEPROM (database kartu), 248 KB flash                     |
| Storage planned | SD card slot di Ethernet shield (CS = D4) — belum dipakai |

Pinout PN532 & catatan ada di header `src/main.cpp`.

---

## 3. Arsitektur Software

```
src/
├── main.cpp                          → entry: setup() + loop() utama
├── asset/
│   ├── commond.cpp                   → struct umum, helper system
│   ├── rfid.cpp                      → baca PN532, validasi UID, trigger relay
│   ├── memory.cpp                    → load/save database & device class ke EEPROM
│   ├── time.cpp                      → NTP sync, system time formatter
│   ├── auth.cpp                      → bearer token check
│   ├── acc.cpp                       → buzzer + LED state machine
│   └── main_ethernet_.cpp            → server HTTP + DNS + send_eventLog
include/
├── commond.h                         → semua struct, enum, model defines
├── auth.h
├── sec_tkn.h           (gitignored)  → token bearer asli
└── sec_tkn_example.h                 → template token (di-commit)
```

### Modul utama

- **`ethernet_state`** — server HTTP di port 8000, parser request, dispatcher ke handler per-route, plus DNS resolver dengan TTL cache 6 jam untuk log server (`LOG_SERVER_HOST = locker-logs.qyubit.io`).
- **`rfid_state`** — polling PN532, decode UID 4-byte, lookup ke `database_s[Size_Siswa]`, set flag `eth.send_log[i]` ketika tap valid.
- **`storage_state`** — wrapper EEPROM untuk database kartu + device class (string identifier per unit).
- **`buzzer_state`** — state-machine non-blocking untuk pola bunyi (denied / accepted / multi-loop).
- **`NTPConfig`** — sync waktu UTC+7 dari `216.239.35.0`, dipakai untuk timestamp event log.
- **`auth_state`** — verifikasi `Authorization: Bearer <token>` untuk endpoint write.

Loop utama (`src/main.cpp:156`) di-prioritaskan: relay/buzzer dulu, baru `eth.loop()` & `rfid.read_crd()` ketika MCU idle. Tujuannya supaya bukaan locker tidak ter-delay HTTP traffic.

---

## 4. Status Implementasi

Legend: ✅ jalan stabil · 🟡 jalan tapi perlu polish · 🔧 belum / TODO

### 4.1 Core

| Fitur                                           | Status |
| ----------------------------------------------- | ------ |
| Baca kartu PN532 (I²C)                          | ✅     |
| Mapping UID → locker dari EEPROM                | ✅     |
| Buka relay + indikator buzzer/LED               | ✅     |
| 7 varian model (`L0002`…`L0512`) via `#define`  | ✅     |
| Persistence DB siswa di EEPROM                  | ✅     |
| Bypass loader 30 kartu (`bypass_add_card()`)    | ✅     |
| Watchdog software restart (`sys.software_*`)    | ✅     |

### 4.2 Networking

| Fitur                                              | Status |
| -------------------------------------------------- | ------ |
| Init Ethernet shield, static IP fallback `.0.8`    | ✅     |
| Deteksi link status (W5100 unknown-link safe)      | ✅     |
| Reconnect logic (interval 5 s, maintain 500 ms)    | ✅     |
| HTTP server port 8000 + parser request manual      | ✅     |
| NTP sync UTC+7                                     | ✅     |
| DNS resolve log server + cache TTL 6 jam           | ✅     |
| Retry sekali kalau connect log server gagal        | ✅     |
| Persist event saat jaringan down                   | 🔧     |

### 4.3 HTTP API (port 8000)

| Endpoint                             | Status | Auth   |
| ------------------------------------ | ------ | ------ |
| `GET  /info`                         | ✅     | —      |
| `GET  /students`                     | ✅     | —      |
| `GET  /students/{n}`                 | ✅     | —      |
| `POST /students`                     | ✅     | Bearer |
| `POST /students/{n}` (open)          | ✅     | Bearer |
| `DELETE /students/{n}`               | ✅     | Bearer |
| `POST /reset`                        | ✅     | Bearer |
| `POST /restart`                      | ✅     | Bearer |
| `GET  /sd-log?n=N` (read SD buffer)  | 🔧     | Bearer |

Detail format request/response: lihat **`README_handle_client.md`**.

### 4.4 Event Log → Server

| Hal                                                 | Status |
| --------------------------------------------------- | ------ |
| POST JSON ke `LOG_SERVER_HOST/event-log`            | ✅     |
| Bearer token dari `include/sec_tkn.h`               | ✅     |
| Drop event diam-diam kalau jaringan down            | 🟡 by-design, perlu queue |
| Status HTTP response tidak diparsing (best-effort)  | 🟡     |
| Buffer offline di SD card                           | 🔧     |
| Re-upload otomatis setelah jaringan balik           | 🔧     |

---

## 5. Roadmap (TODO files)

Dua file plan terpisah supaya gampang dikerjakan paralel:

### 5.1 `todo.txt` — OTA Firmware Update

Goal: bisa update firmware tanpa fisik bongkar setiap unit. Mega 2560 stock pakai bootloader STK500v2 yang **tidak** support OTA. Fase:

- **Fase 0** Pilih mekanisme (Ariadne TFTP / SD-bootloader / HTTP→SD).
- **Fase 1** Burn bootloader baru via ISP, set fuse `BOOTSZ`+`BOOTRST`.
- **Fase 2** Tambah modul `ota_trigger.{h,cpp}`, tulis magic byte EEPROM + `wdt_enable` reboot ke bootloader.
- **Fase 3** Pipeline build & deploy (PIO post-build → dist/, server TFTP).
- **Fase 4** Tes happy-path + power-loss + RAM/flash budget.
- **Fase 5** Burn bootloader ke seluruh unit lapangan, SOP rilis per-batch.

### 5.2 `sd_log.txt` — Event Log SD Card (fallback offline)

Goal: event tap RFID tidak hilang ketika log server / Ethernet down. Strategi:

- **Storage**: 1 file aktif `/log/event.log` + rotate ke `event.old` saat capai 32 KB (≈ 700 event).
- **Format**: CSV `<epoch>,<idx>,<uid>,<dev_class>,<flag>` (~40-50 byte/baris).
- **Modul**: `sd_log.{h,cpp}` — `begin()` / `append()` / `tail(out, n=10)` / `replay_pending()`.
- **Hook**: dipasang sebelum `send_eventLog()` (flag `PEND`), update jadi `SENT` setelah POST sukses.
- **Endpoint baru**: `GET /sd-log?n=10` streaming baris terakhir (hemat RAM).
- **Sinkron OTA**: share `SD.begin()` kalau OTA-via-SD dipilih (todo.txt fase 0 opsi B/C).

Fase 1+2 sudah cukup buat goal utama (rekam + tampilin); fase 3 (replay otomatis) opsional.

---

## 6. Konfigurasi & Build

### Build

```bash
pio run -e megaatmega2560        # compile
pio run -e megaatmega2560 -t upload   # flash via USB
```

Atau pakai `push.bat` (script convenience).

### Library deps (lihat `platformio.ini`)

- `adafruit/SdFat - Adafruit Fork@^2.3.53`  *(siap pakai untuk SD log + OTA)*
- `arduino-libraries/Ethernet@^2.0.0`
- `bblanchon/ArduinoJson@^7.4.2`
- `adafruit/Adafruit PN532@^1.3.4`
- `paulstoffregen/Time@^1.6.1`

### Flag konfigurasi (`include/commond.h`)

- **Pilih SATU model** dengan `#define modelL0xxx` (varian `L0002`/`L0004`/`L0016`/`L0032`/`L0064`/`L0128`/`L0256`/`L0512`). Compile-time error kalau salah pilih.
- **Debug toggles** (default semua off): `DEBUG_ETH`, `DEBUG_RFID`, `DEBUG_MEM`, `DEBUG_ACC`, `DEBUG_TIME`, `DEBUG_SYS`. Aktifkan satu-satu sesuai modul yang lagi dibikin.
- **Log server**: `LOG_SERVER_HOST`, `LOG_SERVER_PORT`, `LOG_SERVER_DNS_TTL_MS`. Pindah server cukup update DNS A record, tidak perlu reflash.

### Token

Copy `include/sec_tkn_example.h` → `include/sec_tkn.h`, isi `token_admin` (untuk HTTP write) dan `token_sck_log` (Bearer ke log server). File asli **tidak di-commit**.

---

## 7. Resource Budget

| Resource           | Pemakaian saat ini    | Sisa          |
| ------------------ | --------------------- | ------------- |
| SRAM               | ~5233 / 8192 B (~64%) | ~3 KB         |
| Flash app          | ~52 / 248 KB          | ~196 KB       |
| EEPROM             | dipakai DB kartu      | aman          |

**Catatan headroom:**

- Hindari `String` baru di modul tambahan — pakai `char[]` + `snprintf`.
- Target post-OTA + post-SD-log: SRAM tetap < 80% (= < 6553 B), flash tetap < 90%.
- SdFat full ~15-20 KB flash, ~500 B static RAM — masih masuk budget.

---

## 8. Known Issues & Catatan Operasional

- **Event drop saat network down** — by-design sekarang. Akan diperbaiki via SD log buffer (`sd_log.txt`).
- **Status HTTP log server tidak diparsing** — selama TCP write sukses, controller anggap event delivered. Server harus di-monitor terpisah.
- **W5100 link status kadang `Unknown`** — sudah ditangani di `ethernetCableConnected()`, jangan trigger logic OTA/SD ketika kondisi ini.
- **Stock STK500v2 bootloader** — OTA belum mungkin sampai Ariadne / varian SD-bootloader di-burn. Akses fisik per-unit dibutuhkan **sekali** untuk migrasi.
- **Mapping pin relay per-model** — kalau bikin batch baru dengan layout PCB beda, tambah varian `modelL0xxx` di `commond.h` dengan urutan `pin_IO[]` baru, **jangan modifikasi yang lama** (unit lapangan masih pakai).
- **DNS wajib valid** — kalau pakai static IP, panggil `Ethernet.begin(mac, ip, dns, gw)` lengkap. Tanpa DNS, log server tidak akan ke-resolve.

---

## 9. Referensi Dokumen

| File                         | Isi                                              |
| ---------------------------- | ------------------------------------------------ |
| `LAPORAN_PROJECT.md`         | (dokumen ini) — ringkasan + status + roadmap     |
| `README_handle_client.md`    | spesifikasi lengkap HTTP API controller          |
| `todo.txt`                   | plan eksekusi OTA firmware update                |
| `sd_log.txt`                 | plan eksekusi event log SD card buffer           |
| `include/commond.h`          | source-of-truth untuk pin/model/struct           |
| `include/sec_tkn_example.h`  | template token (copy → `sec_tkn.h`)              |
