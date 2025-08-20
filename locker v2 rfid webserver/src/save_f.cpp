#include <save_f.h>

void save_f_state::begin(const int sizeADDR)
{
    EEPROM.begin(sizeADDR);
    return;
}
int saveString(int addr, const String &data)
{
    byte len = data.length();
    EEPROM.write(addr, len);
    for (int i = 0; i < len; i++)
    {
        EEPROM.write(addr + 1 + i, data[i]);
    }
    return addr + 1 + len;
}

String readString(int addr)
{
    byte len = EEPROM.read(addr);
    if (len == 0xFF || len == 0)
        return "";
    char buf[len + 1];
    for (int i = 0; i < len; i++)
    {
        buf[i] = EEPROM.read(addr + 1 + i);
    }
    buf[len] = '\0';
    return String(buf);
}

void save_f_state::save_data(int addr, const String &data)
{
    Serial.print("save: ");
    Serial.println("EEPROM EXECUTED");
    saveString(addr, data);
    EEPROM.commit();
}

bool save_f_state::load_data(int addr_rst,int addr_ssid, int addr_psw, String &ssid, String &psw)
{
    bool resetFlag = EEPROM.read(addr_rst);
    if (resetFlag == true)
    {
        Serial.print("reset:");
        Serial.println(resetFlag);
        return false;
    }
    
    ssid = readString(addr_ssid);
    if (ssid.length() == 0)
    {
        Serial.print("ssid:");
        Serial.println("no ssid");
        return false;
    }
    psw = readString(addr_psw);
    if (psw.length() == 0)
    {
        Serial.print("psw:");
        Serial.println("no psw");
        return false;
    }
    
    Serial.print("return:");
    Serial.println(1);
    return true;
}
void save_f_state::save_data_reset(int addr, const bool &data)
{
    bool resetFlag = EEPROM.read(addr);
    Serial.print("reset IS: ");
    Serial.print(resetFlag);
    Serial.println(" EEPROM EXECUTED");
    EEPROM.write(addr, data);
    EEPROM.commit();
    Serial.print("reset status now: ");
    Serial.println(EEPROM.read(addr));
}
