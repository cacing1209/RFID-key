#include <commond.h>

void DB_STATE::save_data(const char *filename)
{


    
    String payload;
    file = sd->open(filename, O_WRONLY | O_CREAT | O_TRUNC);
    if (!file)
        Serial.println("gagal buka file");

}
bool DB_STATE::load_data(const char *filename)
{
    file = sd->open(filename, O_RDONLY);
    if (!file)
        Serial.println("gagal buka file");
}
void Aksesoris_state::on(int Ringetone)
{
    unsigned long currentTime = millis();

    if (Ringetone == 3000)
    {
        if (Status == state_ON)
        {

            if (currentTime - LastOn > Interval)
            {
                digitalWrite(pin, !(digitalRead(pin) == HIGH));
                LastOn = currentTime;
            }
        }
        else
        {
            LastOn = currentTime;
            digitalWrite(pin, LOW);
        }
    }
    else
    {
        static bool ONX;
        if (Status == state_ON)
        {
            if (currentTime - LastOn > Interval)
            {
                ONX = !ONX;
                LastOn = currentTime;
                Status = state_OFF;
            }
            else
                analogWrite(pin, Ringetone);
        }
        else
        {
            analogWrite(pin, Tone00);
            ONX = false;
            LastOn = currentTime;
        }
    }
}