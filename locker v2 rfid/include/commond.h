#ifndef COMMOND_H
#define COMMOND_H

#include <Arduino.h>
#include <SdFat.h>
#include <../lib/Ethernet-2.0.2/src/Ethernet.h>

#define sizeRelay 32
const int pin_IO[sizeRelay] = {9, 23, 24, 25, 26, 27,
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
#define PIN_buzzer 22


// RFID card definitions
#define total_card_rfid 30
#define size_mahasiswa 30
#define size_uid 12

enum class action_Card : uint8_t
{
    Register = 0x01,
    Remove,
    Equal,
    Read,
    Write,
    None
};
enum class Status_db : uint8_t
{
    Available = 0x64,
    Not_Available
};

enum class acc_mode : uint8_t
{
    mode_fastloop2X = 0xCC,
    mode_fastloop4X,
    mode_fastloop8X,
};

enum class acc_action : uint8_t
{
    acc_on = 0x28,
    acc_off
};
struct acc_state
{
    acc_mode mode;
    byte pin;
    byte flip_flop;
    acc_action act;
    unsigned long Interval;
    unsigned long LastOn;
    void main();
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
    bool shoow_rl;
};

#include <Adafruit_PN532.h>
struct rfid_state
{
    bool enable_debug = false;
    action_Card action = action_Card::None;
    char read_crd(const database_s *data, Adafruit_PN532 *nfc, Relay_state *rl);
    signed char registered(const database_s *db);
    bool open_doors(Relay_state *rl);
    long interval;
    uint8_t size_uid_incoming = size_uid;
    uint8_t uid_incoming[size_uid];
    rfid_state::rfid_state(const long interval_read);
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
    bool get_data(database_s *db);
    void process_response(database_s *db, storage_state *memory);
    bool enable_debug;

    EthernetClient client;
    char server[50] = "192.168.100.11";
    // char server[50] = "192.168.100.9";
    int port = 8000;
    // int port = 5000;
    String endpoint = "/siswa.json";
    unsigned long last_fetch = 0;
    unsigned long fetch_interval = 60000;
    bool initialized = false;
    bool get_new_data = false, last_get_data = false;
    bool request_sent = false;
    unsigned long timeout_start = 0;
    bool headers_ended = false;
    String response = "";
    char last_char = 0;
    int newline_count = 0;
};

#endif
