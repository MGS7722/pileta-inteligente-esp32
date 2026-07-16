// ============================================================
//   PILETA INTELIGENTE  —  Control con ESP32 + Telegram
// ============================================================
//
//   Un solo programa que maneja los tres sistemas de la pileta
//   sobre el mismo ESP32, controlables desde un bot de Telegram:
//
//     1) CALENTADOR   -> Sensor de temperatura DS18B20 + relé.
//                        Automático con histéresis, o forzado
//                        ON/OFF desde Telegram.
//     2) LUCES DISCO  -> Sensor de sonido + 4 LEDs que bailan
//                        al ritmo de la música (análisis FFT).
//                        Automático, o forzado ON/OFF.
//     3) PANTALLA LCD -> Muestra temperatura y estado en vivo.
//
//   ------------------------------------------------------------
//   ANTES DE CARGAR:
//     - Completá tus datos en el archivo  config.h
//     - Instalá las librerías indicadas en el README.md
//       (OJO: ArduinoJson tiene que ser la versión 6, NO la 7)
//   ------------------------------------------------------------
//
//   CONEXIONES (pines del ESP32):
//     GPIO4   -> DS18B20 (dato)   [resistencia 4.7k a 3.3V]
//     GPIO26  -> Relé (señal S)   [calentador]
//     GPIO21  -> LCD SDA (I2C)
//     GPIO22  -> LCD SCL (I2C)
//     GPIO16  -> LED 1 (luces disco)
//     GPIO17  -> LED 2
//     GPIO18  -> LED 3
//     GPIO19  -> LED 4
//     GPIO34  -> Sensor de sonido (salida analógica)
// ============================================================

#include "config.h"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <arduinoFFT.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// ============================================================
//   PINES
// ============================================================

#define PIN_DS18B20 4     // Sensor de temperatura del agua
#define PIN_RELE    26    // Señal del relé que prende el calentador

#define LED_1 16          // Luces disco
#define LED_2 17
#define LED_3 18
#define LED_4 19

#define MIC_PIN 34        // Sensor de sonido. ADC1 (no usar ADC2: choca con WiFi).

// ============================================================
//   AJUSTES DEL CALENTADOR
// ============================================================

const float TEMP_OBJETIVO = 23.0;   // Temperatura buscada (subir a ~30 para uso real)
const float HISTERESIS    = 5.0;    // Margen para no prender/apagar a cada rato

// El relé es activo-bajo: LOW lo prende, HIGH lo apaga.
const int RELE_ON  = LOW;
const int RELE_OFF = HIGH;

// ============================================================
//   AJUSTES DE LAS LUCES / SONIDO (análisis FFT)
// ============================================================

#define SAMPLES 128                 // Cantidad de muestras (debe ser potencia de 2)
#define SAMPLING_FREQUENCY 10000    // Frecuencia de muestreo en Hz

// Bandas de frecuencia para distinguir el tipo de música
const int BANDA_GRAVES_MIN = 60;
const int BANDA_GRAVES_MAX = 250;
const int BANDA_AGUDOS_MIN = 2000;
const int BANDA_AGUDOS_MAX = 6000;

// Si graves > agudos * UMBRAL => predomina el bajo (Música A)
const float  UMBRAL_CLASIFICACION = 1.3;
const double UMBRAL_SILENCIO       = 100.0;  // Energía mínima para considerar que hay sonido

// ============================================================
//   TIEMPOS
// ============================================================

const unsigned long INTERVALO_TEMP     = 2000;   // Cada cuánto se lee la temperatura (ms)
const unsigned long INTERVALO_TELEGRAM = 2500;   // Cada cuánto se revisan mensajes (ms)
const unsigned long WIFI_TIMEOUT_MS    = 15000;  // Cuánto esperar al WiFi antes de rendirse

// ============================================================
//   OBJETOS PRINCIPALES
// ============================================================

OneWire            oneWire(PIN_DS18B20);
DallasTemperature  sensores(&oneWire);
LiquidCrystal_I2C  lcd(0x27, 16, 2);

double vReal[SAMPLES];
double vImag[SAMPLES];
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

WiFiClientSecure     client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ============================================================
//   ESTADO DEL SISTEMA
// ============================================================

