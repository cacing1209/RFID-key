#include <commond.h>

void get_data(database_s *db)
{
}
void ethernet_state::loop_ethernet(database_s *main_data)
{
    uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    uint8_t ip[] = {192, 168, 0, 1};
    Ethernet.begin(mac, ip);
    if (enable_debug)
    {
        Serial.println("ip_local" + String(Ethernet.localIP()));
    }
}