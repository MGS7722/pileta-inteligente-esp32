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
//     2) LUCES DISCO  -> Micrófono + tira WS2812 de 15 píxeles.
//                        El sonido se analiza por FFT y se separa en
//                        GRAVES / MEDIOS / AGUDOS; cada efecto pinta
//                        la tira con esas tres bandas y destella en
//                        cada golpe del ritmo. Arranca APAGADA.
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
//   CONEXIONES (pines del ESP32) — ver detalle en CONEXIONES.md:
//     GPIO4   -> DS18B20 (dato)   [resistencia 4.7k a 3.3V]
//     GPIO26  -> Relé (señal S)   [calentador]
//     GPIO21  -> LCD SDA (I2C)
//     GPIO22  -> LCD SCL (I2C)
//     GPIO16  -> Tira WS2812, entrada de DATOS (con 440 ohm en serie)
//     GPIO34  -> Micrófono KY-037, salida AO (analógica). Módulo a 5V
//     GPIO13,25,27 -> L298N motor A (IN1, IN2, ENA-PWM)
//     GPIO32,33,18 -> L298N motor B (IN3, IN4, ENB-PWM)
//     GPIO23  -> Fin de carrera CERRADO (con pull-up interno)
//     GPIO19  -> Fin de carrera ABIERTO (con pull-up interno)
//
//   Libres tras el cambio a la tira: GPIO17, GPIO14, GPIO5, GPIO35.
// ============================================================

#include "config.h"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include <Adafruit_NeoPixel.h>
#include <arduinoFFT.h>

// ============================================================
//   PINES
// ============================================================

// --- Calentador (Sistema 1) ---
#define PIN_DS18B20 4     // Sensor de temperatura del agua
#define PIN_RELE    26    // Señal del relé que prende el calentador

// --- Luces disco (Sistema 2): tira WS2812 + micrófono ---
//
// La tira lleva UN solo cable de datos: cada píxel se pasa al siguiente lo que
// no le corresponde. Va con una resistencia de 440 ohm (dos de 220 en serie) en
// serie con la entrada DIN, como especifica Adafruit, para proteger el primer
// píxel de los picos de conmutación.
//
// GPIO16 se elige a propósito: GPIO5 y GPIO14 emiten un pulso mientras el ESP32
// arranca, y un pulso en la línea de datos deja píxeles encendidos en colores al
// azar hasta que el programa toma el control. GPIO16 y GPIO17 están tranquilos.
#define TIRA_PIN    16    // Tira WS2812 — entrada DIN (por 440 ohm)
#define MIC_PIN     34    // Micrófono KY-037, salida AO. ADC1 (el ADC2 choca con WiFi).

// --- Cobertor (Sistema 3): L298N + 2 motores + 2 fines de carrera ---
//
// Los pines del cobertor son los únicos que NO pueden hacer nada raro durante
// el arranque, porque del otro lado hay motores. El ESP32 tiene dos grupos de
// pines problemáticos y ninguno de los dos toca este sistema:
//
//   - GPIO5 y GPIO14 emiten un pulso mientras el chip arranca. Con el ENB del
//     L298N ahí, ese pulso habilita el puente H antes de que el programa haya
//     puesto orden: el motor puede pegar un tirón en cada encendido. Por eso
//     el ENB del motor B se mudó de GPIO14 a GPIO18.
//   - GPIO5 es además pin de arranque (strapping): el chip lee su nivel al
//     encender. El fin de carrera ABIERTO lo conectaba a masa, o sea que la
//     posición del cobertor decidía un ajuste interno del chip. Por eso se
//     mudó a GPIO19. (Verificado en la documentación de Espressif: GPIO5 fija
//     el "timing del esclavo SDIO", que este proyecto no usa, así que la placa
//     arrancaba igual — pero un pin de arranque no debe quedar atado a dónde
//     está la lona, y el pulso contra un contacto a masa tampoco es sano.)
//
// GPIO18 y GPIO19 quedaron libres al mudar dos colores de LED a esos pines.
#define MOTOR_A_IN1 13    // Motor A = lado del rodillo de la lona
#define MOTOR_A_IN2 25
#define MOTOR_A_EN  27    // PWM velocidad motor A
#define MOTOR_B_IN3 32    // Motor B = lado que tira de los cables
#define MOTOR_B_IN4 33
#define MOTOR_B_EN  18    // PWM velocidad motor B
#define FC_CERRADO  23    // Fin de carrera: cobertor cerrado  (pull-up INTERNO)
#define FC_ABIERTO  19    // Fin de carrera: cobertor abierto  (pull-up INTERNO)

// ============================================================
//   AJUSTES DEL CALENTADOR
// ============================================================

// La temperatura objetivo se cambia desde Telegram con /temperatura y queda
// guardada en la memoria del ESP32 (sobrevive reinicios). 23.0 es el valor
// inicial de fábrica. `volatile` porque la escribe Telegram (núcleo 0) y la lee
// el loop (núcleo 1).
volatile float tempObjetivo = 23.0;
const float HISTERESIS    = 5.0;    // Margen para no prender/apagar a cada rato

// El módulo relé de este proyecto es ACTIVO-ALTO: HIGH energiza la bobina y
// cierra el contacto, LOW lo suelta. Verificado en hardware el 2026-08-06: con
// GPIO26 en HIGH el LED de estado se enciende y el calefactor consume; con LOW
// se apaga. (Ojo: hay módulos activo-bajo idénticos por fuera; si alguna vez se
// cambia el módulo, hay que volver a comprobar esto antes de conectar la carga.)
const int RELE_ON  = HIGH;
const int RELE_OFF = LOW;

// ============================================================
//   AJUSTES DE LA TIRA WS2812
// ============================================================

// Tope de píxeles que el programa puede manejar. Reserva la memoria del cuadro
// una sola vez, al arrancar: 150 x 3 bytes = 450 bytes, nada para el ESP32.
const int MAX_LEDS = 150;

// Cuántos píxeles hay conectados de verdad. Se ajusta con /leds y queda guardado
// en la memoria del ESP32, así se puede cortar más tira sin recompilar.
// 15 píxeles = 50 cm de una tira de 30 LED/m.
//
// Lo escribe Telegram (núcleo 0) y lo lee el dibujo (núcleo 1). Es seguro sin
// candados: en el ESP32 escribir un entero alineado es atómico, así que nunca se
// lee un valor a medio escribir. Lo peor que puede pasar es que un único cuadro
// se dibuje con el número nuevo antes de que la tira se reconfigure, y la
// librería simplemente ignora los píxeles que se salen de su largo actual.
int numLeds = 15;

// Brillo máximo, en porcentaje. Es un tope: el limitador de corriente puede
// bajarlo todavía más si el cuadro consumiría demasiado.
int brilloPorcentaje = 70;

// PRESUPUESTO DE CORRIENTE, en miliamperes. Es la pieza que permite alimentar la
// tira del propio ESP32 sin una tercera fuente: antes de mandar cada cuadro se
// calcula lo que consumiría y, si se pasa de este número, se baja el brillo de
// todo el cuadro hasta que entre. La tira NO PUEDE superar este límite.
//
//   120 mA -> ESP32 alimentado por el USB de la notebook (el riel de 5V ya
//             alimenta al LCD, al relé y al micrófono: queda poco margen)
//   500 mA -> ESP32 alimentado por un cargador de celular de 2A
int corrienteMaximaMa = 120;

// Consumo de un canal (rojo, verde o azul) a fondo, en miliamperes. Un píxel en
// blanco pleno son los tres canales juntos: 60 mA. Se usa el valor de catálogo,
// que queda del lado seguro para un limitador.
const float MA_POR_CANAL = 20.0f / 255.0f;

// Consumo del chip de cada píxel aunque esté apagado.
const float MA_PIXEL_EN_REPOSO = 1.0f;

// Orden de los bytes de color. Los WS2812B son GRB, pero hay clones RGB con el
// mismo aspecto. Si /luces_test anuncia ROJO y se ve VERDE, es esto: se corrige
// desde Telegram con /orden, sin recompilar.
bool tiraEsGRB = true;

// Efecto del modo AUTO. Se cambia con /efecto y queda guardado.
enum Efecto { EFECTO_ESPECTRO = 1, EFECTO_MEZCLA, EFECTO_COMETA, EFECTO_ARCOIRIS };
const int EFECTO_MINIMO = 1;
const int EFECTO_MAXIMO = 4;
int efectoActual = EFECTO_ESPECTRO;

// ============================================================
//   AJUSTES DEL ANÁLISIS DE SONIDO (FFT)
// ============================================================

// El micrófono KY-037 entrega por su pin AO la señal CRUDA del electret: la onda
// de sonido completa, sin rectificar (el LM393 de la placa es un comparador, no
// un amplificador, y sólo maneja la salida DO). Por eso se puede analizar el
// espectro y separar graves, medios y agudos.
//
// La FFT se había eliminado en la v3, y con razón: el micrófono estaba a 3,3V y
// daba 17 counts de pico a pico. Alimentado a 5V da hasta 1040 (medido en el
// taller): treinta veces más señal, de sobra para separar tres bandas.
//
// 256 muestras a 10 kHz es el punto justo:
//   - 10 kHz permite analizar hasta 5 kHz (la mitad, por Nyquist), que cubre todo
//     el contenido rítmico de la música. Es la misma frecuencia que usa WLED.
//   - 256 muestras -> ventana de 25,6 ms y resolución de 39 Hz por bin.
//     Con 512 la resolución sería el doble, pero la ventana treparía a 51 ms y el
//     refresco caería a 19 cuadros por segundo: se vería a los tirones.
const int MUESTRAS_FFT = 256;
const unsigned long PERIODO_MUESTREO_US = 100;   // 100 us = 10 kHz
const float FRECUENCIA_MUESTREO_NOMINAL = 10000.0f;

// Cortes de las tres bandas, en hertz.
//
// Los graves arrancan en 78 Hz y no más abajo a propósito: de 39 a 78 Hz cae el
// zumbido de 50 Hz de la red eléctrica, que cualquier micrófono sin blindaje
// capta de la fuente. Incluirlo sería alimentar los graves con el ruido de la
// instalación en vez de con la música. El bombo se detecta igual, por sus
// armónicos de 100 a 200 Hz. Con /espectro se verifica si ese zumbido existe.
const float BANDA_GRAVES_DESDE = 78.0f;    // bombo, bajo, tom
const float BANDA_GRAVES_HASTA = 234.0f;
const float BANDA_MEDIOS_DESDE = 234.0f;   // voz, guitarra, piano, caja
const float BANDA_MEDIOS_HASTA = 2000.0f;
const float BANDA_AGUDOS_DESDE = 2000.0f;  // platos, hi-hat, brillo
const float BANDA_AGUDOS_HASTA = 5000.0f;

// --- Control automático de ganancia (AGC), una por banda ---
//
// Sin AGC el sistema anda con un volumen y con otro no. Y hay un problema extra:
// en música real los graves son 10 o 20 veces más fuertes que los agudos, así que
// con una escala común la banda de agudos quedaría siempre apagada. Por eso cada
// banda se normaliza contra SU PROPIO máximo reciente.
const float AGC_DECAIMIENTO = 0.999f;   // el máximo baja ~8 s si no se renueva

