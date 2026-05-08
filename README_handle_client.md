# `handle_client` HTTP Documentation

Dokumentasi ini menjelaskan bagaimana fungsi `handle_client` di `src/asset/main_ethernet_.cpp` memproses permintaan HTTP, route yang didukung, format JSON request/response, dan aturan otentikasi.

## Ringkasan

`handle_client` menerima koneksi `EthernetClient`, mem-parsing request HTTP, memeriksa otentikasi untuk beberapa metode, lalu meneruskan request ke handler spesifik berdasarkan path dan method.

### Metode HTTP yang didukung

- `GET /info`
- `GET /students`
- `GET /students/{locker_num}`
- `POST /students`
- `POST /students/{locker_num}`
- `DELETE /students/{locker_num}`
- `POST /reset`
- `POST /restart`

> `POST` dan `DELETE` membutuhkan header otorisasi.

## Otentikasi

Untuk request `POST` dan `DELETE`, `handle_client` memeriksa header `Authorization`. Header harus berisi token bearer yang valid.

Contoh header:

```http
Authorization: Bearer <token>
```

Token disamakan dengan nilai internal `auth.token` di `src/asset/auth.cpp`.

Jika token tidak valid, server mengembalikan:

- `401 Unauthorized`

## Format respons umum

Semua respons sukses menggunakan header:

- `HTTP/1.1 200 OK`
- `Content-Type: application/json`
- `Connection: close`

Respons error menggunakan JSON:

```json
{
  "error": "Pesan error",
  "code": 400
}
```

## Endpoints

### GET /info

Mengembalikan informasi status controller.

Response body:

```json
{
  "status": "ok",
  "dev_class": "<device class>",
  "c_name": "<controller name>",
  "location": "<location>",
  "ver": "1.0.0",
  "up_t": 123,
  "ip_a": "192.168.0.8",
  "mac_addr": "AA:BB:CC:DD:EE:FF",
  "total_locker": 32,
  "avail_lock": 20
}
```

### GET /students

Mengembalikan daftar siswa/locker yang sedang digunakan.

Response body:

```json
{
  "students": [
    {
      "locker": 0,
      "card_uid": "0028758077",
      "status": "use"
    },
    {
      "locker": 1,
      "card_uid": "0012345678",
      "status": "use"
    }
  ]
}
```

Hanya locker dengan `Status_db::Not_Available` yang dikirim.

### GET /students/{locker_num}

Mendapatkan status satu locker.

Response sukses jika locker valid:

Jika locker digunakan:

```json
{
  "locker": 5,
  "status": "use",
  "card_uid": "0028758077"
}
```

Jika locker kosong:

```json
{
  "locker": 5,
  "status": "available"
}
```

Jika `locker_num` tidak valid, server mengembalikan `400 Invalid locker number`.

### POST /students

Menambahkan data siswa ke locker.

Request body JSON:

```json
{
  "no": 7,
  "id": "0028758077"
}
```

- `no`: nomor locker (integer)
- `id`: UID kartu dalam format desimal string 10 digit

Syarat:
- `body_len` tidak boleh kosong
- JSON harus valid
- field `no` dan `id` harus ada
- locker harus berada di rentang `0..Size_Siswa-1`
- `id` hanya boleh berisi angka desimal
- UID tidak boleh duplikat dengan locker lain

Response sukses:

```json
{
  "status": "created",
  "locker": 7,
  "card_uid": "0028758077"
}
```

Error yang mungkin:
- `400 Empty body`
- `400 Invalid JSON`
- `400 Missing required fields`
- `400 Invalid locker number`
- `400 Invalid student data`
- `400 Invalid format. Only decimal numbers accepted (e.g., 0028758077)`
- `409 Locker already in use`
- `409 duplicated uid with locker numX`
- `500 Failed to save data`

### POST /students/{locker_num}

Membuka locker yang ditentukan tanpa body JSON.

Response sukses:

```json
{
  "status": "opened",
  "locker": 7
}
```

Jika sistem locker tidak tersedia, server mengembalikan `500 Locker system not available`.

### DELETE /students/{locker_num}

Menghapus data siswa dari locker tertentu.

Response sukses:

```json
{
  "status": "deleted",
  "locker": 7
}
```

Error yang mungkin:
- `500 Database not available`
- `404 Locker not use`
- `500 Failed to save data`

### POST /reset

Mengosongkan semua locker dan menyimpan status baru.

Response sukses:

```json
{
  "status": "reset",
  "message": "All lockers cleared"
}
```

Error yang mungkin:
- `500 Database not available`
- `500 Failed to reset data`

### POST /restart

Memanggil `sys.software_Resatrt()` untuk merestart perangkat lunak.

## Event Logging

Controller secara otomatis mengirim log event ke server eksternal setiap kali ada akses locker (RFID terdeteksi). Loop di `main_ethernet_.cpp` memeriksa flag `send_log[i]`; jika set, fungsi `send_eventLog(uid_decimal, index)` dipanggil dan flag direset.

