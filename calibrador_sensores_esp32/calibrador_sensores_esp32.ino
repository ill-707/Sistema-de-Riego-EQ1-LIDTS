

// ─── PINES 
#define SOIL_MOISTURE_PIN  32
#define WATER_LEVEL_PIN    34


void separador(char c = '-', int n = 54) {
  for (int i = 0; i < n; i++) Serial.print(c);
  Serial.println();
}

void titulo(const char* texto) {
  separador('=');
  Serial.print("  ");
  Serial.println(texto);
  separador('=');
}

// Toma N muestras y devuelve promedio, min y max por referencia
int leerPromedio(int pin, int muestras, int delayMs, int &minVal, int &maxVal) {
  long suma = 0;
  minVal = 4095;
  maxVal = 0;
  for (int i = 0; i < muestras; i++) {
    int v = analogRead(pin);
    suma += v;
    if (v < minVal) minVal = v;
    if (v > maxVal) maxVal = v;
    delay(delayMs);
  }
  return (int)(suma / muestras);
}

// Muestra lecturas en vivo durante N segundos
void monitoreoEnVivo(int pin, const char* etiqueta, int segundos) {
  Serial.print("  Lecturas en vivo (");
  Serial.print(segundos);
  Serial.println(" s):");
  unsigned long tFin = millis() + (unsigned long)segundos * 1000;
  while (millis() < tFin) {
    Serial.print("    ");
    Serial.print(etiqueta);
    Serial.print(" ADC: ");
    Serial.println(analogRead(pin));
    delay(200);
  }
}

// Espera a que el usuario envíe cualquier carácter por el Monitor Serie
void esperarEnter(const char* mensaje = "  Envía cualquier carácter para continuar...") {
  Serial.println(mensaje);
  while (Serial.available() == 0) {
    delay(50);
  }
  // Vaciar el buffer
  while (Serial.available()) Serial.read();
}


//  CALIBRACIÓN SENSOR DE SUELO (FC-28 resistivo)

void calibrarSuelo(int &avgDry, int &avgWet) {
  titulo("CALIBRACION - SENSOR DE SUELO (FC-28)");
  Serial.println(F(
    "\n  El sensor resistivo da:\n"
    "    - Valor ALTO  cuando esta SECO   (= RESISTIVE_DRY)\n"
    "    - Valor BAJO  cuando esta HUMEDO (= RESISTIVE_WET)\n\n"
    "  Necesitaras:\n"
    "    - Un vaso con agua\n"
    "    - Tierra o maceta (opcional)\n"
  ));
  separador();

  // ── Paso 1: Seco (aire) ────────────────────────────────
  Serial.println(F("\n  PASO 1 DE 3 -> VALOR EN SECO"));
  Serial.println(F("  Deja el sensor en el AIRE (sin tocar nada)"));
  esperarEnter();
  monitoreoEnVivo(SOIL_MOISTURE_PIN, "SUELO", 5);
  int mnDry, mxDry;
  avgDry = leerPromedio(SOIL_MOISTURE_PIN, 30, 50, mnDry, mxDry);
  Serial.print(F("  OK Promedio SECO  : ")); Serial.print(avgDry);
  Serial.print(F("  (min: ")); Serial.print(mnDry);
  Serial.print(F("  max: ")); Serial.print(mxDry); Serial.println(F(")"));
  separador();

  // ── Paso 2: Húmedo (agua) ─────────────────────────────
  Serial.println(F("\n  PASO 2 DE 3 -> VALOR EN HUMEDO"));
  Serial.println(F("  Introduce el sensor en agua hasta las marcas"));
  Serial.println(F("  AVISO: No sumergir la parte electronica"));
  esperarEnter();
  monitoreoEnVivo(SOIL_MOISTURE_PIN, "SUELO", 5);
  int mnWet, mxWet;
  avgWet = leerPromedio(SOIL_MOISTURE_PIN, 30, 50, mnWet, mxWet);
  Serial.print(F("  OK Promedio HUMEDO: ")); Serial.print(avgWet);
  Serial.print(F("  (min: ")); Serial.print(mnWet);
  Serial.print(F("  max: ")); Serial.print(mxWet); Serial.println(F(")"));
  separador();

  // ── Paso 3: Verificación ──────────────────────────────
  Serial.println(F("\n  PASO 3 DE 3 -> VERIFICACION"));
  Serial.println(F("  Mueve el sensor entre seco y humedo."));
  Serial.println(F("  Deberias ver el porcentaje cambiar de 0% a 100%."));
  esperarEnter();
  Serial.println(F("  Mostrando porcentaje en vivo (10 s)..."));
  unsigned long tFin = millis() + 10000;
  while (millis() < tFin) {
    int raw = analogRead(SOIL_MOISTURE_PIN);
    float pct = 0.0;
    if (avgDry != avgWet) {
      pct = (float)(avgDry - raw) / (float)(avgDry - avgWet) * 100.0;
      if (pct < 0.0) pct = 0.0;
      if (pct > 100.0) pct = 100.0;
    }
    int barLen = (int)(pct / 5.0);
    Serial.print(F("  ADC: "));
    Serial.print(raw);
    Serial.print(F("  ["));
    for (int i = 0; i < 20; i++) Serial.print(i < barLen ? '#' : '.');
    Serial.print(F("] "));
    Serial.print(pct, 1);
    Serial.println(F("%"));
    delay(200);
  }
  Serial.println();
  separador();

  // ── Resultado ─────────────────────────────────────────
  Serial.println(F("\n  == RESULTADO SENSOR DE SUELO =="));
  Serial.print(F("  RESISTIVE_DRY = ")); Serial.print(avgDry);
  Serial.println(F("  <- copia este valor"));
  Serial.print(F("  RESISTIVE_WET = ")); Serial.print(avgWet);
  Serial.println(F("  <- copia este valor"));

  int rango = abs(avgDry - avgWet);
  Serial.print(F("\n  Rango dinamico: ")); Serial.print(rango);
  Serial.println(F(" puntos ADC"));
  if (rango < 300)
    Serial.println(F("  AVISO: Rango bajo. Revisa conexiones o prueba otro sensor."));
  else if (rango < 800)
    Serial.println(F("  INFO:  Rango aceptable. Funcionara con precision moderada."));
  else
    Serial.println(F("  OK:    Rango excelente. Buena precision esperada."));
}


