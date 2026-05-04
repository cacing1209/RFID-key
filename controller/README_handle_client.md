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

Controller secara otomatis mengirim log event ke server eksternal ketika ada akses locker (RFID terdeteksi). Ini dilakukan melalui fungsi `send_eventLog` yang dipanggil dari `loop()`.

### Request ke Server Log

- **Method**: POST
- **URL**: http://<server_log>:<port>/event-log
- **Headers**:
  - `Content-Type: application/json`
  - `Authorization: Bearer lockerqyubitL0002L0004L0008L000264L000128`
  - `Connection: close`

### Payload JSON

```json
{
  "t": "2023-05-04 12:34:56",
  "no": 0,
  "id": 28758077
}
```

- `t`: timestamp dalam format YYYY-MM-DD HH:MM:SS
- `no`: nomor locker (index)
- `id`: UID kartuformat desimal (unsigned long)

### Contoh curl untuk Event Log

```bash
curl -v -X POST http://<server_log>:<port>/event-log \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer lockerqyubitL0002L0004L0008L000264L000128" \
  -d '{"t":"2023-05-04 12:34:56","no":0,"id":28758077}'
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
