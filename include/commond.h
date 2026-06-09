#ifndef COMMOND_H
#define COMMOND_H
// #define find_p
// #define DEBUG_ETH
// #define DEBUG_RFID
// #define DEBUG_MEM
// #define DEBUG_ACC
// #define DEBUG_TIME
// #define DEBUG_SYS

/*selesai*/
// #define modelJ505
// #define modelJ506
// #define modelJ508
// #define modelJ507


// #define modelL0064
// #define modelL0512
// #define modelL0128
#define modelJ504

#define read_little_end
#include <Arduino.h>

// #include <../lib/Ethernet-2.0.2/src/Ethernet.h>
#define sizeRelay 32

// Log server config — DNS-resolved at runtime so server IP can change
// without re-flashing firmware. Update DNS A record only.
#define LOG_SERVER_HOST "locker-logs.qyubit.io"
#define LOG_SERVER_PORT 80
#define LOG_SERVER_DNS_TTL_MS (6UL * 60UL * 60UL * 1000UL)
#define LOG_SERVER_FALLBACK_DNS_A 8
#define LOG_SERVER_FALLBACK_DNS_B 8
#define LOG_SERVER_FALLBACK_DNS_C 8
#define LOG_SERVER_FALLBACK_DNS_D 8

#define COUNT_MODELS (              \
    (defined(modelJ508) ? 1 : 0) +       \
    (defined(modelJ506) ? 1 : 0) + \
    (defined(modelJ504) ? 1 : 0) + \
    (defined(modelJ505) ? 1 : 0) + \
    (defined(modelL0064) ? 1 : 0) + \
    (defined(modelL0128) ? 1 : 0) + \
    (defined(modelJ507) ? 1 : 0) + \
    (defined(modelL0512) ? 1 : 0))

#if COUNT_MODELS == 0
#error "Error: DEFINE MODEL SEK SUU!"
#elif COUNT_MODELS > 1
#error "Error: PILIH SATU AJA NDENG GENDENG!"
#endif

#ifdef modelJ508
const int pin_IO[sizeRelay] = {29, 33, 43, 25, 34, 44, 28, 63, 49, 38, 62, 40, 24, 39, 46, 64, 31, 37, 35, 23, 41, 48, 22, 45, 32, 30, 26, 65, 42, 47, 27, 36};
#elif defined(modelJ507)
const int pin_IO[sizeRelay] = {26, 29, 22, 23, 41, 47, 31, 45, 63, 25, 34, 64, 49, 65, 28, 40, 27, 33, 48, 30, 46, 42, 35, 39, 44, 43, 62, 37, 38, 36, 24, 32};
#elif defined(modelL0512)
const int pin_IO[sizeRelay] = {35, 22, 65, 26, 44, 45, 62, 48, 31, 38, 37, 23, 33, 39, 28, 43, 42, 29, 25, 40, 32, 64, 30, 46, 34, 27, 49, 41, 24, 47, 36, 63};
#elif defined(modelJ504)
const int pin_IO[sizeRelay] = {47, 41, 23, 38, 46, 31, 35, 27, 64, 43, 44, 33, 29, 49, 37, 25, 26, 34, 24, 63, 28, 30, 32, 22, 39, 45, 65, 40, 48, 42, 62, 36};
#elif defined(modelJ505)
const int pin_IO[sizeRelay] = {37, 36, 24, 33, 48, 26, 62, 44, 41, 47, 38, 32, 22, 30, 45, 65, 43, 29, 35, 40, 64, 49, 39, 25, 63, 23, 31, 27, 42, 28, 34, 46};

#elif defined(modelJ506)
const int pin_IO[sizeRelay] = {29, 28, 26, 30, 32, 27, 65, 43, 22, 47, 41, 42, 39, 45, 34, 40, 62, 24, 31, 25, 48, 33, 49, 38, 37, 36, 35, 64, 23, 44, 46, 63};
#elif defined(modelL0064)
const int pin_IO[sizeRelay] = {36, 65, 44, 49, 23, 37, 30, 24, 31, 22, 35, 25, 43, 26, 28, 45, 62, 39, 42, 38, 47, 48, 64, 41, 63, 33, 46, 32, 40, 34, 29, 27};
#elif defined(modelL0128)
const int pin_IO[sizeRelay] = {49, 39, 29, 35, 37, 25, 30, 44, 26, 42, 63, 48, 41, 22, 43, 28, 23, 62, 45, 27, 65, 46, 32, 34, 36, 24, 64, 47, 38, 31, 40, 33};
#endif
// 50 -> 62
// 51 -> 63
// 52 -> 64
// 53 -> 65

