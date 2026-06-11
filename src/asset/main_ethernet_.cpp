#include <commond.h>
#ifndef find_p
#include <Ethernet.h>
#include <Dns.h>
#include <ArduinoJson.h>
#include <ota_megaXethernetshield.h>
#include <sdf.h>
NTPConfig ntpCfg;

// Satu-satunya instance SdFat dipakai event-log SD (OTA lagi dimatikan, jadi
// gak ada double-init). card.begin() di-(re)run di ethernet_state::begin()
// SETELAH SPI.begin() di setup() — constructor jalan sebelum SPI siap.
sdf_state sd_card;
bool ethernetCableConnected()
{
    // W5100 reports Unknown — don't probe with a TCP connect to gateway:80.
    // That probe burns a hardware socket each call (only 4 total on W5100) and
    // blocks ~1s per failure, starving send_eventLog of free sockets.
    auto link = Ethernet.linkStatus();
    if (link == LinkOFF)
        return false;
    return true;
}
String system_t()
{
    time_t t = now();
    char buffer[20];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
            year(t), month(t), day(t),
            hour(t), minute(t), second(t));
    return String(buffer);
}

void ethernet_state::begin(database_s *db, storage_state *memory)
{
    // Deselect SD (CS pin 4) SEBELUM Ethernet init. W5100 & SD share SPI bus:
    // kalau ada kartu SD kepasang dan CS-nya dibiarin floating, kartu ikut
    // nimbrung di MISO pas W5100 init -> Ethernet.begin/DHCP bisa hang (gejala:
    // macet sehabis "init my pins"). Dulu ke-handle gak sengaja sama card.begin()
    // di constructor sdf_state; sekarang dibikin eksplisit di sini.
    pinMode(CS_P_SD, OUTPUT);
    digitalWrite(CS_P_SD, HIGH); // SD idle
    pinMode(CS_P_ETH, OUTPUT);
    digitalWrite(CS_P_ETH, HIGH); // W5100 idle (Ethernet.init ambil alih)

    db_ptr = db;
    memory_ptr = memory;
    if (memory_ptr)
    {
        memory_ptr->load_device_class(device_class, sizeof(device_class));
    }
    Ethernet.init(CS_P_ETH);
    delay(250);
#ifdef DEBUG_ETH
    Serial.println(":eth:init...");
#endif

    // Skip DHCP entirely when the cable is not plugged in — the default
    // Ethernet.begin(mac) blocks ~60s waiting for a lease. Trust LinkOFF;
    // on Unknown (e.g. W5100) we still try, but with a short timeout.
    EthernetLinkStatus link = Ethernet.linkStatus();
    bool dhcp_ok = false;
    if (link != LinkOFF)
    {
        dhcp_ok = (Ethernet.begin(eth_mac, 3000, 1000) != 0);
    }

    if (!dhcp_ok)
    {
#ifdef DEBUG_ETH
        Serial.println(link == LinkOFF
                           ? "No cable, skipping DHCP"
                           : "Failed to configure Ethernet using DHCP");
#endif
        IPAddress ip(192, 168, 0, 8);
        IPAddress dns(LOG_SERVER_FALLBACK_DNS_A, LOG_SERVER_FALLBACK_DNS_B,
                      LOG_SERVER_FALLBACK_DNS_C, LOG_SERVER_FALLBACK_DNS_D);
        Ethernet.begin(eth_mac, ip, dns);
        eth_connected = false;
        first_initialize = false;
        last_reconnect = millis();
    }
    else
    {
        eth_connected = true;
        first_initialize = true;
    }
#ifdef DEBUG_ETH
    Serial.print("Server is at ");
    Serial.println(Ethernet.localIP());
    Serial.println("port" + String(server));
    if (ethernetCableConnected())
    {
        Serial.println("Ethernet cable connected");
    }
    else
    {
        Serial.println("Cable NOT connected");
    }
    switch (Ethernet.hardwareStatus())
    {
    case EthernetW5100:
        Serial.println("Chip: W5100");
        break;
    case EthernetW5200:
        Serial.println("Chip: W5200");
        break;
    case EthernetW5500:
        Serial.println("Chip: W5500");
        break;
    default:
        Serial.println("Chip: Not found!");
        break;
    }

#endif
    server.begin();
    reset_parser();

    // Init SD utk event-log SETELAH Ethernet siap (W5100 share SPI bus, CS-nya
    // harus idle dulu). Re-begin di sini karena constructor sd_card jalan
    // sebelum SPI.begin() di setup(). Sengaja SEBELUM early-return di bawah:
    // logging SD harus tetap nyala walau Ethernet putus. Gagal begin() -> semua
    // call log_event/log_tail jadi no-op, sketch tetep jalan (gak hang).
    // CS coordination (lihat sd_log.txt): W5100 & SD share SPI bus. W5100 CS
    // HARUS idle (HIGH) sebelum SD init, kalau gak dua chip rebutan MISO ->
    // card.begin() bisa hang. Drive kedua CS HIGH dulu.
    pinMode(CS_P_ETH, OUTPUT);
    digitalWrite(CS_P_ETH, HIGH); // W5100 idle
    pinMode(CS_P_SD, OUTPUT);
    digitalWrite(CS_P_SD, HIGH); // SD idle (begin akan ambil alih)
#ifdef DEBUG_SD
    Serial.println(":sd:begin...");
    Serial.flush();
#endif
    // Cap init clock di 4 MHz: lebih toleran ke wiring/bus share dgn W5100,
    // dan bikin SdFat lebih cepet nyerah (return false) kalau slot kosong —
    // bukan ngegantung. Slot kosong = logging SD di-disable, sketch jalan terus.
    sd_card.sd_isnormal = sd_card.card.begin(CS_P_SD, SD_SCK_MHZ(4));

    // Apapun hasilnya, lepas SD dari bus (CS HIGH) biar DO-nya yg mungkin
    // nyangkut gak ngacak transaksi SPI W5100 (DHCP/HTTP) setelah ini.
    digitalWrite(CS_P_SD, HIGH);
    digitalWrite(CS_P_ETH, HIGH);
#ifdef DEBUG_SD
    Serial.println(sd_card.sd_isnormal ? ":sd:event-log ready"
                                       : ":sd:event-log disabled (no card / not ready)");
#endif

    if (!eth_connected)
    {
#ifdef DEBUG_ETH
        Serial.println(":eth:init end");
#endif
        return;
    }
    ntpCfg.Udp.begin(ntpCfg.localPort);
    unsigned long ntpTime = 0;
#ifdef DEBUG_TIME
    Serial.println("Menghubungi NTP server...");
#endif
    while (ntpTime == 0)
    {
        static byte trying_get_time = 0;
        if (trying_get_time++ > 5)
        {
#ifdef DEBUG_TIME
            Serial.println("gagal sinkronisasi waktu");
#endif
            break;
        }
        else
        {
#ifdef DEBUG_TIME
            Serial.println("Waktu berhasil disinkronkan!");
#endif
        }
        ntpTime = ntpCfg.getNTPTime();
    }
    setTime(ntpTime);
#ifdef DEBUG_ETH
    Serial.println(system_t());
    Serial.println(":eth:init end");
#endif
}

