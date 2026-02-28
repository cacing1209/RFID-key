#ifndef COMMOND_H
#define COMMOND_H
#define DEBUG_ETH
#define DEBUG_MEM
// #define DEBUG_ACC
// #define DEBUG_RFID
#define DEBUG_TIME

// #define modelL0002
// #define modelL0004
// #define modelL0016
// #define modelL0032
#define modelL0128

#define read_little_end
#include <Arduino.h>
#include <SdFat.h>
#include <Ethernet.h>
#include <TimeLib.h>
#include <EthernetUdp.h>

// #include <../lib/Ethernet-2.0.2/src/Ethernet.h>
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
    mode_fastloop1X = 0x06,
    mode_fastloop2X,
    mode_fastloop3X,
    mode_fastloop4X,
    mode_fastloop8X,
    denide
};

enum class acc_action : uint8_t
{
    acc_on = 0x28,
    acc_off
};
struct buzzer_state
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
    buzzer_state::buzzer_state(int feq) : freq(feq) {}
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
    bool reset_t;
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

// === NTP Config ===
const int NTP_PACKET_SIZE = 48;
struct NTPConfig
{
    IPAddress timeServer;
    byte packetBuffer[NTP_PACKET_SIZE];
    EthernetUDP Udp;
    unsigned int localPort;
    void sendNTPpacket();
    unsigned long getNTPTime();
    void update(int interval_sync = 60000);
    // WIB  (UTC+7)
    const long utcOffsetSeconds = 7 * 3600;
    // WITA (UTC+8)
    // const long utcOffsetSeconds = 8 * 3600;
    // WIT  (UTC+9)
    // const long utcOffsetSeconds = 9 * 3600;
    NTPConfig() : timeServer(216, 239, 35, 0), localPort(8888)
    {
        memset(packetBuffer, 0, NTP_PACKET_SIZE);
    }
};

#define S_MAC 6
#include <auth.h>
struct ethernet_state
{
private:
private:
    bool eth_connected = false;
    unsigned long last_maintain = 0;
    unsigned long last_reconnect = 0;
    static const unsigned long MAINTAIN_INTERVAL = 500;
    static const unsigned long RECONNECT_INTERVAL = 5000;
    String auth_header;
    char method[8];
    char path[32];
    bool header_done;
    char last_char;

#define COUNT_MODELS (              \
    (defined(modelL0002) ? 1 : 0) + \
    (defined(modelL0004) ? 1 : 0) + \
    (defined(modelL0016) ? 1 : 0) + \
    (defined(modelL0032) ? 1 : 0) + \
    (defined(modelL0064) ? 1 : 0) + \
    (defined(modelL0128) ? 1 : 0) + \
    (defined(modelL0256) ? 1 : 0) + \
    (defined(modelL0512) ? 1 : 0))

#if COUNT_MODELS == 0
#error "Error: DEFINE MODEL SEK SUU!"
#elif COUNT_MODELS > 1
#error "Error: PILIH SATU AJA NDENG GENDENG!"
#endif

#ifdef modelL0002
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x5C};
    const char *controller_name = "L0002";
#elif defined(modelL0004)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x5D};
    const char *controller_name = "L0004";
#elif defined(modelL0016)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x5E};
#elif defined(modelL0032)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x5F};
#elif defined(modelL0064)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x60};
#elif defined(modelL0128)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x61};
    const char *controller_name = "L0128";
#elif defined(modelL0256)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x62};
#elif defined(modelL0512)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x63};
#endif

    const char *device_class = "A1";
    char *location = "not_set";
    EthernetServer server = EthernetServer(8000);
    char *server_log;
    int portServer_log;

public:
    auth_state auth;
    void begin(database_s *db);
    void loop(database_s *db, storage_state *memory, Relay_state *locker);

    /* request handling */
    void handle_client(EthernetClient &client, database_s *db, storage_state *memory, Relay_state *locker);
    bool parse_request(EthernetClient &client);
    void reset_parser();

    /* endpoint handlers */
    // void handle_info(EthernetClient &client, database_s *db, storage_state *memory, bool update = false);
    void handle_info(EthernetClient &client);
    void handle_get_data(EthernetClient &client, database_s *db);
    void handle_post_student(EthernetClient &client,
                             database_s *db,
                             storage_state *memory);
    void handle_delete_student(EthernetClient &client,
                               database_s *db,
                               storage_state *memory, byte id);
    void handle_reset(EthernetClient &client,
                      database_s *db,
                      storage_state *memory);
    void handle_open_locker(EthernetClient &client, Relay_state *rl, byte locker);
    void handle_get_student(EthernetClient &client, database_s *db, byte locker_no);

    /* response helpers */
    void send_ok(EthernetClient &client, const char *json = "{}");
    void send_error(EthernetClient &client, int code, const char *msg);
    void send_eventLog(const unsigned long uid_decimal, byte index);

    int newline_count;
    static const size_t BODY_SIZE = 2048;
    char body[BODY_SIZE];
    size_t body_len;
    unsigned long start_time;

    database_s *db_ptr = nullptr;
};
#include <avr/wdt.h>
struct system_d
{
    void software_Reset()
    {
        wdt_enable(WDTO_15MS);
        while (1)
        {
        }
    }
};
extern system_d sys;
#endif
