// ============================================================
//   PILETA INTELIGENTE  —  Control con ESP32 + Telegram
// ============================================================
//
//   Un solo programa que maneja los tres sistemas de la pileta
//   sobre el mismo ESP32, controlables desde un bot de Telegram:
//
//     1) CALENTADOR   -> Sensor de temperatura DS18B20 + relé.
//                        Arranca APAGADO; se activa desde Telegram
//                        (automático con histéresis, o forzado ON).
//     2) LUCES DISCO  -> Sensor de sonido + 8 LEDs (4 colores x 2
//                        lados) que bailan al ritmo de la música.
//                        Arrancan APAGADAS; se activan desde Telegram.
//     3) COBERTOR     -> 2 motores por L298N + 2 fines de carrera.
//                        Abre/cierra desde Telegram; frena solo al
//                        llegar al tope.
//     +) PANTALLA LCD -> Muestra temperatura y estado en vivo.
//
//   ------------------------------------------------------------
//   ANTES DE CARGAR:
//     - Completá tus datos en el archivo  config.h
//     - Instalá las librerías indicadas en el README.md
//       (OJO: ArduinoJson tiene que ser la versión 6, NO la 7)
//   ------------------------------------------------------------
//
//   CONEXIONES (pines del ESP32) — ver detalle en COMPONENTES.md:
//     GPIO4   -> DS18B20 (dato)   [resistencia 4.7k a 3.3V]
//     GPIO26  -> Relé (señal S)   [calentador]
//     GPIO21  -> LCD SDA (I2C)
//     GPIO22  -> LCD SCL (I2C)
//     GPIO16  -> LEDs VERDES  (los 2, uno por lado)
//     GPIO17  -> LEDs ROJOS
//     GPIO18  -> LEDs AZULES
//     GPIO19  -> LEDs BLANCOS
//     GPIO34  -> Sensor de sonido (analógico)
//     GPIO13,25,27 -> L298N motor A (IN1, IN2, ENA-PWM)
//     GPIO32,33,14 -> L298N motor B (IN3, IN4, ENB-PWM)
//     GPIO23  -> Fin de carrera CERRADO (con pull-up interno)
//     GPIO5   -> Fin de carrera ABIERTO (con pull-up interno)
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

// --- Calentador (Sistema 1) ---
#define PIN_DS18B20 4     // Sensor de temperatura del agua
#define PIN_RELE    26    // Señal del relé que prende el calentador

// --- Luces disco (Sistema 2) ---
// 8 LEDs = 4 colores x 2 lados. Cada pin maneja los 2 LEDs del mismo
// color (uno de cada lado), cada LED con su resistencia de 220 ohm.
#define LED_VERDE   16
#define LED_ROJO    17
#define LED_AZUL    18
#define LED_BLANCO  19
#define MIC_PIN     34    // Sensor de sonido. ADC1 (no usar ADC2: choca con WiFi).

// --- Cobertor (Sistema 3): L298N + 2 motores + 2 fines de carrera ---
#define MOTOR_A_IN1 13    // Motor A = lado del rodillo de la lona
#define MOTOR_A_IN2 25
#define MOTOR_A_EN  27    // PWM velocidad motor A
#define MOTOR_B_IN3 32    // Motor B = lado que tira de los cables
#define MOTOR_B_IN4 33
#define MOTOR_B_EN  14    // PWM velocidad motor B
#define FC_CERRADO  23    // Fin de carrera: cobertor cerrado  (pull-up INTERNO)
#define FC_ABIERTO  5     // Fin de carrera: cobertor abierto  (pull-up INTERNO)

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

// Bandas de frecuencia para clasificar el tipo de música
const int BANDA_GRAVES_MIN = 60;
const int BANDA_GRAVES_MAX = 250;
const int BANDA_AGUDOS_MIN = 2000;
const int BANDA_AGUDOS_MAX = 6000;

// Si graves > agudos * UMBRAL => predomina el bajo (Música A)
const float  UMBRAL_CLASIFICACION = 1.3;
const double UMBRAL_SILENCIO       = 100.0;  // Energía mínima para considerar que hay sonido

// ============================================================
//   AJUSTES DEL COBERTOR
// ============================================================

const int VELOCIDAD_COBERTOR = 180;             // PWM 0-255 (más bajo = más lento y suave)
const unsigned long TIMEOUT_COBERTOR = 30000;    // Corte de seguridad si no llega al tope (ms)

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
ModoCalentador modoCalentador = CALEF_OFF;   // Por defecto: apagado (se activa desde Telegram)
bool  calentadorEncendido = false;
float ultimaTemp = 0.0;
bool  sensorTempOk = false;

