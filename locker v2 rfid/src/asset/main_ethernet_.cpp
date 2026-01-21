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

        request_sent = true;
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

bool ethernet_state::parse_json(String json_data, database_s *db)
{
    return false;
}

void ethernet_state::process_response(database_s *db)
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
                    if (enable_debug)
                        Serial.println(":eth:Headers ended, start JSON parse");
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

    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, client);

    if (error)
    {
        if (enable_debug)
        {
            Serial.print(":eth:JSON parsing FAILED: ");
            Serial.println(error.c_str());
        }
        client.stop();
        request_sent = false;
        return;
    }

    JsonArray array = doc.as<JsonArray>();
    int idx = 0;

    for (JsonObject item : array)
    {
        if (idx >= size_mahasiswa)
            break;

        db[idx].number_locker = item["nomor_absen"] | 0;

        JsonArray uid = item["uid_card"];
        for (int i = 0; i < size_uid; i++)
            db[idx].card[i] = uid[i] | 0;

        db[idx].statusdb = Status_db::Available;
        idx++;
    }

    if (enable_debug)
    {
        Serial.print(":eth:DATA LOADED = ");
        Serial.println(idx);
    }

    client.stop();
    request_sent = false;
}

void ethernet_state::loop_ethernet(database_s *main_data)
{
    static bool connection = false;

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
                Serial.println(":eth:DHCP failed, trying again...");
        }

        if (enable_debug)
        {
            Serial.print(":eth:IP Address = ");
            Serial.println(Ethernet.localIP());
            Serial.print(":eth:Fetch interval = ");
            Serial.print(fetch_interval);
            Serial.println(" ms");
            Serial.println(":eth:ETHERNET INITIALIZED");
            Serial.println(":eth:------------------------------");
        }

        initialized = true;
        connection = get_data(main_data);
        last_fetch = millis();
    }

    if (!connection)
        return;

    process_response(main_data);

    if (!request_sent && millis() - last_fetch >= fetch_interval)
    {
        if (enable_debug)
        {
            Serial.println(":eth:Fetch interval reached");
            Serial.print(":eth:Time elapsed = ");
            Serial.print(millis() - last_fetch);
            Serial.println(" ms");
        }

        connection = get_data(main_data);
        last_fetch = millis();
    }

    Ethernet.maintain();
}
