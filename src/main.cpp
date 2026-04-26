#include <Arduino.h>
#include "WiFiS3.h"
#include <ArduinoJson.h>
#include <ArduinoGraphics.h>
#include <Arduino_LED_Matrix.h>
#include "arduino_secrets.h"

// GPS-Koordinaten anpassen
#define LATITUDE  "51.3397"   // Leipzig
#define LONGITUDE "12.3731"

#define FETCH_INTERVAL_MS (15UL * 60UL * 1000UL)
#define CLOUD_DURATION_MS  5000UL
#define CLOUD_FRAME_MS      400UL
#define CLOUD_FRAMES           4

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

const char API_HOST[] = "api.open-meteo.com";
const char API_PATH[] =
    "/v1/forecast"
    "?latitude=" LATITUDE "&longitude=" LONGITUDE
    "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code"
    "&timezone=auto";

WiFiClient       client;
ArduinoLEDMatrix matrix;

float temperature = 0.0f;
int   humidity    = 0;
float windSpeed   = 0.0f;
unsigned long lastFetch = 0;

void connectWifi() {
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("WiFi-Modul nicht gefunden!");
        while (true) {}
    }
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print("Verbinde mit "); Serial.println(ssid);
        WiFi.begin(ssid, pass);
        delay(10000);
    }
    Serial.print("WiFi verbunden. IP: "); Serial.println(WiFi.localIP());
}

void fetchWeather() {
    Serial.println("\n=== Wetterdaten ===");
    if (WiFi.status() != WL_CONNECTED) connectWifi();

    if (!client.connect(API_HOST, 80)) {
        Serial.println("Verbindung fehlgeschlagen.");
        return;
    }

    client.print("GET "); client.print(API_PATH); client.println(" HTTP/1.0");
    client.print("Host: "); client.println(API_HOST);
    client.println("Connection: close");
    client.println();

    unsigned long t = millis();
    while (!client.available()) {
        if (millis() - t > 10000) { Serial.println("Timeout."); client.stop(); return; }
    }

    while (client.available()) {
        String line = client.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) break;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, client);
    client.stop();

    if (err) { Serial.print("JSON-Fehler: "); Serial.println(err.c_str()); return; }

    JsonObject cur = doc["current"];
    temperature = cur["temperature_2m"].as<float>();
    humidity    = cur["relative_humidity_2m"].as<int>();
    windSpeed   = cur["wind_speed_10m"].as<float>();

    Serial.print("Temperatur:       "); Serial.print(temperature); Serial.println(" °C");
    Serial.print("Luftfeuchtigkeit: "); Serial.print(humidity);    Serial.println(" %");
    Serial.print("Windgeschw.:      "); Serial.print(windSpeed);   Serial.println(" km/h");
}

// ── LED-Matrix ──────────────────────────────────────────────────────────────
//
// 12×8-Matrix: LED(row,col) → Bit 95-(row*12+col) im 96-Bit-Wert
// frame[0]=Bits 95-64, frame[1]=Bits 63-32, frame[2]=Bits 31-0
static void setPixel(uint32_t f[3], int col, int row, bool on) {
    if (col < 0 || col > 11 || row < 0 || row > 7) return;
    int bit = 95 - (row * 12 + col);
    int idx = 2 - bit / 32;
    int pos = bit % 32;
    if (on) f[idx] |=  (1UL << pos);
    else    f[idx] &= ~(1UL << pos);
}

// 4 Animations-Frames: [0] Wolke, [1-3] Regen wandert nach unten
static uint32_t cloudFrames[CLOUD_FRAMES][3];

static void buildCloudFrames() {
    const int rainCols[] = { 2, 6, 9 };

    for (int fr = 0; fr < CLOUD_FRAMES; fr++) {
        cloudFrames[fr][0] = cloudFrames[fr][1] = cloudFrames[fr][2] = 0;

        // Wolkenkörper (Reihen 0-4)
        setPixel(cloudFrames[fr], 4, 0, true);
        setPixel(cloudFrames[fr], 5, 0, true);
        for (int c = 3; c <= 6; c++) setPixel(cloudFrames[fr], c, 1, true);
        for (int c = 2; c <= 7; c++) setPixel(cloudFrames[fr], c, 2, true);
        for (int c = 1; c <= 9; c++) setPixel(cloudFrames[fr], c, 3, true);
        for (int c = 0; c <= 10; c++) setPixel(cloudFrames[fr], c, 4, true);
    }

    // Regen-Frames 1-3: Tropfen wandern von Reihe 5 → 7
    for (int drop = 0; drop < 3; drop++) {
        for (int row = 5; row <= 7; row++) {
            setPixel(cloudFrames[row - 4], rainCols[drop], row, true);
        }
    }
}

void showCloud(unsigned long durationMs) {
    unsigned long start = millis();
    int frame = 0;
    while (millis() - start < durationMs) {
        uint32_t f[3];
        memcpy(f, cloudFrames[frame], sizeof(f));
        matrix.loadFrame(f);
        frame = (frame + 1) % CLOUD_FRAMES;
        delay(CLOUD_FRAME_MS);
    }
}

void scrollText(const char* text) {
    matrix.beginDraw();
    matrix.stroke(0xFFFFFFFF);
    matrix.textScrollSpeed(500);
    matrix.textFont(Font_5x7);
    matrix.beginText(0, 1, 0xFFFFFF);
    matrix.print(text);
    matrix.endText(SCROLL_LEFT);
    matrix.endDraw();
}

void showWeatherDisplay() {
    char buf[16];
    char tmp[8];

    // Temperatur, z.B. "18.5C"
    dtostrf(temperature, 1, 1, tmp);
    snprintf(buf, sizeof(buf), "%sC", tmp);
    scrollText(buf);

    // Luftfeuchtigkeit, z.B. "65%"
    snprintf(buf, sizeof(buf), "%d%%", humidity);
    scrollText(buf);

    // Windgeschwindigkeit, z.B. "12km/h"
    snprintf(buf, sizeof(buf), "%dkm/h", (int)lroundf(windSpeed));
    scrollText(buf);
    showCloud(CLOUD_DURATION_MS);
}

void setup() {
    Serial.begin(9600);
    unsigned long t = millis();
    while (!Serial && millis() - t < 5000) {}

    matrix.begin();
    buildCloudFrames();

    connectWifi();
    fetchWeather();
    lastFetch = millis();
}

void loop() {
    if (millis() - lastFetch >= FETCH_INTERVAL_MS) {
        fetchWeather();
        lastFetch = millis();
    }
    showWeatherDisplay();
}