// --- Luces ---
enum TipoMusica { MUSICA_A, MUSICA_B, SIN_SONIDO };
TipoMusica tipoMusicaActual = SIN_SONIDO;

enum ModoLuces { LUCES_AUTO, LUCES_ON, LUCES_OFF };
ModoLuces modoLuces = LUCES_OFF;   // Por defecto: apagadas (se activan desde Telegram)

double ultimaEnergiaGraves = 0;
double ultimaEnergiaAgudos = 0;
double ultimaEnergiaTotal  = 0;
double ultimoVolumen = 0;
int    ultimoBrillo  = 0;

// --- Cobertor ---
enum EstadoCobertor { COB_PARADO, COB_ABRIENDO, COB_CERRANDO };
EstadoCobertor estadoCobertor = COB_PARADO;
unsigned long inicioMovimientoCobertor = 0;
String chatCobertor = "";   // A quién avisarle por Telegram cuando termina el movimiento

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

  // Luces (arrancan APAGADAS; se activan desde Telegram)
  pinMode(LED_VERDE,  OUTPUT);
  pinMode(LED_ROJO,   OUTPUT);
  pinMode(LED_AZUL,   OUTPUT);
  pinMode(LED_BLANCO, OUTPUT);
  apagarTodasLasLuces();

  // Cobertor (motores frenados al arrancar)
  setupCobertor();

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
  Serial.println("Calentador: OFF | Luces: OFF | Cobertor: parado (todo se maneja desde Telegram).");
}

// ============================================================
//   LOOP
// ============================================================

void loop() {
  // 1) Sonido y luces → en cada vuelta.
  muestrearAudio();
  clasificarMusica();
  actualizarLucesSegunModo();

  // 2) Cobertor → máquina de estados no bloqueante (frena al llegar al tope).
  actualizarCobertor();

  // 3) Temperatura + LCD → cada INTERVALO_TEMP.
  if (millis() - ultimoTiempoTemp >= INTERVALO_TEMP) {
    ultimoTiempoTemp = millis();
    actualizarTemperatura();
  }

  // 4) Telegram → cada INTERVALO_TELEGRAM.
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
  Serial.print(modoLucesTexto());
  Serial.print(" | Cobertor: ");
  Serial.println(estadoCobertorTexto());
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

  int colores[4] = {LED_VERDE, LED_ROJO, LED_AZUL, LED_BLANCO};

  switch (tipoMusicaActual) {

    case MUSICA_A: {
      // Graves: los 4 colores laten juntos.
      for (int i = 0; i < 4; i++) analogWrite(colores[i], brillo);
      break;
    }

    case MUSICA_B: {
      // Agudos/voz: barrido secuencial de un color a la vez.
      static int posicion = 0;
      static unsigned long ultimoPaso = 0;
      unsigned long ahora = millis();

      if (ahora - ultimoPaso > 100) {
        ultimoPaso = ahora;
        posicion = (posicion + 1) % 4;
      }

      for (int i = 0; i < 4; i++) {
        analogWrite(colores[i], (i == posicion) ? brillo : 0);
      }
      break;
    }

    case SIN_SONIDO:
    default: {
      // Sin música: se apagan y esperan a que vuelva a sonar algo.
      apagarTodasLasLuces();
      break;
    }
  }
}

void prenderTodasLasLuces() {
  analogWrite(LED_VERDE,  255);
  analogWrite(LED_ROJO,   255);
  analogWrite(LED_AZUL,   255);
  analogWrite(LED_BLANCO, 255);
}

void apagarTodasLasLuces() {
  analogWrite(LED_VERDE,  0);
  analogWrite(LED_ROJO,   0);
  analogWrite(LED_AZUL,   0);
  analogWrite(LED_BLANCO, 0);
}

// ============================================================
//   COBERTOR
// ============================================================

void setupCobertor() {
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_A_EN,  OUTPUT);
  pinMode(MOTOR_B_IN3, OUTPUT);
  pinMode(MOTOR_B_IN4, OUTPUT);
  pinMode(MOTOR_B_EN,  OUTPUT);

  pinMode(FC_CERRADO, INPUT_PULLUP);   // pull-up interno
  pinMode(FC_ABIERTO, INPUT_PULLUP);   // pull-up interno (no hace falta resistencia externa)

  cobertorFrenar();
}

// Un fin de carrera está "tocado" cuando el pin queda en LOW.
bool finDeCarreraTocado(int pin) {
  return digitalRead(pin) == LOW;
}

// --- Control de cada motor (el L298N usa IN para dirección y EN para velocidad) ---

// Motor A libre: gira arrastrado por la lona (no frena).
void motorA_libre() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);
  analogWrite(MOTOR_A_EN, 0);
}