// --- Calentador ---
enum ModoCalentador { CALEF_AUTO, CALEF_ON, CALEF_OFF };
ModoCalentador modoCalentador = CALEF_AUTO;   // Por defecto: automático por temperatura
bool  calentadorEncendido = false;
float ultimaTemp = 0.0;
bool  sensorTempOk = false;

// --- Luces ---
enum TipoMusica { MUSICA_A, MUSICA_B, SIN_SONIDO };
TipoMusica tipoMusicaActual = SIN_SONIDO;

enum ModoLuces { LUCES_AUTO, LUCES_ON, LUCES_OFF };
ModoLuces modoLuces = LUCES_AUTO;   // Por defecto: reactivas al sonido

double ultimaEnergiaGraves = 0;
double ultimaEnergiaAgudos = 0;
double ultimaEnergiaTotal  = 0;
double ultimoVolumen = 0;
int    ultimoBrillo  = 0;

// --- Tiempos y conexión ---
unsigned long ultimoTiempoTemp    = 0;
unsigned long ultimoCheckTelegram = 0;
bool telegramListo = false;

// ============================================================
//   SETUP
// ============================================================

void setup() {
  Serial.begin(115200);

  // Calentador apagado al arrancar
  pinMode(PIN_RELE, OUTPUT);
  digitalWrite(PIN_RELE, RELE_OFF);

  // Luces
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);
  prenderTodasLasLuces();   // Arranque visible; después manda el modo AUTO

  // Sensor de temperatura
  sensores.begin();
  sensores.setResolution(12);

  // Pantalla
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Control Pileta");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(2000);
  lcd.clear();

  // Sensor de sonido
  pinMode(MIC_PIN, INPUT);
  analogReadResolution(12);                    // Lecturas de 0 a 4095
  analogSetPinAttenuation(MIC_PIN, ADC_11db);   // Rango ~0-3.3V

  // WiFi + Telegram
  conectarWiFiTelegram();

  Serial.println("Sistema listo.");
  Serial.println("Calentador: AUTO | Luces: AUTO (reactivas al sonido).");
}

// ============================================================
//   LOOP
// ============================================================

void loop() {
  // 1) Sonido: se muestrea siempre para que las luces y /audio
  //    tengan datos actualizados.
  muestrearAudio();
  clasificarMusica();
  actualizarLucesSegunModo();

  // 2) Temperatura + LCD, cada INTERVALO_TEMP.
  if (millis() - ultimoTiempoTemp >= INTERVALO_TEMP) {
    ultimoTiempoTemp = millis();
    actualizarTemperatura();
  }

  // 3) Telegram, cada INTERVALO_TELEGRAM.
  procesarTelegram();
}

// ============================================================
//   CALENTADOR
// ============================================================

void actualizarTemperatura() {
  sensores.requestTemperatures();
  float temp = sensores.getTempCByIndex(0);

  // Si el sensor está desconectado, apagamos el calentador SIEMPRE
  // (medida de seguridad, aunque esté en modo manual ON).
  if (temp == DEVICE_DISCONNECTED_C) {
    sensorTempOk = false;
    apagarCalentador();

    lcd.setCursor(0, 0);
    lcd.print("ERROR SENSOR!   ");
    lcd.setCursor(0, 1);
    lcd.print("Revisar cables  ");

    Serial.println("ERROR: sensor desconectado. Calentador apagado por seguridad.");
    return;
  }

  sensorTempOk = true;
  ultimaTemp = temp;

  // Decidir el estado del calentador según el modo elegido.
  switch (modoCalentador) {
    case CALEF_ON:
      if (!calentadorEncendido) prenderCalentador();
      break;

    case CALEF_OFF:
      if (calentadorEncendido) apagarCalentador();
      break;

    case CALEF_AUTO:
    default:
      if (!calentadorEncendido && temp < (TEMP_OBJETIVO - HISTERESIS)) {
        prenderCalentador();
      } else if (calentadorEncendido && temp >= TEMP_OBJETIVO) {
        apagarCalentador();
      }
      break;
  }

  // Mostrar en la pantalla
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp, 1);
  lcd.print((char)223);   // símbolo de grado
  lcd.print("C     ");

  lcd.setCursor(0, 1);
  lcd.print(calentadorEncendido ? "Calor: ON       " : "Calor: OFF      ");

  // Mostrar por el Monitor Serie
  Serial.print("Temp: ");
  Serial.print(temp, 1);
  Serial.print(" C | Calentador: ");
  Serial.print(calentadorEncendido ? "ON" : "OFF");
  Serial.print(" (");
  Serial.print(modoCalentadorTexto());
  Serial.print(") | Luces: ");
  Serial.println(modoLucesTexto());
}

