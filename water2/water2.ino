

#define BLYNK_TEMPLATE_ID   "TMPL2aaHExTBQ"
#define BLYNK_TEMPLATE_NAME "Regadero automatico"
#define BLYNK_AUTH_TOKEN    "gFf74dx4zgzJUdj7OxDe4p_SkjMS0L7d"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WIFI
char ssid[] = "UnachWiFi";     //red
char pass[] = "";  // contraseña

// ─── PINES 
#define DHT_PIN           4
#define SOIL_MOISTURE_PIN 32
#define WATER_LEVEL_PIN   34
#define LED_PIN           2
#define PUMP_PIN          5

// ─── OLED 
#define OLED_SDA  21
#define OLED_SCL  22
#define OLED_W    128
#define OLED_H    64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);

// ─── DHT22 
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);

// ─── CALIBRACIÓN SENSOR SUELO 
#define RESISTIVE_DRY 4095
#define RESISTIVE_WET 1741

// ─── CALIBRACIÓN SENSOR AGUA 
#define WATER_EMPTY     0
#define WATER_FULL      3675
#define WATER_THRESHOLD 80

// ─── UMBRALES SUELO 
#define SOIL_LOW  40
#define SOIL_HIGH 60

// ─── HISTÉRESIS AGUA 
#define WATER_LOW  15
#define WATER_HIGH 27

// ─── RELÉ 
#define PUMP_ON  LOW   // relé activo LOW
#define PUMP_OFF HIGH

// ─── V-PINS BLYNK 
#define VPIN_SUELO   V1
#define VPIN_TEMP    V2
#define VPIN_HUM     V3
#define VPIN_AGUA    V4
#define VPIN_BOMBA   V5
#define VPIN_ALARMA  V6
#define VPIN_MODO    V7

// ─── ESTADOS GLOBALES 
bool  aguaBaja  = false;
bool  modoAuto  = true;
bool  bombaManual = false;

// ─── TIMER BLYNK 
BlynkTimer timer;

// LECTURAS
float readSoil() {
    int raw = analogRead(SOIL_MOISTURE_PIN);
    float pct = (float)(RESISTIVE_DRY - raw) / (RESISTIVE_DRY - RESISTIVE_WET) * 100.0;
    return constrain(pct, 0.0, 100.0);
}

int readWater() {
    int raw = analogRead(WATER_LEVEL_PIN);
    if (raw < WATER_THRESHOLD) return 0;
    int pct = (int)((float)(raw - WATER_EMPTY) / (WATER_FULL - WATER_EMPTY) * 100.0);
    return constrain(pct, 0, 100);
}


//  CONTROL BOMBA

void setBomba(bool encender) {
    digitalWrite(PUMP_PIN, encender ? PUMP_ON : PUMP_OFF);
    digitalWrite(LED_PIN,  encender ? HIGH : LOW);
    // Sincronizar estado en Blynk
    Blynk.virtualWrite(VPIN_BOMBA, encender ? 1 : 0);
}

void updatePump(float soilPct, int wlevel) {
    // Histéresis agua
    if (wlevel < WATER_LOW)  aguaBaja = true;
    if (wlevel >= WATER_HIGH) aguaBaja = false;

    // En modo manual Blynk controla la bomba
    if (!modoAuto) return;

    if (aguaBaja) {
        setBomba(false);
        return;
    }
    if (soilPct <= SOIL_LOW)  setBomba(true);
    if (soilPct >= SOIL_HIGH) setBomba(false);
}


//  OLED

void oledBarra(int y, int pct) {
    int fill = map(pct, 0, 100, 0, 40);
    oled.drawRect(85, y, 40, 7, WHITE);
    if (fill > 0) oled.fillRect(85, y, fill, 7, WHITE);
}