void ethernet_state::loop(database_s *db, storage_state *memory, Relay_state *locker)
{
    // Proses event tap DULU — sebelum gate jaringan apa pun di bawah. SD log
    // harus tetap jalan walau Ethernet putus / loop lagi di-throttle; itu inti
    // dari fallback (dulu blok ini di ekor loop, jadi gak kepanggil pas offline).
    for (size_t i = 0; i < Size_Siswa; i++)
    {
        if (send_log[i])
        {
            unsigned long uid_decimal = 0;
            for (int x = 3; x >= 0; x--)
                uid_decimal = (uid_decimal << 8) | db[i].card[x];
            send_eventLog(uid_decimal, i);
            send_log[i] = false;
        }
    }

    const unsigned long now = millis();
    static unsigned long last_checkcable = 0;
    const long interval_reCheck_cable = 120000;
    if (now - last_checkcable < interval_reCheck_cable)
        return;
    // static byte counting_try = 0;
    const bool cable = ethernetCableConnected();
    if (!cable)
    {
        if (first_initialize || interupt_trigger)
        {
            last_checkcable = millis();
#ifdef DEBUG_ETH
            Serial.println("cable disconnect");
#endif
            return;
        }
        else
        {
            if (now - last_reconnect > 120000)
            {
#ifdef DEBUG_ETH
                Serial.println("cable disconnect,with reinit");
#endif
                first_initialize = false;
            }
            else
                return;
        }
    }
    if (!eth_connected && interupt_trigger)
    {
        last_reconnect = now;
        last_checkcable = now;
        Serial.println("return with interupt");
        interupt_trigger = false;
        return;
    }
    if (!eth_connected)
    {
        if (now - last_reconnect >= RECONNECT_INTERVAL)
        {
            last_reconnect = millis();
#ifdef DEBUG_ETH
            Serial.println("try dhcp");
#endif
            // Short DHCP timeout so a missing lease doesn't stall the loop.
            if (Ethernet.begin(eth_mac, 3000, 1000) != 0)
            {
                eth_connected = true;
                first_initialize = true;
                server.begin();
                ntpCfg.Udp.begin(ntpCfg.localPort);
                ntpCfg.update(0);
                if (ntpCfg.getNTPTime() != 0)
                    setTime(ntpCfg.getNTPTime());
#ifdef DEBUG_ETH
                Serial.println(String("t:") + system_t());
                Serial.print("Reconnect OK, IP: ");
                Serial.println(Ethernet.localIP());
#endif
            }
#ifdef DEBUG_ETH
            Serial.println("try dhcp end");
#endif
        }
        return;
    }

    if (now - last_maintain >= MAINTAIN_INTERVAL)
    {
        last_maintain = now;
        byte result = Ethernet.maintain();
        if (result == 1 || result == 3)
        {
            eth_connected = false;
#ifdef DEBUG_ETH
            Serial.println("DHCP renew gagal");
#endif
            return;
        }
    }
    ntpCfg.update();
    EthernetClient client = server.available();
    bool handled_http = false;
    if (client)
    {
        // OTA dimatikan sementara — updater = nullptr (route /update di-skip).
        handle_client(client, db, memory, locker, nullptr);
        delay(1);
        client.stop();
        handled_http = true;
    }
    // static bool res = true;
    // if (res)
    // {
    //     handle_reset(client, db, memory);
    //     res = false;
    // }

    /* Give the W5100 a moment for the just-closed server socket to fully
     transition to CLOSED before the next send_eventLog (di awal loop
     berikutnya) calls socketBegin() — without this gap the next TCP connect
     intermittently fails on the first try.
    */
    if (handled_http)
        delay(30);
}