void prenderCalentador() {
  calentadorEncendido = true;
  digitalWrite(PIN_RELE, RELE_ON);
  Serial.println(">>> Calentador ENCENDIDO");
}

void apagarCalentador() {
  calentadorEncendido = false;
  digitalWrite(PIN_RELE, RELE_OFF);
  Serial.println(">>> Calentador APAGADO");
}

// ============================================================
//   SONIDO + LUCES
// ============================================================

// Toma SAMPLES muestras del micrófono a SAMPLING_FREQUENCY Hz y calcula la FFT.
void muestrearAudio() {
  unsigned long periodo = 1000000UL / SAMPLING_FREQUENCY;  // microsegundos entre muestras

  for (int i = 0; i < SAMPLES; i++) {
    unsigned long tiempoMicros = micros();
    vReal[i] = analogRead(MIC_PIN);
    vImag[i] = 0;
    while (micros() - tiempoMicros < periodo) {
      // espera para mantener constante la frecuencia de muestreo
    }
  }

  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  FFT.compute(FFTDirection::Forward);
  FFT.complexToMagnitude();
}

// Compara la energía de graves vs. agudos y define el tipo de música.
void clasificarMusica() {
  double energiaGraves = 0;
  double energiaAgudos = 0;

  for (int i = 1; i < (SAMPLES / 2); i++) {
    double frecuencia = (i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES;

    if (frecuencia >= BANDA_GRAVES_MIN && frecuencia <= BANDA_GRAVES_MAX) {
      energiaGraves += vReal[i];
    }
    if (frecuencia >= BANDA_AGUDOS_MIN && frecuencia <= BANDA_AGUDOS_MAX) {
      energiaAgudos += vReal[i];
    }
  }

  ultimaEnergiaGraves = energiaGraves;
  ultimaEnergiaAgudos = energiaAgudos;
  ultimaEnergiaTotal  = energiaGraves + energiaAgudos;

  if (ultimaEnergiaTotal < UMBRAL_SILENCIO) {
    tipoMusicaActual = SIN_SONIDO;
  } else if (energiaGraves > energiaAgudos * UMBRAL_CLASIFICACION) {
    tipoMusicaActual = MUSICA_A;   // Predominan los graves (reggaetón, electrónica)
  } else {
    tipoMusicaActual = MUSICA_B;   // Predominan agudos/medios (voz, acústico)
  }
}

// Aplica el modo de luces elegido (AUTO / ON / OFF).
void actualizarLucesSegunModo() {
  if (modoLuces == LUCES_OFF) {
    apagarTodasLasLuces();
    return;
  }
  if (modoLuces == LUCES_ON) {
    prenderTodasLasLuces();
    return;
  }
  actualizarLucesSonido();   // LUCES_AUTO
}

// Modo AUTO: patrón de luces según el tipo de música detectado.
void actualizarLucesSonido() {
  // Volumen general (promedio del espectro), usado para la intensidad.
  double volumen = 0;
  for (int i = 1; i < (SAMPLES / 2); i++) {
    volumen += vReal[i];
  }
  volumen /= (SAMPLES / 2);

  int brillo = constrain(map((long)volumen, 0, 500, 0, 255), 0, 255);

  ultimoVolumen = volumen;
  ultimoBrillo  = brillo;

  switch (tipoMusicaActual) {

    case MUSICA_A: {
      // Graves: las 4 luces laten juntas.
      analogWrite(LED_1, brillo);
      analogWrite(LED_2, brillo);
      analogWrite(LED_3, brillo);
      analogWrite(LED_4, brillo);
      break;
    }

    case MUSICA_B: {
      // Agudos/voz: barrido secuencial de una luz a la vez.
      static int posicion = 0;
      static unsigned long ultimoPaso = 0;
      unsigned long ahora = millis();

      if (ahora - ultimoPaso > 100) {
        ultimoPaso = ahora;
        posicion = (posicion + 1) % 4;
      }

      int pines[4] = {LED_1, LED_2, LED_3, LED_4};
      for (int i = 0; i < 4; i++) {
        analogWrite(pines[i], (i == posicion) ? brillo : 0);
      }
      break;
    }

    case SIN_SONIDO:
    default: {
      // Sin música: luces fijas encendidas.
      prenderTodasLasLuces();
      break;
    }
  }
}

void prenderTodasLasLuces() {
  analogWrite(LED_1, 255);
  analogWrite(LED_2, 255);
  analogWrite(LED_3, 255);
  analogWrite(LED_4, 255);
}

void apagarTodasLasLuces() {
  analogWrite(LED_1, 0);
  analogWrite(LED_2, 0);
  analogWrite(LED_3, 0);
  analogWrite(LED_4, 0);
}

// ============================================================
//   TELEGRAM
// ============================================================

void conectarWiFiTelegram() {
  Serial.println();
  Serial.print("Conectando a WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi conectado.");
    Serial.print("IP del ESP32: ");
    Serial.println(WiFi.localIP());

    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    telegramListo = true;
    Serial.println("Bot de Telegram listo.");
  } else {
    telegramListo = false;
    Serial.println("No se pudo conectar a WiFi. El sistema sigue funcionando sin Telegram.");
  }
}

