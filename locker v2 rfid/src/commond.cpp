// #include <commond.h>

// void Data_state::save_data(const char *filename)
// {
//     action = idle;
//     file = sd->open(filename, O_WRONLY | O_CREAT | O_TRUNC);
//     if (!file)
//     {
//         Serial.println("Gagal membuka file untuk menulis");
//         return;
//     }

//     file.println("[");

//     for (size_t i = 0; i < total_card_rfid; i++)
//     {
//         StaticJsonDocument<256> doc;

//         JsonObject obj = doc.to<JsonObject>();
//         obj["mahasiswa"] = String(i);

//         JsonArray uidArray = obj.createNestedArray("uid");
//         for (size_t x = 0; x < size_rfid; x++)
//         {
//             uidArray.add(rfid.card[i][x]);
//         }

//         serializeJsonPretty(doc, file);

//         if (i < total_card_rfid - 1)
//             file.println(",");
//     }

//     file.println("]");

//     Serial.println("JSON berhasil disimpan");
//     file.close();
// }

// void Aksesoris_state::on(int Ringetone)
// {
//     unsigned long currentTime = millis();

//     if (Ringetone == 3000)
//     {
//         if (Status == state_ON_fastloop)
//         {
//             if (currentTime - LastOn >= flipflopinterval01)
//             {
//                 digitalWrite(pin, !digitalRead(pin));
//                 LastOn = currentTime;
//             }
//         }
//         else if (Status == state_ON)
//         {
//             if (currentTime - LastOn >= Interval)
//             {
//                 digitalWrite(pin, !digitalRead(pin));
//                 LastOn = currentTime;
//             }
//         }
//         else
//         {
//             digitalWrite(pin, LOW);
//             LastOn = currentTime;
//         }
//     }
//     else
//     {
//         static byte count = 0;
//         const byte bitfalse = 4;
//         if (Status == state_ON_fastloop)
//         {
//             if (count >= bitfalse)
//             {
//                 count = 0;
//                 Status = state_OFF;
//             }
//             else if (currentTime - LastOn >= flipflopinterval02)
//             {
//                 LastOn = currentTime;
//                 if (digitalRead(pin) == HIGH)
//                     count++;
//                 digitalWrite(pin, !digitalRead(pin) == HIGH);
//             }
//         }
//         else if (Status == state_ON)
//         {
//             if (currentTime - LastOn >= Interval)
//             {
//                 LastOn = currentTime;
//                 Status = state_OFF;
//             }
//             else
//             {
//                 digitalWrite(pin, HIGH);
//             }
//         }
//         else
//         {
//             digitalWrite(pin, LOW);
//             count = 0;
//             LastOn = currentTime;
//         }
//     }
// }

// void Data_state::load_data(const char *filename)
// {
//     file = sd->open(filename, O_RDONLY);
//     if (!file)
//     {
//         Serial.println("Gagal buka file");
//         return;
//     }

//     size_t i = 0;
//     bool insideArray = false;
//     const size_t bufferSize = 512;
//     DynamicJsonDocument doc(bufferSize);

//     while (file.available() && i < total_card_rfid)
//     {
//         char c = file.peek();

//         while (isspace(c))
//         {
//             file.read(); // discard
//             if (!file.available())
//                 break;
//             c = file.peek();
//         }

//         if (!insideArray)
//         {
//             if (c == '[')
//             {
//                 file.read();
//                 insideArray = true;
//                 continue;
//             }
//             else
//             {
//                 Serial.println("Format JSON tidak valid: tidak diawali '['");
//                 file.close();
//                 return;
//             }
//         }

//         if (c == ']')
//         {
//             break;
//         }

//         DeserializationError error = deserializeJson(doc, file);

//         if (error)
//         {
//             Serial.print("Gagal parsing JSON object ke-");
//             Serial.print(i);
//             Serial.print(": ");
//             Serial.println(error.c_str());
//             break;
//         }

//         JsonObject obj = doc.as<JsonObject>();
//         JsonArray uidArray = obj["uid"];

//         for (size_t x = 0; x < uidArray.size() && x < size_rfid; x++)
//         {
//             rfid.card[i][x] = uidArray[x];
//         }
//         // Serial.print("Mahasiswa ");
//         // Serial.print(obj["mahasiswa"].as<const char *>());
//         // Serial.print(": ");
//         // for (size_t x = 0; x < size_rfid; x++)
//         // {
//         //     Serial.print(rfid.card[i][x]);
//         //     Serial.print(" ");
//         // }
//         // Serial.println();

//         i++;

//         // Baca ',' antar object
//         while (file.available())
//         {
//             char d = file.peek();
//             if (isspace(d))
//             {
//                 file.read();
//                 continue;
//             }
//             if (d == ',')
//             {
//                 file.read();
//                 break;
//             }
//             if (d == ']')
//             {
//                 break;
//             }
//             break;
//         }

//         doc.clear();
//     }

//     file.close();
//     Serial.println("Data UID berhasil dimuat (sd card).");
// }
