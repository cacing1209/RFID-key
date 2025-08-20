#include <server_esp.h>
AppConfig appConfig;
StudentData studentData;
WebServerData webServerData;
WiFiConfig wifiConfig;

void initializeApp()
{
    Serial.println("\n=== QYUBIT Absensi System ===");
    appConfig.deviceName = "QYUBIT DEVICE" + String(ESP.getChipId());
    appConfig.wifiSSID = wifiConfig.ssid;
    appConfig.wifiPassword = wifiConfig.password;
    appConfig.resetButtonPin = RESET_BTN_PIN;
    appConfig.maxStudents = MAX_MAHASISWA;
    appConfig.serverPort = 80;
    appConfig.resetDelay = 10000;

    clearStudentData();

    wifiConfig.isConnected = false;
    // wifiConfig.ssid = appConfig.wifiSSID;
    // wifiConfig.password = appConfig.wifiPassword;

    // Initialize web server
    webServerData.server = new ESP8266WebServer(appConfig.serverPort);
    webServerData.isRunning = false;
    webServerData.lastRequest = 0;
    pinMode(Indicator_led, OUTPUT);
    pinMode(appConfig.resetButtonPin, INPUT_PULLUP);
    digitalWrite(Indicator_led, LOW);
    Serial.println("init app sukses");
}

void connectToWiFi()
{
    Serial.println("Connecting to WiFi...");
    Serial.println("SSID: " + wifiConfig.ssid);

    WiFi.begin(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());

    Serial.print("Connecting");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        wifiConfig.isConnected = true;
        wifiConfig.ipAddress = WiFi.localIP().toString();
        wifiConfig.rssi = WiFi.RSSI();

        Serial.println("\nWiFi Connected!");
        Serial.println("SSID: " + wifiConfig.ssid);
        Serial.println("IP Address: " + wifiConfig.ipAddress);
        Serial.println("Signal Strength: " + String(wifiConfig.rssi) + " dBm");
    }
    else
    {
        Serial.println("\nFailed to connect to WiFi!");
        Serial.println("Please check your WiFi credentials in appConfig");
    }
}

void setupWebServer()
{
    Serial.println("Setting up web server...");

    // Setup routes
    webServerData.server->on("/", handleRoot);
    webServerData.server->on("/submit", HTTP_POST, handleSubmit);

    // Start server
    webServerData.server->begin();
    webServerData.isRunning = true;

    Serial.println("Web server started on port " + String(appConfig.serverPort));
    Serial.println("Access: http://" + wifiConfig.ipAddress);
}

void handleWebRequests()
{
    if (webServerData.isRunning)
    {
        webServerData.server->handleClient();
        webServerData.lastRequest = millis();
    }
}

bool handleResetButton()
{
    static unsigned long lastPress = 0;
    static bool buttonPressed = false;

    bool currentState = (digitalRead(appConfig.resetButtonPin) == LOW);
    if (currentState && !buttonPressed)
    {
        lastPress = millis();
        buttonPressed = true;
    }
    else if (!currentState && buttonPressed)
    {
        buttonPressed = false;
    }

    if (buttonPressed && (millis() - lastPress > appConfig.resetDelay))
    {
        Serial.println("Reset button pressed - restarting device");
        return true;
    }
    return false;
}

void handleRoot()
{
    String html = generateFormHTML();
    webServerData.server->send(200, "text/html", html);
    Serial.println("Root page served");
}

void handleSubmit()
{
    if (!webServerData.server->hasArg("plain"))
    {
        webServerData.server->send(400, "text/plain", "Bad Request - No data received");
        return;
    }

    String body = webServerData.server->arg("plain");
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        Serial.print("JSON parsing error: ");
        Serial.println(error.c_str());
        webServerData.server->send(400, "application/json", "{\"status\":\"Invalid JSON\"}");
        return;
    }

    // Process the data
    processStudentData(doc);

    // Send success response
    webServerData.server->send(200, "application/json", "{\"status\":\"success\"}");
}