//  CALIBRACIÓN SENSOR DE NIVEL DE AGUA

void calibrarAgua(int &avgEmpty, int &avgFull, int &threshold) {
  titulo("CALIBRACION - SENSOR DE NIVEL DE AGUA");
  Serial.println(F(
    "\n  Necesitaras un recipiente con agua donde puedas\n"
    "  subir y bajar el nivel del sensor manualmente.\n\n"
    "  El sensor da:\n"
    "    - Valor BAJO  fuera del agua  (= WATER_EMPTY)\n"
    "    - Valor ALTO  sumergido       (= WATER_FULL)\n"
  ));
  separador();

  // ── Paso 1: Vacío ─────────────────────────────────────
  Serial.println(F("\n  PASO 1 DE 3 -> SIN AGUA"));
  Serial.println(F("  Manten el sensor FUERA del agua / en el aire"));
  esperarEnter();
  monitoreoEnVivo(WATER_LEVEL_PIN, "AGUA ", 5);
  int mnE, mxE;
  avgEmpty = leerPromedio(WATER_LEVEL_PIN, 30, 50, mnE, mxE);
  Serial.print(F("  OK Promedio VACIO : ")); Serial.print(avgEmpty);
  Serial.print(F("  (min: ")); Serial.print(mnE);
  Serial.print(F("  max: ")); Serial.print(mxE); Serial.println(F(")"));

  int margen = (mxE - mnE) * 2 + 50;
  if (margen < 80) margen = 80;
  threshold = avgEmpty + margen;
  separador();

  // ── Paso 2: Lleno ─────────────────────────────────────
  Serial.println(F("\n  PASO 2 DE 3 -> NIVEL MAXIMO"));
  Serial.println(F("  Sumerge el sensor hasta el nivel MAXIMO que usaras"));
  esperarEnter();
  monitoreoEnVivo(WATER_LEVEL_PIN, "AGUA ", 5);
  int mnF, mxF;
  avgFull = leerPromedio(WATER_LEVEL_PIN, 30, 50, mnF, mxF);
  Serial.print(F("  OK Promedio LLENO : ")); Serial.print(avgFull);
  Serial.print(F("  (min: ")); Serial.print(mnF);
  Serial.print(F("  max: ")); Serial.print(mxF); Serial.println(F(")"));
  separador();

  // ── Paso 3: Verificación ──────────────────────────────
  Serial.println(F("\n  PASO 3 DE 3 -> VERIFICACION"));
  Serial.println(F("  Sube y baja el sensor en el agua."));
  Serial.println(F("  Deberias ver el nivel cambiar de 0% a 100%."));
  esperarEnter();
  Serial.println(F("  Mostrando nivel en vivo (10 s)..."));
  unsigned long tFin = millis() + 10000;
  while (millis() < tFin) {
    int raw = analogRead(WATER_LEVEL_PIN);
    float pct = 0.0;
    if (raw >= threshold && avgFull != avgEmpty) {
      pct = (float)(raw - avgEmpty) / (float)(avgFull - avgEmpty) * 100.0;
      if (pct < 0.0) pct = 0.0;
      if (pct > 100.0) pct = 100.0;
    }
    int barLen = (int)(pct / 5.0);
    Serial.print(F("  ADC: "));
    Serial.print(raw);
    Serial.print(F("  ["));
    for (int i = 0; i < 20; i++) Serial.print(i < barLen ? '#' : '.');
    Serial.print(F("] "));
    Serial.print(pct, 1);
    Serial.println(F("%"));
    delay(200);
  }
  Serial.println();
  separador();

  // ── Resultado ─────────────────────────────────────────
  Serial.println(F("\n  == RESULTADO SENSOR DE AGUA =="));
  Serial.print(F("  WATER_EMPTY     = ")); Serial.print(avgEmpty);
  Serial.println(F("  <- copia este valor"));
  Serial.print(F("  WATER_FULL      = ")); Serial.print(avgFull);
  Serial.println(F("  <- copia este valor"));
  Serial.print(F("  WATER_THRESHOLD = ")); Serial.print(threshold);
  Serial.println(F("  <- umbral deteccion"));

  int rango = abs(avgFull - avgEmpty);
  Serial.print(F("\n  Rango dinamico: ")); Serial.print(rango);
  Serial.println(F(" puntos ADC"));
  if (rango < 200)
    Serial.println(F("  AVISO: Rango muy bajo. Revisa posicion y conexiones."));
  else if (rango < 500)
    Serial.println(F("  INFO:  Rango aceptable."));
  else
    Serial.println(F("  OK:    Rango bueno."));
}


