#include <commond.h>

void acc_state::main()
{
    unsigned long current_t = millis();

    if (act == acc_action::acc_on)
    {
        switch (mode)
        {
        case acc_mode::mode_fastloop4X:
            if (flip_flop >= 8) act = acc_action::acc_off;
            break;

        case acc_mode::mode_fastloop2X:
            if (flip_flop >= 4) act = acc_action::acc_off;
            break;

        case acc_mode::mode_fastloop8X:
            if (flip_flop >= 16) act = acc_action::acc_off;
            break;

        default:
            act = acc_action::acc_off;
            break;
        }

        if (act == acc_action::acc_on &&
            current_t - LastOn >= Interval)
        {
            LastOn = current_t;
            digitalWrite(pin, !digitalRead(pin));
            flip_flop++;
        }
    }
    else
    {
        digitalWrite(pin, LOW);
        flip_flop = 0;
    }
}