void oledUpdate(float soil, int water, float temp, float hum) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(WHITE);

    // Línea 1: Temp y humedad
    oled.setCursor(0, 0);
    if (!isnan(temp)) {
        oled.printf("T:%.1fC H:%.0f%%", temp, hum);
    } else {
        oled.print("T:-- H:--");
    }

    // Línea 2: Suelo
    oled.setCursor(0, 16);
    oled.printf("Suelo:%.0f%%", soil);
    oledBarra(16, (int)soil);

    // Línea 3: Agua
    oled.setCursor(0, 32);
    oled.printf("Agua: %d%%", water);
    oledBarra(32, water);

    // Línea 4: Bomba y modo
    oled.setCursor(0, 48);
    bool bombaOn = (digitalRead(PUMP_PIN) == PUMP_ON);
    const char* modo = modoAuto ? "AUTO" : "MAN";
    oled.printf("B:%s [%s]", bombaOn ? "ON " : "OFF", modo);

    oled.display();
}


//  BLYNK - RECIBIR COMANDOS


// V5 → Botón bomba manual
BLYNK_WRITE(VPIN_BOMBA) {
    if (!modoAuto) {
        bombaManual = param.asInt();
        setBomba(bombaManual);
        Serial.printf("  [MANUAL] Bomba: %s\n", bombaManual ? "ON" : "OFF");
    }
}

// V7 → Switch modo auto/manual
BLYNK_WRITE(VPIN_MODO) {
    modoAuto = (param.asInt() == 1);
    Serial.printf("  Modo: %s\n", modoAuto ? "AUTO" : "MANUAL");
    // Al volver a auto, apagar bomba por seguridad
    if (modoAuto) setBomba(false);
}


//  TAREA PRINCIPAL (cada 5 segundos)

void tareaLecturas() {
    float soilPct = readSoil();
    int   wlevel  = readWater();
    float temp    = dht.readTemperature();
    float hum     = dht.readHumidity();

    // Control bomba automático
    updatePump(soilPct, wlevel);

    // Alarma sequía V6
    bool sequia = (soilPct < SOIL_LOW);
    Blynk.virtualWrite(VPIN_ALARMA, sequia ? 1 : 0);
    if (sequia) {
        Blynk.logEvent("sequia", "Humedad baja: " + String((int)soilPct) + "%");
    }

    // Enviar datos a Blynk
    Blynk.virtualWrite(VPIN_SUELO, (int)soilPct);
    Blynk.virtualWrite(VPIN_AGUA,  wlevel);
    if (!isnan(temp)) {
        Blynk.virtualWrite(VPIN_TEMP, temp);
        Blynk.virtualWrite(VPIN_HUM,  (int)hum);
    }

    // OLED
    oledUpdate(soilPct, wlevel, temp, hum);

    // Serial
    Serial.printf("\n--- %lu ms ---\n", millis());
    Serial.printf("  Suelo: %.1f%%\n", soilPct);
    Serial.printf("  Agua:  %d%%  [aguaBaja=%s]\n", wlevel, aguaBaja ? "true" : "false");
    if (!isnan(temp)) {
        Serial.printf("  Temp: %.1f°C  Hum: %.1f%%\n", temp, hum);
    } else {
        Serial.println("  Temp/Hum: error");
    }
    Serial.printf("  Modo: %s  Bomba: %s\n",
        modoAuto ? "AUTO" : "MANUAL",
        digitalRead(PUMP_PIN) == PUMP_ON ? "ON" : "OFF");
}


//  SETUP

void setup() {
    Serial.begin(115200);

    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LED_PIN,  OUTPUT);
    digitalWrite(PUMP_PIN, PUMP_OFF);
    digitalWrite(LED_PIN,  LOW);

    // OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("Error OLED");
    }
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(WHITE);
    oled.setCursor(16, 10);
    oled.print("Riego ESP32");
    oled.setCursor(8, 40);
    oled.print("Conectando...");
    oled.display();

    // DHT
    dht.begin();

    // Blynk + WiFi
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

    // Estado inicial en Blynk
    Blynk.virtualWrite(VPIN_BOMBA,  0);
    Blynk.virtualWrite(VPIN_ALARMA, 0);
    Blynk.virtualWrite(VPIN_MODO,   1);

    // Timer: lecturas cada 5 segundos
    timer.setInterval(5000L, tareaLecturas);

    Serial.println("  Sistema listo.");
}


void loop() {
    Blynk.run();
    timer.run();
}