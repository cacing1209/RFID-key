#!/bin/bash
# ═══════════════════════════════════════════════════
#   Arduino Locker System - CLI Tool
#   Target: src/asset/main_ethernet_.cpp
#   Usage:  ./locker.sh <command> [args]
# ═══════════════════════════════════════════════════

BASE_URL="http://192.168.0.208:8000"
CT="Content-Type: application/json"

# ── Load token dari include/sec_tkn.h ──────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TKN_FILE="${SCRIPT_DIR}/../include/sec_tkn.h"
API_KEY=""
if [[ -f "$TKN_FILE" ]]; then
  API_KEY=$(grep -E '^[[:space:]]*#define[[:space:]]+token_sck_client' "$TKN_FILE" \
              | sed -E 's/.*"([^"]+)".*/\1/')
fi
AUTH="Authorization: ${API_KEY}"

# ── Warna ───────────────────────────────────────────
G="\033[92m"; R="\033[91m"; Y="\033[93m"; B="\033[94m"; C="\033[96m"; BOLD="\033[1m"; RST="\033[0m"

ok()   { echo -e "${G}[OK]${RST} $*"; }
err()  { echo -e "${R}[ERR]${RST} $*"; }
info() { echo -e "${B}[INFO]${RST} $*"; }

# ── Pretty print JSON ───────────────────────────────
pretty() {
  if command -v python3 &>/dev/null; then
    python3 -m json.tool 2>/dev/null || cat
  else
    cat
  fi
}

# ── HTTP status helper (-w) ─────────────────────────
hr() { echo -e "${C}── HTTP %{http_code} ── time %{time_total}s ──${RST}\n"; }

# ═══════════════════════════════════════════════════
#   COMMANDS
# ═══════════════════════════════════════════════════

cmd_info() {
  info "GET /info"
  curl -s -w "\n${C}── HTTP %{http_code}${RST}\n" "${BASE_URL}/info" | pretty
}

cmd_set_class() {
  # POST /info  body: {"dev_class": "..."}
  local dev_class=$1
  if [[ -z "$dev_class" ]]; then
    err "Usage: $0 set-class <dev_class>"
    err "Contoh: $0 set-class XII-RPL-1"
    exit 1
  fi
  info "POST /info  dev_class=${dev_class}"
  curl -s -w "\n${C}── HTTP %{http_code}${RST}\n" -X POST "${BASE_URL}/info" \
    -H "${CT}" -H "${AUTH}" \
    -d "{\"dev_class\": \"${dev_class}\"}" | pretty
}

cmd_register() {
  # POST /students  body: {"no": <int>, "id": "<decimal_uid>"}
  local locker=$1 uid=$2
  if [[ -z "$locker" || -z "$uid" ]]; then
    err "Usage: $0 register <locker_no> <card_uid>"
    err "Contoh: $0 register 0 0028758077"
    exit 1
  fi
  info "POST /students  locker=${locker} uid=${uid}"
  curl -s -w "\n${C}── HTTP %{http_code}${RST}\n" -X POST "${BASE_URL}/students" \
    -H "${CT}" -H "${AUTH}" \
    -d "{\"no\": ${locker}, \"id\": \"${uid}\"}" | pretty
}

cmd_open() {
  # POST /students/<n>  (buka locker)
  local locker=$1
  if [[ -z "$locker" ]]; then
    err "Usage: $0 open <locker_no>"
    exit 1
  fi
  info "POST /students/${locker}  (open)"
  curl -s -w "\n${C}── HTTP %{http_code}${RST}\n" -X POST "${BASE_URL}/students/${locker}" \
    -H "${CT}" -H "${AUTH}" | pretty
}

cmd_delete() {
  # DELETE /students/<n>
  local locker=$1
  if [[ -z "$locker" ]]; then
    err "Usage: $0 delete <locker_no>"
    exit 1
  fi
  info "DELETE /students/${locker}"
  curl -s -w "\n${C}── HTTP %{http_code}${RST}\n" -X DELETE "${BASE_URL}/students/${locker}" \
    -H "${CT}" -H "${AUTH}" | pretty
}

cmd_get() {
  # GET /students  atau  GET /students/<n>
  local locker=$1
  if [[ -z "$locker" ]]; then
    info "GET /students"
    curl -s -w "\n${C}── HTTP %{http_code}${RST}\n" "${BASE_URL}/students" | pretty
  else
    info "GET /students/${locker}"
    curl -s -w "\n${C}── HTTP %{http_code}${RST}\n" "${BASE_URL}/students/${locker}" | pretty
  fi
}