// PISO DE RUIDO POR BANDA — el "squelch" que usa WLED.
//
// Es la pieza que faltaba, y apareció midiendo en el taller el 2026-08-13: con
// un tono PURO de 1000 Hz sonando, las tres bandas marcaban 72 / 100 / 100. Las
// dos bandas donde no había absolutamente nada mostraban el máximo.
//
// La culpa no era del AGC sino de cómo se lo usaba. Al normalizar contra el
// máximo reciente, una banda sin señal ve caer ese máximo hasta el piso, y
// entonces su propio ruido pasa a valer 100%. Acotar el máximo no alcanza: con
// un piso de 12 y un ruido de 9, la banda seguiría mostrando 75%. Hay que
// RESTAR el ruido antes de normalizar, no sólo acotarlo.
//
// Medido ese mismo día: en silencio las tres bandas dan 6,8 - 9,4 de crudo; con
// música fuerte, 18,5 - 24,3.
//
// **12 es el valor de fábrica, deliberadamente conservador** —en un lugar
// desconocido es preferible que la tira se quede quieta a que baile sola con el
// ruido—, pero el valor bueno depende del lugar Y de la fuente de audio, y por
// eso se ajusta desde Telegram con /piso, sin recompilar. En el taller quedó en
// **10**: la música salía del parlante de un celular, que no emite graves, y con
// 12 la banda de graves (crudo ~11,8) se apagaba entera. Con 10 el silencio
// sigue dando cero —el ruido no pasa de 7,5— y el riff de bajo entra con
// holgura (crudo 14 a 17).
//
// Lo escribe Telegram (núcleo 0) y lo lee el análisis (núcleo 1), igual que
// numLeds: en el ESP32 escribir un entero alineado es atómico, así que nunca se
// lee un valor a medio escribir y no hacen falta candados.
int pisoRuidoBanda = 12;

// Piso del AGC, ya sobre la señal ÚTIL (lo que quedó por encima del piso de
// ruido). Evita dividir por casi cero cuando la música arranca de golpe.
const float AGC_PISO = 5.0f;

// --- Suavizado visual: ataque instantáneo, caída suave ---
// El golpe tiene que verse en el acto, pero apagarse con elegancia. Sin esto, a
// 35 cuadros por segundo la tira parpadea de forma desagradable.
const float CAIDA_NIVEL = 0.25f;   // cuánto se acerca al valor nuevo al bajar

// --- Detección de golpes (beat) ---
//
// El beat NO sale de la FFT: sale de comparar la energía instantánea de los
// GRAVES contra el promedio del último segundo (algoritmo clásico de energía
// sonora, flipcode/GameDev). A diferencia de la base exponencial que usaba la
// versión anterior —que con música sostenida se inflaba hasta tapar los propios
// golpes que debía detectar— un promedio de ventana deslizante no puede
// dispararse sin techo y se recupera en un segundo exacto.
const int HISTORIAL_BEAT = 32;   // 32 ventanas de ~28 ms = 0,9 segundos

// El umbral se calcula solo a partir de la dispersión del historial: si los
// golpes están muy marcados se puede ser menos exigente; si la señal es plana hay
// que exigir más para no disparar con ruido. Se usa el coeficiente de variación
// (desvío / promedio) porque es adimensional y vale en cualquier escala.
//   C = FACTOR_BASE - FACTOR_RANGO * min(1, desvio/promedio)
// Música marcada -> C ~ 1,20.  Señal plana -> C ~ 1,45.
const float FACTOR_BASE  = 1.50f;
const float FACTOR_RANGO = 0.50f;
const float FACTOR_MINIMO = 1.15f;

// Sin tiempo refractario, un solo golpe de bombo dispara tres veces seguidas.
const unsigned long REFRACTARIO_BEAT_MS = 150;
const unsigned long DURACION_DESTELLO_MS = 110;   // cuánto dura el flash del golpe

// Umbral de "hay música", en pico a pico crudo del ADC. Por debajo de esto la
// tira pasa a modo respiración.
//
// RECALIBRADO en el taller el 2026-08-13, con la tira ya montada. El valor
// anterior (45) salía de las mediciones del 2026-08-06, donde el silencio daba
// 9-36. Con el ruido ambiente real del taller —conversaciones, herramientas— el
// piso trepa bastante más, y el umbral quedaba POR DEBAJO del ruido: el sistema
// declaraba "música detectada" sin que sonara nada, y el AGC se alimentaba de
// ruido. Medido ese día, 15 lecturas alternadas:
//
//   ruido ambiente (conversaciones) ...  32 - 56
//   música por un parlante ........... 180 - 591
//
// 90 es casi el doble del ruido máximo y la mitad de la música más floja: deja
// margen limpio de los dos lados.
const int SONIDO_MINIMO = 90;

// Memoria de "hay música": evita que la tira se apague entre dos beats.
const unsigned long MEMORIA_MUSICA_MS = 800;

// ============================================================
//   AJUSTES DEL COBERTOR
// ============================================================

// Velocidad de los motores, en PWM de 0 a 255. Se ajusta desde Telegram con
// /velocidad (en porcentaje) y queda guardada en la memoria del ESP32, así se
// calibra en el taller sin recompilar. 115 (~45%) es el valor de fábrica.
int velocidadCobertor = 115;

// Piso físico: por debajo de ~20% el motor no vence su propio rozamiento, zumba
// y no llega a girar. No es un problema del programa, es el motor.
const int VELOCIDAD_MINIMA_PORCENTAJE = 20;

const unsigned long TIMEOUT_COBERTOR = 30000;    // Corte de seguridad si no llega al tope (ms)

// Prueba de taller (/motor_a y /motor_b): cuánto gira un motor solo, para
// verificar cableado y sentido de giro con el mecanismo todavía desarmado.
const unsigned long DURACION_PRUEBA_MOTOR_MS = 2000;

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

Preferences preferencias;   // Memoria no volátil (guarda la temperatura objetivo)

WiFiClientSecure     client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// La tira se crea con el tope de píxeles y se ajusta al número real en el setup.
// Un solo objeto en todo el programa: la librería guarda el buffer del RMT en
// variables estáticas compartidas, así que dos instancias se pisarían.
Adafruit_NeoPixel tira(MAX_LEDS, TIRA_PIN, NEO_GRB + NEO_KHZ800);

// Buffers de la FFT. vReal entra con las muestras y sale con las magnitudes de
// cada frecuencia; vImag es el espacio de trabajo que necesita el algoritmo.
float vReal[MUESTRAS_FFT];
float vImag[MUESTRAS_FFT];

// El último parámetro (true) precalcula los factores de la ventana una sola vez,
// en lugar de recalcularlos en cada cuadro.
ArduinoFFT<float> fft(vReal, vImag, MUESTRAS_FFT, FRECUENCIA_MUESTREO_NOMINAL, true);

// ============================================================
//   ESTADO DEL SISTEMA
// ============================================================

// --- Calentador ---
// `volatile` por lo mismo que el modo de las luces: lo escribe Telegram en el
// núcleo 0 y lo lee el loop en el núcleo 1.
enum ModoCalentador { CALEF_AUTO, CALEF_ON, CALEF_OFF };
volatile ModoCalentador modoCalentador = CALEF_OFF;   // Arranca apagado
bool  calentadorEncendido = false;
float ultimaTemp = 0.0;
bool  sensorTempOk = false;

// --- Luces ---
// `volatile` porque Telegram (núcleo 0) escribe el modo y el loop (núcleo 1) lo
// lee: sin eso, el compilador puede guardarse el valor en un registro dentro del
// loop y no enterarse nunca de que llegó un comando.
enum ModoLuces { LUCES_AUTO, LUCES_ON, LUCES_OFF };
volatile ModoLuces modoLuces = LUCES_OFF;   // Arranca apagada; se activa desde Telegram

// --- Medición cruda del micrófono (la calcula el núcleo 1 en cada ventana) ---
int   ultimoPicoAPico = 0;    // max - min de la ventana: el "volumen"
int   ultimoMinimo    = 0;
int   ultimoMaximo    = 0;
int   ultimoDC        = 0;    // punto de polarización del micrófono
float frecuenciaRealHz = FRECUENCIA_MUESTREO_NOMINAL;   // medida, no supuesta

// --- Seguimiento del pico a pico en una ventana de varios segundos ---
//
// Cada lectura de /diag es una foto de 25,6 ms: en el taller salta entre 32 y 56
// de una medición a la siguiente, porque el ruido ambiente realmente es así.
// Estimar un piso de ruido con fotos sueltas costó quince mandadas de /diag;
// con esto se hace de una.
//
// Son dos ventanas: la que está corriendo y la anterior ya cerrada. /diag
// informa el mínimo y el máximo de las dos juntas, así que siempre mira entre 5
// y 10 segundos de historia y nunca arranca de cero justo cuando se lo consulta.
const unsigned long VENTANA_P2P_MS = 5000;
int p2pMinimoActual = 4095;
int p2pMaximoActual = 0;
int p2pMinimoPrevio = 4095;
int p2pMaximoPrevio = 0;
unsigned long inicioVentanaP2p = 0;

// --- Las tres bandas del espectro ---
//
// La señal pasa por tres etapas y cada una queda guardada, porque /espectro las
// muestra todas: sin ver el crudo al lado del útil no hay forma de calibrar el
// piso de ruido en el taller.
struct Banda {
  float crudo;       // lo que sale de la FFT, sin tocar
  float util;        // crudo menos el piso de ruido, nunca negativo
  float maximoAgc;   // referencia del AGC, en la escala de `util`
  float nivel;       // 0 a 1: lo que dibujan los efectos
};
Banda graves = {0, 0, AGC_PISO, 0};
Banda medios = {0, 0, AGC_PISO, 0};
Banda agudos = {0, 0, AGC_PISO, 0};

float volumenGeneral = 0.0f;   // 0 a 1, promedio de las tres bandas ya normalizadas
float frecuenciaDominanteHz = 0.0f;

// --- Detección de golpes ---
float historialGraves[HISTORIAL_BEAT];   // energía de graves del último segundo
int   posHistorial = 0;
bool  historialLleno = false;
float promedioGraves = 0.0f;   // los publica /espectro para poder calibrar
float umbralBeat = 0.0f;
bool  hayBeat = false;
unsigned long ultimoBeat = 0;
unsigned long inicioDestello = 0;

bool hayMusica = false;
unsigned long ultimoMomentoConSonido = 0;

// --- El cuadro que se va a dibujar ---
// Los efectos pintan acá los colores IDEALES, sin preocuparse por el consumo.
// volcarCuadro() es el único que habla con la tira: aplica el brillo, el límite
// de corriente y la corrección de gamma, y recién ahí manda los datos.
uint8_t cuadro[MAX_LEDS][3];
int corrienteEstimadaMa = 0;   // consumo del último cuadro (lo muestra /status)

// --- Pedidos de reconfiguración de la tira ---
// Cambiar la cantidad de píxeles o el orden de colores reasigna memoria dentro
// de la librería. Hacerlo desde Telegram (núcleo 0) mientras el loop (núcleo 1)
// está enviando datos es un cuelgue seguro, así que el comando deja el pedido
// acá y el loop lo aplica entre dos cuadros.
volatile bool tiraNecesitaReconfigurar = false;

// --- Traza de calibración (comando /trace) ---
// Vuelca por el Monitor Serie el pulso del sonido en vivo, para poder calibrar
// con datos reales en lugar de a ojo. Arranca APAGADA: no ensucia el uso normal.
volatile bool trazaSonidoActiva = false;
unsigned long ultimaTraza = 0;
const unsigned long INTERVALO_TRAZA_MS = 100;

// --- Volcado de la onda cruda (comando /onda) ---
// La captura tiene que correr en el núcleo 1, que es el dueño del ADC, así que
// el comando sólo levanta el pedido y el loop hace el trabajo.
volatile bool pedidoVolcarOnda = false;

// --- Prueba de la tira (comando /luces_test) ---
// Igual que la onda: el comando pide, el loop dibuja.
volatile bool pruebaTiraActiva = false;
unsigned long inicioPruebaTira = 0;
const unsigned long PASO_PRUEBA_MS = 1200;   // cuánto dura cada color de la prueba

// --- Cobertor ---
// Los mensajes que cruzan de un núcleo al otro viajan en buffers de tamaño fijo,
// nunca en un String: dos núcleos tocando el heap del mismo String terminan en
// memoria corrupta y en reinicios imposibles de explicar.
const size_t CHAT_ID_MAXIMO = 32;    // los chat_id de Telegram son numéricos
const size_t AVISO_MAXIMO   = 160;   // alcanza de sobra para los avisos del cobertor

