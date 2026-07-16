# 🏊 Pileta Inteligente — ESP32 + Telegram

Un solo programa para el ESP32 que controla los tres sistemas de la pileta y se maneja
desde un bot de Telegram:

1. **Calentador** — sensor de temperatura DS18B20 + relé. Arranca **apagado**; se activa desde Telegram (automático o forzado ON).
2. **Luces disco** — sensor de sonido + 4 LEDs que bailan al ritmo de la música. Arrancan **apagadas**; se activan desde Telegram.
3. **Pantalla LCD** — muestra la temperatura y el estado en vivo.

---

## ✅ Cómo ponerlo en marcha (4 pasos)

### 1) Instalar las librerías

En el Arduino IDE → **Gestor de Librerías** (ícono de los libritos), instalá estas 6:

| Librería | Versión | Autor |
|---|---|---|
| OneWire | última | Paul Stoffregen |
| DallasTemperature | última | Miles Burton |
| LiquidCrystal I2C | última | Frank de Brabander |
| arduinoFFT | 2.x (última) | Enrique Condes |
| UniversalTelegramBot | 1.3.0 | Brian Lough |
| **ArduinoJson** | **6.21.5 (¡la 6, NO la 7!)** | Benoît Blanchon |

> ⚠️ **MUY IMPORTANTE:** ArduinoJson tiene que ser la **versión 6** (por ejemplo 6.21.5).
> Con la versión 7 **el proyecto NO compila** (UniversalTelegramBot todavía no la soporta).
> En el Gestor de Librerías, buscá *ArduinoJson*, elegí la versión **6.21.5** en el
> desplegable e instalá.

También necesitás tener instalada la placa **ESP32** (Gestor de Tarjetas → "esp32" de Espressif).

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