//  RESUMEN FINAL

void imprimirResumen(int dry, int wet, int wEmpty, int wFull, int wThr) {
  titulo("RESUMEN - COPIA ESTOS VALORES EN TUS SKETCHES");

  Serial.println(F("\n  // ── Sensor de suelo resistivo FC-28 ──"));
  Serial.print(F("  const int RESISTIVE_DRY = ")); Serial.print(dry);  Serial.println(F(";"));
  Serial.print(F("  const int RESISTIVE_WET = ")); Serial.print(wet);  Serial.println(F(";"));

  Serial.println(F("\n  // ── Sensor de nivel de agua ─────────"));
  Serial.print(F("  const int WATER_EMPTY     = ")); Serial.print(wEmpty); Serial.println(F(";"));
  Serial.print(F("  const int WATER_FULL      = ")); Serial.print(wFull);  Serial.println(F(";"));
  Serial.print(F("  const int WATER_THRESHOLD = ")); Serial.print(wThr);   Serial.println(F(";"));

  Serial.println(F("\n  // ── Funcion readWater() ─────────────"));
  Serial.println(F("  int readWater() {"));
  Serial.println(F("    int raw = analogRead(WATER_LEVEL_PIN);"));
  Serial.print(F("    if (raw < ")); Serial.print(wThr); Serial.println(F(") return 0;"));
  Serial.print(F("    int val = (int)((float)(raw - ")); Serial.print(wEmpty);
  Serial.print(F(") / (float)(")); Serial.print(wFull);
  Serial.print(F(" - ")); Serial.print(wEmpty); Serial.println(F(") * 100);"));
  Serial.println(F("    return constrain(val, 0, 100);"));
  Serial.println(F("  }"));

  Serial.println(F("\n  // ── Funcion readSoil() ──────────────"));
  Serial.println(F("  int readSoil() {"));
  Serial.println(F("    int raw = analogRead(SOIL_MOISTURE_PIN);"));
  Serial.print(F("    int pct = (int)((float)(")); Serial.print(dry);
  Serial.print(F(" - raw) / (float)(")); Serial.print(dry);
  Serial.print(F(" - ")); Serial.print(wet); Serial.println(F(") * 100);"));
  Serial.println(F("    return constrain(pct, 0, 100);"));
  Serial.println(F("  }"));

  separador('=');
}


