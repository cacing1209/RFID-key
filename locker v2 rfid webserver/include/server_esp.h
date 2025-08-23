#ifndef SERVER_ESP
#define SERVER_ESP

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#define MAX_MAHASISWA 4
#define Indicator_led 5
#define RESET_BTN_PIN 4

struct AppConfig
{
    String deviceName;
    String wifiSSID;
    String wifiPassword;
    int resetButtonPin;
    int maxStudents;
    int serverPort;
    unsigned long resetDelay;
};

struct StudentData
{
    String uid[MAX_MAHASISWA];
    int count;
    bool isDataReceived;
};

struct WiFiConfig
{
    String ssid;
    String password;
    bool isConnected;
    String ipAddress;
    int rssi;
};

struct WebServerData
{
    ESP8266WebServer *server;
    bool isRunning;
    unsigned long lastRequest;
};

extern AppConfig appConfig;
extern StudentData studentData;
extern WiFiConfig wifiConfig;
extern WebServerData webServerData;

void initializeApp();
void connectToWiFi();
void setupWebServer();
void handleWebRequests();
bool handleResetButton();

void handleRoot();
void handleSubmit();

String generateFormHTML();
void processStudentData(const StaticJsonDocument<2048> &doc);
void printStudentData();
void clearStudentData();


#endif