// Motor A enrolla la lona (movimiento de ABRIR).
void motorA_enrollar() {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  analogWrite(MOTOR_A_EN, VELOCIDAD_COBERTOR);
}

// Motor B libre: suelta cable, gira arrastrado.
void motorB_libre() {
  digitalWrite(MOTOR_B_IN3, LOW);
  digitalWrite(MOTOR_B_IN4, LOW);
  analogWrite(MOTOR_B_EN, 0);
}

// Motor B tira de los cables (movimiento de CERRAR).
void motorB_tirar() {
  digitalWrite(MOTOR_B_IN3, HIGH);
  digitalWrite(MOTOR_B_IN4, LOW);
  analogWrite(MOTOR_B_EN, VELOCIDAD_COBERTOR);
}

void cobertorFrenar() {
  motorA_libre();
  motorB_libre();
}

// Empieza a abrir: el rodillo enrolla la lona, el otro lado suelta cable.
void cobertorAbrir() {
  estadoCobertor = COB_ABRIENDO;
  inicioMovimientoCobertor = millis();
  motorB_libre();
  motorA_enrollar();
  Serial.println(">>> Cobertor: ABRIENDO");
}

// Empieza a cerrar: los cables tiran la lona, el rodillo la suelta.
void cobertorCerrar() {
  estadoCobertor = COB_CERRANDO;
  inicioMovimientoCobertor = millis();
  motorA_libre();
  motorB_tirar();
  Serial.println(">>> Cobertor: CERRANDO");
}

void cobertorParar() {
  estadoCobertor = COB_PARADO;
  cobertorFrenar();
  Serial.println(">>> Cobertor: PARADO");
}

// Máquina de estados: se llama en cada loop. Frena al llegar al tope
// o si pasa demasiado tiempo (seguridad).
void actualizarCobertor() {
  if (estadoCobertor == COB_PARADO) return;

  if (estadoCobertor == COB_ABRIENDO && finDeCarreraTocado(FC_ABIERTO)) {
    cobertorParar();
    avisarCobertor("Cobertor ABIERTO ✅");
    return;
  }

  if (estadoCobertor == COB_CERRANDO && finDeCarreraTocado(FC_CERRADO)) {
    cobertorParar();
    avisarCobertor("Cobertor CERRADO ✅");
    return;
  }

  // Corte de seguridad: si tardó demasiado, algo se trabó.
  if (millis() - inicioMovimientoCobertor > TIMEOUT_COBERTOR) {
    cobertorParar();
    avisarCobertor("⚠️ Cobertor detenido por seguridad (tardó demasiado). Revisá que no esté trabado.");
  }
}

// Avisa por Telegram a quien pidió el movimiento (si hay alguien y hay WiFi).
void avisarCobertor(String msg) {
  if (chatCobertor.length() > 0 && WiFi.status() == WL_CONNECTED) {
    bot.sendMessage(chatCobertor, msg, "");
  }
  chatCobertor = "";
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
    bot.sendMessage(chat_id, "Luces en AUTO: bailan con la música (si no hay música, quedan apagadas).", "");
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

  // --- Cobertor ---
  else if (text == "/cobertor_abrir") {
    if (finDeCarreraTocado(FC_ABIERTO)) {
      bot.sendMessage(chat_id, "El cobertor ya está abierto.", "");
    } else {
      chatCobertor = chat_id;
      cobertorAbrir();
      bot.sendMessage(chat_id, "Abriendo el cobertor... te aviso cuando termine.", "");
    }
  }
  else if (text == "/cobertor_cerrar") {
    if (finDeCarreraTocado(FC_CERRADO)) {
      bot.sendMessage(chat_id, "El cobertor ya está cerrado.", "");
    } else {
      chatCobertor = chat_id;
      cobertorCerrar();
      bot.sendMessage(chat_id, "Cerrando el cobertor... te aviso cuando termine.", "");
    }
  }
  else if (text == "/cobertor_parar") {
    cobertorParar();
    chatCobertor = "";
    bot.sendMessage(chat_id, "Cobertor detenido.", "");
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
  s += "COBERTOR:\n";
  s += "/cobertor_abrir - destapar la pileta\n";
  s += "/cobertor_cerrar - tapar la pileta\n";
  s += "/cobertor_parar - frenar el cobertor\n\n";
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
  s += "Cobertor: " + estadoCobertorTexto() + "\n";
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

String estadoCobertorTexto() {
  if (estadoCobertor == COB_ABRIENDO) return "abriendo...";
  if (estadoCobertor == COB_CERRANDO) return "cerrando...";
  if (finDeCarreraTocado(FC_CERRADO)) return "cerrado";
  if (finDeCarreraTocado(FC_ABIERTO)) return "abierto";
  return "parado (posición intermedia)";
}