// COB_PRUEBA es el modo de taller: mueve UN motor unos segundos sin mirar los
// fines de carrera, para verificar cableado y sentido de giro (ver probarMotor).
enum EstadoCobertor { COB_PARADO, COB_ABRIENDO, COB_CERRANDO, COB_PRUEBA };

// Las órdenes del cobertor llegan por Telegram (núcleo 0) y las vigila el loop
// (núcleo 1). `volatile` le prohíbe al compilador guardarse estas variables en un
// registro dentro del loop: sin eso, el loop podría no enterarse nunca de que
// llegó una orden nueva.
volatile EstadoCobertor estadoCobertor = COB_PARADO;
volatile unsigned long  inicioMovimientoCobertor = 0;
volatile char motorEnPrueba = '-';   // 'A' o 'B' mientras dura COB_PRUEBA

// A quién avisarle por Telegram cuando termina el movimiento.
char chatCobertor[CHAT_ID_MAXIMO] = "";

// --- Lectura NO BLOQUEANTE del sensor de temperatura ---
// Con el modo por defecto de la librería, requestTemperatures() se queda esperando
// los 750 ms que tarda el DS18B20 en convertir a 12 bits, y durante ese rato el
// programa entero queda congelado: el LCD no se refresca y las luces se clavan.
// Acá la lectura se parte en dos fases —se pide la conversión, el loop sigue
// trabajando, y el valor se recoge cuando el sensor terminó—, así el loop nunca
// se detiene por el sensor.
const unsigned long CONVERSION_DS18B20_MS = 750;   // 12 bits: 750 ms (hoja de datos)
bool conversionEnCurso = false;
unsigned long inicioConversion = 0;

// --- Tiempos y conexión ---
unsigned long ultimoTiempoTemp    = 0;
unsigned long ultimoCheckTelegram = 0;
bool telegramListo = false;

// Latido de la tarea de Telegram: ella lo actualiza en cada vuelta y el loop lo
// mira desde el otro núcleo para saber si sigue viva.
//
// Pasó en el taller el 2026-08-06: el loop seguía corriendo —la pantalla y el
// sensor andaban perfecto, sin un solo reinicio— pero la tarea de Telegram se
// había quedado esperando una conexión que nunca llegó, y el bot no respondía
// más. Desde afuera era invisible: la placa "parecía" sana. Con este latido, el
// síntoma aparece en el Monitor Serie en vez de tener que adivinarlo.
volatile unsigned long ultimoLatidoTelegram = 0;

// Si el WiFi se cae, el core reintenta solo; pero si pasa demasiado tiempo sin
// volver, conviene forzar una reconexión limpia en vez de esperar para siempre.
const unsigned long REINTENTO_WIFI_MS = 20000;
unsigned long ultimoIntentoWiFi = 0;

// --- Buzón de avisos hacia Telegram ---
// El aviso de "cobertor abierto/cerrado" nace en el loop (núcleo 1), pero el
// objeto `bot` lo usa SOLAMENTE la tarea de Telegram (núcleo 0): dos núcleos
// escribiendo por la misma conexión segura al mismo tiempo es un cuelgue seguro.
// Así que el loop deja el mensaje acá y la tarea lo despacha cuando puede.
// El flag se levanta DESPUÉS de escribir el texto y se baja DESPUÉS de leerlo,
// para que la tarea nunca lea un mensaje a medio escribir.
volatile bool avisoPendiente = false;
char avisoTexto[AVISO_MAXIMO];
char avisoChat[CHAT_ID_MAXIMO];

// ============================================================
//   SETUP
// ============================================================

void setup() {
  Serial.begin(115200);

  // Por qué se reinició la última vez. Si la tira consume de más y hace caer los
  // 5V, el ESP32 se reinicia por brownout y arranca como si nada —calentador en
  // OFF, luces en OFF— sin que nadie sepa por qué. Esto lo hace visible.
  informarMotivoDelReinicio();

  // Calentador apagado al arrancar
  pinMode(PIN_RELE, OUTPUT);
  digitalWrite(PIN_RELE, RELE_OFF);

  // Configuración guardada (si nunca se configuró, quedan los valores de fábrica)
  preferencias.begin("pileta", false);
  tempObjetivo      = preferencias.getFloat("tempObj", tempObjetivo);
  velocidadCobertor = preferencias.getUChar("velCobertor", velocidadCobertor);
  numLeds           = preferencias.getUShort("numLeds", numLeds);
  brilloPorcentaje  = preferencias.getUChar("brillo", brilloPorcentaje);
  corrienteMaximaMa = preferencias.getUShort("corriente", corrienteMaximaMa);
  efectoActual      = preferencias.getUChar("efecto", efectoActual);
  tiraEsGRB         = preferencias.getBool("tiraGRB", tiraEsGRB);
  pisoRuidoBanda    = preferencias.getUChar("pisoRuido", pisoRuidoBanda);

  // Los valores guardados podrían venir de una versión anterior con otros
  // rangos: se acotan antes de usarlos, para que un número absurdo en memoria
  // no deje la tira sin arrancar.
  numLeds           = constrain(numLeds, 1, MAX_LEDS);
  brilloPorcentaje  = constrain(brilloPorcentaje, 0, 100);
  corrienteMaximaMa = constrain(corrienteMaximaMa, 50, 2000);
  efectoActual      = constrain(efectoActual, EFECTO_MINIMO, EFECTO_MAXIMO);
  pisoRuidoBanda    = constrain(pisoRuidoBanda, 0, 200);

  // Tira de luces (arranca APAGADA; se activa desde Telegram)
  tira.begin();
  aplicarConfiguracionTira();
  limpiarCuadro();
  volcarCuadro();

  // Cobertor (motores frenados al arrancar)
  setupCobertor();

  // Sensor de temperatura
  sensores.begin();
  sensores.setResolution(12);
  sensores.setWaitForConversion(false);   // no bloquear el loop mientras convierte

  // Pantalla
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Control Pileta");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(2000);
  lcd.clear();

  // Micrófono (salida AO, analógica). Es el único micrófono conectado.
  pinMode(MIC_PIN, INPUT);
  analogReadResolution(12);                     // Lecturas de 0 a 4095
  analogSetPinAttenuation(MIC_PIN, ADC_11db);   // Rango ~0-3.3V

  // WiFi + Telegram
  conectarWiFiTelegram();

  // Telegram queda corriendo en el NÚCLEO 0, en su propia tarea: sus esperas de
  // varios segundos ya no frenan las luces ni la pantalla (ver tareaTelegram).
  // 8 KB de pila porque la conexión segura necesita bastante espacio.
  xTaskCreatePinnedToCore(
    tareaTelegram,      // función
    "telegram",         // nombre (para depurar)
    8192,               // pila en bytes
    NULL,               // parámetros
    1,                  // prioridad
    NULL,               // handle (no hace falta guardarlo)
    0                   // núcleo 0 — el mismo donde el ESP32 maneja el WiFi
  );

  Serial.println("Sistema listo.");
  Serial.println("Calentador: OFF | Luces: OFF | Cobertor: parado (todo se maneja desde Telegram).");
}

// ============================================================
//   LOOP
// ============================================================

// El loop es el dueño exclusivo de dos cosas: el ADC del micrófono y la tira de
// luces. Ningún comando de Telegram las toca directamente (ver §"pedidos").
//
// Una vuelta completa tarda ~28 ms, casi todos gastados en capturar el sonido.
// Eso da unos 35 cuadros por segundo, suficiente para que el ojo lo vea fluido.
void loop() {
  // 1) Pedidos que sólo este núcleo puede atender (reconfigurar la tira, volcar
  //    la onda cruda). Se resuelven antes de dibujar, nunca en el medio.
  atenderPedidosPendientes();

  // 2) Sonido: captura, FFT, bandas, AGC y detección de golpes.
  analizarSonido();

  // 3) Luces: pinta el cuadro y lo manda a la tira.
  actualizarLuces();

  // 4) Cobertor → máquina de estados no bloqueante (frena al llegar al tope).
  actualizarCobertor();

  // 5) Temperatura + LCD → lectura en dos fases, sin bloquear: la propia función
  //    lleva sus tiempos (pide la conversión y recoge el valor cuando está listo).
  actualizarTemperatura();

  // Telegram NO se atiende acá: vive en su propia tarea, en el otro núcleo.
  // Ver tareaTelegram(). Así sus esperas de varios segundos no frenan el show.
}

// Por qué arrancó el ESP32 la última vez. Un reinicio por caída de tensión o por
// watchdog es invisible desde afuera —la placa simplemente aparece "recién
// encendida"— y es exactamente el tipo de falla que cuesta días encontrar.
void informarMotivoDelReinicio() {
  Serial.println();
  Serial.print("Motivo del ultimo arranque: ");

  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:
      Serial.println("encendido normal.");
      break;
    case ESP_RST_BROWNOUT:
      Serial.println("*** CAIDA DE TENSION (brownout) ***");
      Serial.println("  La alimentacion de 5V no alcanzo. Si pasa al encender las");
      Serial.println("  luces, bajar el presupuesto con /corriente.");
      break;
    case ESP_RST_TASK_WDT:
    case ESP_RST_INT_WDT:
    case ESP_RST_WDT:
      Serial.println("*** WATCHDOG: una tarea dejo de responder ***");
      break;
    case ESP_RST_PANIC:
      Serial.println("*** PANIC: el programa fallo (excepcion) ***");
      break;
    case ESP_RST_SW:
      Serial.println("reinicio pedido por software.");
      break;
    default:
      Serial.println("desconocido.");
      break;
  }
}

// ============================================================
//   CALENTADOR
// ============================================================

