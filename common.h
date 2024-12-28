#ifndef common_h
#include <Arduino.h>
#include <MFRC522.h>
#define totalCard 50
#define totalCardAdministor 4
#define sizeCard 10 // 10 bit

enum statusCard
{
    Administrator,
    Member,
};

struct card
{
    byte IdCard[sizeCard];
    statusCard RoleCard;
};

enum access_card
{
    Read,
    Lock,
    Addcard,
    removeCard,
};

enum actionLED
{
    flip_flop,
    ON_2x,
    ON_4X,
};
struct LED
{

    uint8_t delayLED;
    void animationLED(uint8_t indexbutton_LED, actionLED action);
};
struct systemCardProgram
{

    access_card access;
    uint8_t CardTotal;
    card mycard;
    void ReadCard(MFRC522::Uid IncomingCard, HardwareSerial *serial);
    bool checkIncomingCard(MFRC522::Uid *IncomingCard, card *myCard,
                           uint8_t Admin[totalCardAdministor][sizeCard]);
};

enum statusCar
{
    ON,
    OFF,
    Standby
};
struct CAR
{
    void begin();
    statusCar statusEngine;
    LED led;
    systemCardProgram cardSystem;
    uint8_t delayPress_Start;
    bool RelayStart, RelayDinamoAmper, RelayKontak;
    void startManual(uint8_t indexbutton_Start);
};
#endif