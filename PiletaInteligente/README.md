# 🏊 Pileta Inteligente — ESP32 + Telegram

Un solo programa para el ESP32 que controla los tres sistemas de la pileta y se maneja
desde un bot de Telegram:

1. **Calentador** — sensor de temperatura DS18B20 + relé. Arranca **apagado**; se activa desde Telegram (automático o forzado ON).
2. **Luces disco** — sensor de sonido + 4 LEDs que bailan al ritmo de la música. Arrancan **apagadas**; se activan desde Telegram.
3. **Pantalla LCD** — muestra la temperatura y el estado en vivo.

---

## ✅ Cómo ponerlo en marcha (4 pasos)

### 1) Instalar la placa y las librerías (versiones EXACTAS)

Primero, en **Gestor de Tarjetas** instalá el core de la placa:

| Placa | Versión exacta | Autor |
|---|---|---|
| esp32 | **3.3.10** | Espressif Systems |

> ⚠️ El core **3.x** es obligatorio: el programa usa `analogWrite()` en los LEDs,
> que en el core 2.x no existe.

Después, en **Gestor de Librerías**, instalá estas 6 con **estas versiones** (son las
que ya están probadas y funcionando):

| Librería | Versión exacta | Autor |
|---|---|---|
| OneWire | **2.3.8** | Paul Stoffregen |
| DallasTemperature | **4.0.6** | Miles Burton |
| LiquidCrystal I2C | **1.1.2** | Frank de Brabander |
| arduinoFFT | **2.0.4** | Enrique Condes |
| UniversalTelegramBot | **1.3.0** | Brian Lough |
| **ArduinoJson** | **6.21.5** | Benoît Blanchon |

> ⚠️ **LO MÁS IMPORTANTE:** ArduinoJson tiene que ser la **6.21.5**, NO la versión 7.
> Con la 7 el proyecto **no compila** (UniversalTelegramBot todavía no la soporta).
> En el Gestor de Librerías, buscá *ArduinoJson*, elegí **6.21.5** en el desplegable de
> versión e instalá.

### 2) Poner tus datos

Abrí el archivo **`config.h`** y completá:

- El **nombre y la clave de tu WiFi**.
- El **token de tu bot** de Telegram (te lo da **@BotFather**).

No hace falta tocar nada más del programa.

### 3) Conectar los componentes

| Pin del ESP32 | Se conecta a |
|---|---|
| GPIO4  | DS18B20 (dato) — con resistencia 4.7kΩ a 3.3V |
| GPIO26 | Relé (señal S) → calentador |
| GPIO21 | LCD **SDA** (I2C) |
| GPIO22 | LCD **SCL** (I2C) |
| GPIO16 | LED 1 (luces disco) |
| GPIO17 | LED 2 |
| GPIO18 | LED 3 |
| GPIO19 | LED 4 |
| GPIO34 | Sensor de sonido (salida analógica) |

### 4) Cargar al ESP32

Elegí la placa **ESP32 Dev Module**, el puerto COM correcto, y dale a **Subir**.
Abrí el **Monitor Serie** a **115200 baudios** para ver la IP y los mensajes.

---

## 🧩 Cómo está organizado el código

El proyecto son **2 archivos** que importan:

- **`config.h`** → tus datos privados (WiFi + token). **Es lo único que se edita.**
- **`PiletaInteligente.ino`** → todo el programa. No hace falta tocarlo.

Dentro del `.ino`, de arriba hacia abajo, está dividido en bloques bien marcados con
títulos (`// ==========`). Cada bloque tiene una única responsabilidad:

| Bloque | Qué contiene |
|---|---|
| **Encabezado** | Comentario inicial: qué hace el programa y el mapa de conexiones |
| **PINES** | Qué pin del ESP32 va a cada componente |
| **AJUSTES DEL CALENTADOR** | Temperatura objetivo, histéresis y cómo funciona el relé |
| **AJUSTES DE LUCES / SONIDO** | Parámetros del análisis de sonido (FFT) y bandas de frecuencia |
| **TIEMPOS** | Cada cuánto se lee la temperatura, se revisa Telegram y el timeout de WiFi |
| **OBJETOS PRINCIPALES** | Sensores, pantalla, FFT, WiFi y el bot |
| **ESTADO DEL SISTEMA** | Variables y los "modos" (calentador y luces: AUTO / ON / OFF) |
| **setup()** | Se ejecuta **una vez** al encender: configura todo y conecta el WiFi |
| **loop()** | Se repite **siempre** (ver abajo) |
| **CALENTADOR** | Funciones para leer la temperatura y prender/apagar el relé |
| **SONIDO + LUCES** | Muestreo del micrófono, análisis FFT y patrones de las luces |
| **TELEGRAM** | Conexión, lectura de comandos y ejecución |
| **MENSAJES** | Arma los textos que el bot responde (/status, /temp, /audio, ayuda) |

### La idea clave: el `loop()` hace 3 cosas "a la vez"

En cada vuelta, el `loop()` atiende los tres sistemas, cada uno con su propio ritmo:

1. **Sonido y luces** → en cada vuelta (muestrea el micrófono y actualiza los LEDs).
2. **Temperatura y calentador** → cada 2 segundos.
3. **Telegram** → cada 2,5 segundos (revisa si llegó algún comando).

### Cómo mandan los comandos de Telegram

El truco para entender todo: los comandos **no prenden cosas directamente**, sino que
cambian un **"modo"**. Hay dos variables de modo:

- `modoCalentador` → `CALEF_AUTO` / `CALEF_ON` / `CALEF_OFF`
- `modoLuces` → `LUCES_AUTO` / `LUCES_ON` / `LUCES_OFF`

Ambos arrancan en **OFF**. Cuando llega un comando (ej. `/calentador_auto`), lo único que
hace es cambiar el modo. Después, el resto del código (el bloque del calentador y el de
las luces) **actúa según el modo** en el que estén. Por eso es fácil de seguir: los
comandos configuran, y los bloques ejecutan.

---

## 📲 Comandos de Telegram

Escribile `/start` al bot para ver el menú. Comandos:

**Luces** (arrancan apagadas)
- `/luces_auto` — bailan con la música (si no hay música, quedan apagadas)
- `/luces_on` — todas prendidas fijas
- `/luces_off` — todas apagadas

**Calentador** (arranca apagado)
- `/calentador_auto` — que se regule solo por la temperatura
- `/calentador_on` — forzar encendido
- `/calentador_off` — forzar apagado

**Información**
- `/status` — estado general
- `/temp` — temperatura y calentador
- `/audio` — datos del sonido
- `/ip` — IP del ESP32

---

## ⚠️ Si son varios usando el proyecto (¡leer!)

Telegram **no permite que dos ESP32 usen el MISMO token al mismo tiempo**: si dos
placas con el mismo token están encendidas a la vez, se roban los mensajes entre sí
y ninguna anda bien.

Dos formas de resolverlo:

- **Cada uno con su propio bot** (recomendado): cada persona crea su bot con
  @BotFather y pone **su** token en `config.h`. Así cada uno maneja su placa sin pisarse.
- **Un solo bot compartido**: solo **una** placa puede tener el Telegram encendido por vez.

---

## 🚧 Pendiente

- **Sistema 3 — Cobertor motorizado**: todavía no está en este programa (faltan los
  motores). Se suma más adelante con el driver L298N.
