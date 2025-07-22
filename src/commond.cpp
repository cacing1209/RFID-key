#include <commond.h>

void DB_STATE::save_data(const char *filename)
{
    file = sd->open(filename, O_WRONLY | O_CREAT | O_TRUNC);
    if (!file)
        Serial.println("gagal buka file");
}
void DB_STATE::load_data(const char *filename)
{
    file = sd->open(filename, O_RDONLY);
    if (!file)
        Serial.println("gagal buka file");
        // doc[]
}