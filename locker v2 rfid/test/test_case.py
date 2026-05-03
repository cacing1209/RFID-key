#!/usr/bin/env python3

import requests
import json
import time

# BASE_URL = "http://93.144.178.18:8000"
BASE_URL = "http://192.168.0.100:8000"
API_KEY = "lockerqyubitL0002L0004L0008L000264L000128"
AUTH = API_KEY
HEADERS = {"Content-Type": "application/json", "Authorization": AUTH}
TIMEOUT = 5

# ─────────────────────────────────────────────
PASS = "\033[92m[PASS]\033[0m"
FAIL = "\033[91m[FAIL]\033[0m"
WARN = "\033[93m[WARN]\033[0m"
INFO = "\033[94m[INFO]\033[0m"

total = 0
passed = 0
bugs = []


def check(name, condition, detail="", is_bug=False):
    global total, passed
    total += 1
    if condition:
        passed += 1
        print(f"  {PASS} {name}")
    else:
        print(f"  {FAIL} {name}")
        if detail:
            print(f"         → {detail}")
        if is_bug:
            bugs.append(f"{name}: {detail}")


def section(title):
    print(f"\n{'='*55}")
    print(f"  {title}")
    print(f"{'='*55}")


def req(method, path, body=None, auth=True, timeout=TIMEOUT):
    hdrs = dict(HEADERS) if auth else {"Content-Type": "application/json"}
    try:
        r = getattr(requests, method)(
            BASE_URL + path, json=body, headers=hdrs, timeout=timeout
        )
        return r
    except requests.exceptions.ConnectionError:
        return None
    except requests.exceptions.Timeout:
        return None


# ══════════════════════════════════════════════
section("0 · KONEKSI")
# ══════════════════════════════════════════════
r = req("get", "/info", auth=False)
if r is None:
    print(
        f"  {FAIL} Tidak bisa koneksi ke {BASE_URL}. Jalankan ulang setelah Arduino online."
    )
    exit(1)
check("GET /info merespons", r is not None)
check("Status 200", r.status_code == 200, f"dapat {r.status_code}", is_bug=True)

try:
    info = r.json()
    print(f"  {INFO} IP     : {info.get('ip_a','?')}")
    print(f"  {INFO} C-Name : {info.get('c_name','?')}")
    print(f"  {INFO} Locker : {info.get('total_locker','?')}")
    TOTAL_LOCKER = int(info.get("total_locker", 5))
except:
    TOTAL_LOCKER = 5

# ══════════════════════════════════════════════
section("1 · GET /info  (no auth)")
# ══════════════════════════════════════════════
r = req("get", "/info", auth=False)
check("Status 200", r.status_code == 200)
j = r.json()
for field in [
    "status",
    "dev_class",
    "c_name",
    "location",
    "ver",
    "up_t",
    "ip_a",
    "mac_addr",
    "total_locker",
    "avail_lock",
]:
    check(f"  field '{field}' ada", field in j, f"field hilang", is_bug=True)