// sangat membantu untuk mapping awal relay pin
// tinggal define ajah
#ifdef find_p
struct mapping_p
{
    bool bypass;
    byte pin[sizeRelay];
    void begin();
    void main();
    bool pins_avaiable();
    void shorting_pins();
};
#else
#include <Ethernet.h>
#include <TimeLib.h>
#include <EthernetUdp.h>
#define eth_cs 10
/* #pinout#
 * PN532 RFID menggunakan I2C:
 * SDA = Pin 20 (Arduino Mega)
 * SCL = Pin 21 (Arduino Mega)
 *

 * */

// #define CS_SD 2

#define PIN_led 68
#define PIN_buzzer 69

#define total_card_rfid 30
#define Size_Siswa 30
#define size_uid 12

enum class Status_db : uint8_t
{
    Available = 0x64,
    Not_Available
};

enum class bz_mode : uint8_t
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
    bz_mode mode;
    byte pin;
    acc_action act;
    unsigned long Interval;

    byte flip_flop;
    bool buzzerState;
    unsigned long LastOn;

    bool in_action();

    uint8_t getMaxFlip();
    void toggleBuzzer();
    void stopBuzzer();
    void resetState(unsigned long now);
    void handleDenide(unsigned long now);
    buzzer_state::buzzer_state(int i_bz) : Interval(i_bz) {}
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
};
struct Relay_state
{
    int interval;
    Status_RL status;
    byte pin;
    unsigned long last_t;
    bool open_loker;
};

#include <Adafruit_PN532.h>
struct rfid_state
{
    char read_crd(const database_s *data, Adafruit_PN532 *nfc, Relay_state *rl);
    signed char card_isregistered(const database_s *db);
    bool open_doors(Relay_state *rl, bool *send_log, Adafruit_PN532 *nfc);
    long interval_reExecute;
    int interval_current_Read_C;
    uint8_t size_uid_incoming = size_uid;
    uint8_t uid_incoming[size_uid];
    void init_sensor(Adafruit_PN532 *nfc);
    rfid_state::rfid_state(const long interval_executed = 50, const int interval_current_read = 100);

#ifdef little_endian

#endif
};
struct storage_state
{
    int SIZE_MEMORY;

    bool load_data(database_s *db);
    bool save_data(database_s *db, database_s *new_db);

    bool load_device_class(char *out, size_t max_len);
    bool save_device_class(const char *name);

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
    bool eth_connected = false;
    bool first_initialize = false;
    unsigned long last_maintain = 0;
    unsigned long last_reconnect = 0;
    static const unsigned long MAINTAIN_INTERVAL = 500;
    static const unsigned long RECONNECT_INTERVAL = 5000;
    String auth_header;
    char method[8];
    char path[32];
    bool header_done;
    char last_char;

#ifdef modelJ508
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x5C};
    const char *controller_name = "J508";
#elif defined(modelJ506)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x5D};
    const char *controller_name = "J506";
#elif defined(modelJ504)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x5E};
    const char *controller_name = "J504";
#elif defined(modelJ505)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x5F};
    const char *controller_name = "J505";
#elif defined(modelL0064)
    const char *controller_name = "L0064";
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x60};
#elif defined(modelL0128)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x61};
    const char *controller_name = "L0128";
#elif defined(modelJ507)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x62};
    const char *controller_name = "J507";
#elif defined(modelL0512)
    byte eth_mac[S_MAC] = {0x02, 0xA1, 0x01, 0x16, 0x3D, 0x63};
    const char *controller_name = "L0512";
#endif

    char device_class[32] = "not_set";
    const char *firmware_ver = "v1.0.0";
    EthernetServer server = EthernetServer(8000);

    IPAddress log_server_ip;
    unsigned long log_server_resolved_at = 0;
    bool log_server_ip_valid = false;
    bool resolve_log_server();

public:
    auth_state auth;
    bool interupt_trigger = false;
    void begin(database_s *db, storage_state *memory);
    void loop(database_s *db, storage_state *memory, Relay_state *locker);

    /* request handling */
    void handle_client(EthernetClient &client, database_s *db, storage_state *memory, Relay_state *locker);
    bool parse_request(EthernetClient &client);
    void reset_parser();

    /* endpoint handlers */
    // void handle_info(EthernetClient &client, database_s *db, storage_state *memory, bool update = false);
    void handle_info(EthernetClient &client);
    void handle_get_data(EthernetClient &client, database_s *db);
    void handle_post_info(EthernetClient &client, storage_state *memory);
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
    bool send_log[Size_Siswa];
    int newline_count;
    static const size_t BODY_SIZE = 2048;
    char body[BODY_SIZE];
    size_t body_len;
    unsigned long start_time;

    database_s *db_ptr = nullptr;
    storage_state *memory_ptr = nullptr;
};
#include <avr/wdt.h>
struct system_d
{
    void software_Resatrt()
    {
        wdt_enable(WDTO_15MS);
        while (1)
        {
        }
    }
};
extern system_d sys;
#endif
#endif
