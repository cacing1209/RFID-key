#!/usr/bin/env python3
"""
Locker Controller Scanner v2
Support: local subnet, public IP, custom port

Jalankan:
  python3 scan_controller.py                         # auto scan local
  python3 scan_controller.py --ip 93.144.178.16      # scan public IP
  python3 scan_controller.py --subnet 192.168.1      # scan subnet manual
  python3 scan_controller.py --ip 93.144.178.16 --port 8000
"""

import socket
import requests
import argparse
import threading
import time
import sys
import json
from concurrent.futures import ThreadPoolExecutor, as_completed

# ─────────────────────────────────────────────
GREEN  = "\033[92m"
RED    = "\033[91m"
YELLOW = "\033[93m"
BLUE   = "\033[94m"
CYAN   = "\033[96m"
BOLD   = "\033[1m"
RESET  = "\033[0m"

TIMEOUT       = 2.0
DEFAULT_PORT  = 8000        # ← port dari EthernetServer(8000)
ENDPOINT      = "/info"

found_controllers = []
lock     = threading.Lock()
scanned  = 0
total_hosts = 0

# ─────────────────────────────────────────────
def banner():
    print(f"""
{CYAN}{BOLD}╔══════════════════════════════════════════════╗
║    Arduino Locker Controller Scanner v2     ║
║         Support Public IP + Port 8000       ║
╚══════════════════════════════════════════════╝{RESET}
""")

def progress_bar(current, total, width=40):
    pct  = current / total if total else 0
    fill = int(width * pct)
    bar  = "█" * fill + "░" * (width - fill)
    sys.stdout.write(f"\r  [{bar}] {current}/{total} ({pct*100:.1f}%)")
    sys.stdout.flush()

def get_local_info():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        local_ip = s.getsockname()[0]
        s.close()
        subnet = local_ip.rsplit(".", 1)[0]
        return local_ip, subnet
    except:
        return "unknown", "192.168.1"

# ── Model map dari struct kamu ──
MAC_MODEL_MAP = {
    "02:A1:01:16:3D:5C": "J508",
    "02:A1:01:16:3D:5D": "J506",
    "02:A1:01:16:3D:5E": "J504",
    "02:A1:01:16:3D:5F": "J505",
    "02:A1:01:16:3D:60": "J503",
    "02:A1:01:16:3D:61": "J501",
    "02:A1:01:16:3D:62": "J507",
    "02:A1:01:16:3D:63": "J502",
}

def identify_model(mac: str) -> str:
    return MAC_MODEL_MAP.get(mac.upper(), "Unknown Model")

def check_host(ip: str, port: int) -> dict | None:
    global scanned
    url = f"http://{ip}:{port}{ENDPOINT}"
    try:
        r = requests.get(url, timeout=TIMEOUT)
        if r.status_code == 200:
            try:
                j = r.json()
                if any(k in j for k in ["dev_class", "total_locker", "c_name", "mac_addr"]):
                    mac   = j.get("mac_addr", "?")
                    model = identify_model(mac)
                    return {
                        "ip"           : ip,
                        "port"         : port,
                        "url"          : f"http://{ip}:{port}",
                        "status"       : j.get("status", "?"),
                        "dev_class"    : j.get("dev_class", "?"),
                        "c_name"       : j.get("c_name", model),
                        "location"     : j.get("location", "?"),
                        "version"      : j.get("ver", "?"),
                        "uptime_s"     : j.get("up_t", 0),
                        "mac"          : mac,
                        "model"        : model,
                        "total_locker" : j.get("total_locker", "?"),
                        "avail_locker" : j.get("avail_lock", "?"),
                        "raw"          : j,
                    }
            except:
                pass
    except:
        pass
    finally:
        with lock:
            scanned += 1
    return None

