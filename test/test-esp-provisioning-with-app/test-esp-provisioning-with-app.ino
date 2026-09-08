#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ============================================================
// CONFIG
// ============================================================

#define STATUS_LED 2

const char* AP_SSID = "Sterilizer-001";
const char* AP_PASSWORD = "12345678";

const char* ESP32_IP = "192.168.4.1";

const unsigned long WIFI_CONNECT_TIMEOUT = 15000;


// ============================================================
// WEB SERVER
// ============================================================

WebServer server(80);


// ============================================================
// WIFI STATE
// ============================================================

enum WiFiState {
    WIFI_AP_MODE,
    WIFI_CONNECTING,
    WIFI_CONNECTED
};

WiFiState wifiState = WIFI_AP_MODE;

unsigned long wifiConnectStart = 0;


// ============================================================
// LED STATE
// ============================================================

unsigned long ledLastMillis = 0;
bool ledState = false;


// ============================================================
// HTML PAGE
// ============================================================

const char MAIN_PAGE[] PROGMEM = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta name="viewport"
      content="width=device-width, initial-scale=1">

<title>Sterilizer Setup</title>

<style>

body {
    font-family: Arial, sans-serif;
    margin: 0;
    padding: 20px;
    background: #f5f5f5;
}

.container {
    max-width: 500px;
    margin: auto;
    background: white;
    padding: 20px;
    border-radius: 12px;
}

h1 {
    margin-top: 0;
}

.network {
    padding: 14px;
    margin-top: 8px;
    border: 1px solid #ddd;
    border-radius: 8px;
    cursor: pointer;
}

.network:hover {
    background: #f0f0f0;
}

input {
    width: 100%;
    box-sizing: border-box;
    padding: 12px;
    margin-top: 8px;
    margin-bottom: 12px;
}

button {
    width: 100%;
    padding: 12px;
    margin-top: 8px;
}

#status {
    margin-top: 15px;
    padding: 10px;
}

</style>

</head>


<body>

<div class="container">

<h1>Sterilizer Setup</h1>

<p>
Device: <b>Sterilizer-001</b>
</p>


<button onclick="scanWiFi()">
    Scan Wi-Fi
</button>


<div id="networks">
    Press Scan Wi-Fi
</div>


<label>
    Selected SSID
</label>

<input
    id="ssid"
    type="text"
    readonly
>


<label>
    Wi-Fi Password
</label>

<input
    id="password"
    type="password"
    placeholder="Password"
>


<button onclick="connectWiFi()">
    Connect
</button>


<div id="status"></div>

</div>


<script>

async function scanWiFi() {

    const networks =
        document.getElementById("networks");

    networks.innerHTML =
        "Scanning...";


    try {

        const response =
            await fetch("/api/wifi/scan");

        const data =
            await response.json();


        networks.innerHTML = "";


        if (!data.networks ||
            data.networks.length === 0) {

            networks.innerHTML =
                "No Wi-Fi networks found.";

            return;
        }


        data.networks.forEach(function(network) {

            const div =
                document.createElement("div");

            div.className = "network";


            div.innerHTML =
                "<b>" +
                network.ssid +
                "</b><br>" +
                "Signal: " +
                network.rssi +
                " dBm";


            div.onclick = function() {

                document.getElementById("ssid").value =
                    network.ssid;

            };


            networks.appendChild(div);

        });

    }

    catch(error) {

        networks.innerHTML =
            "Scan failed.";

        console.error(error);

    }

}


async function connectWiFi() {

    const ssid =
        document.getElementById("ssid").value;

    const password =
        document.getElementById("password").value;


    if (!ssid) {

        alert("Please select Wi-Fi.");

        return;

    }


    document.getElementById("status").innerText =
        "Connecting...";


    try {

        const response =
            await fetch("/api/wifi/connect", {

                method: "POST",

                headers: {
                    "Content-Type":
                    "application/json"
                },

                body: JSON.stringify({
                    ssid: ssid,
                    password: password
                })

            });


        const data =
            await response.json();


        document.getElementById("status").innerText =
            data.message;

    }

    catch(error) {

        document.getElementById("status").innerText =
            "Connection request failed.";

        console.error(error);

    }

}

</script>

</body>

</html>

)rawliteral";


// ============================================================
// JSON STRING ESCAPE
// ============================================================

String jsonEscape(const String& input) {

    String output;

    for (size_t i = 0; i < input.length(); i++) {

        char c = input[i];

        if (c == '"') {
            output += "\\\"";
        }

        else if (c == '\\') {
            output += "\\\\";
        }

        else if (c == '\n') {
            output += "\\n";
        }

        else if (c == '\r') {
            output += "\\r";
        }

        else {
            output += c;
        }
    }

    return output;
}


// ============================================================
// LED
// ============================================================