void ethernet_state::handle_client(EthernetClient &client, database_s *db, storage_state *memory, Relay_state *locker, Updater_state *updater)
{

    reset_parser();
    // start_time = millis();

    if (!parse_request(client))
    {
        send_error(client, 400, "Bad Request");
        return;
    }

    if (millis() - start_time > 5000)
    {
        send_error(client, 408, "Request Timeout");
        return;
    }

#ifdef DEBUG_ETH
    Serial.print("Method: ");
    Serial.println(method);
    Serial.print("Auth: ");
    Serial.println(auth_header);
    Serial.print("Path: ");
    Serial.println(path);
#endif

    bool needs_auth = false;

    if (strcmp(method, "POST") == 0 ||
        strcmp(method, "DELETE") == 0)
    {
        needs_auth = true;
    }
    // /sd-log = data tap siswa, jangan kebuka publik (lihat sd_log.txt).
    else if (strcmp(method, "GET") == 0 && strncmp(path, "/sd-log", 7) == 0)
    {
        needs_auth = true;
    }

    if (needs_auth && !auth.check_auth(auth_header))
    {
        send_error(client, 401, "Unauthorized");
        return;
    }

    bool route_found = false;

    if (strncmp(path, "/students/", 10) == 0)
    {
        int locker_num = atoi(path + 10);

        if (locker_num >= 0 && locker_num < Size_Siswa)
        {
            route_found = true;

            if (strcmp(method, "DELETE") == 0)
            {
                handle_delete_student(client, db, memory, locker_num);
            }
            else if (strcmp(method, "POST") == 0)
            {
                handle_open_locker(client, locker, locker_num);
            }
            else if (strcmp(method, "GET") == 0)
            {
                handle_get_student(client, db, locker_num);
            }
            else
            {
                send_error(client, 405, "Method Not Allowed");
            }
        }
        else
        {
            send_error(client, 400, "Invalid locker number");
        }
    }
    else if (strcmp(path, "/info") == 0 && strcmp(method, "GET") == 0)
    {
        route_found = true;
        handle_info(client);
    }
    else if (strcmp(path, "/info") == 0 && strcmp(method, "POST") == 0)
    {
        route_found = true;
        handle_post_info(client, memory);
    }
    else if (strcmp(path, "/students") == 0 && strcmp(method, "GET") == 0)
    {
        route_found = true;
        handle_get_data(client, db);
    }
    else if (strcmp(path, "/students") == 0 && strcmp(method, "POST") == 0)
    {
        route_found = true;
        handle_post_student(client, db, memory);
    }
    else if (strcmp(path, "/restart") == 0 && strcmp(method, "POST") == 0)
    {
#ifdef DEBUG_ETH
        Serial.println(":sys:restart");
#endif
        sys.software_Resatrt();
    }
    else if (strcmp(path, "/reset") == 0 && strcmp(method, "POST") == 0)
    {
        route_found = true;
        handle_reset(client, db, memory);
    }
    // OTA dimatikan sementara — route /update di-nonaktifkan.
    // else if (strcmp(path, "/update") == 0 && strcmp(method, "POST") == 0)
    // {
    //     route_found = true;
    //     handle_update(client, updater);
    // }
    else if (strncmp(path, "/sd-log", 7) == 0 && strcmp(method, "GET") == 0)
    {
        route_found = true;
        // Default 10 baris terakhir; ?n=N (1..100) buat override.
        byte n = 10;
        const char *q = strchr(path, '=');
        if (q)
        {
            int v = atoi(q + 1);
            if (v > 0 && v <= 100)
                n = (byte)v;
        }
        handle_sd_log(client, n);
    }

    if (!route_found)
    {
        send_error(client, 404, "Not Found");
    }
}

bool ethernet_state::parse_request(EthernetClient &client)
{
    bool first_line = true;
    String line = "";
    start_time = millis();

    while (client.connected() || client.available())
    {
        if (client.available())
        {
            char c = client.read();

            if (c == '\n')
            {
                if (first_line)
                {
                    int space1 = line.indexOf(' ');
                    int space2 = line.indexOf(' ', space1 + 1);

                    if (space1 == -1 || space2 == -1)
                    {
                        return false;
                    }

                    line.substring(0, space1).toCharArray(method, sizeof(method));
                    line.substring(space1 + 1, space2).toCharArray(path, sizeof(path));

                    first_line = false;
                }
                else if (line.startsWith("Authorization:") || line.startsWith("authorization:"))
                {
                    auth_header = line.substring(14);
                    auth_header.trim();
                }
                else if (line.length() == 0 || (line.length() == 1 && line[0] == '\r'))
                {
                    header_done = true;
                    body_len = 0;

                    unsigned long body_start = millis();
                    while (millis() - body_start < 500)
                    {
                        while (client.available() && body_len < BODY_SIZE - 1)
                            body[body_len++] = client.read();
                        if (!client.connected() && !client.available())
                            break;
                    }
                    body[body_len] = '\0';
                    return true;
                }

#ifdef DEBUG_ETH
                Serial.println(line);
#endif
                line = "";
            }
            else if (c != '\r')
            {
                line += c;
            }
        }

        if (millis() - start_time > 3000)
        {
            return false;
        }
    }

    return false;
}

