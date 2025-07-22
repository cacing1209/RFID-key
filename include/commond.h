#ifndef COMMOND_H
#define COMMOND_H
#include <Arduino.h>
#include <MFRC522.h>
#include <SdFat.h>
#include <ArduinoJson.h>
#define sizeRelay 32
const int pin_IO[sizeRelay] = {22, 23, 24, 25, 26, 27,
                               28, 29, 30, 31, 32, 33,
                               34, 35, 36, 37, 38, 39,
                               40, 41, 42, 43, 44, 45,
                               46, 47, 48, 49, 50, 51, 52, 53};

/* #pinout#
 * rfid chip select for slave
 * sdcard chip select for slave
 * card detect
 * */
#define SSPIN 10
#define RSTPIN_RFID 4
#define CS_RFID 2
#define CS_SD 3
#define CD_SD 5

enum Status_RL
{
    ON,
    OFF
};
struct Relay_state
{
    Status_RL status;
    byte pin;
    unsigned long Last_ON;
    unsigned long TimeON;
};

struct data_local
{
    String payload;
};

struct DB_STATE

{
    JsonDocument doc;
    SdFat32 *sd;
    File32 file;
    bool check_Openedfile(const char *filename);
    void save_data(const char *filename);
    void load_data(const char *filename);
    void new_file();
    void Remove_file();
    void Replace_file();
};

#endif