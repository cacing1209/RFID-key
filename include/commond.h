#ifndef COMMOND_H
#define COMMOND_H
#include <Arduino.h>
#include <SdFat.h>
#include <ArduinoJson.h>

#define sizeRelay 32
const int pin_IO[sizeRelay] = {22, 23, 24, 25, 26, 27,
                               28, 29, 30, 31, 32, 33,
                               34, 35, 36, 37, 38, 39,
                               40, 41, 42, 43, 44, 45,
                               46, 47, 48, 49, 8, 9, 10, 11};

/* #pinout#
 * PN532 RFID menggunakan I2C:
 * SDA = Pin 20 (Arduino Mega)
 * SCL = Pin 21 (Arduino Mega)
 * 
 * SD Card menggunakan SPI:
 * SPI MOSI    MOSI         51
 * SPI MISO    MISO         50
 * SPI SCK     SCK          52
 * CS          CS_SD        2
 * */

// Pin definitions yang masih digunakan untuk SD card
#define CS_SD 2

// Pin definitions untuk aksesori
#define PIN_led 13
#define PIN_buzzer 12

// Tone definitions
#define Tone00 0
#define Tone01 300
#define Tone02 400
#define Tone03 500
#define Tone04 600
#define Tone05 700
#define Tone06 800
#define Tone07 900
#define Tone08 1000

// LED flash definitions
#define flopflopLed01 1
#define flopflopLed02 2
#define flopflopLed03 3
#define flopflopLed04 4
#define flopflopLed05 5

// RFID card definitions
#define total_card_rfid 30
#define size_rfid 10

enum action_Card
{
    Register,
    Remove,
    Equal,
    None
};

enum status
{
    state_ON = 0x121,
    state_ON_fastloop,
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

enum action_sdCard
{
    sv_data,
    ch_data,
    idle
};

struct rfid_state
{
    byte card[total_card_rfid][size_rfid];
    action_Card action = None;
    String CardRegister;
};

struct Data_state
{
    action_sdCard action;
    File32 file;
    SdFat32 *sd;
    rfid_state &rfid;

    void save_data(const char *filename = "/siswa.json");
    void load_data(const char *filename = "/siswa.json");
    Data_state::Data_state(SdFat32 *sdf, rfid_state &rfidf) : sd(sdf), rfid(rfidf) {}
};

#endif