// #include <ESP8266WiFi.h>
// #include <ESP8266WebServer.h>
// #include <WiFiManager.h>
// #include <ArduinoJson.h>
// #define resetbtn 2
// ESP8266WebServer server(80);
// String uidMahasiswa[30];

// void handleSubmit()
// {
//   if (!server.hasArg("plain"))
//   {
//     server.send(400, "text/plain", "Bad Request");
//     return;
//   }

//   String body = server.arg("plain");
//   StaticJsonDocument<2048> doc;
//   DeserializationError error = deserializeJson(doc, body);

//   if (error)
//   {
//     // Serial.print("JSON parsing error: ");
//     // Serial.println(error.c_str());
//     // server.send(400, "application/json", "{\"status\":\"Invalid JSON\"}");
//     return;
//   }
//   delay(2000);
//   Serial.println("send_1");
//   Serial.println("send_1");
//   Serial.println("send_1");
//   for (int i = 0; i < 30; i++)
//   {
//     String key = "uid" + String(i);
//     uidMahasiswa[i] = doc[key] | "";

//     // Tambahin debug tiap UID
//     Serial.print("A");
//     Serial.print(i + 1);
//     Serial.print(": ");
//     Serial.println(uidMahasiswa[i]);
//     delay(300);
//   }
//   Serial.println("send_0");
//   Serial.println("send_0");
//   Serial.println("send_0");
//   server.send(200, "application/json", "{\"status\":\"success\"}");
// }

// void handleRoot()
// {
//   String html = R"=====(<!DOCTYPE html>
//     <html>
//     <head><title>Absensi Mahasiswa</title></head>
//     <body>
//       <h2>Form Absensi Mahasiswa (UID)</h2>
//       <form id="absensiForm">)=====";

//   for (int i = 0; i < 30; i++)
//   {
//     html += "Mahasiswa " + String(i + 1) + ": <input type='text' id='uid" + String(i) + "'><br>";
//   }

//   html += R"=====(<br><button type="button" onclick="submitForm()">Kirim</button></form>

//       <script>
//         function submitForm() {
//           let data = {};
//           for (let i = 0; i < 30; i++) {
//             let uid = document.getElementById('uid' + i).value;
//             data['uid' + i] = uid;
//           }

//           fetch('/submit', {
//             method: 'POST',
//             headers: {
//               'Content-Type': 'application/json'
//             },
//             body: JSON.stringify(data)
//           })
//           .then(response => response.text())
//           .then(result => {
//             alert('Berhasil dikirim!');
//             console.log(result);
//           })
//           .catch(error => {
//             alert('Gagal: ' + error);
//           });
//         }
//       </script>
//     </body>
//     </html>)=====";

//   server.send(200, "text/html", html);
// }

// WiFiManager wm;
// void reset_wifi()
// {
//   static unsigned long lt = 0;
//   bool res;
//   const byte pinRes = resetbtn;
//   if (digitalRead(pinRes) == LOW)
//   {
//     if (millis() - lt > 2000)
//       res = true;
//   }
//   else
//   {
//     res = false;
//     lt = millis();
//   }
//   if (res)
//   {
//     wm.resetSettings();
//     ESP.restart();
//   }
// }

// void setup_wifi()
// {
//   String apName = "QYUBIT DEVICE " + String(ESP.getChipId());

//   wm.setWebServerCallback([]()
//                           {
//     if (wm.server != nullptr) {
//       wm.server->on("/connected", []() {
//         String html = "<html><head><title>WiFi Connected</title></head><body>";
//         html += "<h2> WiFi Berhasil Terkoneksi</h2>";
//         html += "<p>SSID: " + WiFi.SSID() + "</p>";
//         html += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";
//         html += "<p>Silakan catat IP ini.</p>";
//         html += "</body></html>";
//         wm.server->send(200, "text/html", html);
//       });
//     } });

//   // Jalankan AP mode (password opsional)
//   wm.startConfigPortal(apName.c_str());

//   // Setelah klik Save, coba konek WiFi sambil tetap nyalain AP
//   WiFi.mode(WIFI_AP_STA);

//   // Ambil SSID & password terakhir dari WiFiManager (disimpan internal)
//   String ssid = WiFi.SSID();
//   String pass = WiFi.psk();
//   WiFi.begin(ssid.c_str(), pass.c_str());

//   // Tunggu koneksi
//   while (WiFi.status() != WL_CONNECTED)
//   {
//     Serial.print('.');
//     delay(100);
//   }

//   if (WiFi.status() == WL_CONNECTED && wm.server != nullptr)
//   {
//     wm.server->sendHeader("Location", String("/connected"), true);
//     wm.server->send(302, "text/plain", "");
//   }

//   delay(10000);
//   WiFi.softAPdisconnect(true); // matiin AP
// }

// void setup()
// {
//   Serial.begin(115200);
//   pinMode(resetbtn, INPUT);
//   // wm.resetSettings();
//   setup_wifi();
//   // WiFi.begin("BOD", "12345678");
//   while (WiFi.status() != WL_CONNECTED)
//   {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("WiFi Terkoneksi!");
//   Serial.print("IP address: ");
//   Serial.println(WiFi.localIP());
//   Serial.println("Web server started!");

//   server.on("/", handleRoot);
//   server.on("/submit", HTTP_POST, handleSubmit);
//   server.begin();
// }

// void loop()
// {
//   server.handleClient();
//   reset_wifi();
// }
