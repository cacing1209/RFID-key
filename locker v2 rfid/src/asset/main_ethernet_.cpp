#include <Ethernet.h>
#include <ArduinoJson.h>
#include <commond.h>
NTPConfig ntpCfg;
bool ethernetCableConnected()
{
    auto link = Ethernet.linkStatus();
    if (link == LinkON)
    {
        return true;
    }

    if (link == LinkOFF)
    {
        return false;
    }
    EthernetClient testClient;
    IPAddress gateway = Ethernet.gatewayIP();

    if (testClient.connect(gateway, 80))
    {
        testClient.stop();
        return true;
    }

    return false;
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

void ethernet_state::begin(database_s *db)
{
    db_ptr = db;
    Ethernet.init(10);
    delay(250);
#ifdef DEBUG_ETH
    Serial.println(":eth:begin..");
#endif
    if (Ethernet.begin(eth_mac) == 0)
    {
#ifdef DEBUG_ETH
        Serial.println("Failed to configure Ethernet using DHCP");
#endif
        IPAddress ip(192, 168, 0, 8);
        Ethernet.begin(eth_mac, ip);
        eth_connected = false;
        first_initialize = false;
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
    if (!eth_connected)
        return;
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
    Serial.println(":eth:begin end");
#endif
}

void ethernet_state::loop(database_s *db, storage_state *memory, Relay_state *locker)
{
    unsigned long now = millis();
    // static byte counting_try = 0;
    // bool cable = ethernetCableConnected();
    bool cable = true;
    if (!cable)
    {
        if (first_initialize)
        {
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
        interupt_trigger = false;
        return;
    }
    if (!eth_connected)
    {
        if (now - last_reconnect >= RECONNECT_INTERVAL)
        {

#ifdef DEBUG_ETH
            Serial.println("try dhcp");
#endif
            if (Ethernet.begin(eth_mac) != 0)
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
        }
        last_reconnect = millis();
#ifdef DEBUG_ETH
        Serial.println("try dhcp end");
#endif
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
    if (client)
    {
        handle_client(client, db, memory, locker);
        delay(1);
        client.stop();
    }

    for (size_t i = 0; i < size_mahasiswa; i++)
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
}

void ethernet_state::handle_client(EthernetClient &client, database_s *db, storage_state *memory, Relay_state *locker)
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
    Serial.print("Path: ");
    Serial.println(path);
    Serial.print("Auth: ");
    Serial.println(auth_header);
#endif

    bool needs_auth = false;

    if (strcmp(method, "POST") == 0 ||
        strcmp(method, "DELETE") == 0)
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

        if (locker_num >= 0 && locker_num < size_mahasiswa)
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
        sys.software_Reset();
    }
    else if (strcmp(path, "/reset") == 0 && strcmp(method, "POST") == 0)
    {
        route_found = true;
        handle_reset(client, db, memory);
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
    doc["location"] = location;
    doc["ver"] = "1.0.0";
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
    doc["total_locker"] = size_mahasiswa;

    int use = 0;
    if (db_ptr)
    {
        for (int i = 0; i < size_mahasiswa; i++)
        {
            if (db_ptr[i].statusdb == Status_db::Not_Available)
            {
                use++;
            }
        }
    }
    doc["avail_lock"] = size_mahasiswa - use;
#ifdef DEBUG_ETH
    Serial.println("mac:" + String(mac_str));
    Serial.println("c_name" + String(controller_name));
    Serial.println("dev_class" + String(device_class));
#endif

    String json;
    serializeJson(doc, json);

    send_ok(client, json.c_str());
}

void ethernet_state::handle_get_data(EthernetClient &client, database_s *db)
{
    if (!db)
    {
        send_error(client, 500, "Database not available");
        return;
    }

    JsonDocument doc;
    JsonArray students = doc.createNestedArray("students");

    for (int i = 0; i < size_mahasiswa; i++)
    {
        if (db[i].statusdb == Status_db::Not_Available)
        {
            JsonObject student = students.createNestedObject();
            student["locker"] = db[i].number_locker;

            unsigned long uid_decimal =
                ((unsigned long)db[i].card[0]) |
                ((unsigned long)db[i].card[1] << 8) |
                ((unsigned long)db[i].card[2] << 16) |
                ((unsigned long)db[i].card[3] << 24);

            char dec[11];
            sprintf(dec, "%010lu", uid_decimal);
            student["card_uid"] = dec;

            student["status"] = "use";
        }
    }

    String json;
    serializeJson(doc, json);

    send_ok(client, json.c_str());
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

    if (locker_raw < 0 || locker_raw >= size_mahasiswa)
    {
        send_error(client, 400, "Invalid locker number");
        return;
    }

    byte locker = (byte)locker_raw;
    const char *card_uid_str = doc["id"];

    if (!card_uid_str || locker >= size_mahasiswa)
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

    // Check if input is decimal number
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

    database_s new_db[size_mahasiswa];
    for (int i = 0; i < size_mahasiswa; i++)
    {
        new_db[i] = db[i];
    }

    // Clear the card array first
    memset(new_db[locker].card, 0, size_uid);

    // Convert decimal string to 4-byte UID (little-endian format)
    unsigned long uid_decimal = strtoul(uid_str.c_str(), NULL, 10);

    // Store in little-endian format (matches sensor output)
    new_db[locker].card[0] = (uid_decimal) & 0xFF;
    new_db[locker].card[1] = (uid_decimal >> 8) & 0xFF;
    new_db[locker].card[2] = (uid_decimal >> 16) & 0xFF;
    new_db[locker].card[3] = (uid_decimal >> 24) & 0xFF;

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
    locker[locker_num].reset_t = true;

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

    database_s new_db[size_mahasiswa];
    for (int i = 0; i < size_mahasiswa; i++)
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

    database_s new_db[size_mahasiswa];

    for (int i = 0; i < size_mahasiswa; i++)
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

    send_ok(client, json);
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
void ethernet_state::send_eventLog(const unsigned long uid_decimal, byte index)
{
    EthernetClient logClient;

    server_log = "93.144.178.187";
    portServer_log = 3000;

    if (!logClient.connect(server_log, portServer_log))
    {
#ifdef DEBUG_ETH
        Serial.println("Gagal connect ke log server");
#endif
        return;
    }
#ifdef DEBUG_ETH
    Serial.println("send log");
#endif

    StaticJsonDocument<128> doc;
    char buffer[128];
    doc["t"] = system_t();
    doc["no"] = index;
    doc["id"] = uid_decimal;
    size_t len = serializeJson(doc, buffer);

    logClient.println("POST /event-log HTTP/1.1");
    logClient.println("Host: " + String(server_log) + ':' + String(portServer_log));
    logClient.println("Content-Type: application/json");
    logClient.println("X-API-KEY: lockerqyubitL0002L0004L0008L000264L000128");
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
#endif
            timeout = millis();
        }
    }
    logClient.stop();
}