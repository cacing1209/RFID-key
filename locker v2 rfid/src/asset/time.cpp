#include "commond.h"
void NTPConfig::update(int interval_sync)
{
    unsigned long last_sync = 0;
    const unsigned long now = millis();
    if (now - last_sync > interval_sync)
    {
        getNTPTime();
        last_sync = now;
    }
}
void NTPConfig::sendNTPpacket()
{
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0b11100011;
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
        setTime(secsSince1900 - seventyYears + utcOffsetSeconds);
        return secsSince1900 - seventyYears + utcOffsetSeconds;
    }
    setTime(0);
    return 0;
}