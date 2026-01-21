#include "commond.h"
#include <ArduinoJson.h>

bool ethernet_state::get_data(database_s *db)
{
    if (enable_debug)
    {
        Serial.println();
        Serial.println(":eth:------------------------------");
        Serial.println(":eth:GET DATA START");
        Serial.print(":eth:Server = ");
        Serial.println(server);
        Serial.print(":eth:Port = ");
        Serial.println(port);
        Serial.print(":eth:Endpoint = ");
        Serial.println(endpoint);
    }

    if (client.connect(server, port))
    {
        if (enable_debug)
            Serial.println(":eth:TCP connected successfully");

        client.print("GET ");
        client.print(endpoint);
        client.println(" HTTP/1.1");

        client.print("Host: ");
        client.println(server);

        client.println("Connection: close");
        client.println();

        timeout_start = millis();
        headers_ended = false;
        response = "";
        last_char = 0;
        newline_count = 0;
        if (enable_debug)
        {
            Serial.println(":eth:HTTP request sent");
            Serial.print(":eth:Timeout start = ");
            Serial.println(timeout_start);
            Serial.println(":eth:Waiting for response...");
            Serial.println(":eth:------------------------------");
        }
        return true;
    }
    else
    {
        if (enable_debug)
        {
            Serial.println(":eth:TCP connection FAILED");
            Serial.println(":eth:GET DATA ABORTED");
            Serial.println(":eth:------------------------------");
        }
        return false;
    }
}

void ethernet_state::process_response(database_s *db, storage_state *memory)
{

    if (!request_sent)
        return;

    if (!client.connected() || millis() - timeout_start > 5000)
    {
        client.stop();
        request_sent = false;
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
        request_sent = false;
        return;
    }

    // simpan data ke struct
    JsonArray array = doc.as<JsonArray>();
    int idx = 0;
    for (JsonObject item : array)
    {
        int locker = item["no"] | 0;

        if (locker < 0 || locker >= size_mahasiswa)
            continue;

        db[locker].number_locker = locker;
        idx++;
        JsonArray uid = item["id"];

        Serial.println("\nindex locker => " + String(locker));
        for (int i = 0; i < size_uid; i++)
        {
            if (i < uid.size())
            {
                db[locker].card[i] = uid[i];
                Serial.print(db[locker].card[i]);
            }
            else
            {
                db[locker].card[i] = 0;
            }
        }

        db[locker].created_at[0] = '\0';
        db[locker].statusdb = Status_db::Available;
    }
    Serial.print(":eth:DATA OK = ");
    Serial.println(idx);

    if (enable_debug)
    {
        Serial.println(":eth:check database=>");
        for (size_t i = 0; i < size_mahasiswa; i++)
        {
            Serial.print("\n:eth:card index=>" + String(i) + "number locker" + String(db[i].number_locker) + +"card=>");
            for (size_t xp = 0; xp < size_uid; xp++)
            {
                if (xp != 0)
                    Serial.print(',');
                Serial.print(db[i].card[xp]);
            }
        }
    }
    memory->save_data(db); // save data
    client.stop();
    request_sent = false;
}

void ethernet_state::loop_ethernet(database_s *main_data)
{

    if (!initialized)
    {
        if (enable_debug)
        {
            Serial.println(":eth:------------------------------");
            Serial.println(":eth:INITIALIZING ETHERNET");
        }

        uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

        if (Ethernet.begin(mac) == 0)
        {
            if (enable_debug)
                Serial.println("::eth:DHCP gagal");
        }

        if (enable_debug)
        {
            Serial.print(":eth:IP Address = ");
            Serial.println(Ethernet.localIP());
            Serial.print(":eth:Fetch interval = ");
            Serial.print(fetch_interval / 1000);
            Serial.println(" second");
            Serial.println(":eth:ETHERNET INITIALIZED");
            Serial.println(":eth:------------------------------");
        }

        initialized = true;
        request_sent = get_data(main_data);
        last_fetch = millis();
    }

    if ((!request_sent) && millis() - last_fetch >= fetch_interval)
    {
        if (enable_debug)
        {
            Serial.println(":eth:Fetch interval reached");
            Serial.print(":eth:Time elapsed = ");
            Serial.print(millis() - last_fetch);
            Serial.println(" ms");
        }

        request_sent = get_data(main_data);
        last_fetch = millis();
    }
    // process_response(main_data);

    Ethernet.maintain(); // guna dhcp renewww
}
