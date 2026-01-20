#include "commond.h"

void ethernet_state::get_data(database_s *db)
{
    if (enable_debug)
    {
        Serial.println();
        Serial.println(":eth:------------------------------");
        Serial.println(":eth:GET DATA START");
        Serial.print(":eth:Server = ");
        Serial.println(server);
        Serial.print(":eth:Port   = ");
        Serial.println(port);
        Serial.print(":eth:Endpoint = ");
        Serial.println(endpoint);
    }

    if (client.connect(server, port))
    {
        if (enable_debug)
            Serial.println(":eth:TCP connected");

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
            Serial.println(":eth:Waiting response...");
            Serial.println(":eth:------------------------------");
        }
    }
    else
    {
        if (enable_debug)
        {
            Serial.println(":eth:TCP connection FAILED");
            Serial.println(":eth:GET DATA ABORTED");
            Serial.println(":eth:------------------------------");
        }
    }
}


bool ethernet_state::parse_json(String json_data, database_s *db)
{
    int start_idx = json_data.indexOf('[');
    int end_idx = json_data.lastIndexOf(']');

    if (start_idx == -1 || end_idx == -1)
        return false;

    String data = json_data.substring(start_idx + 1, end_idx);
    int current_index = 0;
    int pos = 0;

    // while (pos < (int)data.length() && current_index < size_mahasiswa)
    // {
    //     int obj_start = data.indexOf('{', pos);
    //     int obj_end = data.indexOf('}', pos);

    //     if (obj_start == -1 || obj_end == -1)
    //         break;

    //     String obj = data.substring(obj_start, obj_end + 1);

    //     int locker_start = obj.indexOf("\"locker\":") + 9;
    //     int locker_end = obj.indexOf(",", locker_start);
    //     if (locker_end == -1)
    //         locker_end = obj.indexOf("}", locker_start);
    //     String locker = obj.substring(locker_start, locker_end);
    //     locker.trim();
    //     db[current_index].number_locker = (byte)locker.toInt();

    //     int card_start = obj.indexOf("\"card\":[") + 8;
    //     int card_end = obj.indexOf("]", card_start);
    //     String card_data = obj.substring(card_start, card_end);

    //     int card_idx = 0;
    //     int card_pos = 0;
    //     while (card_pos < (int)card_data.length() && card_idx < size_uid)
    //     {
    //         int comma = card_data.indexOf(',', card_pos);
    //         if (comma == -1)
    //             comma = card_data.length();

    //         String byte_val = card_data.substring(card_pos, comma);
    //         byte_val.trim();
    //         db[current_index].card[card_idx] = (uint8_t)byte_val.toInt();

    //         card_idx++;
    //         card_pos = comma + 1;
    //     }

    //     int created_start = obj.indexOf("\"created_at\":\"") + 14;
    //     int created_end = obj.indexOf("\"", created_start);
    //     String created = obj.substring(created_start, created_end);
    //     created.toCharArray(db[current_index].created_at, sizeof(db[current_index].created_at));

    //     db[current_index].statusdb = Status_db::Available;

    //     if (enable_debug)
    //     {
    //         Serial.print("Loaded locker: ");
    //         Serial.print(db[current_index].number_locker);
    //         Serial.print(" Card: ");
    //         for (int i = 0; i < size_uid; i++)
    //         {
    //             if (db[current_index].card[i] < 0x10)
    //                 Serial.print("0");
    //             Serial.print(db[current_index].card[i], HEX);
    //             Serial.print(" ");
    //         }
    //         Serial.print(" Created: ");
    //         Serial.println(db[current_index].created_at);
    //     }

    //     current_index++;
    //     pos = obj_end + 1;
    // }

    return true;
}

void ethernet_state::process_response(database_s *db)
{
    if (!request_sent)
        return;

    if (!client.connected() || millis() - timeout_start > 5000)
    {
        client.stop();

        if (enable_debug)
        {
            Serial.println("Response:");
            Serial.println(response);
        }

        parse_json(response, db);
        request_sent = false;
        return;
    }

    if (client.available())
    {
        char c = client.read();

        if (!headers_ended)
        {
            if (c == '\n' && last_char == '\r')
            {
                newline_count++;
                if (newline_count >= 2)
                {
                    headers_ended = true;
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
            response += c;
        }
    }
}

void ethernet_state::loop_ethernet(database_s *main_data)
{
    if (!initialized)
    {
        uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

        if (Ethernet.begin(mac) == 0)
        {
            if (enable_debug)
                Serial.println("::eth:DHCP failed, now using static IP");
            // if static
            // IPAddress ip(192, 168, 0, 177);
            // IPAddress dns_server(192, 168, 0, 1);
            // IPAddress gateway(192, 168, 0, 1);
            // IPAddress subnet(255, 255, 255, 0);
            // Ethernet.begin(mac,ip,dns_server,gateway);

            // Ethernet.begin(mac);
        }

        if (enable_debug)
        {
            Serial.print(":eth:IP=> ");
            Serial.println(Ethernet.localIP());
        }

        initialized = true;
        get_data(main_data);
        last_fetch = millis();
    }

    // process_response(main_data);

    if (!request_sent && millis() - last_fetch >= fetch_interval)
    {
        // get_data(main_data);
        last_fetch = millis();
    }

    Ethernet.maintain();
}