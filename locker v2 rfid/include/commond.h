#ifndef COMMOND_H
#define COMMOND_H
// #define DEBUG_ETH
// #define DEBUG_MEM
// #define DEBUG_ACC
// #define DEBUG_RFID
#define read_little_end
#include <Arduino.h>
#include <SdFat.h>
#include <../lib/Ethernet-2.0.2/src/Ethernet.h>
#define sizeRelay 32
const int pin_IO[sizeRelay] = {22, 23, 24, 25, 26, 27,
                               28, 29, 30, 31, 32, 33,
                               34, 35, 36, 37, 38, 39,
                               40, 41, 42, 43, 44, 45,
                               46, 47, 48, 49, 50, 51, 52, 53};
//    0028758077
//    2788524568
//    0563089154
//    0038865439

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

#define CS_SD 2

#define PIN_led 68
#define PIN_buzzer 69

#define total_card_rfid 30
#define size_mahasiswa 30
#define size_uid 12

enum class Status_db : uint8_t
{
    Available = 0x64,
    Not_Available
};

enum class acc_mode : uint8_t
{
    mode_fastloop3X = 0x06,
    mode_fastloop4X = 0x09,
    mode_fastloop8X = 0x10,
    denide
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
    acc_action act;
    unsigned long Interval;
    unsigned int freq;

    byte flip_flop;
    bool buzzerState;
    unsigned long LastOn;

    void main();

    uint8_t getMaxFlip();
    void toggleBuzzer();
    void stopBuzzer();
    void resetState(unsigned long now);
    void handleDenide(unsigned long now);
    acc_state::acc_state(int feq) : freq(feq) {}
};
;

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
    char read_crd(const database_s *data, Adafruit_PN532 *nfc, Relay_state *rl);
    signed char card_isregistered(const database_s *db);
    bool open_doors(Relay_state *rl);
    long interval;
    uint8_t size_uid_incoming = size_uid;
    uint8_t uid_incoming[size_uid];
    void init_sensor(Adafruit_PN532 *nfc);
    rfid_state::rfid_state(const long interval_read);

#ifdef little_endian

#endif
};
struct storage_state
{
    int SIZE_MEMORY;

    bool load_data(database_s *db);
    bool save_data(database_s *db, database_s *new_db);

    void factory_reset(database_s *db);
};
#define S_MAC 6
#include <auth.h>
struct ethernet_state
{
private:
    String auth_header;
    char method[8];
    char path[32];
    bool header_done;
    char last_char;
    byte mac_L0002[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x5C};
    const char *device_class = "A1";
    const char *controller_name = "L0002";
    char *location = "not_set";
    EthernetServer server = EthernetServer(8000);

public:
    auth_state auth;
    void begin(database_s *db);
    void loop(database_s *db, storage_state *memory);

    /* request handling */
    void handle_client(EthernetClient &client,
                       database_s *db,
                       storage_state *memory);
    bool parse_request(EthernetClient &client);
    void reset_parser();

    /* endpoint handlers */
    void handle_info(EthernetClient &client, database_s *db, storage_state *memory, bool update = false);
    void handle_info(EthernetClient &client);
    void handle_get_data(EthernetClient &client, database_s *db);
    void handle_post_student(EthernetClient &client,
                             database_s *db,
                             storage_state *memory);
    void handle_delete_student(EthernetClient &client,
                               database_s *db,
                               storage_state *memory);
    void handle_reset(EthernetClient &client,
                      database_s *db,
                      storage_state *memory);

    /* response helpers */
    void send_ok(EthernetClient &client, const char *json = "{}");
    void send_error(EthernetClient &client, int code, const char *msg);

    int newline_count;
    static const size_t BODY_SIZE = 2048;
    char body[BODY_SIZE];
    size_t body_len;
    unsigned long start_time;

    database_s *db_ptr = nullptr;
};

#endif
// #include <avr/wdt.h>
// void software_Reset() {
//   wdt_enable(WDTO_15MS); // Enable watchdog with 15ms timeout
//   while(1); // Wait for reset
// }
