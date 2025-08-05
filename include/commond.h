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
                               46, 47, 48, 49, 6, 7, 8, 53};
/* #pinout#
 * rfid chip select for slave
 * sdcard chip select for slave

 * card detect
 * SPI MOSI    MOSI         51
 * SPI MISO    MISO         50
 * SPI SCK     SCK          52
 * */

#define RSTPIN_RFID 4
#define SS_RFID 3
#define CS_SD 2
#define PIN_led 13
#define PIN_buzzer 12
#define Tone00 0
#define Tone01 300
#define Tone02 400
#define Tone03 500
#define Tone04 600
#define Tone05 700
#define Tone06 800
#define Tone07 900
#define Tone08 1000

#define flopflopLed01 1
#define flopflopLed02 2
#define flopflopLed03 3
#define flopflopLed04 4
#define flopflopLed05 5

enum status
{
    state_ON = 0x121,
    state_OFF
};
struct Aksesoris_state
{
    byte pin;
    unsigned long Interval;
    unsigned long LastOn;
    status Status;
    void on(int Ringetone = 3000);
};

enum Status_RL
{
    ON = 0x13,
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
struct rfid_state;

struct DB_STATE

{
    JsonDocument doc;
    SdFat32 *sd;
    File32 file;
    rfid_state *rfid;
    bool check_Openedfile(const char *filename);
    void save_data(const char *filename);
    bool load_data(const char *filename = "/mahasiswa.json");
    void new_file();
    void Remove_file();
    void Replace_file();
};

#define total_card_rfid 20
#define size_rfid 10
enum action_Card
{
    Register,
    Remove,
    Equal,
    None
};
struct rfid_state
{
    byte card[total_card_rfid][size_rfid];
    action_Card action = None;
    String CardRegister;
};

// struct Serial_state
// {
// };

#endif