# ══════════════════════════════════════════════
section("2 · AUTH")
# ══════════════════════════════════════════════
r = req("post", "/students", body={"no": 0, "id": "0000000001"}, auth=False)
check(
    "POST tanpa auth → 401",
    r is not None and r.status_code == 401,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

r = req("delete", "/students/0", auth=False)
check(
    "DELETE tanpa auth → 401",
    r is not None and r.status_code == 401,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

r = req("post", "/reset", auth=False)
check(
    "POST /reset tanpa auth → 401",
    r is not None and r.status_code == 401,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

# ══════════════════════════════════════════════
section("3 · RESET (bersihkan semua locker dulu)")
# ══════════════════════════════════════════════
r = req("post", "/reset")
check(
    "POST /reset → 200",
    r is not None and r.status_code == 200,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)
if r:
    j = r.json()
    check(
        "  response.status == 'reset'",
        j.get("status") == "reset",
        f"dapat '{j.get('status')}'",
        is_bug=True,
    )

# ══════════════════════════════════════════════
section("4 · REGISTRASI")
# ══════════════════════════════════════════════

# 4a. Registrasi normal locker 0
r = req("post", "/students", body={"no": 0, "id": "0028758077"})
check(
    "POST /students locker 0 → 200/201",
    r is not None and r.status_code == 200,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)
if r and r.status_code == 200:
    j = r.json()
    check(
        "  response.status == 'created'",
        j.get("status") == "created",
        f"dapat '{j.get('status')}'",
        is_bug=True,
    )
    check(
        "  response.locker == 0",
        j.get("locker") == 0,
        f"dapat {j.get('locker')}",
        is_bug=True,
    )
    check(
        "  card_uid ada di response",
        "card_uid" in j,
        "field card_uid hilang",
        is_bug=True,
    )

# 4b. Registrasi locker lain
r = req("post", "/students", body={"no": 1, "id": "0012345678"})
check(
    "POST /students locker 1 → 200",
    r is not None and r.status_code == 200,
    f"dapat {r.status_code if r else 'timeout'}",
)

# 4c. Duplicate — locker sudah terisi
r = req("post", "/students", body={"no": 0, "id": "0099999999"})
check(
    "POST /students locker 0 duplikat → 409",
    r is not None and r.status_code == 409,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

# 4d. Body kosong
r = req("post", "/students", body={})
check(
    "POST /students body kosong → 400",
    r is not None and r.status_code == 400,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

# 4e. Format card non-decimal
r = req("post", "/students", body={"no": 2, "id": "ABCDEF1234"})
check(
    "POST /students card hex → 400",
    r is not None and r.status_code == 400,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

# 4f. Nomor locker out of range
r = req("post", "/students", body={"no": 999, "id": "0028758077"})
check(
    "POST /students locker 999 (OOR) → 400",
    r is not None and r.status_code == 400,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

# 4g. ⚠️  BUG CANDIDATE — field 'no' negatif
r = req("post", "/students", body={"no": -1, "id": "0028758077"})
check(
    "POST /students locker -1 → 400  [⚠️ byte overflow!]",
    r is not None and r.status_code == 400,
    f"dapat {r.status_code if r else 'timeout'} — 'no' di-cast ke byte, -1 jadi 255!",
    is_bug=True,
)

# ══════════════════════════════════════════════
section("5 · GET /students  (semua)")
# ══════════════════════════════════════════════
r = req("get", "/students", auth=False)
check(
    "GET /students → 200",
    r is not None and r.status_code == 200,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)
if r and r.status_code == 200:
    j = r.json()
    check("  ada key 'students'", "students" in j, "key 'students' hilang", is_bug=True)
    if "students" in j:
        check(
            "  jumlah ≥ 2 (locker 0 & 1 terdaftar)",
            len(j["students"]) >= 2,
            f"dapat {len(j['students'])}",
            is_bug=True,
        )

# ══════════════════════════════════════════════
section("6 · GET /students/<id>")
# ══════════════════════════════════════════════
r = req("get", "/students/0", auth=False)
check(
    "GET /students/0 → 200",
    r is not None and r.status_code == 200,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)
if r and r.status_code == 200:
    j = r.json()
    check(
        "  status == 'use'",
        j.get("status") == "use",
        f"dapat '{j.get('status')}'",
        is_bug=True,
    )
    check("  card_uid ada", "card_uid" in j, "field card_uid hilang", is_bug=True)

r = req("get", "/students/2", auth=False)
check(
    "GET /students/2 (kosong) → status 'available'",
    r is not None and r.status_code == 200 and r.json().get("status") == "available",
    f"dapat status '{r.json().get('status') if r else '?'}'",
    is_bug=True,
)

r = req("get", "/students/999", auth=False)
check(
    "GET /students/999 (OOR) → 400",
    r is not None and r.status_code == 400,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

# ══════════════════════════════════════════════
section("7 · POST /students/<id>  (buka locker)")
# ══════════════════════════════════════════════
r = req("post", "/students/0")
check(
    "POST /students/0 → 200 (buka locker)",
    r is not None and r.status_code == 200,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)
if r and r.status_code == 200:
    j = r.json()
    check(
        "  status == 'opened'",
        j.get("status") == "opened",
        f"dapat '{j.get('status')}'",
        is_bug=True,
    )

# ══════════════════════════════════════════════
section("8 · DELETE /students/<id>")
# ══════════════════════════════════════════════
r = req("delete", "/students/0")
check(
    "DELETE /students/0 (terdaftar) → 200",
    r is not None and r.status_code == 200,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)
if r and r.status_code == 200:
    j = r.json()
    check(
        "  status == 'deleted'",
        j.get("status") == "deleted",
        f"dapat '{j.get('status')}'",
        is_bug=True,
    )

r = req("delete", "/students/0")
check(
    "DELETE /students/0 lagi (sudah kosong) → 404",
    r is not None and r.status_code == 404,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

r = req("delete", "/students/999")
check(
    "DELETE /students/999 (OOR) → 400",
    r is not None and r.status_code == 400,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

# ══════════════════════════════════════════════
section("9 · RESET PENUH")
# ══════════════════════════════════════════════
r = req("post", "/students", body={"no": 0, "id": "0028758077"})
r = req("post", "/students", body={"no": 2, "id": "0011111111"})

r = req("post", "/reset")
check(
    "POST /reset → 200 (ada data)",
    r is not None and r.status_code == 200,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

r = req("get", "/students", auth=False)
if r and r.status_code == 200:
    j = r.json()
    students_after = j.get("students", [])
    check(
        "  semua locker kosong setelah reset",
        len(students_after) == 0,
        f"masih ada {len(students_after)} locker terisi",
        is_bug=True,
    )

# ══════════════════════════════════════════════
section("10 · ROUTE TIDAK ADA & METHOD SALAH")
# ══════════════════════════════════════════════
r = req("get", "/notfound", auth=False)
check(
    "GET /notfound → 404",
    r is not None and r.status_code == 404,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

r = req("delete", "/info")
check(
    "DELETE /info → 404 atau 405",
    r is not None and r.status_code in (404, 405),
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

# ══════════════════════════════════════════════
section("11 · RESTART & UPTIME")
# ══════════════════════════════════════════════

# 11a. POST /restart tanpa auth → 401
r = req("post", "/restart", auth=False)
check(
    "POST /restart tanpa auth → 401",
    r is not None and r.status_code == 401,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

# 11b. uptime naik setiap detik
r1 = req("get", "/info", auth=False)
time.sleep(2)
r2 = req("get", "/info", auth=False)
if r1 and r2 and r1.status_code == 200 and r2.status_code == 200:
    up1 = r1.json().get("up_t", 0)
    up2 = r2.json().get("up_t", 0)
    check(
        "uptime bertambah setiap detik",
        up2 > up1,
        f"up_t tidak naik: {up1} → {up2}",
        is_bug=True,
    )

# 11c. POST /restart dengan auth → Arduino restart (koneksi drop adalah normal)
# print(f"\n  {WARN} Melewati POST /restart dengan auth — akan me-restart Arduino")
# print(       "         Jalankan manual jika mau test: POST /restart + Authorization")

# ══════════════════════════════════════════════
section("12 · EVENT LOG (send_eventLog)")
# ══════════════════════════════════════════════

# Setup: daftarkan locker dulu
req("post", "/reset")
time.sleep(0.3)
req("post", "/students", body={"no": 0, "id": "0028758077"})

r = req("post", "/students/0")
check(
    "POST /students/0 (buka locker) → 200 sebelum event log dikirim",
    r is not None and r.status_code == 200,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

time.sleep(2)  # beri waktu Arduino kirim event log

# Cek Arduino tidak hang setelah send_eventLog
r = req("get", "/info", auth=False)
check(
    "GET /info OK setelah send_eventLog (Arduino tidak hang)",
    r is not None and r.status_code == 200,
    f"Arduino tidak merespons — kemungkinan hang di send_eventLog() / EthernetClient conflict",
    is_bug=True,
)

# Buka locker kedua berturut-turut
req("post", "/students", body={"no": 1, "id": "0012345678"})
req("post", "/students/1")
time.sleep(2)
r = req("get", "/info", auth=False)
check(
    "GET /info OK setelah 2x event log (stress test)",
    r is not None and r.status_code == 200,
    f"Arduino tidak merespons setelah 2x event log",
    is_bug=True,
)

# ══════════════════════════════════════════════
section("13 · KONSISTENSI DATA")
# ══════════════════════════════════════════════

req("post", "/reset")
time.sleep(0.3)
req("post", "/students", body={"no": 0, "id": "0028758077"})
req("post", "/students", body={"no": 1, "id": "0012345678"})
req("post", "/students", body={"no": 2, "id": "0099999999"})
time.sleep(0.3)

# 13a. avail_lock di /info harus konsisten dengan /students
r_info = req("get", "/info", auth=False)
r_list = req("get", "/students", auth=False)
if r_info and r_list and r_info.status_code == 200 and r_list.status_code == 200:
    j_info = r_info.json()
    j_list = r_list.json()
    total_l = int(j_info.get("total_locker", 0))
    avail_l = int(j_info.get("avail_lock", 0))
    used_info = total_l - avail_l
    used_list = len(j_list.get("students", []))
    check(
        "avail_lock di /info konsisten dengan jumlah /students",
        used_info == used_list,
        f"/info bilang {used_info} terpakai, /students ada {used_list} entry",
        is_bug=True,
    )

# 13b. card_uid harus 10 digit
r = req("get", "/students/0", auth=False)
if r and r.status_code == 200:
    uid = r.json().get("card_uid", "")
    check(
        "card_uid 10 digit dengan leading zero",
        len(uid) == 10 and uid.isdigit(),
        f"dapat '{uid}' (len={len(uid)})",
        is_bug=True,
    )

# 13c. Daftar ulang setelah delete → harus OK
req("delete", "/students/0")
time.sleep(0.3)
r = req("post", "/students", body={"no": 0, "id": "0028758077"})
check(
    "Registrasi ulang locker setelah delete → 200",
    r is not None and r.status_code == 200,
    f"dapat {r.status_code if r else 'timeout'}",
    is_bug=True,
)

# 13d. UID sama di locker berbeda — cek policy duplikasi
r = req("post", "/students", body={"no": 3, "id": "0028758077"})
if r:
    if r.status_code == 409:
        print(f"  {PASS} Duplikat UID ditolak (bagus — policy ketat)")
        passed += 1
    else:
        print(
            f"  {WARN} Duplikat UID diterima di locker lain (status {r.status_code}) — cek policy"
        )
    total += 1

# 13e. Data persist setelah GET berulang
r1 = req("get", "/students/0", auth=False)
r2 = req("get", "/students/0", auth=False)
if r1 and r2 and r1.status_code == 200 and r2.status_code == 200:
    uid1 = r1.json().get("card_uid")
    uid2 = r2.json().get("card_uid")
    check(
        "Data konsisten antara 2x GET /students/0",
        uid1 == uid2,
        f"uid berbeda: {uid1} vs {uid2}",
        is_bug=True,
    )

req("post", "/reset")

# ══════════════════════════════════════════════
section("PASSED")
# ══════════════════════════════════════════════

# print(f"\n  {WARN} BUG #1 — handle_info() overload, body kosong tidak return setelah send_error")
# print(       "         send_error(400, 'empty body') dipanggil tapi tidak ada 'return'")
# print(       "         → eksekusi lanjut ke deserializeJson dengan body kosong")
# bugs.append("BUG #1: handle_info() body kosong tidak return setelah send_error → lanjut eksekusi")

# print(f"\n  {WARN} BUG #2 — deserializeJson error check terbalik")
# print(       "         if (!file) send_error  ← harusnya  if (file) send_error")
# print(       "         JSON invalid malah tidak dikirim error, JSON valid malah dikirim error")
# bugs.append("BUG #2: if (!file) seharusnya if (file) untuk handle JSON parse error")

# print(f"\n  {INFO} BUG #3 — send_eventLog EthernetClient [ACKNOWLEDGED]")
# print(       "         Solusi: buat EthernetClient logClient baru di dalam send_eventLog()")
# print(       "         Hapus parameter &client, gunakan socket terpisah")

# print(f"\n  {INFO} BUG #4 — byte locker overflow [FIXED ✓]")
# print(       "         Sudah difix: int locker_raw = doc['no'] + validasi range")

# print(f"\n  {WARN} BUG #5 — Ethernet.init() di tengah handle_info")
# print(       "         Ethernet.init() reset SPI shield saat sedang handle request aktif")
# print(       "         → bisa disconnect / response tidak terkirim")
# bugs.append("BUG #5: Ethernet.init() dipanggil di handle_info() saat request aktif — hapus baris ini")

# print(f"\n  {INFO} BUG #6 — ntpCfg.update() [NORMAL - ada di file lain ✓]")
# print(f"\n  {INFO} BUG #7 — controller_name model lain [SUDAH DEFINE DI FILE LAIN ✓]")

# Test live: c_name harus string valid
r = req("get", "/info", auth=False)
if r and r.status_code == 200:
    cname = r.json().get("c_name", "")
    is_valid = (
        cname and all(32 <= ord(c) < 127 for c in str(cname)) and len(str(cname)) < 20
    )
    check(
        "c_name valid (printable ASCII, <20 char)",
        is_valid,
        f"c_name = '{cname}'",
        is_bug=True,
    )

# ══════════════════════════════════════════════
section("RINGKASAN")
# ══════════════════════════════════════════════
print(f"\n  Total test  : {total}")
print(f"  Passed      : {passed}")
print(f"  Failed      : {total - passed}")
print(f"  Score       : {passed}/{total} ({100*passed//total}%)")

if bugs:
    print(f"\n  {WARN} BUG / ANOMALI DITEMUKAN ({len(bugs)}):")
    for i, b in enumerate(bugs, 1):
        print(f"    {i}. {b}")

print()