String generateFormHTML()
{
    String html = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Absensi Mahasiswa</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            padding: 30px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
        }
        .header {
            text-align: center;
            margin-bottom: 30px;
            color: #667eea;
        }
        .header h1 {
            font-size: 28px;
            margin-bottom: 10px;
        }
        .form-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 15px;
            margin-bottom: 30px;
        }
        .form-item {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        .form-item label {
            min-width: 120px;
            font-weight: 500;
            color: #333;
        }
        .form-item input {
            flex: 1;
            padding: 10px 15px;
            border: 2px solid #e1e1e1;
            border-radius: 8px;
            font-size: 14px;
            transition: border-color 0.3s;
        }
        .form-item input:focus {
            outline: none;
            border-color: #667eea;
        }
        .actions {
            text-align: center;
            margin-top: 30px;
        }
        .btn {
            padding: 15px 40px;
            background: linear-gradient(135deg, #667eea, #764ba2);
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.3s;
        }
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
        }
        .status {
            margin-top: 20px;
            padding: 15px;
            border-radius: 8px;
            text-align: center;
            font-weight: 500;
        }
        .status.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .status.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
    </style>
</head>
<body>
    <div class='container'>
        <div class='header'>
            <h1>📋 Absensi Mahasiswa</h1>
            <p>Masukkan UID untuk setiap mahasiswa</p>
        </div>
        
        <form id='absensiForm'>
            <div class='form-grid'>
    )=====";

    for (int i = 0; i < appConfig.maxStudents; i++)
    {
        html += "<div class='form-item'>";
        html += "<label>Mahasiswa " + String(i + 1) + ":</label>";
        html += "<input type='text' id='uid" + String(i) + "' placeholder='Masukkan UID'>";
        html += "</div>";
    }

    html += R"=====(
            </div>
            
            <div class='actions'>
                <button type='button' class='btn' onclick='submitForm()'>📤 Kirim Data</button>
            </div>
        </form>
        
        <div id='status' class='status' style='display: none;'></div>
    </div>

    <script>
        function submitForm() {
            let data = {};
            let filledCount = 0;
            
            for (let i = 0; i < )=====";

    html += String(appConfig.maxStudents);

    html += R"=====(; i++) {
                let uid = document.getElementById('uid' + i).value.trim();
                data['uid' + i] = uid;
                if (uid !== '') filledCount++;
            }
            
            if (filledCount === 0) {
                showStatus('Harap isi setidaknya satu UID!', 'error');
                return;
            }
            
            showStatus('Mengirim data...', 'info');
            
            fetch('/submit', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(data)
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
                return response.json();
            })
            .then(result => {
                showStatus('Data berhasil dikirim! (' + filledCount + ' mahasiswa)', 'success');
                console.log('Success:', result);
            })
            .catch(error => {
                showStatus('Gagal mengirim data: ' + error.message, 'error');
                console.error('Error:', error);
            });
        }
        
        function showStatus(message, type) {
            const statusDiv = document.getElementById('status');
            statusDiv.textContent = message;
            statusDiv.className = 'status ' + type;
            statusDiv.style.display = 'block';
            
            if (type === 'success') {
                setTimeout(() => {
                    statusDiv.style.display = 'none';
                }, 5000);
            }
        }
    </script>
</body>
</html>
    )=====";

    return html;
}

void processStudentData(const StaticJsonDocument<2048> &doc)
{
    Serial.println("=== PROCESSING STUDENT DATA ===");
    Serial.println("send_1");
    Serial.println("send_1");
    Serial.println("send_1");

    delay(2000);

    studentData.isDataReceived = true;
    studentData.count = 0;

    for (int i = 0; i < appConfig.maxStudents; i++)
    {
        String key = "uid" + String(i);
        studentData.uid[i] = doc[key] | "";

        // Print each UID
        Serial.print("A");
        Serial.print(i + 1);
        Serial.print(":");
        Serial.println(studentData.uid[i]);

        // Count non-empty UIDs
        if (studentData.uid[i].length() > 0)
        {
            studentData.count++;
        }

        delay(353);
    }

    Serial.println("send_0");
    Serial.println("send_0");
    Serial.println("send_0");
    Serial.println("Total students with UID: " + String(studentData.count));
    Serial.println("=== DATA PROCESSING COMPLETE ===");
}

void printStudentData()
{
    Serial.println("=== CURRENT STUDENT DATA ===");
    for (int i = 0; i < appConfig.maxStudents; i++)
    {
        if (studentData.uid[i].length() > 0)
        {
            Serial.println("Student " + String(i + 1) + ": " + studentData.uid[i]);
        }
    }
    Serial.println("Total: " + String(studentData.count) + " students");
}

void clearStudentData()
{
    for (int i = 0; i < MAX_MAHASISWA; i++)
    {
        studentData.uid[i] = "";
    }
    studentData.count = 0;
    studentData.isDataReceived = false;
}