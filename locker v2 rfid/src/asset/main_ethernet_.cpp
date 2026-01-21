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

        // Send HTTP GET request
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

// This function is now integrated into process_response()
// Keeping it here for backward compatibility if needed elsewhere
bool ethernet_state::parse_json(String json_data, database_s *db)
{
    // Function body moved to process_response() for better memory management
    return false;
}

void ethernet_state::process_response(database_s *db)
{
    if (!request_sent)
        return;

    // Check timeout or connection closed
    if (!client.connected() || millis() - timeout_start > 5000)
    {
        client.stop();

        if (enable_debug)
        {
            Serial.println(":eth:------------------------------");
            Serial.println(":eth:RESPONSE RECEIVED");
            Serial.print(":eth:Response time = ");
            Serial.print(millis() - timeout_start);
            Serial.println(" ms");
            Serial.print(":eth:Response length = ");
            Serial.println(response.length());
            Serial.println(":eth:Raw Response:");
            Serial.println(response);
            Serial.println(":eth:------------------------------");
        }

        if (response.length() > 0)
        {
            // Parse immediately while response still has data
            if (enable_debug)
            {
                Serial.println(":eth:------------------------------");
                Serial.println(":eth:PARSING JSON START");
                Serial.print(":eth:JSON Length = ");
                Serial.println(response.length());
            }

            StaticJsonDocument<4096> doc;
            DeserializationError error = deserializeJson(doc, response);

            if (error)
            {
                if (enable_debug)
                {
                    Serial.print(":eth:JSON parsing FAILED: ");
                    Serial.println(error.c_str());
                    Serial.println(":eth:------------------------------");
                }
            }
            else
            {
                if (enable_debug)
                    Serial.println(":eth:JSON parsing SUCCESS");

                JsonArray array = doc.as<JsonArray>();
                int loaded_count = 0;

                for (JsonObject item : array)
                {
                    if (loaded_count >= size_mahasiswa)
                    {
                        if (enable_debug)
                            Serial.println(":eth:Database full, stopping parse");
                        break;
                    }

                    // Get nomor_absen (akan jadi number_locker)
                    if (item.containsKey("nomor_absen"))
                    {
                        db[loaded_count].number_locker = item["nomor_absen"];
                    }
                    else
                    {
                        if (enable_debug)
                            Serial.println(":eth:Missing nomor_absen field, skipping");
                        continue;
                    }

                    // Get uid_card array
                    if (item.containsKey("uid_card"))
                    {
                        JsonArray card_array = item["uid_card"];
                        int card_idx = 0;
                        
                        for (JsonVariant value : card_array)
                        {
                            if (card_idx >= size_uid)
                                break;
                                
                            db[loaded_count].card[card_idx] = value.as<uint8_t>();
                            card_idx++;
                        }

                        // Fill remaining with zeros if card array is smaller
                        while (card_idx < size_uid)
                        {
                            db[loaded_count].card[card_idx] = 0;
                            card_idx++;
                        }
                    }
                    else
                    {
                        if (enable_debug)
                            Serial.println(":eth:Missing uid_card field, skipping");
                        continue;
                    }

                    // Get created_at (optional)
                    if (item.containsKey("created_at"))
                    {
                        const char* created = item["created_at"];
                        strncpy(db[loaded_count].created_at, created, sizeof(db[loaded_count].created_at) - 1);
                        db[loaded_count].created_at[sizeof(db[loaded_count].created_at) - 1] = '\0';
                    }
                    else
                    {
                        strcpy(db[loaded_count].created_at, "N/A");
                    }

                    // Set status as available
                    db[loaded_count].statusdb = Status_db::Available;

                    if (enable_debug)
                    {
                        Serial.println(":eth:------------------------------");
                        Serial.print(":eth:Loaded Entry #");
                        Serial.println(loaded_count + 1);
                        Serial.print(":eth:Locker Number = ");
                        Serial.println(db[loaded_count].number_locker);
                        Serial.print(":eth:Card UID = ");
                        for (int i = 0; i < size_uid; i++)
                        {
                            if (db[loaded_count].card[i] < 0x10)
                                Serial.print("0");
                            Serial.print(db[loaded_count].card[i], HEX);
                            Serial.print(" ");
                        }
                        Serial.println();
                        Serial.print(":eth:Created At = ");
                        Serial.println(db[loaded_count].created_at);
                    }

                    loaded_count++;
                }

                if (enable_debug)
                {
                    Serial.println(":eth:------------------------------");
                    Serial.print(":eth:Total entries loaded = ");
                    Serial.println(loaded_count);
                    Serial.println(":eth:PARSING JSON COMPLETE");
                    Serial.println(":eth:------------------------------");
                }
            }
        }
        else
        {
            if (enable_debug)
                Serial.println(":eth:Empty response received");
        }

        // Clear response after processing
        response = "";
        request_sent = false;
        return;
    }

    // Read incoming data
    while (client.available())
    {
        char c = client.read();

        if (!headers_ended)
        {
            // Skip HTTP headers
            if (c == '\n' && last_char == '\r')
            {
                newline_count++;
                if (newline_count >= 2)
                {
                    headers_ended = true;
                    if (enable_debug)
                        Serial.println(":eth:Headers ended, reading body...");
                }
            }
            else if (c != '\r' && c != '\n')
            {
                newline_count = 0;
            }
            last_char = c;
        }
        else
        {
            // Append body content
            response += c;
        }
    }
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
            
            // Uncomment for static IP fallback
            // IPAddress ip(192, 168, 0, 177);
            // IPAddress dns_server(192, 168, 0, 1);
            // IPAddress gateway(192, 168, 0, 1);
            // IPAddress subnet(255, 255, 255, 0);
            // Ethernet.begin(mac, ip, dns_server, gateway, subnet);
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

    // Process incoming response
    process_response(main_data);

    // Periodic fetch
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

    // Maintain DHCP lease
    Ethernet.maintain();
}