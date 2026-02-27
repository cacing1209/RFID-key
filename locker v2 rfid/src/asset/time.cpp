#include "commond.h"

void NTPConfig::sendNTPpacket()
{
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0b11100011;
    // ...
    Udp.beginPacket(timeServer, 123);
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}

unsigned long NTPConfig::getNTPTime()
{
    sendNTPpacket();
    delay(1500);

    if (Udp.parsePacket())
    {
        Udp.read(packetBuffer, NTP_PACKET_SIZE);
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        const unsigned long seventyYears = 2208988800UL;
        return secsSince1900 - seventyYears + utcOffsetSeconds;
    }
    return 0;
}