cmd_reset() {
  echo -e "${Y}⚠ Reset SEMUA locker? (y/N)${RST}"
  read -r confirm
  if [[ "$confirm" == "y" || "$confirm" == "Y" ]]; then
    info "POST /reset"
    curl -s -w "\n${C}── HTTP %{http_code}${RST}\n" -X POST "${BASE_URL}/reset" \
      -H "${CT}" -H "${AUTH}" | pretty
  else
    info "Dibatalkan."
  fi
}

cmd_restart() {
  echo -e "${Y}⚠ Restart Arduino? (y/N)${RST}"
  read -r confirm
  if [[ "$confirm" == "y" || "$confirm" == "Y" ]]; then
    info "POST /restart"
    curl -s -w "\n${C}── HTTP %{http_code}${RST}\n" -X POST "${BASE_URL}/restart" \
      -H "${CT}" -H "${AUTH}" | pretty
  else
    info "Dibatalkan."
  fi
}

cmd_status() {
  info "Status semua locker"
  curl -s "${BASE_URL}/students" | \
    python3 -c "
import sys, json
data = json.load(sys.stdin)
students = data.get('students', [])
print(f'  Total terpakai: {len(students)}')
for s in students:
    print(f'  Locker #{s[\"locker\"]:>2} → {s[\"card_uid\"]} [{s[\"status\"]}]')
" 2>/dev/null || curl -s "${BASE_URL}/students" | pretty
}

# ── Test auth: POST/DELETE tanpa Authorization → 401
cmd_test_noauth() {
  info "Test POST /students tanpa auth (harap 401)"
  curl -s -w "\n${C}── HTTP %{http_code}${RST}\n" -X POST "${BASE_URL}/students" \
    -H "${CT}" -d '{"no":0,"id":"0000000001"}' | pretty
  info "Test DELETE /students/0 tanpa auth (harap 401)"
  curl -s -w "\n${C}── HTTP %{http_code}${RST}\n" -X DELETE "${BASE_URL}/students/0" \
    -H "${CT}" | pretty
  info "Test POST /reset tanpa auth (harap 401)"
  curl -s -w "\n${C}── HTTP %{http_code}${RST}\n" -X POST "${BASE_URL}/reset" \
    -H "${CT}" | pretty
}

cmd_token() {
  if [[ -z "$API_KEY" ]]; then
    err "Token tidak terbaca dari ${TKN_FILE}"
    exit 1
  fi
  info "Token loaded dari: ${TKN_FILE}"
  echo "  ${API_KEY}"
}

# ═══════════════════════════════════════════════════
#   HELP
# ═══════════════════════════════════════════════════
usage() {
  echo -e "
${BOLD}${C}Arduino Locker CLI${RST}  ${B}→${RST} ${BASE_URL}
${B}Token src${RST}: ${TKN_FILE}
${B}Token   ${RST}: ${API_KEY:-${R}<tidak ditemukan>${RST}}

${BOLD}Usage:${RST}
  ./main_command.sh <command> [args]

${BOLD}Commands:${RST}
  ${G}info${RST}                        GET  /info        (no auth)
  ${G}set-class${RST} <dev_class>       POST /info        (auth)
  ${G}status${RST}                      GET  /students    (parsed)
  ${G}get${RST}                         GET  /students    (no auth)
  ${G}get${RST}     <locker_no>         GET  /students/n  (no auth)
  ${G}register${RST} <locker_no> <uid>  POST /students    (auth)
  ${G}open${RST}    <locker_no>         POST /students/n  (auth)
  ${G}delete${RST}  <locker_no>         DELETE /students/n (auth)
  ${G}reset${RST}                       POST /reset       (auth, konfirmasi)
  ${G}restart${RST}                     POST /restart     (auth, konfirmasi)
  ${G}test-noauth${RST}                 POST/DELETE tanpa auth → harap 401
  ${G}token${RST}                       Tampilkan token yang dipakai

${BOLD}Contoh:${RST}
  ./main_command.sh info
  ./main_command.sh set-class XII-RPL-1
  ./main_command.sh register 0 0028758077
  ./main_command.sh open 0
  ./main_command.sh get 0
  ./main_command.sh delete 0
  ./main_command.sh status
  ./main_command.sh reset
  ./main_command.sh test-noauth
"
}

# ═══════════════════════════════════════════════════
#   MAIN
# ═══════════════════════════════════════════════════
case "$1" in
  info)        cmd_info ;;
  set-class)   cmd_set_class "$2" ;;
  register)    cmd_register "$2" "$3" ;;
  open)        cmd_open "$2" ;;
  delete)      cmd_delete "$2" ;;
  get)         cmd_get "$2" ;;
  reset)       cmd_reset ;;
  restart)     cmd_restart ;;
  status)      cmd_status ;;
  test-noauth) cmd_test_noauth ;;
  token)       cmd_token ;;
  *)           usage ;;
esac
