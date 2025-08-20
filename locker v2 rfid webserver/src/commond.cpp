#include <commond.h>

WiFiPortal wifiPortal;

// Compressed CSS stored in PROGMEM
const char CSS[] PROGMEM = R"(<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;display:flex;justify-content:center;align-items:center;color:#333}
.container{background:#fff;padding:30px;border-radius:15px;box-shadow:0 15px 30px rgba(0,0,0,0.1);width:100%;max-width:400px;text-align:center}
h1{color:#667eea;margin-bottom:20px;font-size:24px}
.form-group{margin-bottom:15px;text-align:left}
label{display:block;margin-bottom:5px;color:#666;font-weight:500}
input[type="text"],input[type="password"]{width:100%;padding:12px;border:2px solid #e1e1e1;border-radius:8px;font-size:14px}
input:focus{outline:none;border-color:#667eea}
.btn{width:100%;padding:12px;background:linear-gradient(135deg,#667eea,#764ba2);color:#fff;border:none;border-radius:8px;font-size:14px;font-weight:600;cursor:pointer;margin:5px 0}
.btn:hover{transform:translateY(-1px)}
.btn-secondary{background:#6c757d}
.network-item{background:#f8f9fa;border:1px solid #e9ecef;border-radius:8px;padding:10px;margin:8px 0;cursor:pointer;font-size:14px}
.network-item:hover{border-color:#667eea;background:#e7f3ff}
.network-item.selected{border-color:#667eea;background:#e7f3ff}
.status-box{padding:15px;margin:15px 0;border-radius:8px;font-weight:600;font-size:14px}
.status-success{background:#28a745;color:#fff}
.status-error{background:#dc3545;color:#fff}
.status-info{background:#17a2b8;color:#fff}
.error{color:#e74c3c;margin-top:10px;padding:8px;background:#fdf2f2;border-radius:5px;font-size:14px}
</style>)";

// Minimal JavaScript stored in PROGMEM  
const char JS[] PROGMEM = R"(<script>
let sel='';
function selectNetwork(s){sel=s;document.getElementById('ssid').value=s;document.querySelectorAll('.network-item').forEach(i=>i.classList.remove('selected'));event.target.classList.add('selected');}
function saveConfig(){let s=document.getElementById('ssid').value||sel;if(!s){alert('Select network!');return;}let p=document.getElementById('pass').value;fetch('/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(s)+'&password='+encodeURIComponent(p)}).then(()=>window.location.href='/config');}
function exitSetup(){if(confirm('Exit setup?'))fetch('/exit').then(()=>alert('Setup done!'));}
</script>)";

WiFiPortal::WiFiPortal()
{
    server = nullptr;
    dnsServer = nullptr;
    isAuthenticated = false;
    setupComplete = false;
    connStatus.status = "idle";
    networkCount = 0;
}

WiFiPortal::~WiFiPortal()
{
    if (server) delete server;
    if (dnsServer) delete dnsServer;
}

void WiFiPortal::init(const PortalConfig &cfg)
{
    config = cfg;
    server = new ESP8266WebServer(80);
    dnsServer = new DNSServer();
    Serial.println("WiFi Portal initialized");
}

void WiFiPortal::start()
{
    Serial.println("Starting WiFi Portal...");
    
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(config.apIP, config.apIP, config.netMsk);
    WiFi.softAP(config.ap_ssid, config.ap_password);
    
    dnsServer->start(53, "*", config.apIP);
    
    server->on("/", [this](){ this->handleRoot(); });
    server->on("/auth", HTTP_POST, [this](){ this->handleAuth(); });
    server->on("/config", [this](){ this->handleConfig(); });
    server->on("/scan", [this](){ this->handleScan(); });
    server->on("/connect", HTTP_POST, [this](){ this->handleConnect(); });
    server->on("/exit", [this](){ this->handleExit(); });
    server->onNotFound([this](){ this->handleNotFound(); });
    
    server->begin();
    scanWiFiNetworks();
    
    while (!setupComplete)
    {
        dnsServer->processNextRequest();
        server->handleClient();
        yield(); // Prevent watchdog reset
    }
}

void WiFiPortal::stop()
{
    Serial.println("Stopping portal...");
    if (server) server->stop();
    if (dnsServer) dnsServer->stop();
    WiFi.softAPdisconnect(true);
    
    if (WiFi.status() == WL_CONNECTED)
    {
        get_config.SSID = WiFi.SSID();
        get_config.password = WiFi.psk();
        Serial.println("Connected: " + WiFi.SSID() + " IP: " + WiFi.localIP().toString());
    }
}

bool WiFiPortal::isComplete() { return setupComplete; }
ConnectionStatus WiFiPortal::getStatus() { return connStatus; }

void WiFiPortal::scanWiFiNetworks()
{
    WiFi.scanDelete();
    int n = WiFi.scanNetworks();
    networkCount = (n > 5) ? 5 : n; // Limit to 5 networks
    
    if (networkCount == 0) return;
    
    // Store networks
    for (int i = 0; i < networkCount; i++)
    {
        networks[i].ssid = WiFi.SSID(i);
        networks[i].rssi = WiFi.RSSI(i);
        networks[i].isSecure = (WiFi.encryptionType(i) != ENC_TYPE_NONE);
    }
    
    // Simple bubble sort by signal strength
    for (int i = 0; i < networkCount - 1; i++)
    {
        for (int j = 0; j < networkCount - i - 1; j++)
        {
            if (networks[j].rssi < networks[j + 1].rssi)
            {
                NetworkData temp = networks[j];
                networks[j] = networks[j + 1];
                networks[j + 1] = temp;
            }
        }
    }
}

String WiFiPortal::getNetworkList()
{
    if (networkCount == 0) return "<div class='network-item'>No networks found</div>";
    
    String html = "";
    for (int i = 0; i < networkCount; i++)
    {
        String signal = (networks[i].rssi > -60) ? "📶" : (networks[i].rssi > -75) ? "📶" : "📶";
        String security = networks[i].isSecure ? "🔒" : "🔓";
        
        html += "<div class='network-item' onclick='selectNetwork(\"" + networks[i].ssid + "\")'>";
        html += signal + " " + networks[i].ssid + " " + security + " (" + String(networks[i].rssi) + ")";
        html += "</div>";
    }
    return html;
}

String WiFiPortal::getStatusBox()
{
    if (connStatus.status == "success")
    {
        return "<div class='status-box status-success'>✅ Connected to " + connStatus.ssid + 
               " IP: " + connStatus.ip + " Signal: " + String(connStatus.rssi) + "</div>";
    }
    else if (connStatus.status == "failed")
    {
        return "<div class='status-box status-error'>❌ Connection failed! Check password.</div>";
    }
    else if (connStatus.status == "connecting")
    {
        return "<div class='status-box status-info'>🔄 Connecting to " + connStatus.ssid + "...</div>";
    }
    return "";
}

void WiFiPortal::sendPage(int type, bool error)
{
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>";
    html += (type == 0) ? "Login" : "WiFi Config";
    html += "</title>";
    html += FPSTR(CSS);
    html += "</head><body><div class='container'>";
    
    if (type == 0) // Login page
    {
        html += "<h1>🔐 QYUBIT CONFIG</h1>";
        html += "<form action='/auth' method='POST'>";
        html += "<div class='form-group'><label>Username</label><input type='text' name='user' required></div>";
        html += "<div class='form-group'><label>Password</label><input type='password' name='pass' required></div>";
        html += "<button type='submit' class='btn'>🚀 LOGIN</button></form>";
        
        if (error) html += "<div class='error'>❌ Invalid credentials!</div>";
    }
    else // Config page
    {
        html += "<h1>📡 WiFi Config</h1>";
        html += "<h3>📶 Networks</h3>";
        html += getNetworkList();
        html += "<div class='form-group'><label>SSID</label><input type='text' id='ssid' placeholder='Select or enter SSID'></div>";
        html += "<div class='form-group'><label>Password</label><input type='password' id='pass' placeholder='WiFi password'></div>";
        html += getStatusBox();
        html += "<button class='btn' onclick='saveConfig()'>💾 Connect</button>";
        html += "<button class='btn btn-secondary' onclick='window.location.href=\"/scan\"'>🔄 Refresh</button>";
        html += "<button class='btn btn-secondary' onclick='exitSetup()'>🚪 Exit</button>";
    }
    
    html += "</div>";
    html += FPSTR(JS);
    html += "</body></html>";
    
    server->send(200, "text/html", html);
}

void WiFiPortal::handleRoot()
{
    sendPage(isAuthenticated ? 1 : 0);
}

void WiFiPortal::handleAuth()
{
    if (server->hasArg("user") && server->hasArg("pass"))
    {
        if (server->arg("user") == config.username && server->arg("pass") == config.password)
        {
            isAuthenticated = true;
            server->sendHeader("Location", "/config");
            server->send(302);
            return;
        }
    }
    sendPage(0, true);
}

void WiFiPortal::handleConfig()
{
    if (!isAuthenticated)
    {
        server->sendHeader("Location", "/");
        server->send(302);
        return;
    }
    sendPage(1);
}

void WiFiPortal::handleScan()
{
    if (!isAuthenticated)
    {
        server->sendHeader("Location", "/");
        server->send(302);
        return;
    }
    scanWiFiNetworks();
    server->sendHeader("Location", "/config");
    server->send(302);
}

void WiFiPortal::handleConnect()
{
    if (!isAuthenticated || !server->hasArg("ssid") || !server->hasArg("password"))
    {
        server->sendHeader("Location", "/");
        server->send(302);
        return;
    }
    
    String ssid = server->arg("ssid");
    String pass = server->arg("password");
    
    connStatus.status = "connecting";
    connStatus.ssid = ssid;
    
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < config.connectTimeout)
    {
        delay(500);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED)
    {
        connStatus.status = "success";
        connStatus.ip = WiFi.localIP().toString();
        connStatus.rssi = WiFi.RSSI();
    }
    else
    {
        connStatus.status = "failed";
    }
    
    sendPage(1);
}

void WiFiPortal::handleExit()
{
    if (!isAuthenticated)
    {
        server->sendHeader("Location", "/");
        server->send(302);
        return;
    }
    
    server->send(200, "text/html", 
        "<!DOCTYPE html><html><head><style>body{font-family:Arial;text-align:center;padding:50px;background:#667eea;color:#fff}</style></head>"
        "<body><h1>🎉 Setup Complete!</h1><p>Device restarting...</p><script>setTimeout(()=>window.close(),2000)</script></body></html>");
    
    setupComplete = true;
}

void WiFiPortal::handleNotFound()
{
    server->sendHeader("Location", "/");
    server->send(302);
}