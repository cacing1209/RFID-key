#ifndef SAVE_F_H
#define SAVE_F_H
#include <Arduino.h>
#include <EEPROM.h>
#define size_EEPROM 512

struct save_f_state
{
    bool reset;
    void save_data(int addr, const String &data);
    bool load_data(int addr_rst,int addr_ssid, int addr_psw, String &ssid, String &psw);
    void begin(const int sizeADDR = size_EEPROM);
    void save_data_reset(int addr, const bool &data = false);
};

#endif