### Konfigurasi server log

Server log **tidak** lagi di-hardcode IP-nya — alamat di-resolve runtime via DNS. Konstanta di `include/commond.h`:

| Konstanta | Nilai | Keterangan |
|---|---|---|
| `LOG_SERVER_HOST` | `locker-logs.qyubit.io` | hostname target log server |
| `LOG_SERVER_PORT` | `80` | TCP port |
| `LOG_SERVER_DNS_TTL_MS` | `6 * 60 * 60 * 1000` (6 jam) | umur cache hasil DNS sebelum re-resolve |

> Untuk pindah server / ganti IP, **cukup update DNS A record** dari `LOG_SERVER_HOST`. Tidak perlu reflash firmware.
>
> Syarat: konfigurasi Ethernet harus punya gateway + DNS server yang valid. Jika pakai static IP, gunakan `Ethernet.begin(mac, ip, dns, gw)` — DNS tidak boleh kosong.

### Workflow `send_eventLog`

1. **Cek cache DNS** — jika `log_server_ip` belum valid atau umur cache > `LOG_SERVER_DNS_TTL_MS`, panggil `resolve_log_server()` (UDP DNS query via `dns.getHostByName(LOG_SERVER_HOST, ...)`).
2. **Connect TCP** ke `log_server_ip:LOG_SERVER_PORT`.
3. **Retry sekali** jika connect gagal: invalidate cache → re-resolve → connect ulang. Masih gagal → fungsi `return` diam (event di-drop, tidak ada queue persist).
4. **Build JSON** ke `StaticJsonDocument<256>` dan **POST** `/event-log`.
5. **Drain response** sampai idle 3 detik, lalu `logClient.stop()`. Status HTTP **tidak diparsing** — selama TCP write sukses, fungsi anggap selesai (best-effort).

### Request ke Server Log

- **Method**: `POST`
- **URL**: `http://<LOG_SERVER_HOST>:<LOG_SERVER_PORT>/event-log`
- **Headers**:
  - `Host: <LOG_SERVER_HOST>`
  - `Content-Type: application/json`
  - `Authorization: Bearer <token_sck_log>` — token dari macro `token_sck_log` di `include/sec_tkn.h` (lihat `sec_tkn_example.h` sebagai template)
  - `Connection: close`
  - `Content-Length: <len>`

### Payload JSON

```json
{
  "t": "2023-05-04 12:34:56",
  "no": 0,
  "id": 28758077,
  "dev_class": "<device class>"
}
```

- `t`: timestamp dari `system_t()` (format `YYYY-MM-DD HH:MM:SS`)
- `no`: nomor locker / index `send_log[i]`
- `id`: UID kartu dalam desimal (`unsigned long`, hasil pack `db[i].card[0..3]` little-endian)
- `dev_class`: kelas perangkat yang melapor

### Failure modes

| Kondisi | Akibat |
|---|---|
| DNS resolver mati / hostname tidak resolve | `resolve_log_server()` gagal, event di-drop diam |
| TCP connect gagal 2x (setelah re-resolve) | event di-drop diam |
| Server balas non-2xx | tetap dianggap sukses (response tidak diparsing) |
| Timeout drain >3 s | koneksi ditutup paksa via `stop()` |

Tidak ada retry queue / persistence — kalau jaringan down, event yang terlewat **hilang**.

### Contoh curl untuk Event Log

```bash
curl -v -X POST http://locker-logs.qyubit.io/event-log \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <token_sck_log>" \
  -d '{"t":"2023-05-04 12:34:56","no":0,"id":28758077,"dev_class":"locker-A"}'
```

## Contoh `curl`


### GET /info

```bash
curl -v http://<controller-ip>/info
```

### GET /students

```bash
curl -v http://<controller-ip>/students
```

### GET /students/{locker_num}

```bash
curl -v http://<controller-ip>/students/7
```

### POST /students

```bash
curl -v -X POST http://<controller-ip>/students \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"no":7,"id":"0028758077"}'
```

### POST /students/{locker_num}

```bash
curl -v -X POST http://<controller-ip>/students/7 \
  -H "Authorization: Bearer <token>"
```

### DELETE /students/{locker_num}

```bash
curl -v -X DELETE http://<controller-ip>/students/7 \
  -H "Authorization: Bearer <token>"
```

### POST /reset

```bash
curl -v -X POST http://<controller-ip>/reset \
  -H "Authorization: Bearer <token>"
```

### POST /restart

```bash
curl -v -X POST http://<controller-ip>/restart \
  -H "Authorization: Bearer <token>"
```

## Ringkas

`handle_client` bertugas:

- parse request HTTP
- baca header `Authorization` jika ada
- verifikasi auth untuk `POST` dan `DELETE`
- pilih handler berdasarkan `path` dan `method`
- kirim `404 Not Found` jika route tidak cocok
- kirim JSON error di semua kondisi gagal

Gunakan dokumentasi ini sebagai referensi untuk membangun client HTTP yang cocok dengan `handle_client`.
