#ifndef COMMOND_H
#define COMMOND_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

// Struct untuk konfigurasi portal
struct PortalConfig
{
    const char *ap_ssid;
    const char *ap_password;
    IPAddress apIP;
    IPAddress netMsk;
    const char *username;
    const char *password;
    int timeout;
    int connectTimeout;
};

// Struct untuk status koneksi
struct ConnectionStatus
{
    String status; // "success", "failed", "connecting", "idle"
    String ssid;
    String ip;
    String message;
    int rssi;
};

// Struct untuk data network
struct NetworkData
{
    String ssid;
    int rssi;
    bool isSecure;
};

struct wifi_config_state
{
    String SSID;
    String password;
};

// Class untuk WiFi Portal
class WiFiPortal
{
private:
    wifi_config_state get_config;
    ESP8266WebServer *server;
    DNSServer *dnsServer;
    PortalConfig config;
    ConnectionStatus connStatus;
    bool isAuthenticated;
    bool setupComplete;
    NetworkData networks[5]; // Fixed array instead of dynamic
    int networkCount;

    // Private methods
    void scanWiFiNetworks();
    void sendPage(int type, bool error = false); // 0=login, 1=config
    String getNetworkList();
    String getStatusBox();

    // Handler methods
    void handleRoot();
    void handleAuth();
    void handleConfig();
    void handleScan();
    void handleConnect();
    void handleExit();
    void handleNotFound();

public:
    WiFiPortal();
    ~WiFiPortal();

    void init(const PortalConfig &cfg);
    void start();
    void stop();
    bool isComplete();
    ConnectionStatus getStatus();
};

// Global instance
extern WiFiPortal wifiPortal;

#endif