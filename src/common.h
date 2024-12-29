#ifndef common_h
#include <HardwareSerial.h>
#include <MFRC522.h>
#define totalCard 5
#define totalCardAdministor 4
#define sizeCard 10 // 10 bit

enum statusCard
{
    Administrator,
    Member,
};

struct cardAdmin
{
    uint16_t Administrator[totalCard];
};

struct cardMember
{
    byte IdCard[sizeCard];
    statusCard RoleCard;
};

enum access_card
{
    Lock,
    Equal,
    Addcard,
    removeCard,
};

struct systemCardProgram
{
    HardwareSerial *Msg;
    access_card access;
    cardAdmin Admin[totalCard];
    cardMember mycard[totalCard];
    void ReadCard(MFRC522::Uid *IncomingCard);
    void printTrue(uint8_t index_card);
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