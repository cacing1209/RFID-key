#include <commond.h>

#ifdef find_p
void mapping_p::begin()
{
    for (size_t i = 0; i < sizeRelay; i++)
    {
        pinMode(pin_IO[i], OUTPUT);
        digitalWrite(pin_IO[i], LOW);
        delay(100);
    }
    delay(250);
    for (size_t i = 0; i < sizeRelay; i++)
    {
        digitalWrite(pin_IO[i], HIGH);
    }
}
bool mapping_p::pins_avaiable()
{
    for (size_t i = 0; i < sizeRelay; i++)
    {
        if (pin[i] != 0)
            return false;
    }
    return true;
}

// void swap(byte &xp, byte &yp)
// {
//     byte *temp;
//     temp = &xp;
//     xp = yp;
//     yp = *temp;
// }
void mapping_p::shorting_pins()
{
    static bool need_reswap = true;
    if (!need_reswap)
        return;
    int n = sizeRelay;
    int new_setup_rl[sizeRelay];
    for (size_t i = 0; i < sizeRelay; i++)
    {
        new_setup_rl[i] = pin_IO[i];
    }

    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - i - 1; j++)
        {
            if (pin[j] > pin[j + 1])
            {
                // swap(pin[j], pin[j + 1]);
                byte temp;
                temp = pin[j];
                pin[j] = pin[j + 1];
                pin[j + 1] = temp;

                byte temp_x;
                temp_x = new_setup_rl[j];
                new_setup_rl[j] = new_setup_rl[j + 1];
                new_setup_rl[j + 1] = temp_x;
            }
        }
    }

    Serial.println("shord arr=>");
    for (size_t i = 0; i < 2; i++)
    {
        Serial.print('{');
        for (size_t index = 0; index < sizeof(pin); index++)
        {
            if (i % 2 == 0)
                Serial.print(pin[index]);
            else
                Serial.print(new_setup_rl[index]);
            Serial.print(',');
        }
        Serial.println('}');
    }
    for (size_t xp = 0; xp < sizeRelay; xp++)
    {
        digitalWrite(new_setup_rl[xp], LOW);
        delay(1000);
        digitalWrite(new_setup_rl[xp], HIGH);
        delay(1000);
    }

    need_reswap = false;
}
void mapping_p::main()
{
    bool wait_input = true;
    if (bypass)
        return;
    if (!pins_avaiable())
    {
        shorting_pins();
        return;
    }
    for (size_t i = 0; i < sizeRelay; i++)
    {
        wait_input = true;

        Serial.print("get_input=>");
        digitalWrite(pin_IO[i], LOW);

        while (wait_input)
        {
            if (Serial.available())
            {
                String input = Serial.readStringUntil('\n');
                input.trim();

                Serial.print("num=>");
                Serial.println(input);

                if (input.length() > 0 && input.length() <= 2)
                {
                    bool already = false;
                    byte num_of_p = input.toInt();
                    for (size_t idx_mtch = 0; idx_mtch < sizeof(pin); idx_mtch++)
                    {
                        if (pin[idx_mtch] == num_of_p)
                        {
                            already = true;
                        }
                    }
                    if (!already && (num_of_p >= 0 || num_of_p <= 32))
                    {
                        pin[i] = num_of_p;
                        wait_input = false;
                    }
                    else
                    {
                        wait_input = true;
                        Serial.println("failed set number");
                    }

                    Serial.println("himpunan=>");
                    Serial.print('{');

                    for (size_t idx = 0; idx < sizeof(pin) / sizeof(pin[0]); idx++)
                    {
                        Serial.print(pin[idx]);
                        Serial.print(',');
                    }

                    Serial.println('}');
                }
                else
                {
                    Serial.println("failed set...overflow number");
                }
            }
        }
    }
}
// himpunan=>
// [29,3,26,21,28,20,32,24,31,18,30,8,27,19,9,22,4,14,13,2,11,6,7,16,17,1,12,23,10,25,5,15,]

// shord arr=>modulL002
// [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,]
// {47,41,23,38,52,43,44,33,36,50,42,48,40,39,53,45,46,31,35,27,25,37,49,29,51,24,34,26,22,32,30,28,}

#endif