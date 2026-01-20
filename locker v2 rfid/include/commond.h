#ifndef COMMOND_H
#define COMMOND_H

#include <Arduino.h>
#include <SdFat.h>
#include <../lib/Ethernet-2.0.2/src/Ethernet.h>
#include <ArduinoJson.h>

#define sizeRelay 32
const int pin_IO[sizeRelay] = {22, 23, 24, 25, 26, 27,
                               28, 29, 30, 31, 32, 33,
                               34, 35, 36, 37, 38, 39,
                               40, 41, 42, 43, 44, 45,
                               46, 47, 48, 49, 50, 51, 52, 53};

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
#define PIN_led 12
#define PIN_buzzer 13

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

#define flipflopinterval01 50
#define flipflopinterval02 75
#define flipflopinterval03 150
#define flipflopinterval04 250
#define flipflopinterval05 550
#define flipflopinterval05 1000

// RFID card definitions
#define total_card_rfid 30
#define size_mahasiswa 30
#define size_uid 12
enum class action_Card
{
    Register = 0x01,
    Remove,
    Equal,
    Read,
    Write,
    None
};
enum class Status_db
{
    Available = 0x01,
    Not_Available
};

enum class ac_status
{
    state_ON = 0x121,
    state_ON_fastloop4X,
    state_ON_fastloop,
    state_OFF
};

struct Aksesoris_state
{
    byte pin;
    unsigned long Interval;
    unsigned long LastOn;
    ac_status status;
    void on(int Ringetone = 3000);
};

enum class Status_RL
{
    ON = 0x13,
    OFF
};

struct database_s
{
    Status_db statusdb = Status_db::Not_Available;
    uint8_t card[size_uid];
    byte number_locker;
    char created_at[25];
};
struct Relay_state
{
    int interval;
    Status_RL status;
    byte pin;
    unsigned long last_t;
};

#include <Adafruit_PN532.h>
struct rfid_state
{
    bool enable_debug = false;
    action_Card action = action_Card::None;
    char read_crd(const database_s *data, Adafruit_PN532 *nfc, Relay_state *rl);
    signed char registered(const database_s *db);
    void open_doors(Relay_state *rl);
    uint8_t size_uid_incoming = size_uid;
    uint8_t uid_incoming[size_uid];
};
struct storage_state
{
    int SIZE_MEMORY;
    bool enable_debug = false;
    bool load_data(database_s *db);
    bool save_data(database_s *db);
    void factory_reset(database_s *db);
};
struct ethernet_state
{
    void loop_ethernet(database_s *main_data);
    bool enable_debug;
};

#endif