//  MONITOR RÁPIDO

void monitorRapido(int segundos = 30) {
  char buf[60];
  snprintf(buf, sizeof(buf), "MONITOR EN VIVO - %d segundos", segundos);
  titulo(buf);
  Serial.print(F("  ADC suelo (pin "));
  Serial.print(SOIL_MOISTURE_PIN);
  Serial.print(F(") y agua (pin "));
  Serial.print(WATER_LEVEL_PIN);
  Serial.println(F(")"));
  separador();
  unsigned long tFin = millis() + (unsigned long)segundos * 1000;
  while (millis() < tFin) {
    int s = analogRead(SOIL_MOISTURE_PIN);
    int w = analogRead(WATER_LEVEL_PIN);
    Serial.print(F("  Suelo: ")); Serial.print(s);
    Serial.print(F("   Agua: ")); Serial.println(w);
    delay(300);
  }
  Serial.println();
}


//  MENÚ PRINCIPAL

char mostrarMenu() {
  titulo("CALIBRADOR DE SENSORES - ESP32");
  Serial.println(F(
    "\n  Opciones:\n"
    "    1 -> Calibrar sensor de SUELO solamente\n"
    "    2 -> Calibrar sensor de AGUA  solamente\n"
    "    3 -> Calibrar AMBOS sensores (recomendado)\n"
    "    4 -> Monitor en vivo (ver valores crudos)\n"
  ));
  Serial.println(F("  Envia el numero de opcion (1-4):"));
  while (Serial.available() == 0) delay(50);
  char op = Serial.read();
  while (Serial.available()) Serial.read(); // limpiar buffer
  return op;
}

void setup() {
  Serial.begin(115200);
  delay(1000); // espera a que el Monitor Serie se conecte


  analogReadResolution(12);

  int dry = 0, wet = 0, wEmpty = 0, wFull = 0, wThr = 0;
  bool tieneSuelo = false, tieneAgua = false;

  char opcion = mostrarMenu();

  if (opcion == '1') {
    calibrarSuelo(dry, wet);
    tieneSuelo = true;
  } else if (opcion == '2') {
    calibrarAgua(wEmpty, wFull, wThr);
    tieneAgua = true;
  } else if (opcion == '3') {
    calibrarSuelo(dry, wet);
    tieneSuelo = true;
    Serial.println(F("\n  Continuando con sensor de agua...\n"));
    delay(2000);
    calibrarAgua(wEmpty, wFull, wThr);
    tieneAgua = true;
  } else if (opcion == '4') {
    monitorRapido(30);
    Serial.println(F("  Monitor finalizado. Reinicia para volver al menu."));
    return;
  } else {
    Serial.println(F("  Opcion no valida. Reinicia el ESP32 para intentarlo de nuevo."));
    return;
  }

  if (tieneSuelo && tieneAgua) {
    imprimirResumen(dry, wet, wEmpty, wFull, wThr);
  } else if (tieneSuelo) {
    titulo("VALORES SENSOR DE SUELO");
    Serial.print(F("  RESISTIVE_DRY = ")); Serial.println(dry);
    Serial.print(F("  RESISTIVE_WET = ")); Serial.println(wet);
  } else if (tieneAgua) {
    titulo("VALORES SENSOR DE AGUA");
    Serial.print(F("  WATER_EMPTY     = ")); Serial.println(wEmpty);
    Serial.print(F("  WATER_FULL      = ")); Serial.println(wFull);
    Serial.print(F("  WATER_THRESHOLD = ")); Serial.println(wThr);
  }

  Serial.println(F("\n  Calibracion completa. Reinicia el ESP32 para repetir."));
}

void loop() {
 
  delay(1000);
}
