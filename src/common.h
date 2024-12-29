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
    uint16_t Administrator[totalCard];
};

enum access_card
{
    Read,
    Equal,
    Addcard,
    removeCard,
};

struct systemCardProgram
{
    access_card access;
    card mycard[totalCard];
    void ReadCard(MFRC522::Uid IncomingCard, HardwareSerial *serial);
    bool checkIncomingCard(MFRC522::Uid *IncomingCard);
};

enum actionLED
{
    flip_flop,
    ON_2x,
    ON_4X,
};
struct LED
{
    void animationLED(uint8_t indexbutton_LED, actionLED action, uint8_t delayLED);
};
enum statusCar
{
    ON,
    OFF,
    Standby
};
struct CAR
{
    LED led;
    void begin(uint8_t pin_Start, uint8_t pinRelay_Kontak, uint8_t pinRelay_Start);
    uint8_t indexbutton_Start, indexRelay_Kontak, indexRelay_Start;
    statusCar statusEngine;
    systemCardProgram cardSystem;
    int delayPress_Start;
    bool RelayStart, RelayDinamoAmper, RelayKontak;
    void startManual();
};

#endif