void ethernet_state::reset_parser()
{
    method[0] = '\0';
    path[0] = '\0';
    header_done = false;
    last_char = '\0';
    newline_count = 0;
    body[0] = '\0';
    body_len = 0;
    auth_header = "";
    start_time = 0;
}

// void ethernet_state::handle_info(EthernetClient &client, database_s *db, storage_state *memory, bool update)
// {
//     if (!db || !memory)
//     {
//         send_error(client, 500, "Database not available");
//         return;
//     }
//     if (body_len == 0)
//     {
//         send_error(client, 400, "empty body");
//         return;
//     }
//     JsonDocument doc;
//     DeserializationError file = deserializeJson(doc, body);
//     if (file)
//     {
//         send_error(client, 500, "error parsing json");
//         return;
//     }
//     if (String(doc.containsKey("location")) != String(location))
//     {
//     }
// }
void ethernet_state::handle_info(EthernetClient &client)
{
    StaticJsonDocument<512> doc;
    doc["status"] = "ok";
    doc["dev_class"] = device_class;
    doc["c_name"] = controller_name;
    doc["ver"] = firmware_ver;
    doc["up_t"] = millis() / 1000;

    IPAddress ip = Ethernet.localIP();
    char ip_str[16];
    sprintf(ip_str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    doc["ip_a"] = ip_str;

    // MAC address
    Ethernet.MACAddress(eth_mac);
    char mac_str[18];
    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
            eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
    doc["mac_addr"] = mac_str;
    doc["total_locker"] = Size_Siswa;

    int use = 0;
    if (db_ptr)
    {
        for (int i = 0; i < Size_Siswa; i++)
        {
            if (db_ptr[i].statusdb == Status_db::Not_Available)
            {
                use++;
            }
        }
    }
    doc["avail_lock"] = Size_Siswa - use;
#ifdef DEBUG_ETH
    Serial.println("mac:" + String(mac_str));
    Serial.println("c_name" + String(controller_name));
    Serial.println("dev_class" + String(device_class));
#endif

    String json;
    serializeJson(doc, json);

    send_ok(client, json.c_str());
}

void ethernet_state::handle_post_info(EthernetClient &client, storage_state *memory)
{
    if (!memory)
    {
        send_error(client, 500, "Storage not available");
        return;
    }

    if (body_len == 0)
    {
        send_error(client, 400, "Empty body");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error)
    {
        send_error(client, 400, "Invalid JSON");
        return;
    }

    if (!doc.containsKey("dev_class"))
    {
        send_error(client, 400, "Missing dev_class field");
        return;
    }

    const char *new_class = doc["dev_class"];
    if (!new_class || strlen(new_class) == 0)
    {
        send_error(client, 400, "Empty dev_class");
        return;
    }

    if (strlen(new_class) >= sizeof(device_class))
    {
        send_error(client, 400, "dev_class too long");
        return;
    }

    if (!memory->save_device_class(new_class))
    {
        send_error(client, 500, "Failed to save dev_class");
        return;
    }

    strncpy(device_class, new_class, sizeof(device_class) - 1);
    device_class[sizeof(device_class) - 1] = '\0';

#ifdef DEBUG_ETH
    Serial.println(String("dev_class updated: ") + device_class);
#endif

    StaticJsonDocument<128> response;
    response["status"] = "updated";
    response["dev_class"] = device_class;

    char json[128];
    serializeJson(response, json, sizeof(json));
    send_ok(client, json);
}
void ethernet_state::handle_get_data(EthernetClient &client, database_s *db)
{
    if (!db)
    {
        send_error(client, 500, "Database not available");
#ifdef DEBUG_ETH
        Serial.println(":eth:database not ready");
#endif
        return;
    }

#ifdef DEBUG_ETH
    Serial.println(":eth:get all data");
#endif

    // Stream langsung ke client supaya tidak bergantung heap (JsonDocument +
    // String reallocation pernah produce JSON kosong/truncate di Mega ketika
    // memori sempit, bikin dashboard nampilin semua locker sebagai "free").
    // Format per entry: {"locker":N,"card_uid":"DDDDDDDDDD","status":"use"}
    // Base length 51 (single-digit N). Tambah 1 char per digit ekstra.
    const size_t base_envelope = 15; // strlen("{\"students\":[]}")
    const size_t base_entry = 51;

    size_t body_len = base_envelope;
    int count = 0;
    for (int i = 0; i < Size_Siswa; i++)
    {
        if (db[i].statusdb == Status_db::Not_Available)
        {
            count++;
            body_len += base_entry;
            if (i >= 10)
                body_len += 1;
            if (i >= 100)
                body_len += 1;
        }
    }
    if (count > 1)
        body_len += (count - 1); // koma antar entry

    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: application/json"));
    client.println(F("Connection: close"));
    client.print(F("Content-Length: "));
    client.println(body_len);
    client.println();

    client.print(F("{\"students\":["));
    bool first = true;
    for (int i = 0; i < Size_Siswa; i++)
    {
        if (db[i].statusdb != Status_db::Not_Available)
            continue;

        if (!first)
            client.print(',');
        first = false;

        unsigned long uid_decimal =
            ((unsigned long)db[i].card[0]) |
            ((unsigned long)db[i].card[1] << 8) |
            ((unsigned long)db[i].card[2] << 16) |
            ((unsigned long)db[i].card[3] << 24);

        char dec[11];
        sprintf(dec, "%010lu", uid_decimal);

        client.print(F("{\"locker\":"));
        client.print(i);
        client.print(F(",\"card_uid\":\""));
        client.print(dec);
        client.print(F("\",\"status\":\"use\"}"));
    }
    client.print(F("]}"));
}

void ethernet_state::handle_post_student(EthernetClient &client, database_s *db, storage_state *memory)
{
    if (!db || !memory)
    {
        send_error(client, 500, "Database not available");
        return;
    }

    if (body_len == 0)
    {
        send_error(client, 400, "Empty body");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        send_error(client, 400, "Invalid JSON");
        return;
    }

    if (!doc.containsKey("no") || !doc.containsKey("id"))
    {
        send_error(client, 400, "Missing required fields");
        return;
    }
    int locker_raw = doc["no"];

    if (locker_raw < 0 || locker_raw >= Size_Siswa)
    {
        send_error(client, 400, "Invalid locker number");
        return;
    }

    byte locker = (byte)locker_raw;
    const char *card_uid_str = doc["id"];

    if (!card_uid_str || locker >= Size_Siswa)
    {
        send_error(client, 400, "Invalid student data");
        return;
    }

    if (db[locker].statusdb == Status_db::Not_Available)
    {
        send_error(client, 409, "Locker already in use");
        return;
    }

    String uid_str = card_uid_str;

    bool is_decimal = true;
    for (size_t i = 0; i < uid_str.length(); i++)
    {
        char c = uid_str[i];
        if (!isdigit(c))
        {
            is_decimal = false;
            break;
        }
    }

    if (!is_decimal)
    {
        send_error(client, 400, "Invalid format. Only decimal numbers accepted (e.g., 0028758077)");
        return;
    }

    database_s new_db[Size_Siswa];
    for (int i = 0; i < Size_Siswa; i++)
    {
        new_db[i] = db[i];
    }

    memset(new_db[locker].card, 0, size_uid);

    unsigned long uid_decimal = strtoul(uid_str.c_str(), NULL, 10);

    new_db[locker].card[0] = (uid_decimal) & 0xFF;
    new_db[locker].card[1] = (uid_decimal >> 8) & 0xFF;
    new_db[locker].card[2] = (uid_decimal >> 16) & 0xFF;
    new_db[locker].card[3] = (uid_decimal >> 24) & 0xFF;
    for (size_t xp = 0; xp < Size_Siswa; xp++)
    {
        if (xp == locker)
            continue;
        if (db[xp].statusdb != Status_db::Not_Available)
            continue;

        if (memcmp(db[xp].card, new_db[locker].card, size_uid) == 0)
        {
            String msg = "duplicated uid with locker num" + String(xp);
#ifdef DEBUG_ETH
            Serial.println(":eth:error same uid card with locker " + String(xp));
#endif
            send_error(client, 409, msg.c_str());
            memset(new_db[locker].card, 0, size_uid);
            return;
        }
    }

#ifdef DEBUG_ETH
    Serial.print("Stored decimal UID: ");
    char formatted_uid[11];
    sprintf(formatted_uid, "%010lu", uid_decimal);
    Serial.print(formatted_uid);
    Serial.print(" as bytes: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print(new_db[locker].card[i]);
        Serial.print(" ");
    }
    Serial.println();
#endif

    new_db[locker].number_locker = locker;
    new_db[locker].statusdb = Status_db::Not_Available;

    if (!memory->save_data(db, new_db))
    {
        send_error(client, 500, "Failed to save data");
        return;
    }

    StaticJsonDocument<128> response;
    response["status"] = "created";
    response["locker"] = locker;

    // Format decimal dengan leading zero (10 digit)
    char dec[11];
    sprintf(dec, "%010lu", uid_decimal);
    response["card_uid"] = dec;

    char json[128];
    serializeJson(response, json, sizeof(json));
    send_ok(client, json);
}
void ethernet_state::handle_open_locker(EthernetClient &client,
                                        Relay_state *locker,
                                        byte locker_num)
{
    if (!locker)
    {
        send_error(client, 500, "Locker system not available");
        return;
    }

    // Activate relay to open locker
    locker[locker_num].status = Status_RL::ON;
    locker[locker_num].open_loker = true;

#ifdef DEBUG_ETH
    Serial.print("Opening locker: ");
    Serial.println(locker_num);
#endif

    StaticJsonDocument<128> response;
    response["status"] = "opened";
    response["locker"] = locker_num;

    char json[128];
    serializeJson(response, json, sizeof(json));
    send_ok(client, json);
}
void ethernet_state::handle_get_student(EthernetClient &client,
                                        database_s *db,
                                        byte locker_num)
{
    if (!db)
    {
        send_error(client, 500, "Database not available");
        return;
    }

    StaticJsonDocument<256> response;
    response["locker"] = locker_num;

    if (db[locker_num].statusdb == Status_db::Not_Available)
    {
        response["status"] = "use";

        unsigned long uid_decimal =
            ((unsigned long)db[locker_num].card[0]) |
            ((unsigned long)db[locker_num].card[1] << 8) |
            ((unsigned long)db[locker_num].card[2] << 16) |
            ((unsigned long)db[locker_num].card[3] << 24);

        char formatted_uid[11];
        sprintf(formatted_uid, "%010lu", uid_decimal);
        response["card_uid"] = formatted_uid;
    }
    else
    {
        // Locker is available/free
        response["status"] = "available";
    }

    char json[256];
    serializeJson(response, json, sizeof(json));
    send_ok(client, json);
}
void ethernet_state::handle_delete_student(EthernetClient &client,
                                           database_s *db,
                                           storage_state *memory,
                                           byte locker_num)
{
    if (!db || !memory)
    {
        send_error(client, 500, "Database not available");
        return;
    }

    if (db[locker_num].statusdb == Status_db::Available)
    {
        send_error(client, 404, "Locker not use");
        return;
    }

    database_s new_db[Size_Siswa];
    for (int i = 0; i < Size_Siswa; i++)
    {
        new_db[i] = db[i];
    }

    new_db[locker_num].statusdb = Status_db::Available;
    memset(new_db[locker_num].card, 0, size_uid);
    new_db[locker_num].number_locker = locker_num;

    if (!memory->save_data(db, new_db))
    {
        send_error(client, 500, "Failed to save data");
        return;
    }

#ifdef DEBUG_ETH
    Serial.print("Deleted student from locker: ");
    Serial.println(locker_num);
#endif

    StaticJsonDocument<128> response;
    response["status"] = "deleted";
    response["locker"] = locker_num;

    char json[128];
    serializeJson(response, json, sizeof(json));
    send_ok(client, json);
}

void ethernet_state::handle_reset(EthernetClient &client,
                                  database_s *db,
                                  storage_state *memory)
{
    if (!db || !memory)
    {
        send_error(client, 500, "Database not available");
        return;
    }

    database_s new_db[Size_Siswa];

    for (int i = 0; i < Size_Siswa; i++)
    {
        new_db[i].statusdb = Status_db::Available;
        memset(new_db[i].card, 0, size_uid);
        new_db[i].number_locker = i;
    }

    if (!memory->save_data(db, new_db))
    {
        send_error(client, 500, "Failed to reset data");
        return;
    }

    StaticJsonDocument<128> response;
    response["status"] = "reset";
    response["message"] = "All lockers cleared";

    char json[128];
    serializeJson(response, json, sizeof(json));
#ifdef DEBUG_ETH
    Serial.println(":eth:reset database");
#endif
    send_ok(client, json);
}

// === OTA Update Handler ===
void ethernet_state::handle_update(EthernetClient &client, Updater_state *updater)
{
#ifdef DEBUG_OTA
    Serial.println(":ota:update endpoint called");
#endif

    if (body_len == 0)
    {
        send_error(client, 400, "Empty body");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error)
    {
        send_error(client, 400, "Invalid JSON");
        return;
    }

    bool should_download = false;
    bool should_flash = false;

    if (doc.containsKey("download") && doc["download"].is<bool>())
    {
        should_download = doc["download"];
    }

    if (doc.containsKey("flash") && doc["flash"].is<bool>())
    {
        should_flash = doc["flash"];
    }

    if (!should_download && !should_flash)
    {
        send_error(client, 400, "No action specified (download/flash)");
        return;
    }

#ifdef DEBUG_OTA
    Serial.print(":ota:download=");
    Serial.print(should_download);
    Serial.print(" flash=");
    Serial.println(should_flash);
#endif

    StaticJsonDocument<256> response;
    response["status"] = "ok";

    if (should_download)
    {
        bool dl_result = download_firmware(FW_BIN_NAME, updater);
        response["download"] = dl_result ? "success" : "failed";

#ifdef DEBUG_OTA
        Serial.print(":ota:download result: ");
        Serial.println(dl_result);
#endif
    }

    // Flashing is delegated to the avr_boot SD bootloader. We only stage the
    // image (FIRMWARE.BIN already on SD) and reset into the bootloader.
    bool will_reboot = false;
    if (should_flash && updater)
    {
        if (updater->check_firmware(FW_BIN_NAME))
        {
            response["flash"] = "rebooting";
            will_reboot = true;
        }
        else
        {
            response["flash"] = "no firmware on sd";
        }
    }
    else if (should_flash)
    {
        response["flash"] = "no updater";
    }

    char json[256];
    serializeJson(response, json, sizeof(json));
    send_ok(client, json);

    // Reset into avr_boot AFTER the response is sent. Does not return.
    if (will_reboot)
    {
        delay(500); // Tunggu agar client terima response
        updater->stage_and_reboot();
    }
}

void ethernet_state::send_ok(EthernetClient &client, const char *json)
{
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(strlen(json));
    client.println();
    client.println(json);
}

void ethernet_state::send_error(EthernetClient &client, int code, const char *msg)
{
    JsonDocument doc;
    doc["error"] = msg;
    doc["code"] = code;

    char json[128];
    serializeJson(doc, json, sizeof(json));

    client.print("HTTP/1.1 ");
    client.print(code);
    client.print(" ");
    client.println(msg);
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(strlen(json));
    client.println();
    client.println(json);
}
bool ethernet_state::resolve_log_server()
{
    DNSClient dns;
    IPAddress dns_ip = Ethernet.dnsServerIP();
    if (dns_ip == IPAddress(0, 0, 0, 0))
    {
        dns_ip = IPAddress(LOG_SERVER_FALLBACK_DNS_A, LOG_SERVER_FALLBACK_DNS_B,
                           LOG_SERVER_FALLBACK_DNS_C, LOG_SERVER_FALLBACK_DNS_D);
    }
    dns.begin(dns_ip);

    IPAddress resolved;
    if (dns.getHostByName(LOG_SERVER_HOST, resolved) != 1)
    {
#ifdef DEBUG_ETH
        Serial.println("DNS resolve gagal: " LOG_SERVER_HOST);
#endif
        return false;
    }

    log_server_ip = resolved;
    log_server_resolved_at = millis();
    log_server_ip_valid = true;
#ifdef DEBUG_ETH
    Serial.print("DNS " LOG_SERVER_HOST " -> ");
    Serial.println(log_server_ip);
#endif
    return true;
}

// Kirim event ke HTTP log server. Return true kalau request kekirim (sampai
// drain respons), false kalau gagal (DNS/connect) — dipakai send_eventLog buat
// nentuin flag SENT vs PEND di SD.
bool ethernet_state::transmit_eventLog(const unsigned long uid_decimal, byte index)
{
    EthernetClient logClient;
    // W5100 default 1000ms is tight — TCP handshake + ARP can exceed it after
    // a fresh socket allocation. Give 3s so a SYN retransmit can complete.
    logClient.setConnectionTimeout(3000);
    String key = token_sck_log;
    bool stale = !log_server_ip_valid || (millis() - log_server_resolved_at) > LOG_SERVER_DNS_TTL_MS;

    if (stale && !resolve_log_server() && !log_server_ip_valid)
    {
        return false;
    }

    if (!logClient.connect(log_server_ip, LOG_SERVER_PORT))
    {
#ifdef DEBUG_ETH
        Serial.println("Gagal connect ke log server, invalidate cache");
#endif
        log_server_ip_valid = false;
        // Let the W5100 finish any socket-state transitions (TIME_WAIT/CLOSED)
        // left behind by the just-handled HTTP server connection before we
        // ask socketBegin() for a fresh slot.
        delay(50);
        Ethernet.maintain();
        if (!resolve_log_server())
        {
            return false;
        }
        if (!logClient.connect(log_server_ip, LOG_SERVER_PORT))
        {
#ifdef DEBUG_ETH
            Serial.println("Connect retry gagal");
#endif
            return false;
        }
    }
#ifdef DEBUG_ETH
    Serial.println("send log");
#endif
    StaticJsonDocument<256> doc;
    char buffer[256];
    doc["t"] = system_t();
    doc["no"] = index;
    doc["id"] = uid_decimal;
    doc["dev_class"] = device_class;
    size_t len = serializeJson(doc, buffer);

    logClient.println("POST /event-log HTTP/1.1");
    logClient.println("Host: " LOG_SERVER_HOST);
    logClient.println("Content-Type: application/json");
    logClient.println("Authorization:Bearer " + key);
    logClient.println("Connection: close");
    logClient.print("Content-Length: ");
    logClient.println(len);
    logClient.println();
    logClient.write((uint8_t *)buffer, len);

    unsigned long timeout = millis();
    while (logClient.connected() && millis() - timeout < 3000)
    {
        while (logClient.available())
        {
#ifdef DEBUG_ETH
            Serial.write(logClient.read());
#else
            logClient.read();
#endif
            timeout = millis();
        }
    }
    logClient.stop();
    return true;
}

// Catat event tap: kirim ke log server, lalu SELALU append ke SD sebagai
// fallback/audit lokal (flag SENT kalau kekirim, PEND kalau belum sempat).
// Append jalan walau network gagal -> event gak ilang (lihat sd_log.txt).
void ethernet_state::send_eventLog(const unsigned long uid_decimal, byte index)
{
    unsigned long epoch = now(); // sumber waktu sama dgn system_t()
    bool sent = false;
    // Coba kirim ke log server CUMA kalau emang lagi konek. Kalau offline,
    // langsung catat PEND ke SD tanpa buang waktu nyoba connect (3s timeout).
    if (eth_connected && ethernetCableConnected())
        sent = transmit_eventLog(uid_decimal, index);
#ifdef DEBUG_SD
    Serial.print(":sd:event tap idx=");
    Serial.print(index);
    Serial.print(" uid=");
    Serial.print(uid_decimal);
    Serial.println(sent ? " -> SENT" : " -> PEND (offline/gagal kirim)");
#endif
    sd_card.log_event(epoch, index, uid_decimal, device_class,
                      sent ? "SENT" : "PEND");
}

// GET /sd-log?n=N -> stream N baris terakhir event log ke client (plain text).
void ethernet_state::handle_sd_log(EthernetClient &client, byte n)
{
    // Streaming, panjang gak diketahui di depan -> tanpa Content-Length,
    // client baca sampai koneksi ditutup (handle_client stop() setelah ini).
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    if (sd_card.log_tail(client, n) == 0)
        client.println("# (kosong / SD tidak tersedia)");
}

// === OTA Download Firmware ===
bool ethernet_state::download_firmware(const char *filename, class Updater_state *updater)
{
    if (!memory_ptr)
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:memory not available");
#endif
        return false;
    }

    if (!updater)
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:updater not available");
#endif
        return false;
    }

    const char *asset_path = path_upgrade;
    const char *github_host = "api.github.com";
    const int github_port = 443; // HTTPS
    const char *github_token = token_upgrade_ota;

    EthernetClient dlClient;
    dlClient.setConnectionTimeout(5000);