def scan_subnet(subnet: str, port: int, workers: int = 150):
    global total_hosts, scanned
    scanned = 0
    hosts = [f"{subnet}.{i}" for i in range(1, 255)]
    total_hosts = len(hosts)

    print(f"  {BLUE}Scanning {subnet}.1 – {subnet}.254 port {port} ({total_hosts} hosts)...{RESET}\n")

    with ThreadPoolExecutor(max_workers=workers) as executor:
        futures = {executor.submit(check_host, ip, port): ip for ip in hosts}
        for future in as_completed(futures):
            progress_bar(scanned, total_hosts)
            result = future.result()
            if result:
                with lock:
                    found_controllers.append(result)

    print()
    return found_controllers

def scan_single(ip: str, ports: list):
    """Scan satu IP dengan beberapa port kandidat."""
    global total_hosts, scanned
    total_hosts = len(ports)
    scanned = 0

    print(f"  {BLUE}Scanning {ip} port {ports} ...{RESET}\n")
    for port in ports:
        print(f"  Mencoba {ip}:{port} ...", end=" ", flush=True)
        result = check_host(ip, port)
        if result:
            print(f"{GREEN}FOUND!{RESET}")
            found_controllers.append(result)
        else:
            print(f"{RED}tidak ada respons{RESET}")
    return found_controllers

def format_uptime(seconds):
    if not isinstance(seconds, (int, float)):
        return "?"
    h = int(seconds) // 3600
    m = (int(seconds) % 3600) // 60
    s = int(seconds) % 60
    if h > 0:   return f"{h}j {m}m {s}d"
    elif m > 0: return f"{m}m {s}d"
    else:       return f"{s}d"

def print_controller(c: dict, idx: int):
    avail = c['avail_locker']
    total = c['total_locker']
    used  = (int(total) - int(avail)) if str(total).isdigit() and str(avail).isdigit() else 0
    bar_total = int(total) if str(total).isdigit() else 10
    bar_used  = int(used)  if str(used).isdigit()  else 0
    bar_free  = bar_total - bar_used

    print(f"""
  {GREEN}{BOLD}[{idx}] Controller Ditemukan!{RESET}
  ┌──────────────────────────────────────────────┐
  │  IP        : {CYAN}{c['ip']}{RESET}
  │  Port      : {CYAN}{c['port']}{RESET}
  │  URL       : {CYAN}{c['url']}{RESET}
  │  Model     : {BOLD}{c['model']}{RESET}
  │  C-Name    : {c['c_name']}
  │  Dev Class : {c['dev_class']}
  │  Lokasi    : {c['location']}
  │  Version   : {c['version']}
  │  MAC       : {c['mac']}
  │  Uptime    : {format_uptime(c['uptime_s'])}
  │  Locker    : {RED}{'■' * bar_used}{RESET}{GREEN}{'□' * bar_free}{RESET}  {bar_used}/{bar_total}
  └──────────────────────────────────────────────┘""")

def deep_check(c: dict, api_key: str = "locker-secret-123"):
    base = c['url']
    print(f"\n  {YELLOW}--- Deep Check: {base} ---{RESET}")

    AUTH    = {"Authorization": f"Bearer {api_key}", "Content-Type": "application/json"}
    NO_AUTH = {"Content-Type": "application/json"}

    # (method, path, body, use_auth, expect_status)
    checks = [
        ("GET",    "/info",       None,                   False, 200),
        ("GET",    "/students",   None,                   False, 200),
        ("GET",    "/students/0", None,                   False, 200),
        ("POST",   "/students",   None,                   False, 401),  # no auth
        ("POST",   "/students",   {"no":99,"id":"TEST"},  True,  400),  # invalid
        ("DELETE", "/students/0", None,                   False, 401),  # no auth
        ("POST",   "/reset",      None,                   False, 401),  # no auth
        ("GET",    "/notfound",   None,                   False, 404),
    ]

    all_ok = True
    for method, path, body, auth, expect in checks:
        hdrs = AUTH if auth else NO_AUTH
        try:
            fn = getattr(requests, method.lower())
            r  = fn(f"{base}{path}", json=body, headers=hdrs, timeout=TIMEOUT)
            ok = r.status_code == expect
            color = GREEN if ok else RED
            mark  = "✓" if ok else "✗"
            print(f"  {color}{mark}{RESET} {method:<7} {path:<22} → {r.status_code} (expect {expect})")
            if not ok:
                all_ok = False
        except Exception as e:
            print(f"  {RED}✗{RESET} {method:<7} {path:<22} → ERROR: {e}")
            all_ok = False

    status = f"{GREEN}{BOLD}Semua OK!{RESET}" if all_ok else f"{YELLOW}Ada anomali — cek kode Arduino{RESET}"
    print(f"\n  {status}")