// Lectura en dos fases para no frenar el loop (ver CONVERSION_DS18B20_MS):
// primero se le pide la conversión al sensor, y recién cuando terminó —varias
// vueltas de loop después— se recoge el valor y se actúa sobre el calentador.
void actualizarTemperatura() {
  // Fase 1: ¿toca pedir una lectura nueva?
  if (!conversionEnCurso) {
    if (millis() - ultimoTiempoTemp < INTERVALO_TEMP) return;
    sensores.requestTemperatures();   // vuelve enseguida: setWaitForConversion(false)
    conversionEnCurso = true;
    inicioConversion = millis();
    return;
  }

  // Fase 2: el sensor sigue convirtiendo, todavía no hay nada que leer.
  if (millis() - inicioConversion < CONVERSION_DS18B20_MS) return;

  conversionEnCurso = false;
  ultimoTiempoTemp  = millis();

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
      if (!calentadorEncendido && temp < (tempObjetivo - HISTERESIS)) {
        prenderCalentador();
      } else if (calentadorEncendido && temp >= tempObjetivo) {
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
  Serial.print(estadoCobertorTexto());

  // Salud de la conexión. Si el número de "Telegram late hace" empieza a crecer
  // sin parar, la tarea del otro núcleo se colgó aunque todo lo demás ande bien.
  Serial.print(" | WiFi: ");
  Serial.print(WiFi.status() == WL_CONNECTED ? "OK" : "CAIDO");
  Serial.print(" | Telegram late hace ");
  Serial.print((millis() - ultimoLatidoTelegram) / 1000);
  Serial.println("s");
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
//   SONIDO — captura, FFT y análisis de las tres bandas
// ============================================================
//
// Todo este bloque corre en el NÚCLEO 1 (dentro del loop), que es el único dueño
// del ADC. Ninguna función de acá puede llamarse desde la tarea de Telegram: dos
// núcleos usando el mismo ADC dan lecturas corrompidas.

// Una vuelta completa del análisis: capturar, transformar, repartir en bandas,
// normalizar y buscar el golpe.
void analizarSonido() {
  capturarMuestras();

  // ¿Hay música? Se decide con el pico a pico crudo, que es la medida calibrada
  // con 116 mediciones reales del taller. La memoria de 800 ms evita que la tira
  // se apague en el silencio que hay entre dos golpes.
  if (ultimoPicoAPico >= SONIDO_MINIMO) ultimoMomentoConSonido = millis();
  hayMusica = (millis() - ultimoMomentoConSonido) < MEMORIA_MUSICA_MS;

  calcularEspectro();

  graves.crudo = energiaDeBanda(BANDA_GRAVES_DESDE, BANDA_GRAVES_HASTA);
  medios.crudo = energiaDeBanda(BANDA_MEDIOS_DESDE, BANDA_MEDIOS_HASTA);
  agudos.crudo = energiaDeBanda(BANDA_AGUDOS_DESDE, BANDA_AGUDOS_HASTA);

  normalizarBanda(graves);
  normalizarBanda(medios);
  normalizarBanda(agudos);

  volumenGeneral = (graves.nivel + medios.nivel + agudos.nivel) / 3.0f;
  frecuenciaDominanteHz = calcularFrecuenciaDominante();

  detectarBeat();
  emitirTrazaSonido();
}

// Llena vReal con las muestras del micrófono y, de paso, saca las medidas
// crudas (mínimo, máximo, pico a pico y punto de polarización).
//
// Mide además cuánto tardó DE VERDAD: analogRead() no siempre tarda lo mismo, y
// si el muestreo sale a 9,2 kHz en vez de 10 todas las bandas quedan corridas un
// 8 % sin que nadie se entere. Los cortes se calculan con la frecuencia real.
void capturarMuestras() {
  int  minimo = 4095;
  int  maximo = 0;
  long suma   = 0;

  unsigned long inicio = micros();

  for (int i = 0; i < MUESTRAS_FFT; i++) {
    unsigned long t = micros();

    int lectura = analogRead(MIC_PIN);
    vReal[i] = (float)lectura;
    vImag[i] = 0.0f;

    if (lectura < minimo) minimo = lectura;
    if (lectura > maximo) maximo = lectura;
    suma += lectura;

    while (micros() - t < PERIODO_MUESTREO_US) {
      // espera activa para mantener constante la frecuencia de muestreo
    }
  }

  unsigned long duracion = micros() - inicio;
  if (duracion > 0) {
    frecuenciaRealHz = (MUESTRAS_FFT * 1000000.0f) / (float)duracion;
  }

  ultimoMinimo    = minimo;
  ultimoMaximo    = maximo;
  ultimoPicoAPico = maximo - minimo;
  ultimoDC        = (int)(suma / MUESTRAS_FFT);

  seguirPicoAPico();
}

// Lleva el mínimo y el máximo del pico a pico de los últimos segundos, para que
// /diag pueda mostrar un rango y no una sola foto instantánea (ver la
// declaración de las variables). Cuando la ventana en curso cumple su tiempo,
// pasa a ser "la anterior" y empieza una nueva.
void seguirPicoAPico() {
  if (ultimoPicoAPico < p2pMinimoActual) p2pMinimoActual = ultimoPicoAPico;
  if (ultimoPicoAPico > p2pMaximoActual) p2pMaximoActual = ultimoPicoAPico;

  if (millis() - inicioVentanaP2p >= VENTANA_P2P_MS) {
    p2pMinimoPrevio  = p2pMinimoActual;
    p2pMaximoPrevio  = p2pMaximoActual;
    p2pMinimoActual  = 4095;
    p2pMaximoActual  = 0;
    inicioVentanaP2p = millis();
  }
}

// Convierte las muestras en un espectro: al salir, vReal[i] es la magnitud de la
// frecuencia i * frecuenciaReal / MUESTRAS_FFT.
void calcularEspectro() {
  // 1) Quitar el punto de polarización del micrófono (~1,6 V). Sin esto, ese
  //    offset aparece como un pico gigante en la primera frecuencia y contamina
  //    los graves. Era uno de los errores de la FFT vieja.
  fft.dcRemoval();

  // 2) Ventana de Hann: sin ella, los bordes abruptos de cada bloque de muestras
  //    generan frecuencias falsas repartidas por todo el espectro.
  fft.windowing(FFTWindow::Hann, FFTDirection::Forward);

  // 3) La transformada, y de números complejos a magnitudes.
  fft.compute(FFTDirection::Forward);
  fft.complexToMagnitude();
}

// Energía media de una banda de frecuencias, comprimida con raíz cuadrada (lo
// mismo que hace WLED): el rango dinámico de una FFT es enorme y sin comprimir
// sólo se vería el pico más fuerte.
float energiaDeBanda(float desdeHz, float hastaHz) {
  int primerBin = (int)(desdeHz * MUESTRAS_FFT / frecuenciaRealHz);
  int ultimoBin = (int)(hastaHz * MUESTRAS_FFT / frecuenciaRealHz);

  // El bin 0 es lo que quedó del componente continuo: nunca se usa.
  if (primerBin < 1) primerBin = 1;
  if (ultimoBin > (MUESTRAS_FFT / 2) - 1) ultimoBin = (MUESTRAS_FFT / 2) - 1;
  if (ultimoBin < primerBin) return 0.0f;

  float suma = 0.0f;
  for (int i = primerBin; i <= ultimoBin; i++) suma += vReal[i];

  // Promedio por bin, no suma: si no, la banda de agudos (77 bins) le ganaría
  // siempre a la de graves (5 bins) por ser más ancha, no por sonar más fuerte.
  return sqrtf(suma / (float)(ultimoBin - primerBin + 1));
}

// Puerta de ruido + control automático de ganancia, uno por banda.
//
// El AGC existe porque sin él el sistema anda con un volumen y no con otro, y
// porque en música real los graves son 10 o 20 veces más fuertes que los agudos:
// con una escala común la banda de agudos quedaría siempre apagada. Cada banda
// se mide contra SU PROPIO máximo reciente y termina yendo de 0 a 1.
//
// Pero el AGC solo tiene un defecto fatal, verificado en el taller: amplifica el
// silencio. Una banda sin señal ve caer su máximo hasta el piso y su propio
// ruido pasa a valer 100% — con un tono puro de 1000 Hz, las bandas de graves y
// agudos marcaban el máximo teniendo cero contenido.
//
// Por eso el AGC no trabaja sobre el crudo sino sobre la señal ÚTIL: lo que
// queda DESPUÉS de restarle el piso de ruido. Lo que no supera ese piso no es
// música, es el ruido propio del micrófono y del ADC, y vale exactamente cero.
void normalizarBanda(Banda &banda) {
  banda.util = banda.crudo - (float)pisoRuidoBanda;
  if (banda.util < 0.0f) banda.util = 0.0f;

  if (banda.util > banda.maximoAgc) {
    banda.maximoAgc = banda.util;           // sube al instante
  } else {
    banda.maximoAgc *= AGC_DECAIMIENTO;     // baja de a poco
  }

  // Sin este tope inferior, la primera nota después de un silencio se dividiría
  // por un máximo casi nulo y saltaría al 100% de golpe.
  if (banda.maximoAgc < AGC_PISO) banda.maximoAgc = AGC_PISO;

  float objetivo = constrain(banda.util / banda.maximoAgc, 0.0f, 1.0f);

  // Ataque instantáneo, caída suave: el golpe se ve YA, y se apaga con
  // elegancia. Sin esto, a 35 cuadros por segundo la tira parpadea feo.
  if (objetivo > banda.nivel) {
    banda.nivel = objetivo;
  } else {
    banda.nivel += (objetivo - banda.nivel) * CAIDA_NIVEL;
  }
}

// La frecuencia que más suena en este instante. Sirve para diagnóstico
// (/espectro) y para saber si hay zumbido de red contaminando la medición.
float calcularFrecuenciaDominante() {
  int   mejorBin = 1;
  float mejor    = 0.0f;

  for (int i = 1; i < MUESTRAS_FFT / 2; i++) {
    if (vReal[i] > mejor) {
      mejor    = vReal[i];
      mejorBin = i;
    }
  }
  return mejorBin * frecuenciaRealHz / MUESTRAS_FFT;
}

// Detección de golpes por energía sonora.
//
// El beat NO sale de la FFT: sale de comparar la energía instantánea de los
// GRAVES contra el promedio del último segundo. Se usa la energía ÚTIL y no la
// normalizada, porque el AGC justamente borra la información de "este golpe es
// más fuerte que el promedio", que es la que hace falta acá.
//
// Que sea la ÚTIL y no la cruda importa: el piso de ruido es un valor constante
// que se suma por igual al golpe y al promedio, y achata el contraste entre los
// dos. Descontándolo, la relación golpe/promedio crece y los golpes flojos que
// antes quedaban justo debajo del umbral ahora se detectan.
void detectarBeat() {
  hayBeat = false;

  int cantidad = historialLleno ? HISTORIAL_BEAT : posHistorial;

  // Con menos de un cuarto de segundo de historia, el promedio no significa nada
  // todavía: mejor no disparar que disparar cualquier cosa.
  if (cantidad >= 8) {
    float suma = 0.0f;
    for (int i = 0; i < cantidad; i++) suma += historialGraves[i];
    promedioGraves = suma / cantidad;

    float sumaCuadrados = 0.0f;
    for (int i = 0; i < cantidad; i++) {
      float diferencia = historialGraves[i] - promedioGraves;
      sumaCuadrados += diferencia * diferencia;
    }
    float desvio = sqrtf(sumaCuadrados / cantidad);

    // El umbral se ajusta solo: si los golpes están muy marcados (mucha
    // dispersión) se puede ser menos exigente; si la señal es plana hay que
    // exigir más para no disparar con ruido. Se usa el desvío dividido el
    // promedio porque es adimensional y vale en cualquier escala.
    float dispersion = (promedioGraves > 0.01f) ? (desvio / promedioGraves) : 0.0f;
    float factor = FACTOR_BASE - FACTOR_RANGO * fminf(1.0f, dispersion);
    if (factor < FACTOR_MINIMO) factor = FACTOR_MINIMO;

    umbralBeat = promedioGraves * factor;

    unsigned long ahora = millis();
    if (hayMusica && graves.util > umbralBeat &&
        (ahora - ultimoBeat) >= REFRACTARIO_BEAT_MS) {
      hayBeat        = true;
      ultimoBeat     = ahora;
      inicioDestello = ahora;
    }
  }

  // Recién ahora entra al historial: si entrara antes, el golpe se estaría
  // comparando contra un promedio que ya lo incluye y se taparía a sí mismo.
  historialGraves[posHistorial] = graves.util;
  posHistorial++;
  if (posHistorial >= HISTORIAL_BEAT) {
    posHistorial   = 0;
    historialLleno = true;
  }
}

// Vuelca una línea de traza por el Monitor Serie (sólo si /trace está activa).
// Además del ritmo fijo, imprime SIEMPRE que hay un golpe: así ningún evento
// queda afuera del registro aunque dure menos que el intervalo.
void emitirTrazaSonido() {
  if (!trazaSonidoActiva) return;
  if (!hayBeat && (millis() - ultimaTraza < INTERVALO_TRAZA_MS)) return;
  ultimaTraza = millis();

  Serial.print("TRAZA t=");    Serial.print(millis());
  Serial.print(" p2p=");       Serial.print(ultimoPicoAPico);
  Serial.print(" graves=");    Serial.print(graves.crudo, 1);
  Serial.print(" medios=");    Serial.print(medios.crudo, 1);
  Serial.print(" agudos=");    Serial.print(agudos.crudo, 1);
  Serial.print(" utilGraves="); Serial.print(graves.util, 1);
  Serial.print(" piso=");      Serial.print(pisoRuidoBanda);
  Serial.print(" promGraves="); Serial.print(promedioGraves, 1);
  Serial.print(" umbral=");    Serial.print(umbralBeat, 1);
  Serial.print(" beat=");      Serial.print(hayBeat ? 1 : 0);
  Serial.print(" musica=");    Serial.print(hayMusica ? 1 : 0);
  Serial.print(" domHz=");     Serial.print(frecuenciaDominanteHz, 0);
  Serial.print(" fs=");        Serial.println(frecuenciaRealHz, 0);
}

// ============================================================
//   LUCES — efectos sobre la tira WS2812
// ============================================================
//
// Los efectos NO hablan con la tira: pintan en `cuadro[]` los colores ideales,
// sin preocuparse por el consumo ni por el brillo. volcarCuadro() es el único
// que manda datos, y es el que aplica el brillo, el límite de corriente y la
// corrección de gamma. Un solo lugar decide, y es auditable de un vistazo.

void actualizarLuces() {
  // La prueba de /luces_test manda sobre todo lo demás mientras dura.
  if (pruebaTiraActiva) {
    dibujarPruebaTira();
    volcarCuadro();
    return;
  }

  // Copia local: el modo puede cambiar desde el otro núcleo en cualquier momento,
  // y toda esta función tiene que trabajar con un único valor coherente.
  ModoLuces modo = modoLuces;
  static ModoLuces modoPrevio = LUCES_ON;   // distinto de OFF: fuerza el 1er volcado

  if (modo == LUCES_OFF) {
    // Apagadas es el estado por defecto: se manda el cuadro negro una sola vez y
    // después la tira queda quieta, sin tráfico de datos innecesario.
    if (modoPrevio != LUCES_OFF) {
      limpiarCuadro();
      volcarCuadro();
    }
    modoPrevio = modo;
    return;
  }
  modoPrevio = modo;

  if (modo == LUCES_ON) {
    // Blanco cálido: más agradable que el blanco puro y consume bastante menos,
    // porque el canal azul va a menos de la mitad.
    for (int i = 0; i < numLeds; i++) pintarPixel(i, 255, 190, 120);
    volcarCuadro();
    return;
  }

  // --- Modo AUTO ---
  if (!hayMusica) {
    efectoRespiracion();
  } else {
    switch (efectoActual) {
      case EFECTO_MEZCLA:   efectoMezcla();   break;
      case EFECTO_COMETA:   efectoCometa();   break;   // no limpia: deja estela
      case EFECTO_ARCOIRIS: efectoArcoiris(); break;
      case EFECTO_ESPECTRO:
      default:              efectoEspectro(); break;
    }
    aplicarDestello();
  }

  volcarCuadro();
}

// --- Efecto 1: ESPECTRO ---
// La tira dividida en tres zonas; cada una es la barra de nivel de su banda.
// Es el analizador de espectro clásico: se ve exactamente qué hace la música.
void efectoEspectro() {
  limpiarCuadro();

  int porZona = numLeds / 3;
  if (porZona < 1) porZona = 1;

  dibujarBarra(0,             porZona,                 graves.nivel, 255,   0,   0);
  dibujarBarra(porZona,       porZona,                 medios.nivel,   0, 255,   0);
  dibujarBarra(porZona * 2,   numLeds - porZona * 2,   agudos.nivel,   0,  60, 255);
}

// --- Efecto 2: MEZCLA ---
// Toda la tira toma un solo color, mezclado con las tres bandas: rojo = graves,
// verde = medios, azul = agudos. Una canción con mucho bajo se ve roja; un
// pasaje de platos, celeste. La tira "respira" el color de la música.
void efectoMezcla() {
  uint8_t r = (uint8_t)(graves.nivel * 255.0f);
  uint8_t g = (uint8_t)(medios.nivel * 255.0f);
  uint8_t b = (uint8_t)(agudos.nivel * 255.0f);

  for (int i = 0; i < numLeds; i++) pintarPixel(i, r, g, b);
}

// --- Efecto 3: COMETA ---
// Un cometa recorre la tira dejando estela. La velocidad la fija el volumen y el
// color, la banda que domina. En cada golpe rebota y cambia de tono.
void efectoCometa() {
  static float    posicion   = 0.0f;
  static int      direccion  = 1;
  static uint16_t tono       = 0;

  // No se limpia el cuadro: lo que quedó del cuadro anterior se atenúa, y eso
  // es exactamente la estela.
  atenuarCuadro(0.70f);

  posicion += (0.15f + volumenGeneral * 1.10f) * direccion;

  if (posicion >= numLeds - 1) { posicion = numLeds - 1; direccion = -1; }
  if (posicion <= 0)           { posicion = 0;           direccion =  1; }

  if (hayBeat) {
    direccion = -direccion;
    tono += 9000;      // salto de color en cada golpe
  }

  // El tono sigue a la banda dominante: graves al rojo, medios al verde,
  // agudos al celeste.
  uint16_t tonoBanda;
  if (graves.nivel >= medios.nivel && graves.nivel >= agudos.nivel)      tonoBanda = 0;
  else if (medios.nivel >= agudos.nivel)                                 tonoBanda = 21845;
  else                                                                   tonoBanda = 38000;

  int cabeza = (int)posicion;
  pintarHSV(cabeza, tono + tonoBanda, 230, 255);
}

// --- Efecto 4: ARCOÍRIS ---
// Degradado completo que gira sobre la tira. La velocidad sigue al volumen y
// cada golpe le pega un salto de fase más un pico de brillo.
void efectoArcoiris() {
  static uint16_t fase = 0;

  fase += (uint16_t)(150 + volumenGeneral * 1400.0f);
  if (hayBeat) fase += 7000;

  uint8_t valor = (uint8_t)(70.0f + volumenGeneral * 185.0f);

  for (int i = 0; i < numLeds; i++) {
    uint16_t tono = fase + (uint16_t)((uint32_t)i * 65536UL / (uint32_t)numLeds);
    pintarHSV(i, tono, 255, valor);
  }
}

// --- Sin música: respiración ---
// La tira va a pasar la mayor parte del tiempo así, con lo cual tiene que verse
// linda también en silencio. Un ciclo lento de brillo con el tono derivando.
void efectoRespiracion() {
  static uint16_t tono = 0;
  tono += 25;   // deriva lenta por la rueda de color

  // Ciclo de 5 segundos, suave en los dos extremos (por eso el coseno y no una
  // rampa: una rampa se ve como un parpadeo con un corte brusco).
  float fase   = (millis() % 5000) / 5000.0f;
  float brillo = 0.18f + 0.32f * (0.5f - 0.5f * cosf(fase * TWO_PI));

  for (int i = 0; i < numLeds; i++) {
    pintarHSV(i, tono + (uint16_t)(i * 500), 215, (uint8_t)(brillo * 255.0f));
  }
}

// --- Destello del golpe ---
// Una onda blanca que se abre desde el centro hacia los dos extremos y se apaga.
// Es lo que le da el "latido" de discoteca. Se suma encima del efecto que esté
// corriendo, así todos los efectos laten igual.
void aplicarDestello() {
  unsigned long transcurrido = millis() - inicioDestello;
  if (transcurrido >= DURACION_DESTELLO_MS) return;

  float avance     = (float)transcurrido / (float)DURACION_DESTELLO_MS;  // 0 -> 1
  float intensidad = 1.0f - avance;                                      // se apaga
  float centro     = (numLeds - 1) / 2.0f;
  float radio      = avance * (centro + 1.5f);                           // se abre

  for (int i = 0; i < numLeds; i++) {
    // Cuánto le toca a este píxel: máximo justo sobre el frente de la onda.
    float distanciaAlFrente = fabsf(fabsf(i - centro) - radio);
    float cercania = 1.0f - distanciaAlFrente;
    if (cercania <= 0.0f) continue;

    aclararPixel(i, intensidad * cercania);
  }
}

// --- Prueba de /luces_test ---
// Rojo, verde, azul y blanco, 1,2 s cada uno. Sirve para dos cosas: confirmar
// cuántos píxeles responden de verdad, y verificar el orden de colores (si
// anuncia ROJO y se ve VERDE, el chip es RGB y se arregla con /orden).
void dibujarPruebaTira() {
  unsigned long transcurrido = millis() - inicioPruebaTira;
  int paso = transcurrido / PASO_PRUEBA_MS;

  if (paso > 3) {
    pruebaTiraActiva = false;
    limpiarCuadro();
    return;
  }

  uint8_t r = 0, g = 0, b = 0;
  switch (paso) {
    case 0:  r = 255;                   break;
    case 1:            g = 255;         break;
    case 2:                     b = 255; break;
    default: r = 255;  g = 255; b = 255; break;
  }

  for (int i = 0; i < numLeds; i++) pintarPixel(i, r, g, b);
}

// ============================================================
//   LUCES — dibujo en el cuadro y volcado a la tira
// ============================================================

void limpiarCuadro() {
  memset(cuadro, 0, sizeof(cuadro));
}

void pintarPixel(int i, uint8_t r, uint8_t g, uint8_t b) {
  if (i < 0 || i >= numLeds) return;
  cuadro[i][0] = r;
  cuadro[i][1] = g;
  cuadro[i][2] = b;
}

// Pinta usando tono/saturación/valor, que es como se piensa el color cuando se
// diseña un efecto: "el mismo color pero más oscuro" es bajar el valor, en vez
// de tener que recalcular tres números.
void pintarHSV(int i, uint16_t tono, uint8_t saturacion, uint8_t valor) {
  uint32_t color = Adafruit_NeoPixel::ColorHSV(tono, saturacion, valor);
  pintarPixel(i, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
}

// Baja el brillo de todo el cuadro. Llamado en cada vuelta, es lo que genera la
// estela del cometa: cada píxel se va apagando solo.
void atenuarCuadro(float factor) {
  for (int i = 0; i < numLeds; i++) {
    cuadro[i][0] = (uint8_t)(cuadro[i][0] * factor);
    cuadro[i][1] = (uint8_t)(cuadro[i][1] * factor);
    cuadro[i][2] = (uint8_t)(cuadro[i][2] * factor);
  }
}

// Lleva un píxel hacia el blanco, sin pisarlo del todo: el destello se suma al
// color que el efecto ya había puesto, en vez de borrarlo.
void aclararPixel(int i, float fuerza) {
  if (i < 0 || i >= numLeds) return;
  fuerza = constrain(fuerza, 0.0f, 1.0f);

  for (int c = 0; c < 3; c++) {
    float valor = cuadro[i][c] + (255.0f - cuadro[i][c]) * fuerza;
    cuadro[i][c] = (uint8_t)constrain(valor, 0.0f, 255.0f);
  }
}

// Dibuja una barra de nivel dentro de una zona de la tira.
//
// El último píxel se enciende a brillo parcial en vez de saltar de golpe: con
// sólo 5 píxeles por banda, ese detalle es la diferencia entre una barra que se
// mueve con la música y una que da saltos de a un escalón.
void dibujarBarra(int inicio, int largo, float nivel, uint8_t r, uint8_t g, uint8_t b) {
  if (largo <= 0) return;

  float exacto  = constrain(nivel, 0.0f, 1.0f) * largo;
  int   enteros = (int)exacto;
  float resto   = exacto - enteros;

  for (int i = 0; i < largo; i++) {
    if (i < enteros) {
      pintarPixel(inicio + i, r, g, b);
    } else if (i == enteros) {
      pintarPixel(inicio + i, (uint8_t)(r * resto), (uint8_t)(g * resto), (uint8_t)(b * resto));
    }
  }
}

// El único lugar del programa que le manda datos a la tira.
//
// Aplica, en este orden: el brillo elegido, la corrección de gamma y el límite
// de corriente. El límite se calcula sobre el color FINAL —el que el LED va a
// mostrar de verdad— porque la corrección de gamma cambia mucho el consumo: un
// valor de 128 termina siendo 34 en el LED, o sea la cuarta parte de corriente.
void volcarCuadro() {
  float escalaBrillo = brilloPorcentaje / 100.0f;

  // Primera pasada: cuánto consumiría este cuadro tal como está.
  long sumaCanales = 0;
  for (int i = 0; i < numLeds; i++) {
    for (int c = 0; c < 3; c++) {
      sumaCanales += Adafruit_NeoPixel::gamma8((uint8_t)(cuadro[i][c] * escalaBrillo));
    }
  }

  float mAColor  = sumaCanales * MA_POR_CANAL;
  float mAReposo = numLeds * MA_PIXEL_EN_REPOSO;

  // Lo que queda del presupuesto después de pagar el consumo en reposo de los
  // chips, que no se puede evitar por software.
  float mADisponibles = corrienteMaximaMa - mAReposo;
  if (mADisponibles < 0.0f) mADisponibles = 0.0f;

  // Si se pasa, se atenúa todo el cuadro por igual. Como el escalado se hace
  // sobre el valor final, la corriente baja en la misma proporción.
  float limite = 1.0f;
  if (mAColor > mADisponibles && mAColor > 0.01f) {
    limite = mADisponibles / mAColor;
  }

  corrienteEstimadaMa = (int)(mAColor * limite + mAReposo);

  // Segunda pasada: el color definitivo.
  for (int i = 0; i < numLeds; i++) {
    uint8_t r = (uint8_t)(Adafruit_NeoPixel::gamma8((uint8_t)(cuadro[i][0] * escalaBrillo)) * limite);
    uint8_t g = (uint8_t)(Adafruit_NeoPixel::gamma8((uint8_t)(cuadro[i][1] * escalaBrillo)) * limite);
    uint8_t b = (uint8_t)(Adafruit_NeoPixel::gamma8((uint8_t)(cuadro[i][2] * escalaBrillo)) * limite);
    tira.setPixelColor(i, r, g, b);
  }

  tira.show();
}

// Aplica la cantidad de píxeles y el orden de colores. Reasigna memoria dentro
// de la librería, así que SÓLO puede llamarse desde el núcleo 1 y nunca en el
// medio de un envío (ver atenderPedidosPendientes).
void aplicarConfiguracionTira() {
  tira.updateLength(numLeds);
  tira.updateType(tiraEsGRB ? (NEO_GRB + NEO_KHZ800) : (NEO_RGB + NEO_KHZ800));

  // Si la tira creció, los píxeles nuevos mostrarían lo que hubiera quedado en
  // el cuadro de una configuración anterior.
  limpiarCuadro();

  // El brillo de la librería queda al máximo a propósito: lo aplicamos nosotros
  // en volcarCuadro(). setBrightness() reescala el buffer de forma destructiva y,
  // llamado en cada cuadro, va perdiendo resolución hasta ensuciar los colores.
  tira.setBrightness(255);

  tira.clear();
  tira.show();
}

// Trabajos que pidió Telegram pero que sólo el núcleo 1 puede hacer: tocar el
// ADC o reasignar la memoria de la tira. Se resuelven al principio de la vuelta,
// nunca en el medio de un cuadro.
void atenderPedidosPendientes() {
  if (tiraNecesitaReconfigurar) {
    tiraNecesitaReconfigurar = false;
    aplicarConfiguracionTira();
  }

  if (pedidoVolcarOnda) {
    pedidoVolcarOnda = false;
    volcarOndaCruda();
  }
}

// Vuelca por el Monitor Serie la onda cruda del micrófono, en CSV, para poder
// analizarla en la PC (por ejemplo, para verificar el contenido de frecuencias o
// buscar el zumbido de la red).
//
// Captura primero y escribe después: el puerto serie es unas siete veces más
// lento que el muestreo, así que imprimir dentro del bucle deformaría la onda
// que se está tratando de medir.
void volcarOndaCruda() {
  unsigned long inicio = micros();

  for (int i = 0; i < MUESTRAS_FFT; i++) {
    unsigned long t = micros();
    vReal[i] = (float)analogRead(MIC_PIN);
    while (micros() - t < PERIODO_MUESTREO_US) {
      // espera activa para mantener constante la frecuencia de muestreo
    }
  }

  unsigned long duracion = micros() - inicio;
  float hz = (duracion > 0) ? (MUESTRAS_FFT * 1000000.0f / (float)duracion) : 0.0f;

  Serial.println();
  Serial.println("=== ONDA CRUDA DEL MICROFONO ===");
  Serial.print("muestras=");        Serial.println(MUESTRAS_FFT);
  Serial.print("frecuencia_hz=");   Serial.println(hz, 1);
  Serial.println("indice,valor");

  for (int i = 0; i < MUESTRAS_FFT; i++) {
    Serial.print(i);
    Serial.print(",");
    Serial.println((int)vReal[i]);
  }

  Serial.println("=== FIN DE LA ONDA ===");
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

// La velocidad se guarda en PWM (0-255) porque es lo que entiende el L298N, pero
// se muestra y se pide en porcentaje, que es lo que entiende cualquiera.
int velocidadPorcentaje() {
  return (velocidadCobertor * 100) / 255;
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
  analogWrite(MOTOR_A_EN, velocidadCobertor);
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
  analogWrite(MOTOR_B_EN, velocidadCobertor);
}

void cobertorFrenar() {
  motorA_libre();
  motorB_libre();
}

// Empieza a abrir: el rodillo enrolla la lona, el otro lado suelta cable.
//
// OJO con el orden de las dos primeras líneas: la orden nace en la tarea de
// Telegram (núcleo 0) y el loop la vigila desde el otro núcleo. Si el estado se
// escribiera primero, el loop podría verlo con el reloj del movimiento anterior
// —de hace varios minutos— y cortar en el acto "por seguridad". Primero el
// reloj, último el estado: cuando el loop ve el estado nuevo, ya está todo listo.
void cobertorAbrir() {
  inicioMovimientoCobertor = millis();
  motorB_libre();
  motorA_enrollar();
  estadoCobertor = COB_ABRIENDO;
  Serial.println(">>> Cobertor: ABRIENDO");
}

// Empieza a cerrar: los cables tiran la lona, el rodillo la suelta.
void cobertorCerrar() {
  inicioMovimientoCobertor = millis();
  motorA_libre();
  motorB_tirar();
  estadoCobertor = COB_CERRANDO;
  Serial.println(">>> Cobertor: CERRANDO");
}

void cobertorParar() {
  estadoCobertor = COB_PARADO;
  motorEnPrueba = '-';
  cobertorFrenar();
  Serial.println(">>> Cobertor: PARADO");
}

// --- Prueba de taller: un motor solo, unos segundos ---
//
// Sirve para verificar el cableado del L298N y el sentido de giro ANTES de montar
// el mecanismo: no mira los fines de carrera (todavía no están puestos) y frena
// sola a los DURACION_PRUEBA_MOTOR_MS. Usarla con los motores DESACOPLADOS.
void probarMotor(char cual) {
  inicioMovimientoCobertor = millis();

  if (cual == 'A') {
    motorB_libre();
    motorA_enrollar();
  } else {
    motorA_libre();
    motorB_tirar();
  }

  motorEnPrueba  = cual;
  estadoCobertor = COB_PRUEBA;   // último, por lo mismo que en cobertorAbrir()

  Serial.print(">>> Prueba del motor ");
  Serial.println(cual);
}

// Máquina de estados: se llama en cada loop. Frena al llegar al tope
// o si pasa demasiado tiempo (seguridad).
void actualizarCobertor() {
  if (estadoCobertor == COB_PARADO) return;

  // Prueba de taller: frena sola al cumplirse el tiempo. No mira los fines de
  // carrera a propósito (se usa cuando todavía no están montados).
  if (estadoCobertor == COB_PRUEBA) {
    if (millis() - inicioMovimientoCobertor >= DURACION_PRUEBA_MOTOR_MS) {
      cobertorParar();
    }
    return;
  }

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

// Guarda a quién hay que avisarle cuando termine el movimiento. Se llama SIEMPRE
// desde la tarea de Telegram, que es el único escritor de chatCobertor.
void recordarChatDelCobertor(const String &chat_id) {
  snprintf(chatCobertor, sizeof(chatCobertor), "%s", chat_id.c_str());
}

// Deja el aviso en el buzón para que lo mande la tarea de Telegram. No envía acá
// mismo porque esto corre en el loop (núcleo 1) y `bot` es de la otra tarea.
//
// El loop sólo LEE chatCobertor. Mantener un solo escritor por variable es lo que
// hace segura la conversación entre los dos núcleos sin necesidad de candados.
void avisarCobertor(const char *msg) {
  if (chatCobertor[0] == '\0' || avisoPendiente) return;

  snprintf(avisoChat,  sizeof(avisoChat),  "%s", chatCobertor);
  snprintf(avisoTexto, sizeof(avisoTexto), "%s", msg);
  avisoPendiente = true;   // último: recién ahora el mensaje está completo
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
    client.setTimeout(2000);   // Sin esto, una consulta lenta congela el show de luces
    telegramListo = true;
    Serial.println("Bot de Telegram listo.");
  } else {
    telegramListo = false;
    Serial.println("No se pudo conectar a WiFi. El sistema sigue funcionando sin Telegram.");
  }
}

// ============================================================
//   TAREA DE TELEGRAM — corre en el NÚCLEO 0, aparte del loop
// ============================================================
//
// Consultar Telegram abre una conexión segura nueva cada vez, y ese saludo TLS
// bloquea hasta 3 segundos. Medido con /trace el 2026-08-06: 21 de cada 70
// mediciones tenían huecos de ~3000 ms, o sea que el programa pasaba más de la
// mitad del tiempo congelado ahí — con el LCD sin refrescar y las luces clavadas.
//
// La solución es darle a Telegram su propio hilo, fijado al núcleo 0 (el mismo
// donde el ESP32 maneja el WiFi), y dejar loop() corriendo libre en el núcleo 1
// para el sonido, las luces, el cobertor y la pantalla. Ahora el bloqueo sigue
// existiendo, pero le pasa a una tarea que no le importa esperar.
//
// El vTaskDelay NO es opcional: sin él la tarea nunca cede el CPU y el watchdog
// del núcleo 0 reinicia la placa.
void tareaTelegram(void *parametros) {
  for (;;) {
    ultimoLatidoTelegram = millis();   // "sigo viva" (lo lee el loop, ver actualizarTemperatura)
    procesarTelegram();
    enviarAvisoPendiente();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Despacha el aviso que el loop haya dejado en el buzón (ver avisarCobertor).
void enviarAvisoPendiente() {
  if (!avisoPendiente) return;

  if (WiFi.status() == WL_CONNECTED && telegramListo) {
    bot.sendMessage(avisoChat, avisoTexto, "");
  }
  avisoPendiente = false;   // último: recién ahora el buzón queda libre
}

void procesarTelegram() {
  if (millis() - ultimoCheckTelegram < INTERVALO_TELEGRAM) {
    return;
  }
  ultimoCheckTelegram = millis();

  // Sin WiFi no hay nada que consultar. El core reintenta conectarse solo, pero
  // si no vuelve por su cuenta se fuerza una reconexión limpia cada tanto.
  if (WiFi.status() != WL_CONNECTED) {
    telegramListo = false;
    if (millis() - ultimoIntentoWiFi > REINTENTO_WIFI_MS) {
      ultimoIntentoWiFi = millis();
      Serial.println("WiFi caido: reintentando conectar...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    return;
  }

  if (!telegramListo) {
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    client.setTimeout(2000);
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
  //
  // Los comandos SÓLO cambian el modo. Nunca tocan la tira directamente: esto
  // corre en el núcleo 0 (Telegram) y la tira es del núcleo 1. Mandarle datos
  // desde acá sería pisar un envío en curso, y encima desde el núcleo donde vive
  // el WiFi, que es justo lo que produce los parpadeos. El loop dibuja el cambio
  // en la vuelta siguiente, unos 28 ms después.
  else if (text == "/luces_auto" || text == "/auto") {
    modoLuces = LUCES_AUTO;
    bot.sendMessage(chat_id, "Luces en AUTO: " + nombreDelEfecto() +
                             ".\nCambia el efecto con /efecto 1 a 4.", "");
  }
  else if (text == "/luces_on") {
    modoLuces = LUCES_ON;
    bot.sendMessage(chat_id, "Luces ON: tira encendida fija (blanco cálido) al " +
                             String(brilloPorcentaje) + "% de brillo.", "");
  }
  else if (text == "/luces_off") {
    modoLuces = LUCES_OFF;
    bot.sendMessage(chat_id, "Luces OFF: tira apagada.", "");
  }
  else if (text == "/luces_test") {
    inicioPruebaTira = millis();
    pruebaTiraActiva = true;
    bot.sendMessage(chat_id,
      "PRUEBA DE LA TIRA (" + String(numLeds) + " pixeles)\n\n"
      "Va a mostrar, 1,2 segundos cada uno:\n"
      "1) ROJO   2) VERDE   3) AZUL   4) BLANCO\n\n"
      "Mirá dos cosas:\n"
      "- Que se enciendan TODOS los pixeles (si no, ajustá /leds N).\n"
      "- Que los colores coincidan. Si anuncia ROJO y ves VERDE, tu tira usa "
      "otro orden de colores: mandá /orden y repetí la prueba.", "");
  }
  else if (text == "/orden") {
    tiraEsGRB = !tiraEsGRB;
    preferencias.putBool("tiraGRB", tiraEsGRB);
    tiraNecesitaReconfigurar = true;   // lo aplica el loop, no este núcleo
    bot.sendMessage(chat_id, "Orden de colores: " + String(tiraEsGRB ? "GRB" : "RGB") +
                             ". Guardado.\nProbá de nuevo con /luces_test.", "");
  }
  else if (text.startsWith("/efecto")) {
    String arg = text.substring(7);
    arg.trim();
    int numero = arg.toInt();

    if (arg.length() == 0) {
      bot.sendMessage(chat_id, "Efecto actual: " + String(efectoActual) + " — " +
                               nombreDelEfecto() + "\n\n" + listaDeEfectos(), "");
    } else if (numero < EFECTO_MINIMO || numero > EFECTO_MAXIMO) {
      bot.sendMessage(chat_id, "Efecto invalido.\n\n" + listaDeEfectos(), "");
    } else {
      efectoActual = numero;
      preferencias.putUChar("efecto", (uint8_t)efectoActual);
      bot.sendMessage(chat_id, "Efecto " + String(efectoActual) + ": " +
                               nombreDelEfecto() + ". Guardado.", "");
    }
  }
  else if (text.startsWith("/leds")) {
    String arg = text.substring(5);
    arg.trim();
    int cantidad = arg.toInt();

    if (arg.length() == 0) {
      bot.sendMessage(chat_id, "La tira esta configurada con " + String(numLeds) +
                               " pixeles.\nPara cambiarlo: /leds 15\n"
                               "(una tira de 30 LED/m tiene 15 pixeles cada 50 cm)", "");
    } else if (cantidad < 1 || cantidad > MAX_LEDS) {
      bot.sendMessage(chat_id, "Valor invalido: usa un numero entre 1 y " +
                               String(MAX_LEDS) + ".", "");
    } else {
      numLeds = cantidad;
      preferencias.putUShort("numLeds", (uint16_t)numLeds);
      tiraNecesitaReconfigurar = true;
      bot.sendMessage(chat_id, "Tira configurada con " + String(numLeds) +
                               " pixeles. Guardado.\nVerificalo con /luces_test.", "");
    }
  }
  else if (text.startsWith("/brillo")) {
    String arg = text.substring(7);
    arg.trim();
    int porcentaje = arg.toInt();

    if (arg.length() == 0) {
      bot.sendMessage(chat_id, "Brillo maximo: " + String(brilloPorcentaje) +
                               "%.\nPara cambiarlo: /brillo 60", "");
    } else if (porcentaje < 0 || porcentaje > 100) {
      bot.sendMessage(chat_id, "Valor invalido: usa un numero entre 0 y 100.", "");
    } else {
      brilloPorcentaje = porcentaje;
      preferencias.putUChar("brillo", (uint8_t)brilloPorcentaje);
      bot.sendMessage(chat_id, "Brillo maximo: " + String(brilloPorcentaje) +
                               "%. Guardado.\n"
                               "Ojo: el limite de corriente puede bajarlo todavia mas "
                               "(mira /status).", "");
    }
  }
  else if (text.startsWith("/corriente")) {
    String arg = text.substring(10);
    arg.trim();
    int miliamperes = arg.toInt();

    if (arg.length() == 0) {
      bot.sendMessage(chat_id, armarCorriente(), "");
    } else if (miliamperes < 50 || miliamperes > 2000) {
      bot.sendMessage(chat_id, "Valor invalido: usa un numero entre 50 y 2000 mA.", "");
    } else {
      corrienteMaximaMa = miliamperes;
      preferencias.putUShort("corriente", (uint16_t)corrienteMaximaMa);
      bot.sendMessage(chat_id, "Presupuesto de corriente: " + String(corrienteMaximaMa) +
                               " mA. Guardado.\n" + consejoDeCorriente(), "");
    }
  }
  // El piso de ruido depende del lugar donde esté instalada la pileta, no del
  // programa: un taller con gente hablando y un patio de noche no tienen el
  // mismo ruido de fondo. Por eso se calibra desde Telegram y queda guardado.
  else if (text.startsWith("/piso")) {
    String arg = text.substring(5);
    arg.trim();
    int valor = arg.toInt();

    if (arg.length() == 0) {
      bot.sendMessage(chat_id,
        "Piso de ruido por banda: " + String(pisoRuidoBanda) + "\n\n"
        "Lo que no supera este valor no cuenta como sonido: es el ruido del\n"
        "microfono y del ambiente, y vale cero.\n\n"
        "Para calibrarlo: mandá /espectro EN SILENCIO y poné el piso un poco\n"
        "por encima del CRUDO mas alto que veas.\n"
        "Para cambiarlo: /piso 12", "");
    } else if (valor < 0 || valor > 200) {
      bot.sendMessage(chat_id, "Valor invalido: usa un numero entre 0 y 200.", "");
    } else {
      pisoRuidoBanda = valor;
      preferencias.putUChar("pisoRuido", (uint8_t)pisoRuidoBanda);
      bot.sendMessage(chat_id, "Piso de ruido por banda: " + String(pisoRuidoBanda) +
                               ". Guardado.\n"
                               "La ganancia automatica se reacomoda en unos segundos.\n"
                               "Verificalo con /espectro.", "");
    }
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
      recordarChatDelCobertor(chat_id);
      cobertorAbrir();
      bot.sendMessage(chat_id, "Abriendo el cobertor... te aviso cuando termine.", "");
    }
  }
  else if (text == "/cobertor_cerrar") {
    if (finDeCarreraTocado(FC_CERRADO)) {
      bot.sendMessage(chat_id, "El cobertor ya está cerrado.", "");
    } else {
      recordarChatDelCobertor(chat_id);
      cobertorCerrar();
      bot.sendMessage(chat_id, "Cerrando el cobertor... te aviso cuando termine.", "");
    }
  }
  else if (text == "/cobertor_parar") {
    cobertorParar();
    chatCobertor[0] = '\0';   // lo paré a mano: no hace falta avisar nada
    bot.sendMessage(chat_id, "Cobertor detenido.", "");
  }

  // --- Prueba de los motores (taller) ---
  else if (text == "/motor_a" || text == "/motor_b") {
    char cual = (text == "/motor_a") ? 'A' : 'B';

    if (estadoCobertor == COB_ABRIENDO || estadoCobertor == COB_CERRANDO) {
      bot.sendMessage(chat_id, "El cobertor se está moviendo. Frenalo con /cobertor_parar "
                               "antes de probar un motor suelto.", "");
    } else {
      probarMotor(cual);
      bot.sendMessage(chat_id, String("Probando el motor ") + cual + " durante " +
                               String(DURACION_PRUEBA_MOTOR_MS / 1000) + " segundos.\n" +
                               "Hacelo con los motores DESACOPLADOS del mecanismo. "
                               "Si gira para el lado equivocado, invertí sus dos cables "
                               "en el L298N (OUT1<->OUT2 o OUT3<->OUT4).", "");
    }
  }

  // --- Consultas ---
  else if (text == "/status") {
    bot.sendMessage(chat_id, armarStatus(), "");
  }
  else if (text == "/temp") {
    bot.sendMessage(chat_id, armarTemp(), "");
  }
  else if (text.startsWith("/temperatura")) {
    String arg = text.substring(12);   // lo que viene después de "/temperatura"
    arg.trim();
    float nueva = arg.toFloat();
    if (arg.length() == 0) {
      bot.sendMessage(chat_id, "Temperatura objetivo actual: " + String(tempObjetivo, 1) +
                               " C.\nPara cambiarla: /temperatura 28", "");
    } else if (nueva < 15 || nueva > 35) {
      bot.sendMessage(chat_id, "Valor invalido. Usa un numero entre 15 y 35. Ej: /temperatura 28", "");
    } else {
      tempObjetivo = nueva;
      preferencias.putFloat("tempObj", tempObjetivo);
      bot.sendMessage(chat_id, "Temperatura objetivo: " + String(tempObjetivo, 1) +
                               " C. Guardada: sobrevive reinicios.", "");
    }
  }
  else if (text.startsWith("/velocidad")) {
    String arg = text.substring(10);   // lo que viene después de "/velocidad"
    arg.trim();
    int porcentaje = arg.toInt();

    if (arg.length() == 0) {
      bot.sendMessage(chat_id, "Velocidad de los motores: " + String(velocidadPorcentaje()) +
                               "%.\nPara cambiarla: /velocidad 35", "");
    } else if (porcentaje < VELOCIDAD_MINIMA_PORCENTAJE || porcentaje > 100) {
      bot.sendMessage(chat_id, "Valor invalido: usa un numero entre " +
                               String(VELOCIDAD_MINIMA_PORCENTAJE) + " y 100. Ej: /velocidad 35\n"
                               "Por debajo de " + String(VELOCIDAD_MINIMA_PORCENTAJE) +
                               "% el motor zumba pero no llega a girar.", "");
    } else {
      velocidadCobertor = (porcentaje * 255) / 100;
      preferencias.putUChar("velCobertor", (uint8_t)velocidadCobertor);
      bot.sendMessage(chat_id, "Velocidad de los motores: " + String(porcentaje) +
                               "%. Guardada: sobrevive reinicios.\n"
                               "Probala con /motor_a o /motor_b.", "");
    }
  }
  else if (text == "/audio") {
    bot.sendMessage(chat_id, armarAudio(), "");
  }
  else if (text == "/espectro") {
    bot.sendMessage(chat_id, armarEspectro(), "");
  }
  else if (text == "/diag") {
    bot.sendMessage(chat_id, armarDiagnosticoMicrofono(), "");
  }
  else if (text == "/onda") {
    // La captura la hace el loop: el ADC es del núcleo 1 y este código corre en
    // el 0. Acá sólo se deja el pedido.
    pedidoVolcarOnda = true;
    bot.sendMessage(chat_id,
      "Volcando 256 muestras crudas del microfono por el Monitor Serie (115200).\n"
      "Copiá el bloque entre '=== ONDA CRUDA ===' y '=== FIN DE LA ONDA ===' "
      "para analizarlo en la PC.", "");
  }
  else if (text == "/trace") {
    trazaSonidoActiva = !trazaSonidoActiva;
    bot.sendMessage(chat_id, trazaSonidoActiva
      ? "Traza de sonido ACTIVADA: los datos salen por el Monitor Serie (115200)."
      : "Traza de sonido apagada.", "");
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
  s += "/luces_on - encendidas fijas\n";
  s += "/luces_off - apagadas\n";
  s += "/efecto 1 a 4 - qué efecto usar en AUTO\n";
  s += "/brillo 70 - brillo máximo (%)\n";
  s += "/luces_test - probar la tira color por color\n\n";
  s += "CALENTADOR:\n";
  s += "/calentador_auto - automático por temperatura\n";
  s += "/calentador_on - forzar encendido\n";
  s += "/calentador_off - forzar apagado\n";
  s += "/temperatura 28 - cambiar la temperatura objetivo\n\n";
  s += "COBERTOR:\n";
  s += "/cobertor_abrir - destapar la pileta\n";
  s += "/cobertor_cerrar - tapar la pileta\n";
  s += "/cobertor_parar - frenar el cobertor\n";
  s += "/motor_a, /motor_b - probar un motor solo (taller)\n";
  s += "/velocidad 35 - que tan rapido se mueven los motores (%)\n\n";
  s += "INFORMACIÓN:\n";
  s += "/status - estado general\n";
  s += "/temp - temperatura\n";
  s += "/audio - volumen y bandas en vivo\n";
  s += "/espectro - detalle del análisis de frecuencias\n\n";
  s += "AJUSTES FINOS (taller):\n";
  s += "/leds 21 - cuántos pixeles tiene la tira\n";
  s += "/corriente 120 - presupuesto de corriente (mA)\n";
  s += "/orden - invertir el orden de colores (GRB/RGB)\n";
  s += "/piso 12 - piso de ruido del microfono\n";
  s += "/diag - diagnostico del microfono\n";
  s += "/trace - traza del sonido por Monitor Serie\n";
  s += "/onda - volcar la onda cruda por Monitor Serie\n";
  s += "/ip - IP del ESP32";
  return s;
}

String nombreDelEfecto() {
  switch (efectoActual) {
    case EFECTO_ESPECTRO: return "ESPECTRO (una barra por banda: graves, medios, agudos)";
    case EFECTO_MEZCLA:   return "MEZCLA (el color sale de mezclar las tres bandas)";
    case EFECTO_COMETA:   return "COMETA (recorre la tira y rebota en cada golpe)";
    case EFECTO_ARCOIRIS: return "ARCOIRIS (degradado que gira con la música)";
    default:              return "?";
  }
}

String listaDeEfectos() {
  String s = "Efectos disponibles:\n";
  s += "/efecto 1 - ESPECTRO: la tira en 3 zonas, una barra por banda\n";
  s += "             (graves rojo, medios verde, agudos azul)\n";
  s += "/efecto 2 - MEZCLA: toda la tira de un color, mezclado con\n";
  s += "             las 3 bandas. Mucho bajo = rojo, platos = celeste\n";
  s += "/efecto 3 - COMETA: recorre la tira dejando estela y rebota\n";
  s += "             en cada golpe\n";
  s += "/efecto 4 - ARCOIRIS: degradado que gira más rápido cuanto\n";
  s += "             más fuerte suena";
  return s;
}

// Consejo según con qué esté alimentado el ESP32. El riel de 5V no está libre:
// ya alimenta al LCD, al relé y al micrófono, además del propio ESP32.
String consejoDeCorriente() {
  String s = "Referencia:\n";
  s += "- 120 mA si el ESP32 está enchufado al USB de la notebook\n";
  s += "- 500 mA si está en un cargador de celular de 2A\n";
  s += "Si el ESP32 se reinicia al encender las luces, bajá este número.";
  return s;
}

String armarCorriente() {
  String s = "PRESUPUESTO DE CORRIENTE\n\n";
  s += "Límite configurado: " + String(corrienteMaximaMa) + " mA\n";
  s += "Consumo del último cuadro: " + String(corrienteEstimadaMa) + " mA\n";
  s += "Pixeles: " + String(numLeds) + "\n\n";
  s += "Para cambiarlo: /corriente 120\n\n";
  s += consejoDeCorriente();
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

  s += "Luces: " + modoLucesTexto();
  if (modoLuces == LUCES_AUTO) s += " — efecto " + String(efectoActual);
  s += "\n";
  s += "Tira: " + String(numLeds) + " pixeles, brillo " + String(brilloPorcentaje) + "%\n";
  s += "Consumo tira: " + String(corrienteEstimadaMa) + " de " +
       String(corrienteMaximaMa) + " mA\n";
  s += "Sonido: ";
  s += (hayMusica ? "musica detectada" : "silencio");
  s += "\n";
  s += "Cobertor: " + estadoCobertorTexto() + "\n";
  s += "Velocidad motores: " + String(velocidadPorcentaje()) + "%\n";
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
  s += "Objetivo: " + String(tempObjetivo, 1) + " C\n";
  s += "Histéresis: " + String(HISTERESIS, 1) + " C\n";
  s += "Calentador: ";
  s += (calentadorEncendido ? "ON" : "OFF");
  s += " (" + modoCalentadorTexto() + ")";
  return s;
}

// Diagnóstico del micrófono: los valores crudos del ADC de la última ventana.
//
// NO mide por su cuenta a propósito. Esto corre en el núcleo 0 (Telegram) y el
// ADC pertenece al núcleo 1: dos núcleos usando el mismo ADC dan lecturas
// corrompidas. Se reportan los números que el loop ya midió, que además son
// exactamente los que está usando el sistema para decidir.
String armarDiagnosticoMicrofono() {
  String s = "DIAGNOSTICO DEL MICROFONO\n";
  s += "(valores crudos del ADC, 0-4095)\n\n";
  s += "Minimo: " + String(ultimoMinimo) + "\n";
  s += "Maximo: " + String(ultimoMaximo) + "\n";
  s += "PICO A PICO: " + String(ultimoPicoAPico) + "\n";
  s += "Punto de reposo (DC): " + String(ultimoDC) + "\n";
  s += "Muestreo real: " + String(frecuenciaRealHz, 0) + " Hz\n\n";

  // Una sola ventana son 25,6 ms: una foto. Para calibrar hace falta el rango
  // de un rato, que es lo que dice si el umbral de musica esta bien puesto.
  int rangoMinimo = min(p2pMinimoActual, p2pMinimoPrevio);
  int rangoMaximo = max(p2pMaximoActual, p2pMaximoPrevio);

  s += "ULTIMOS ~10 SEGUNDOS\n";
  s += "  Pico a pico: entre " + String(rangoMinimo) + " y " + String(rangoMaximo) + "\n";
  s += "  Umbral de \"hay musica\": " + String(SONIDO_MINIMO) + " -> ahora: ";
  s += (hayMusica ? "musica detectada" : "silencio");
  s += "\n";
  if (rangoMaximo >= SONIDO_MINIMO && rangoMinimo >= SONIDO_MINIMO) {
    s += "  OJO: el minimo del rango ya supera el umbral. Si no hay musica\n";
    s += "  sonando, el ruido de fondo del lugar es mas alto que el umbral.\n";
  }
  s += "\n";

  if (ultimoPicoAPico < 20) {
    s += "=> Senal MUY DEBIL o nula. El sensor casi no esta captando.\n";
  } else if (ultimoPicoAPico < 100) {
    s += "=> Senal DEBIL. Sirve para ruidos fuertes, no para musica.\n";
  } else if (ultimoPicoAPico < 500) {
    s += "=> Senal ACEPTABLE. Alcanza para las luces.\n";
  } else {
    s += "=> Senal FUERTE. De sobra.\n";
  }

  if (ultimoMaximo > 4000) {
    s += "\nOJO: el maximo esta pegado al techo del ADC. La senal se esta\n";
    s += "recortando y el analisis de frecuencias se ensucia.\n";
  }

  s += "\nPara verificar si la tira ensucia la medicion: mandá /diag con las\n";
  s += "luces apagadas y otra vez con /luces_on. Si el pico a pico en silencio\n";
  s += "sube mucho, la tira esta contaminando la alimentacion del microfono.";
  return s;
}

String armarAudio() {
  String s = "AUDIO\n";
  s += "Sonido: ";
  s += (hayMusica ? "musica detectada" : "silencio");
  s += "\n";
  s += "VOLUMEN (pico a pico): " + String(ultimoPicoAPico) + "\n\n";
  s += "BANDAS (0 a 100):\n";
  s += "  Graves: " + barraDeTexto(graves.nivel) + " " + String((int)(graves.nivel * 100)) + "\n";
  s += "  Medios: " + barraDeTexto(medios.nivel) + " " + String((int)(medios.nivel * 100)) + "\n";
  s += "  Agudos: " + barraDeTexto(agudos.nivel) + " " + String((int)(agudos.nivel * 100)) + "\n\n";
  s += "Frecuencia dominante: " + String(frecuenciaDominanteHz, 0) + " Hz\n";
  s += "Modo luces: " + modoLucesTexto();
  if (modoLuces == LUCES_AUTO) s += " — " + nombreDelEfecto();
  return s;
}

// Una barra hecha con caracteres, para ver el nivel de un vistazo en el chat.
String barraDeTexto(float nivel) {
  const int ANCHO = 10;
  int llenos = (int)(constrain(nivel, 0.0f, 1.0f) * ANCHO + 0.5f);
  String s = "";
  for (int i = 0; i < ANCHO; i++) s += (i < llenos) ? "█" : "·";
  return s;
}

// Detalle del análisis de frecuencias. Es la herramienta para calibrar el
// sistema de luces en el taller: muestra los valores crudos (antes del control
// automático de ganancia), la referencia con la que se normaliza cada banda y el
// umbral con el que se están detectando los golpes.
String armarEspectro() {
  String s = "ANALISIS DE ESPECTRO\n";
  s += "FFT de " + String(MUESTRAS_FFT) + " muestras a " +
       String(frecuenciaRealHz, 0) + " Hz\n";
  s += "Resolucion: " + String(frecuenciaRealHz / MUESTRAS_FFT, 1) + " Hz por banda\n\n";

  s += "Piso de ruido por banda: " + String(pisoRuidoBanda) + "  (se ajusta con /piso)\n\n";

  s += "BANDA    CRUDO   UTIL   TOPE  NIVEL\n";
  s += lineaDeBanda("Graves", graves);
  s += lineaDeBanda("Medios", medios);
  s += lineaDeBanda("Agudos", agudos);
  s += "\n";

  s += "GOLPES (sobre los graves):\n";
  s += "  Promedio del ultimo segundo: " + String(promedioGraves, 1) + "\n";
  s += "  Dispara a partir de: " + String(umbralBeat, 1) + "\n\n";

  s += "Frecuencia dominante: " + String(frecuenciaDominanteHz, 0) + " Hz\n";
  if (frecuenciaDominanteHz > 40 && frecuenciaDominanteHz < 70) {
    s += "  OJO: 50 Hz es el zumbido de la red electrica. Si aparece en\n";
    s += "  silencio, el microfono esta captando ruido de la fuente.\n";
  }

  s += "\nCRUDO es lo que sale de la FFT. UTIL es lo que queda despues de\n";
  s += "restarle el piso de ruido: lo que no lo supera NO es musica, es el\n";
  s += "ruido del microfono, y vale cero. TOPE es la referencia del control\n";
  s += "automatico de ganancia, que mide cada banda contra su propio maximo\n";
  s += "reciente para que los agudos (siempre mas debiles) se vean igual que\n";
  s += "los graves.\n\n";
  s += "Para calibrar el piso: mandá /espectro EN SILENCIO y poné /piso un\n";
  s += "poco por encima del CRUDO mas alto que veas. Asi el silencio queda en\n";
  s += "cero y la musica sigue entrando entera.";
  return s;
}

// Una fila de la tabla de /espectro. Aparte para que las tres salgan iguales y
// no haya tres copias de la misma concatenación con un typo en alguna.
String lineaDeBanda(const char *nombre, const Banda &banda) {
  return String(nombre) + "  " + String(banda.crudo, 1) +
         "   " + String(banda.util, 1) +
         "   " + String(banda.maximoAgc, 1) +
         "   " + String((int)(banda.nivel * 100)) + "%\n";
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

String estadoCobertorTexto() {
  if (estadoCobertor == COB_ABRIENDO) return "abriendo...";
  if (estadoCobertor == COB_CERRANDO) return "cerrando...";
  if (estadoCobertor == COB_PRUEBA)   return String("probando el motor ") + (char)motorEnPrueba;
  if (finDeCarreraTocado(FC_CERRADO)) return "cerrado";
  if (finDeCarreraTocado(FC_ABIERTO)) return "abierto";
  return "parado (posición intermedia)";
}
