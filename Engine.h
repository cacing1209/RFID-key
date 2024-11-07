#include <SPI.h>
#include <MFRC522.h>

#ifndef Engine_H
#define Engine_H

#define R_start 2
#define R_kontak 3
#define Automatic_ON 3
#define startManual 4
#define buzzer 5
#define Read_voltkontak 6
#define cardtotal 4
#define cardsize 8
#define led 7
#define totalcard 3
#define interval_automatic 500
#define Delay_PUSH 450
#define Delay_Push_buzzer 170
#define Delay_buzzer 350

struct CARD
{
    uint8_t sizeByte = cardsize;
    byte dbCARD[cardtotal][cardsize] = {
        {51, 159, 195, 254},
        {208, 116, 56, 88},
        {5, 131, 31, 150, 139, 209, 0}};
    void initCard(uint8_t size, uint8_t *carduid);
    bool Equal_DBandSOURCE(uint8_t *DB, uint8_t *SC);
};

struct RelayState
{
    uint8_t Engine_Start = R_start,
            Cotact = R_kontak;
};
enum buttonAction
{
    set_EngineSTART,
    set_addCARD,
    set_EngineOFF
};

struct buttonState
{
    uint8_t start_Manual = startManual;
    void Actionled();
};

class EngineRunning
{
private:
    uint8_t pushButton,
        buzzerAction,
        automaticInterval;

    const int ENGINE_voltage = A0, tx = A1, rx = A2, Read_Retret = A3;
    int distance;
    unsigned long duration;

public:
    void ledAnimation(int currentTime_HIGH)
    {
    }
    const char *Read_Voltagecontact()
    {
        return (digitalRead(ENGINE_voltage) >= 450) ? "HIGH" : "LOW";
    }
    int Read_Voltageultrasonic()
    {
        digitalWrite(tx, LOW);
        delayMicroseconds(2);
        digitalWrite(tx, HIGH);
        delayMicroseconds(10);
        digitalWrite(tx, LOW);
        duration = pulseIn(rx, HIGH);
        return distance = duration * 0.034 / 2;
    }
    void begin()
    {
        /* default Yages Gess Yaa..pokoknya itu dah anjg */
        pushButton = Delay_PUSH,
        buzzerAction = Delay_buzzer,
        automaticInterval = interval_automatic;
    }
};

#endif