def save_result(controllers: list) -> str:
    filename = f"scan_result_{int(time.time())}.json"
    data = {
        "scan_time"   : time.strftime("%Y-%m-%d %H:%M:%S"),
        "total_found" : len(controllers),
        "controllers" : [{k: v for k, v in c.items() if k != "raw"} for c in controllers]
    }
    with open(filename, "w") as f:
        json.dump(data, f, indent=2)
    return filename

# ══════════════════════════════════════════════
def main():
    parser = argparse.ArgumentParser(description="Scan Arduino Locker Controller")
    parser.add_argument("--ip",      type=str, help="IP target (local/public), e.g. 93.144.178.16")
    parser.add_argument("--subnet",  type=str, help="Subnet target, e.g. 192.168.1")
    parser.add_argument("--port",    type=int, default=DEFAULT_PORT, help=f"Port HTTP (default: {DEFAULT_PORT})")
    parser.add_argument("--ports",   type=str, default="8000,80,8080,3000", help="Port kandidat saat --ip mode")
    parser.add_argument("--deep",    action="store_true", help="Deep check endpoint setelah ditemukan")
    parser.add_argument("--save",    action="store_true", help="Simpan hasil ke JSON")
    parser.add_argument("--key",     type=str, default="locker-secret-123", help="API Key")
    parser.add_argument("--workers", type=int, default=150, help="Thread workers (default: 150)")
    args = parser.parse_args()

    banner()

    local_ip, auto_subnet = get_local_info()
    print(f"  {BLUE}IP lokal      : {local_ip}{RESET}")

    start = time.time()

    if args.ip:
        port_list = [int(p.strip()) for p in args.ports.split(",")]
        if args.port not in port_list:
            port_list.insert(0, args.port)
        print(f"  {BLUE}Target IP     : {args.ip}{RESET}")
        print(f"  {BLUE}Port kandidat : {port_list}{RESET}\n")
        results = scan_single(args.ip, port_list)
    else:
        subnet = args.subnet or auto_subnet
        print(f"  {BLUE}Target subnet : {subnet}.0/24{RESET}")
        print(f"  {BLUE}Port          : {args.port}{RESET}")
        results = scan_subnet(subnet, args.port, args.workers)

    elapsed = time.time() - start
    print(f"\n  Selesai dalam {elapsed:.1f} detik")

    if not results:
        print(f"\n  {RED}Tidak ada controller ditemukan.{RESET}")
    else:
        print(f"\n  {GREEN}{BOLD}Ditemukan {len(results)} controller:{RESET}")
        for i, c in enumerate(results, 1):
            print_controller(c, i)
            if args.deep:
                deep_check(c, args.key)

        if args.save:
            fname = save_result(results)
            print(f"\n  {BLUE}Hasil disimpan: {fname}{RESET}")

        c = results[0]
        print(f"""
  {YELLOW}─── NICE ──────────────────────────────{RESET}
  Edit BASE_URL di test_locker.py:
    BASE_URL = "{c['url']}"
  RUN:
    python3 test_locker.py
  {YELLOW}──────────────────────────────────────────────────────{RESET}
""")

if __name__ == "__main__":
    main()
