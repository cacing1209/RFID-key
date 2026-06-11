#include "commond.h"
#ifndef find_p
void NTPConfig::update(int interval_sync)
{
    // BUG FIX: last_sync DULU lokal (selalu 0) -> kondisi di bawah true terus
    // sehabis uptime 60s -> getNTPTime() (yang delay(1000)!) kepanggil TIAP
    // loop -> controller lemot ~1 request/detik, scan & dashboard app sering
    // ke-timeout (request ke-proses tapi telat, app keburu nyerah).
    // Jadiin static: resync NTP cuma tiap `interval_sync` (default 60s). Sekalian
    // beneran update jam-nya (dulu hasil getNTPTime() kebuang percuma).
    static unsigned long last_sync = 0;
    const unsigned long now = millis();
    if (now - last_sync > (unsigned long)interval_sync)
    {
        last_sync = now;
        unsigned long t = getNTPTime();
        if (t)
            setTime(t);
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
    delay(1000);

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
#endif