void updateStatusLED() {

    unsigned long now = millis();

    unsigned long interval;


    // --------------------------------------------------------
    // AP / READY
    // ON 100ms
    // OFF 500ms
    // --------------------------------------------------------

    if (wifiState == WIFI_AP_MODE) {

        if (ledState) {

            interval = 100;

        } else {

            interval = 500;
        }
    }


    // --------------------------------------------------------
    // CONNECTING
    // ON 200ms
    // OFF 200ms
    // --------------------------------------------------------

    else if (wifiState == WIFI_CONNECTING) {

        interval = 200;
    }


    // --------------------------------------------------------
    // CONNECTED
    // ON
    // --------------------------------------------------------

    else {

        digitalWrite(
            STATUS_LED,
            HIGH
        );

        ledState = true;

        return;
    }


    if (now - ledLastMillis >= interval) {

        ledLastMillis = now;

        ledState = !ledState;

        digitalWrite(
            STATUS_LED,
            ledState
        );
    }
}


// ============================================================
// START AP
// ============================================================

void startAP() {

    Serial.println();
    Serial.println("==============================");
    Serial.println("Starting AP Mode");
    Serial.println("==============================");


    WiFi.disconnect(true);

    delay(300);


    WiFi.mode(WIFI_AP);


    bool result =
        WiFi.softAP(
            AP_SSID,
            AP_PASSWORD
        );


    if (result) {

        Serial.println("AP started.");

        Serial.print("SSID: ");
        Serial.println(AP_SSID);

        Serial.print("Password: ");
        Serial.println(AP_PASSWORD);

        Serial.print("IP: ");
        Serial.println(
            WiFi.softAPIP()
        );

    }

    else {

        Serial.println(
            "Failed to start AP."
        );
    }


    wifiState = WIFI_AP_MODE;

    ledState = false;

    digitalWrite(
        STATUS_LED,
        LOW
    );
}


// ============================================================
// API - WIFI SCAN
// ============================================================

void handleWiFiScan() {

    Serial.println();
    Serial.println("API: /api/wifi/scan");

    Serial.println(
        "Scanning Wi-Fi..."
    );


    int count =
        WiFi.scanNetworks(
            false,
            true
        );


    Serial.print(
        "Found networks: "
    );

    Serial.println(count);


    String json;

    json.reserve(2048);

    json = "{\"networks\":[";


    bool first = true;


    for (int i = 0; i < count; i++) {

        String ssid =
            WiFi.SSID(i);

        int rssi =
            WiFi.RSSI(i);


        // ข้าม SSID ว่าง
        if (ssid.length() == 0) {
            continue;
        }


        if (!first) {
            json += ",";
        }

        first = false;


        json += "{";

        json += "\"ssid\":\"";
        json += jsonEscape(ssid);
        json += "\",";

        json += "\"rssi\":";
        json += String(rssi);

        json += "}";
    }


    json += "]}";


    WiFi.scanDelete();


    server.send(
        200,
        "application/json",
        json
    );


    Serial.println(
        "Scan response sent."
    );
}


// ============================================================
// API - WIFI CONNECT
// ============================================================

void handleWiFiConnect() {

    Serial.println();
    Serial.println(
        "API: /api/wifi/connect"
    );


    if (!server.hasArg("plain")) {

        server.send(
            400,
            "application/json",
            "{\"success\":false,\"message\":\"Missing request body\"}"
        );

        return;
    }


    String body =
        server.arg("plain");


    Serial.print("Body: ");
    Serial.println(body);


    // --------------------------------------------------------
    // อ่าน SSID
    // --------------------------------------------------------

    int ssidStart =
        body.indexOf("\"ssid\"");


    int passwordStart =
        body.indexOf("\"password\"");


    if (ssidStart < 0 ||
        passwordStart < 0) {

        server.send(
            400,
            "application/json",
            "{\"success\":false,\"message\":\"Invalid JSON\"}"
        );

        return;
    }


    // --------------------------------------------------------
    // Parse SSID
    // --------------------------------------------------------

    int ssidColon =
        body.indexOf(
            ':',
            ssidStart
        );

    int ssidQuote1 =
        body.indexOf(
            '"',
            ssidColon + 1
        );

    int ssidQuote2 =
        body.indexOf(
            '"',
            ssidQuote1 + 1
        );


    // --------------------------------------------------------
    // Parse Password
    // --------------------------------------------------------

    int passColon =
        body.indexOf(
            ':',
            passwordStart
        );

    int passQuote1 =
        body.indexOf(
            '"',
            passColon + 1
        );

    int passQuote2 =
        body.indexOf(
            '"',
            passQuote1 + 1
        );


    if (ssidQuote1 < 0 ||
        ssidQuote2 < 0 ||
        passQuote1 < 0 ||
        passQuote2 < 0) {

        server.send(
            400,
            "application/json",
            "{\"success\":false,\"message\":\"Invalid parameters\"}"
        );

        return;
    }


    String ssid =
        body.substring(
            ssidQuote1 + 1,
            ssidQuote2
        );


    String password =
        body.substring(
            passQuote1 + 1,
            passQuote2
        );


    Serial.print("SSID: ");
    Serial.println(ssid);

    Serial.print("Password length: ");
    Serial.println(password.length());


    if (ssid.length() == 0) {

        server.send(
            400,
            "application/json",
            "{\"success\":false,\"message\":\"SSID is empty\"}"
        );

        return;
    }


    // --------------------------------------------------------
    // เริ่ม Connect
    // --------------------------------------------------------

    WiFi.mode(WIFI_AP_STA);


    WiFi.begin(
        ssid.c_str(),
        password.c_str()
    );


    wifiState =
        WIFI_CONNECTING;


    wifiConnectStart =
        millis();


    server.send(
        200,
        "application/json",
        "{\"success\":true,\"message\":\"Connecting to Wi-Fi...\"}"
    );


    Serial.println(
        "Wi-Fi connection started."
    );
}