#ifdef DEBUG_OTA
    Serial.println(":ota:connecting to github...");
#endif

    // HTTPS tidak fully supported di W5100, fallback ke HTTP jika diperlukan
    // Untuk sekarang, kita coba HTTP ke server proxy atau direct HTTP
    if (!dlClient.connect(github_host, 80))
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:github connect failed");
#endif
        return false;
    }

#ifdef DEBUG_OTA
    Serial.println(":ota:connected, requesting firmware...");
#endif

    // Send HTTP GET request dengan Accept header untuk binary download
    dlClient.print("GET ");
    dlClient.print(asset_path);
    dlClient.println(" HTTP/1.1");
    dlClient.print("Host: ");
    dlClient.println(github_host);
    dlClient.print("Authorization: Bearer ");
    dlClient.println(github_token);
    dlClient.println("Accept: application/octet-stream");
    dlClient.println("User-Agent: RFID-Controller/1.0");
    dlClient.println("Connection: close");
    dlClient.println();

    // Wait for response headers
    unsigned long timeout = millis();
    bool headers_done = false;
    int response_code = 200;
    size_t file_size = 0;
    bool receiving_body = false;
    int newline_count_dl = 0;

    while (dlClient.connected() && millis() - timeout < 10000)
    {
        if (dlClient.available())
        {
            timeout = millis();
            char c = dlClient.read();

            if (!headers_done)
            {
                if (c == '\n')
                {
                    newline_count_dl++;
                    if (newline_count_dl == 4)
                    {
                        headers_done = true;
                        receiving_body = true;
#ifdef DEBUG_OTA
                        Serial.println(":ota:headers parsed, reading body...");
#endif
                        break;
                    }
                }
                else if (c != '\r')
                {
                    newline_count_dl = 0;
                }
            }
        }
    }

    if (!receiving_body)
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:failed to parse response headers");
#endif
        dlClient.stop();
        return false;
    }

    if (!updater->sd_is_healthy())
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:SD card not healthy");
#endif
        dlClient.stop();
        return false;
    }

    // Check available free space (need ~150KB for firmware)
    size_t free_space = updater->sd_get_free_space();
    if (free_space < 160000) // ~156KB safety margin
    {
#ifdef DEBUG_OTA
        Serial.print(":ota:insufficient SD space. Free: ");
        Serial.print(free_space);
        Serial.println(" bytes");
#endif
        dlClient.stop();
        return false;
    }

    SdFile fwFile;
    if (!fwFile.open(filename, O_CREAT | O_TRUNC | O_WRITE))
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:cannot open file on SD");
#endif
        dlClient.stop();
        return false;
    }

