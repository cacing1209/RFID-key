#include <Ethernet.h>
#include <ArduinoJson.h>
#include <commond.h>

void ethernet_state::begin(database_s *db)
{
    db_ptr = db;

    if (Ethernet.begin(mac_L0002) == 0)
    {
#ifdef DEBUG_ETH
        Serial.println("Failed to configure Ethernet using DHCP");
#endif
        IPAddress ip(192, 168, 1, 177);
        Ethernet.begin(mac_L0002, ip);
    }

    server.begin();

#ifdef DEBUG_ETH
    Serial.print("Server is at ");
    Serial.println(Ethernet.localIP());
    Serial.println("port" + String(server));

#endif
    reset_parser();
}

void ethernet_state::loop(database_s *db, storage_state *memory)
{
    Ethernet.maintain();

    EthernetClient client = server.available();

    if (client)
    {
#ifdef DEBUG_ETH
        Serial.println("New client connected");
#endif

        handle_client(client, db, memory);

        delay(1);
        client.stop();

#ifdef DEBUG_ETH
        {
            Serial.println("Client disconnected");
        }
#endif
    }
}

void ethernet_state::handle_client(EthernetClient &client, database_s *db, storage_state *memory)
{
    reset_parser();
    start_time = millis();

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
    {
        Serial.print("Method: ");
        Serial.println(method);
        Serial.print("Path: ");
        Serial.println(path);
    }
#endif
    if (strcmp(path, "/info") == 0)
    {
        handle_info(client);
    }
    else if (strcmp(path, "/data") == 0 && strcmp(method, "GET") == 0)
    {
        handle_get_data(client, db);
    }
    else if (strcmp(path, "/students") == 0 && strcmp(method, "POST") == 0)
    {
        handle_post_student(client, db, memory);
    }
    else if (strcmp(path, "/students") == 0 && strcmp(method, "DELETE") == 0)
    {
        handle_delete_student(client, db, memory);
    }

    else if (strcmp(path, "/reset") == 0 && strcmp(method, "POST") == 0)
    {
        handle_reset(client, db, memory);
    }
    else
    {
        send_error(client, 404, "Not Found");
    }
}

bool ethernet_state::parse_request(EthernetClient &client)
{
    bool first_line = true;
    String line = "";

    while (client.connected())
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
                else if (line.length() == 0 || (line.length() == 1 && line[0] == '\r'))
                {
                    header_done = true;

                    body_len = 0;
                    while (client.available() && body_len < BODY_SIZE - 1)
                    {
                        body[body_len++] = client.read();
                    }
                    body[body_len] = '\0';

                    return true;
                }

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
    start_time = 0;
}

// Implementasi endpoint handlers
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
    Ethernet.MACAddress(mac_L0002);
    char mac_str[18];
    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac_L0002[0], mac_L0002[1], mac_L0002[2], mac_L0002[3], mac_L0002[4], mac_L0002[5]);
    doc["mac_addr"] = mac_str;
    doc["total_locker"] = size_mahasiswa;

    int occupied = 0;
    if (db_ptr)
    {
        for (int i = 0; i < size_mahasiswa; i++)
        {
            if (db_ptr[i].statusdb == Status_db::Not_Available)
            {
                occupied++;
            }
        }
    }
    doc["avail_lock"] = size_mahasiswa - occupied;
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

    DynamicJsonDocument doc(4096);
    JsonArray students = doc.createNestedArray("students");

    int count = 0;
    for (int i = 0; i < size_mahasiswa; i++)
    {
        if (db[i].statusdb == Status_db::Not_Available)
        {
            JsonObject student = students.createNestedObject();
            student["locker"] = db[i].number_locker;

            String card_uid = "";
            for (int j = 0; j < size_uid; j++)
            {
                if (db[i].card[j] < 0x10)
                    card_uid += "0";
                card_uid += String(db[i].card[j], HEX);
            }
            student["card_uid"] = card_uid;
            student["status"] = "occupied";
            count++;
        }
    }

    doc["total"] = count;
    doc["device_class"] = device_class;
    doc["max_students"] = size_mahasiswa;

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
    
    StaticJsonDocument<512> doc;
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
    
    byte locker = doc["no"];
    const char *card_uid_str = doc["id"];
    
    if (!card_uid_str || locker >= size_mahasiswa)
    {
        send_error(client, 400, "Invalid student data");
        return;
    }
    
    // Check if locker is already occupied
    if (db[locker].statusdb == Status_db::Not_Available)
    {
        send_error(client, 409, "Locker already in use");
        return;
    }
    
    String uid_str = String(card_uid_str);
    
    // NEW: Check if input is decimal number (like "0028758077") or hex (like "01B6C95D")
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
    
    database_s new_db[size_mahasiswa];
    for (int i = 0; i < size_mahasiswa; i++)
    {
        new_db[i] = db[i];
    }
    
    memset(new_db[locker].card, 0, size_uid);
    
    if (is_decimal)
    {
        unsigned long uid_decimal = strtoul(uid_str.c_str(), NULL, 10);
        new_db[locker].card[0] = (uid_decimal) & 0xFF;
        new_db[locker].card[1] = (uid_decimal >> 8) & 0xFF;
        new_db[locker].card[2] = (uid_decimal >> 16) & 0xFF;
        new_db[locker].card[3] = (uid_decimal >> 24) & 0xFF;
        
#ifdef DEBUG_ETH
        Serial.print("Stored decimal UID: ");
        Serial.print(uid_decimal);
        Serial.print(" as bytes: ");
        for (int i = 0; i < 4; i++)
        {
            Serial.print(new_db[locker].card[i]);
            Serial.print(" ");
        }
        Serial.println();
#endif
    }
    else
    {
        if (uid_str.length() % 2 != 0)
        {
            send_error(client, 400, "Invalid card UID format (must be even number of hex chars)");
            return;
        }
        
        int uid_bytes = uid_str.length() / 2;
        
        if (uid_bytes < 4 || uid_bytes > size_uid)
        {
            send_error(client, 400, "Invalid card UID length (must be 4-12 bytes)");
            return;
        }
        
        for (int i = 0; i < uid_bytes; i++)
        {
            String byteStr = uid_str.substring(i * 2, i * 2 + 2);
            new_db[locker].card[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
        }
    }
    
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
    response["card_uid"] = card_uid_str;
    
    char json[128];
    serializeJson(response, json, sizeof(json));
    send_ok(client, json);
}

void ethernet_state::handle_delete_student(EthernetClient &client,
                                           database_s *db,
                                           storage_state *memory)
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

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        send_error(client, 400, "Invalid JSON");
        return;
    }

    byte locker = doc["locker"];

    if (locker >= size_mahasiswa)
    {
        send_error(client, 400, "Invalid locker number");
        return;
    }

    if (db[locker].statusdb == Status_db::Available)
    {
        send_error(client, 404, "Locker not occupied");
        return;
    }

    database_s new_db[size_mahasiswa];

    for (int i = 0; i < size_mahasiswa; i++)
    {
        new_db[i] = db[i];
    }

    new_db[locker].statusdb = Status_db::Available;
    memset(new_db[locker].card, 0, size_uid);
    new_db[locker].number_locker = locker;

    if (!memory->save_data(db, new_db))
    {
        send_error(client, 500, "Failed to save data");
        return;
    }

    StaticJsonDocument<128> response;
    response["status"] = "deleted";
    response["locker"] = locker;

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
    StaticJsonDocument<128> doc;
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