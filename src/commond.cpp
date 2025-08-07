#include <commond.h>

void Data_state::save_data(const char *filename)
{
    action = idle;
    String payload;

    file = sd->open(filename, O_WRONLY | O_CREAT | O_TRUNC);
    if (!file)
    {
        Serial.println("Gagal membuka file untuk menulis");
        return;
    }

    JsonArray dataArray = doc.to<JsonArray>();
    for (size_t i = 0; i < total_card_rfid; i++)
    {
        JsonObject obj = dataArray.createNestedObject();
        obj["mahasiswa"] = String(i);

        JsonArray uidArray = obj.createNestedArray("uid");
        for (size_t x = 0; x < size_rfid; x++)
        {
            uidArray.add(rfid.card[i][x]);
        }
    }

    if (serializeJsonPretty(doc, file) == 0)
        Serial.println("Gagal menulis JSON");
    else
        Serial.println("JSON berhasil disimpan");

    file.close();
}
void Data_state::load_data(const char *filename)
{
    file = sd->open(filename, O_RDONLY);
    if (!file)
    {
        Serial.println("gagal Buka file");
        return;
    }

    for (size_t i = 0; i < total_card_rfid; i++)
    {
        if (doc["mahasiswa"] == String(i))
        {
            for (size_t x = 0; x < size_rfid; x++)
            {
                doc["uid"][i] = rfid.card[i][x];
                Serial.print(rfid.card[i][x]);
            }
        }
        Serial.println("card" + String(i));
    }
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