#include <commond.h>
#ifndef find_p
bool buzzer_state::in_action()
{
    unsigned long now = millis();

    if (mode == bz_mode::denide)
    {
        handleDenide(now);
        return true;
    }
    if (act != acc_action::acc_on)
    {
        stopBuzzer();
        resetState(now);
        return false;
    }
    else if (now - LastOn >= Interval)
    {
        LastOn = millis();
        toggleBuzzer();
        flip_flop++;

        if (flip_flop >= getMaxFlip())
        {
            act = acc_action::acc_off;
        }
        return true;
    }
}
void buzzer_state::stopBuzzer()
{
    // digitalWrite(pin, LOW);
    noTone(pin);
}
void buzzer_state::resetState(unsigned long now)
{
    flip_flop = 0;
    buzzerState = false;
    LastOn = now;
}
void buzzer_state::handleDenide(unsigned long now)
{
    // if (now - LastOn > 2500)
    // {
    //     toggleBuzzer();
    //     LastOn = now;
    // }
}
uint8_t buzzer_state::getMaxFlip()
{
    switch (mode)
    {
    case bz_mode::mode_fastloop1X:
        return 2;
    case bz_mode::mode_fastloop2X:
        return 4;
    case bz_mode::mode_fastloop3X:
        return 6;
    case bz_mode::mode_fastloop4X:
        return 8;
    case bz_mode::mode_fastloop8X:
        return 16;
    case bz_mode::denide:
        return 4;
    default:
        return 0;
    }
}

void buzzer_state::toggleBuzzer()
{
    buzzerState = !buzzerState;

    if (buzzerState)
        tone(pin, 1000);
    // digitalWrite(pin, HIGH);
    else
        noTone(pin);
    // digitalWrite(pin, LOW);
}
#endif