// ============================================================
// API - STATUS
// ============================================================

void handleStatus() {

    String json;

    json = "{";

    json += "\"state\":\"";


    if (wifiState == WIFI_AP_MODE) {

        json += "ap";

    }

    else if (wifiState == WIFI_CONNECTING) {

        json += "connecting";

    }

    else {

        json += "connected";
    }


    json += "\",";


    json += "\"ssid\":\"";

    if (WiFi.status() == WL_CONNECTED) {

        json += jsonEscape(
            WiFi.SSID()
        );
    }

    json += "\",";


    json += "\"ip\":\"";

    if (WiFi.status() == WL_CONNECTED) {

        json +=
            WiFi.localIP().toString();
    }

    json += "\",";


    json += "\"rssi\":";

    if (WiFi.status() == WL_CONNECTED) {

        json +=
            String(WiFi.RSSI());

    } else {

        json += "0";
    }


    json += "}";


    server.send(
        200,
        "application/json",
        json
    );
}


// ============================================================
// UPDATE WIFI CONNECTION
// ============================================================

void updateWiFiConnection() {

    if (wifiState != WIFI_CONNECTING) {
        return;
    }


    // --------------------------------------------------------
    // Connected
    // --------------------------------------------------------

    if (WiFi.status() == WL_CONNECTED) {

        wifiState =
            WIFI_CONNECTED;


        Serial.println();
        Serial.println("==============================");
        Serial.println("Wi-Fi Connected!");
        Serial.println("==============================");

        Serial.print("SSID: ");
        Serial.println(
            WiFi.SSID()
        );

        Serial.print("IP: ");
        Serial.println(
            WiFi.localIP()
        );

        Serial.print("RSSI: ");
        Serial.println(
            WiFi.RSSI()
        );


        digitalWrite(
            STATUS_LED,
            HIGH
        );

        ledState = true;


        return;
    }


    // --------------------------------------------------------
    // Timeout
    // --------------------------------------------------------

    if (
        millis() - wifiConnectStart
        >= WIFI_CONNECT_TIMEOUT
    ) {

        Serial.println();
        Serial.println(
            "Wi-Fi connection timeout."
        );


        startAP();
    }
}


// ============================================================
// ROOT PAGE
// ============================================================

void handleRoot() {

    server.send_P(
        200,
        "text/html",
        MAIN_PAGE
    );
}


// ============================================================
// NOT FOUND
// ============================================================

void handleNotFound() {

    server.send(
        404,
        "application/json",
        "{\"success\":false,\"message\":\"Not found\"}"
    );
}


// ============================================================
// SETUP
// ============================================================

void setup() {

    Serial.begin(115200);

    delay(500);


    // --------------------------------------------------------
    // LED
    // --------------------------------------------------------

    pinMode(
        STATUS_LED,
        OUTPUT
    );

    digitalWrite(
        STATUS_LED,
        LOW
    );


    // --------------------------------------------------------
    // Start AP
    // --------------------------------------------------------

    startAP();


    // --------------------------------------------------------
    // Web Server
    // --------------------------------------------------------

    server.on(
        "/",
        HTTP_GET,
        handleRoot
    );


    server.on(
        "/api/wifi/scan",
        HTTP_GET,
        handleWiFiScan
    );


    server.on(
        "/api/wifi/connect",
        HTTP_POST,
        handleWiFiConnect
    );


    server.on(
        "/api/status",
        HTTP_GET,
        handleStatus
    );


    server.onNotFound(
        handleNotFound
    );


    server.begin();


    Serial.println();
    Serial.println(
        "HTTP server started."
    );

    Serial.println(
        "Open: http://192.168.4.1/"
    );

    Serial.println(
        "API: /api/wifi/scan"
    );

    Serial.println(
        "API: /api/wifi/connect"
    );

    Serial.println(
        "API: /api/status"
    );
}


// ============================================================
// LOOP
// ============================================================

void loop() {

    server.handleClient();

    updateWiFiConnection();

    updateStatusLED();
}