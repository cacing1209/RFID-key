// #include <commond.h>
// #include <server_esp.h>
// #include <save_f.h>

// #define addres_ssid 1
// #define addres_psw 50

// void setup()
// {
//     Serial.begin(9600);
//     Serial.println("\n=== QYUBIT WiFi Configuration ===");

//     // Konfigurasi portal
//     PortalConfig config = {
//         .ap_ssid = "qyubit-config",
//         .ap_password = "",
//         .apIP = IPAddress(192, 168, 1, 3),
//         .netMsk = IPAddress(255, 255, 255, 0),
//         .username = "Qyubit",
//         .password = "12341234",
//         .timeout = 300,
//         .connectTimeout = 20};

//     // Initialize dan start portal
//     wifiPortal.init(config);
//     wifiPortal.start();
//     wifiPortal.stop();

//     Serial.println("=== Setup Complete ===");
//     Serial.println("Device ready for main program");

//     // Print final connection status
//     ConnectionStatus status = wifiPortal.getStatus();
//     if (status.status == "success")
//     {
//         Serial.println("Connected to: " + status.ssid);
//         Serial.println("IP Address: " + status.ip);
//         Serial.println("Signal: " + String(status.rssi) + " dBm");
//     }
// }

// void loop()
// {
//     if (WiFi.status() == WL_CONNECTED)
//     {
//         Serial.println("Main program running - IP: " + WiFi.localIP().toString());
//         delay(10000);
//     }
//     else
//     {
//         Serial.print(".");
//         delay(1000);
//     }
// }