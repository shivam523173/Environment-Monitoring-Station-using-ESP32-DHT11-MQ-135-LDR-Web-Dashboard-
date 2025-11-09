/*
  Environment Monitoring Station using ESP32
  Sensors: DHT11 (Temp/Humidity), MQ-135 (Air Quality), LDR (Light)
  Actuators: LED (dark indicator), Buzzer (poor air quality)
  Web: HTML dashboard at "/" and JSON API at "/api"

  Notes:
  - Calibrate LDR & MQ_THRESHOLD for your hardware/environment.
  - DHT11 is slow; read sparingly and handle NaN.
  - ADC pins 34/35 are input-only (OK here).
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// -------- Wi-Fi (edit these) --------
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// -------- Sensors / Pins --------
#define DHTPIN   4
#define DHTTYPE  DHT11
const int mq135Pin   = 34;   // ADC1_CH6
const int ldrPin     = 35;   // ADC1_CH7
const int ledPin     = 26;   // Dark indicator
const int buzzerPin  = 27;   // Air quality alarm

// -------- Thresholds (tune these) --------
int LDR_THRESHOLD = 1500;    // lower ADC -> darker (depends on divider)
int MQ_THRESHOLD  = 1800;    // higher ADC -> poorer air

// -------- Globals --------
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

unsigned long lastDhtRead = 0;
float tempC = NAN, humRH = NAN; // cached DHT values

// Forward decl.
void handleRoot();
void handleApi();
void handleNotFound();

String htmlPage(float t, float h, int mq, int ldr) {
  String ledState = (digitalRead(ledPin) == HIGH) ? "ON" : "OFF";
  String buzState = (digitalRead(buzzerPin) == HIGH) ? "ON" : "OFF";
  String ip       = WiFi.localIP().toString();
  long   rssi     = WiFi.RSSI();

  String s = R"====(
  <!DOCTYPE html><html><head><meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <meta http-equiv="refresh" content="2">
  <title>Environment Monitoring</title>
  <style>
    body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial;
         margin:0;background:#0f172a;color:#e2e8f0}
    .wrap{max-width:720px;margin:24px auto;padding:16px}
    h1{font-size:1.5rem;margin:0 0 12px}
    .card{background:#111827;border-radius:12px;padding:16px;margin:12px 0;
          box-shadow:0 6px 20px rgba(0,0,0,.25)}
    .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px}
    .k{font-size:.9rem;color:#93c5fd} .v{font-size:1.2rem}
    .ok{color:#10b981} .bad{color:#ef4444}
    .muted{color:#94a3b8}
    a{color:#93c5fd;text-decoration:none}
  </style></head><body><div class="wrap">
  <h1>Environment Monitoring</h1>
  <div class="card grid">
)====";

  s += "<div><div class='k'>Temperature</div><div class='v'>";
  s += isnan(t) ? "<span class='bad'>N/A</span>" : String(t, 1) + " °C";
  s += "</div></div>";

  s += "<div><div class='k'>Humidity</div><div class='v'>";
  s += isnan(h) ? "<span class='bad'>N/A</span>" : String(h, 1) + " %";
  s += "</div></div>";

  s += "<div><div class='k'>MQ-135</div><div class='v'>" + String(mq) + "</div></div>";
  s += "<div><div class='k'>LDR</div><div class='v'>" + String(ldr) + "</div></div>";

  s += "</div><div class='card grid'>";

  s += "<div><div class='k'>LED (Dark)</div><div class='v ";
  s += (ledState == "ON") ? "ok'>ON" : "bad'>OFF";
  s += "</div></div>";

  s += "<div><div class='k'>Buzzer (Air)</div><div class='v ";
  s += (buzState == "ON") ? "bad'>ON" : "ok'>OFF";
  s += "</div></div>";

  s += "<div><div class='k'>Wi-Fi</div><div class='v'>" + ip + " | RSSI ";
  s += String(rssi) + " dBm</div></div>";

  s += "</div><div class='card muted'>"
       "API: <a href='/api'>/api</a> | Refresh: 2s | "
       "LDR_TH=" + String(LDR_THRESHOLD) + " MQ_TH=" + String(MQ_THRESHOLD) +
       "</div></div></body></html>";
  return s;
}

void setup() {
  Serial.begin(115200);
  delay(50);

  // Pins
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);

  // Sensors
  dht.begin();

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
    if (millis() - t0 > 20000) break; // 20s timeout
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi not connected (serving only after connect).");
  }

  // Routes
  server.on("/", handleRoot);
  server.on("/api", handleApi);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  // Read raw ADCs frequently
  int ldrVal = analogRead(ldrPin);
  int mqVal  = analogRead(mq135Pin);

  // LED: ON when dark
  digitalWrite(ledPin, (ldrVal < LDR_THRESHOLD) ? HIGH : LOW);

  // Buzzer: ON when poor air
  digitalWrite(buzzerPin, (mqVal > MQ_THRESHOLD) ? HIGH : LOW);

  // DHT: slow sensor, read at ~2s cadence
  if (millis() - lastDhtRead > 2000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) tempC = t;
    if (!isnan(h)) humRH = h;
    lastDhtRead = millis();
  }

  delay(50);
}

// -------- Handlers --------
void handleRoot() {
  int mq   = analogRead(mq135Pin);
  int ldr  = analogRead(ldrPin);
  // Use cached DHT values to avoid NaN flashes
  server.send(200, "text/html", htmlPage(tempC, humRH, mq, ldr));
}

void handleApi() {
  int mq   = analogRead(mq135Pin);
  int ldr  = analogRead(ldrPin);
  String json = "{";
  json += "\"temperature_c\":";
  json += isnan(tempC) ? String("null") : String(tempC, 2);
  json += ",\"humidity_pct\":";
  json += isnan(humRH) ? String("null") : String(humRH, 2);
  json += ",\"mq135_raw\":" + String(mq);
  json += ",\"ldr_raw\":" + String(ldr);
  json += ",\"led_dark\":" + String(digitalRead(ledPin) == HIGH ? "true" : "false");
  json += ",\"buzzer_air\":" + String(digitalRead(buzzerPin) == HIGH ? "true" : "false");
  json += ",\"rssi_dbm\":" + String(WiFi.RSSI());
  json += "}";
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}
