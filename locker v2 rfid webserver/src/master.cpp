#include <server_esp.h>
#include <save_f.h>
#include <commond.h>

#define addres_ssid 1
#define addres_psw 50
#define addres_reset 0

save_f_state data;
bool led_main(const int interval, const byte looping)
{
    static byte count = 0;
    const byte led = Indicator_led;
    static unsigned long xx = 0;
    if (millis() - xx > interval)
    {
        digitalWrite(led, !(digitalRead(led) == HIGH));
        xx = millis();
        count++;
    }
    if (count > looping)
        return true;
    return false;
}
void setup()
{
    Serial.begin(115200);

    data.begin();
    Serial.println("initialized...");
    bool dataisAlready =
        data.load_data(addres_reset, addres_ssid, addres_psw, wifiConfig.ssid, wifiConfig.password);
    delay(3000);
    if (dataisAlready)
    {
        initializeApp();
        connectToWiFi();

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }

        Serial.println("\n=== SYSTEM READY ===");
        Serial.println("Device: " + appConfig.deviceName);
        Serial.println("MAC: " + WiFi.macAddress());
        Serial.println("WiFi: " + wifiConfig.ssid);
        Serial.println("IP: " + wifiConfig.ipAddress);

        setupWebServer();

        Serial.println("=== ABSENSI SYSTEM STARTED ===");
    }
    else
    {
        String apname = "qyubit-" + String(ESP.getChipId());
        PortalConfig config = {
            .ap_ssid = apname.c_str(),
            .ap_password = "1234567890",
            .apIP = IPAddress(192, 168, 7, 3),
            .netMsk = IPAddress(255, 255, 255, 0),
            .username = "qyubit",
            .password = "12345678",
            .timeout = 300,
            .connectTimeout = 20};

        // Initialize dan start portal
        wifiPortal.init(config);
        wifiPortal.start();
        wifiPortal.stop();

        Serial.println("=== Setup Complete ===");
        ConnectionStatus status = wifiPortal.getStatus();
        if (status.status == "success")
        {
            Serial.println("Connected to: " + status.ssid);
            Serial.println("IP Address: " + status.ip);
            Serial.println("Signal: " + String(status.rssi) + " dBm");
            wifiConfig.ssid = WiFi.SSID();
            wifiConfig.password = WiFi.psk();
            data.save_data(addres_ssid, wifiConfig.ssid);
            data.save_data(addres_psw, wifiConfig.password);
            data.save_data_reset(addres_reset, false);
            delay(5000);
            Serial.println("esp restart");
            ESP.restart();
        }
    }
}

void loop()
{
    bool reset = handleResetButton();
    handleWebRequests();
    if (reset && led_main(200, 5))
    {
        data.save_data_reset(addres_reset, true);
        Serial.println("esp reset");
        delay(2000);
        ESP.restart();
    }
}