void procesarTelegram() {
  if (millis() - ultimoCheckTelegram < INTERVALO_TELEGRAM) {
    return;
  }
  ultimoCheckTelegram = millis();

  if (WiFi.status() != WL_CONNECTED) {
    telegramListo = false;
    return;
  }

  if (!telegramListo) {
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    telegramListo = true;
  }

  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id   = bot.messages[i].chat_id;
    String text      = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    text.trim();

    // Si se configuró un chat autorizado, se ignora al resto.
    String autorizado = CHAT_ID_AUTORIZADO;
    if (autorizado.length() > 0 && chat_id != autorizado) {
      bot.sendMessage(chat_id, "No autorizado.", "");
      continue;
    }

    Serial.println("----- Telegram -----");
    Serial.print("De: ");   Serial.println(from_name);
    Serial.print("Chat: "); Serial.println(chat_id);
    Serial.print("Msg: ");  Serial.println(text);

    manejarComandoTelegram(chat_id, text, from_name);
  }
}

void manejarComandoTelegram(String chat_id, String text, String from_name) {

  if (text == "/start" || text == "/help") {
    bot.sendMessage(chat_id, armarAyuda(from_name), "");
  }

  // --- Luces ---
  else if (text == "/luces_auto" || text == "/auto") {
    modoLuces = LUCES_AUTO;
    bot.sendMessage(chat_id, "Luces en AUTO: bailan con la música.", "");
  }
  else if (text == "/luces_on") {
    modoLuces = LUCES_ON;
    prenderTodasLasLuces();
    bot.sendMessage(chat_id, "Luces ON: todas prendidas fijas.", "");
  }
  else if (text == "/luces_off") {
    modoLuces = LUCES_OFF;
    apagarTodasLasLuces();
    bot.sendMessage(chat_id, "Luces OFF: todas apagadas.", "");
  }

  // --- Calentador ---
  else if (text == "/calentador_auto") {
    modoCalentador = CALEF_AUTO;
    bot.sendMessage(chat_id, "Calentador en AUTO: se regula solo por la temperatura.", "");
  }
  else if (text == "/calentador_on") {
    modoCalentador = CALEF_ON;
    bot.sendMessage(chat_id, "Calentador ON: forzado encendido (se apaga solo si falla el sensor).", "");
  }
  else if (text == "/calentador_off") {
    modoCalentador = CALEF_OFF;
    bot.sendMessage(chat_id, "Calentador OFF: forzado apagado.", "");
  }

  // --- Consultas ---
  else if (text == "/status") {
    bot.sendMessage(chat_id, armarStatus(), "");
  }
  else if (text == "/temp") {
    bot.sendMessage(chat_id, armarTemp(), "");
  }
  else if (text == "/audio") {
    bot.sendMessage(chat_id, armarAudio(), "");
  }
  else if (text == "/ip") {
    if (WiFi.status() == WL_CONNECTED) {
      bot.sendMessage(chat_id, "IP del ESP32: " + WiFi.localIP().toString(), "");
    } else {
      bot.sendMessage(chat_id, "WiFi no conectado.", "");
    }
  }

  else {
    bot.sendMessage(chat_id, "Comando no reconocido. Escribí /help para ver la lista.", "");
  }
}