#ifdef DEBUG_OTA
    Serial.print(":ota:writing to ");
    Serial.println(filename);
#endif

    // Stream download body langsung ke SD card dengan buffer 512 byte
    uint8_t buffer[512];
    size_t bytes_received = 0;
    size_t bytes_written = 0;
    timeout = millis();
    bool write_error = false;

    while (dlClient.connected() && millis() - timeout < 15000)
    {
        if (dlClient.available())
        {
            timeout = millis();
            int read_len = dlClient.read(buffer, sizeof(buffer));

            if (read_len > 0)
            {
                size_t written = fwFile.write(buffer, read_len);
                if (written == read_len)
                {
                    bytes_received += read_len;
                    bytes_written += written;

#ifdef DEBUG_OTA
                    if (bytes_received % 10240 == 0) // Log setiap 10KB
                    {
                        Serial.print(":ota:received ");
                        Serial.print(bytes_received);
                        Serial.println(" bytes");
                    }
#endif
                }
                else
                {
#ifdef DEBUG_OTA
                    Serial.println(":ota:SD write error - incomplete write");
#endif
                    write_error = true;
                    break; // Exit loop immediately on write error
                }
            }
        }
    }

    // Always close file, even on error
    if (!fwFile.close())
    {
#ifdef DEBUG_OTA
        Serial.println(":ota:file close error");
#endif
        write_error = true;
    }

    dlClient.stop();

    // Validate download completed
    if (write_error || bytes_written < 1000) // Minimum 1KB sanity check
    {
#ifdef DEBUG_OTA
        Serial.print(":ota:download incomplete/failed. Written: ");
        Serial.println(bytes_written);
#endif
        // Clean up incomplete file
        if (updater->sd_file_exists(filename))
        {
            updater->sd_remove_file(filename);
#ifdef DEBUG_OTA
            Serial.println(":ota:incomplete file removed");
#endif
        }
        return false;
    }

#ifdef DEBUG_OTA
    Serial.print(":ota:download complete. Total: ");
    Serial.print(bytes_written);
    Serial.println(" bytes");
#endif

    return true;
}
#endif