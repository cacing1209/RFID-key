#!/bin/bash
# ═══════════════════════════════════════════════════
#   Arduino Locker System - CLI Tool
#   Usage: ./locker.sh <command> [args]
# ═══════════════════════════════════════════════════

BASE_URL="http://192.168.0.208:8000"
API_KEY="lockerqyubitL0002L0004L0008L000264L000128"
AUTH="Authorization: ${API_KEY}"
CT="Content-Type: application/json"

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

# ═══════════════════════════════════════════════════
#   COMMANDS
# ═══════════════════════════════════════════════════

cmd_info() {
  info "GET /info"
  curl -s "${BASE_URL}/info" | pretty
}

cmd_set_class() {
  # Usage: ./locker.sh set-class <dev_class>
  # Contoh: ./locker.sh set-class XII-RPL-1
  local dev_class=$1
  if [[ -z "$dev_class" ]]; then
    err "Usage: $0 set-class <dev_class>"
    err "Contoh: $0 set-class XII-RPL-1"
    exit 1
  fi
  info "Set dev_class = ${dev_class}"
  curl -s -X POST "${BASE_URL}/info" \
    -H "${CT}" -H "${AUTH}" \
    -d "{\"dev_class\": \"${dev_class}\"}" | pretty
}

cmd_register() {
  # Usage: ./locker.sh register <locker_no> <card_uid>
  # Contoh: ./locker.sh register 0 0028758077
  local locker=$1 uid=$2
  if [[ -z "$locker" || -z "$uid" ]]; then
    err "Usage: $0 register <locker_no> <card_uid>"
    err "Contoh: $0 register 0 0028758077"
    exit 1
  fi
  info "Registrasi locker #${locker} dengan card ${uid}"
  curl -s -X POST "${BASE_URL}/students" \
    -H "${CT}" -H "${AUTH}" \
    -d "{\"no\": ${locker}, \"id\": \"${uid}\"}" | pretty
}

cmd_open() {
  # Usage: ./locker.sh open <locker_no>
  local locker=$1
  if [[ -z "$locker" ]]; then
    err "Usage: $0 open <locker_no>"
    err "Contoh: $0 open 0"
    exit 1
  fi
  info "Buka locker #${locker}"
  curl -s -X POST "${BASE_URL}/students/${locker}" \
    -H "${CT}" -H "${AUTH}" | pretty
}

cmd_delete() {
  # Usage: ./locker.sh delete <locker_no>
  local locker=$1
  if [[ -z "$locker" ]]; then
    err "Usage: $0 delete <locker_no>"
    exit 1
  fi
  info "Hapus registrasi locker #${locker}"
  curl -s -X DELETE "${BASE_URL}/students/${locker}" \
    -H "${CT}" -H "${AUTH}" | pretty
}

cmd_get() {
  # Usage: ./locker.sh get [locker_no]
  local locker=$1
  if [[ -z "$locker" ]]; then
    info "GET semua locker"
    curl -s "${BASE_URL}/students" | pretty
  else
    info "GET locker #${locker}"
    curl -s "${BASE_URL}/students/${locker}" | pretty
  fi
}

cmd_reset() {
  echo -e "${Y}⚠ Yakin reset SEMUA locker? (y/N)${RST}"
  read -r confirm
  if [[ "$confirm" == "y" || "$confirm" == "Y" ]]; then
    info "Reset semua locker..."
    curl -s -X POST "${BASE_URL}/reset" \
      -H "${CT}" -H "${AUTH}" | pretty
  else
    info "Dibatalkan."
  fi
}

cmd_restart() {
  echo -e "${Y}⚠ Yakin restart Arduino? (y/N)${RST}"
  read -r confirm
  if [[ "$confirm" == "y" || "$confirm" == "Y" ]]; then
    info "Restart Arduino..."
    curl -s -X POST "${BASE_URL}/restart" \
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

# ═══════════════════════════════════════════════════
#   HELP
# ═══════════════════════════════════════════════════
usage() {
  echo -e "
${BOLD}${C}Arduino Locker CLI${RST}
${B}Target${RST}: ${BASE_URL}

${BOLD}Usage:${RST}
  ./locker.sh <command> [args]

${BOLD}Commands:${RST}
  ${G}info${RST}                        Info controller
  ${G}set-class${RST} <dev_class>       Set nama device_class (disimpan di EEPROM)
  ${G}status${RST}                      Lihat semua locker terpakai
  ${G}get${RST}                         Semua locker
  ${G}get${RST}     <locker_no>         Detail 1 locker
  ${G}register${RST} <locker_no> <uid>  Daftarkan kartu ke locker
  ${G}open${RST}    <locker_no>         Buka locker
  ${G}delete${RST}  <locker_no>         Hapus registrasi locker
  ${G}reset${RST}                       Reset semua locker (konfirmasi)
  ${G}restart${RST}                     Restart Arduino (konfirmasi)

${BOLD}Contoh:${RST}
  ./locker.sh info
  ./locker.sh set-class XII-RPL-1
  ./locker.sh register 0 0028758077
  ./locker.sh open 0
  ./locker.sh get 0
  ./locker.sh delete 0
  ./locker.sh status
  ./locker.sh reset
"
}

# ═══════════════════════════════════════════════════
#   MAIN
# ═══════════════════════════════════════════════════
case "$1" in
  info)      cmd_info ;;
  set-class) cmd_set_class "$2" ;;
  register)  cmd_register "$2" "$3" ;;
  open)      cmd_open "$2" ;;
  delete)    cmd_delete "$2" ;;
  get)       cmd_get "$2" ;;
  reset)     cmd_reset ;;
  restart)   cmd_restart ;;
  status)    cmd_status ;;
  *)         usage ;;
esac