// ============================================================
//   MENSAJES DE TELEGRAM
// ============================================================

String armarAyuda(String from_name) {
  String s = "Hola, " + from_name + " 👋\n";
  s += "Control de la Pileta Inteligente.\n\n";
  s += "LUCES:\n";
  s += "/luces_auto - bailan con la música\n";
  s += "/luces_on - prender todas fijas\n";
  s += "/luces_off - apagar todas\n\n";
  s += "CALENTADOR:\n";
  s += "/calentador_auto - automático por temperatura\n";
  s += "/calentador_on - forzar encendido\n";
  s += "/calentador_off - forzar apagado\n\n";
  s += "INFORMACIÓN:\n";
  s += "/status - estado general\n";
  s += "/temp - temperatura\n";
  s += "/audio - datos del sonido\n";
  s += "/ip - IP del ESP32";
  return s;
}

String armarStatus() {
  String s = "ESTADO DE LA PILETA\n";
  s += "Calentador: ";
  s += (calentadorEncendido ? "ON" : "OFF");
  s += " (" + modoCalentadorTexto() + ")\n";

  if (sensorTempOk) {
    s += "Temp: " + String(ultimaTemp, 1) + " C\n";
  } else {
    s += "Temp: ERROR sensor\n";
  }

  s += "Luces: " + modoLucesTexto() + "\n";
  s += "Música: " + tipoMusicaTexto() + "\n";
  s += "WiFi: ";
  s += (WiFi.status() == WL_CONNECTED ? "conectado" : "desconectado");
  return s;
}

String armarTemp() {
  String s = "TEMPERATURA\n";
  if (sensorTempOk) {
    s += "Actual: " + String(ultimaTemp, 1) + " C\n";
  } else {
    s += "ERROR: sensor desconectado.\n";
  }
  s += "Objetivo: " + String(TEMP_OBJETIVO, 1) + " C\n";
  s += "Histéresis: " + String(HISTERESIS, 1) + " C\n";
  s += "Calentador: ";
  s += (calentadorEncendido ? "ON" : "OFF");
  s += " (" + modoCalentadorTexto() + ")";
  return s;
}

String armarAudio() {
  String s = "AUDIO\n";
  s += "Música: " + tipoMusicaTexto() + "\n";
  s += "Graves: " + String(ultimaEnergiaGraves, 1) + "\n";
  s += "Agudos: " + String(ultimaEnergiaAgudos, 1) + "\n";
  s += "Total: " + String(ultimaEnergiaTotal, 1) + "\n";
  s += "Volumen: " + String(ultimoVolumen, 1) + "\n";
  s += "Brillo: " + String(ultimoBrillo) + "/255\n";
  s += "Modo luces: " + modoLucesTexto();
  return s;
}

String modoCalentadorTexto() {
  switch (modoCalentador) {
    case CALEF_AUTO: return "AUTO";
    case CALEF_ON:   return "ON";
    case CALEF_OFF:  return "OFF";
    default:         return "?";
  }
}

String modoLucesTexto() {
  switch (modoLuces) {
    case LUCES_AUTO: return "AUTO";
    case LUCES_ON:   return "ON";
    case LUCES_OFF:  return "OFF";
    default:         return "?";
  }
}

String tipoMusicaTexto() {
  switch (tipoMusicaActual) {
    case MUSICA_A:   return "graves";
    case MUSICA_B:   return "agudos/voz";
    case SIN_SONIDO: return "sin sonido";
    default:         return "?";
  }
}
