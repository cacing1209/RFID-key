#include "commond.h"
#include <ArduinoJson.h>

// bool ethernet_state::post_data(database_s *db)
// {
//     if (client.connect(server, port))
//     {
//         String payload;
//         JsonDocument js;
//         js["device"] = "arduino";
//         js["status"] = "request";
//         serializeJson(js, payload);
//         Serial.println(payload);
//         // ===== REQUEST LINE =====
//         client.print("POST ");
//         client.print(endpoint);
//         client.println(" HTTP/1.1");

//         client.print("Host: ");
//         client.println(server);
//         client.println("Content-Type: application/json");
//         client.print("Content-Length: ");
//         client.println(payload.length());
//         client.println("Connection: close");
//         client.println();

//         client.print(payload);
//         timeout_start = millis();
//         request_active = true;
//         headers_ended = false;
//         response = "";
//         last_char = 0;
//         newline_count = 0;

//         if (enable_debug)
//         {
//             Serial.println(":eth:POST request sent");
//             Serial.println(payload);
//             Serial.println(":eth:server" + String(server));
//         }

//         return true;
//     }

//     if (enable_debug)
//         Serial.println(":eth:POST connection failed");

//     return false;
// }

void ethernet_state::process_response(database_s *db, storage_state *memory)
{
    if (!client.connected() && !client.available() || !request_active)
        return;

    if (millis() - timeout_start > 3000)
    {
        client.stop();
        request_active = false;
        return;
    }

    if (!headers_ended)
    {
        while (client.available())
        {
            char c = client.read();
            if (c == '\n' && last_char == '\r')
            {
                newline_count++;
                if (newline_count >= 2)
                {
                    headers_ended = true;
                    break;
                }
            }
            else if (c != '\r' && c != '\n')
            {
                newline_count = 0;
            }
            last_char = c;
        }
        return;
    }
    static char jsonBuffer[1200];
    if (enable_debug)
        Serial.println(":eth:json buffer" + String((int)sizeof(jsonBuffer)));
    int len = 0;

    while (client.available() && len < (int)(sizeof(jsonBuffer) - 1))
    {
        jsonBuffer[len++] = client.read();
    }
    jsonBuffer[len] = '\0';

    if (len == 0)
        return;

    DynamicJsonDocument doc(6128);
    DeserializationError error = deserializeJson(doc, jsonBuffer);

    if (error)
    {
        Serial.print(":eth:JSON FAILED: ");
        Serial.println(error.c_str());
        client.stop();
        request_active = false;
        return;
    }

    // simpan data ke struct
    database_s new_db[size_mahasiswa];
    for (size_t x = 0; x < size_mahasiswa; x++)
    {
        for (size_t y = 0; y < size_uid; y++)
        {
            new_db[x].card[y] = db[x].card[y];
        }
        new_db[x].number_locker = db[x].number_locker;
        new_db[x].statusdb = db[x].statusdb;
    }

    JsonArray array = doc.as<JsonArray>();
    int idx = 0;
    for (JsonObject item : array)
    {
        int locker = item["no"] | 0;

        if (locker < 0 || locker >= size_mahasiswa)
            continue;

        new_db[locker].number_locker = locker;
        idx++;
        JsonArray uid = item["id"];

        Serial.println("\nindex locker => " + String(locker));
        for (int i = 0; i < size_uid; i++)
        {
            if (i < uid.size())
            {
                new_db[locker].card[i] = uid[i];
                Serial.print(new_db[locker].card[i]);
            }
            else
            {
                new_db[locker].card[i] = 0;
            }
        }

        new_db[locker].statusdb = Status_db::Available;
    }
    Serial.print(":eth:DATA OK = ");
    Serial.println(idx);

    if (enable_debug)
    {
        Serial.println(":eth:check database=>");
        for (size_t i = 0; i < size_mahasiswa; i++)
        {
            Serial.print("\n:eth:card index=>" + String(i) + "number locker" + String(new_db[i].number_locker) + +"card=>");
            for (size_t xp = 0; xp < size_uid; xp++)
            {
                if (xp != 0)
                    Serial.print(',');
                Serial.print(new_db[i].card[xp]);
            }
        }
    }
    memory->save_data(db, new_db); // save data
    client.stop();
    request_active = false;
}

// void ethernet_state::loop_ethernet(database_s *main_data)
// {

//     if (!initialized)
//     {
//         if (enable_debug)
//         {
//             Serial.println(":eth:------------------------------");
//             Serial.println(":eth:INITIALIZING ETHERNET");
//         }

//         uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

//         if (Ethernet.begin(mac) == 0)
//         {
//             if (enable_debug)
//                 Serial.println("::eth:DHCP gagal");
//         }
//         // 192.168.178.7
//         if (enable_debug)
//         {
//             Serial.print(":eth:IP Address = ");
//             Serial.println(Ethernet.localIP());
//             Serial.println(":eth:ETHERNET INITIALIZED");
//             Serial.println(":eth:------------------------------");
//         }

//         initialized = true;
//         post_data(main_data);
//     }

//     Ethernet.maintain(); // dhcp ip
// }
