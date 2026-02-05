#include <commond.h>

void acc_state::main()
{
    unsigned long now = millis();

    if (act != acc_action::acc_on)
    {
        stopBuzzer();
        resetState(now);
        return;
    }

    if (mode == acc_mode::denide)
    {
        handleDenide(now);
        return;
    }

    if (now - LastOn >= Interval)
    {
        LastOn = now;
        toggleBuzzer();
        flip_flop++;

        if (flip_flop >= getMaxFlip())
        {
            act = acc_action::acc_off;
        }
    }
}
void acc_state::stopBuzzer()
{
    noTone(pin);
}
void acc_state::resetState(unsigned long now)
{
    flip_flop = 0;
    buzzerState = false;
    LastOn = now;
}
void acc_state::handleDenide(unsigned long now)
{
    if (now - LastOn < 5000)
    {
        digitalWrite(pin, HIGH);
    }
    else
    {
        stopBuzzer();
        act = acc_action::acc_off;
        LastOn = now;
    }
}
uint8_t acc_state::getMaxFlip()
{
    switch (mode)
    {
    case acc_mode::mode_fastloop3X:
        return 6;
    case acc_mode::mode_fastloop4X:
        return 8;
    case acc_mode::mode_fastloop8X:
        return 16;
    default:
        return 0;
    }
}

void acc_state::toggleBuzzer()
{
    buzzerState = !buzzerState;

    if (buzzerState)
        digitalWrite(pin, HIGH);
    else
        digitalWrite(pin, LOW);
}
