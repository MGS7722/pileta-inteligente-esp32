# 🏊 Pileta Inteligente — ESP32 + Telegram

Un solo programa para el ESP32 que controla los tres sistemas de la pileta y se maneja
desde un bot de Telegram:

1. **Calentador** — sensor de temperatura DS18B20 + relé. Arranca **apagado**; se activa desde Telegram (automático o forzado ON).
2. **Luces disco** — micrófono + **tira WS2812** de 21 píxeles. El sonido se analiza por FFT y se separa en **graves, medios y agudos**; con esas tres bandas se pintan cuatro efectos distintos, con un destello en cada golpe del ritmo. Arranca apagada; se activa desde Telegram.
3. **Cobertor** — 2 motores por L298N + fines de carrera. Abre/cierra desde Telegram y frena solo al llegar al tope.
4. **Pantalla LCD** — muestra la temperatura y el estado en vivo.

---

## ✅ Cómo ponerlo en marcha (4 pasos)

### 1) Instalar la placa y las librerías (versiones EXACTAS)

Primero, en **Gestor de Tarjetas** instalá el core de la placa:

| Placa | Versión exacta | Autor |
|---|---|---|
| esp32 | **3.3.10** | Espressif Systems |

> ⚠️ El core **3.x** es obligatorio: el programa usa `analogWrite()` para la
> velocidad de los motores del cobertor, que en el core 2.x no existe.

Después, en **Gestor de Librerías**, instalá estas 7 con **estas versiones** (son las
que ya están probadas y funcionando):

| Librería | Versión exacta | Autor |
|---|---|---|
| OneWire | **2.3.8** | Paul Stoffregen |
| DallasTemperature | **4.0.6** | Miles Burton |
| LiquidCrystal I2C | **1.1.2** | Frank de Brabander |
| UniversalTelegramBot | **1.3.0** | Brian Lough |
| **ArduinoJson** | **6.21.5** | Benoît Blanchon |
| **Adafruit NeoPixel** | **1.15.5** | Adafruit |
| **arduinoFFT** | **2.0.4** | Enrique Condes |

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

Dos guías, elegí la que prefieras:
- **`CABLEADO-PASO-A-PASO.md`** → cable por cable, numerado, "para tontos". Ideal para
  armar en la protoboard sin equivocarse.
- **`CONEXIONES.md`** → croquis por sistema + alimentación (visión más de conjunto).

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
| **AJUSTES DE LA TIRA** | Píxeles, brillo, presupuesto de corriente y efectos |
| **AJUSTES DEL ANÁLISIS DE SONIDO** | Tamaño de la FFT, cortes de las bandas, ganancia automática y detección de golpes |
| **TIEMPOS** | Cada cuánto se lee la temperatura, se revisa Telegram y el timeout de WiFi |
| **OBJETOS PRINCIPALES** | Sensores, pantalla, tira, FFT, WiFi y el bot |
| **ESTADO DEL SISTEMA** | Variables y los "modos" (calentador y luces: AUTO / ON / OFF) |
| **setup()** | Se ejecuta **una vez** al encender: configura todo y conecta el WiFi |
| **loop()** | Se repite **siempre** (ver abajo) |
| **CALENTADOR** | Funciones para leer la temperatura y prender/apagar el relé |
| **SONIDO** | Captura, FFT y análisis de las tres bandas |
| **LUCES** | Los efectos, y el volcado a la tira con el límite de corriente |
| **COBERTOR** | Motores, fines de carrera y la máquina de estados del movimiento |
| **TELEGRAM** | Conexión, lectura de comandos y ejecución |
| **MENSAJES** | Arma los textos que el bot responde (/status, /temp, /audio, ayuda) |

### La idea clave: dos núcleos y quién es dueño de qué

El ESP32 tiene **dos núcleos** y el programa los usa a los dos:

- **Núcleo 1** — el `loop()`. Es el **único dueño del micrófono y de la tira**. Una vuelta
  completa tarda ~28 ms (casi todo es capturar el sonido), o sea unos 35 cuadros por segundo.
- **Núcleo 0** — la tarea de Telegram, en el mismo núcleo donde el ESP32 maneja el WiFi.
  Consultar Telegram puede bloquear varios segundos, y ahí no molesta a nadie.

> ⚠️ **Regla que no se rompe:** los comandos de Telegram **nunca** tocan la tira ni el
> micrófono. Sólo cambian una variable, y el loop actúa en la vuelta siguiente. Dos núcleos
> usando el mismo periférico a la vez dan lecturas corrompidas y parpadeos.

En cada vuelta, el `loop()` atiende cada sistema con su propio ritmo:

1. **Sonido y luces** → en cada vuelta.
2. **Cobertor** → en cada vuelta (para frenar apenas toca el fin de carrera).
3. **Temperatura y calentador** → cada 2 segundos.

### Cómo mandan los comandos de Telegram

El truco para entender todo: los comandos **no prenden cosas directamente**, sino que
cambian un **"modo"**. Hay dos variables de modo:

- `modoCalentador` → `CALEF_AUTO` / `CALEF_ON` / `CALEF_OFF`
- `modoLuces` → `LUCES_AUTO` / `LUCES_ON` / `LUCES_OFF`

Ambos arrancan en **OFF**. Cuando llega un comando (ej. `/calentador_auto`), lo único que
hace es cambiar el modo. Después, el resto del código (el bloque del calentador y el de
las luces) **actúa según el modo** en el que estén. Por eso es fácil de seguir: los
comandos configuran, y los bloques ejecutan.

### Cómo funcionan las luces (el análisis del sonido)

El micrófono KY-037 entrega por su pin `AO` la **onda de sonido completa**, sin procesar
(el LM393 de la placa es un comparador, no un amplificador: sólo maneja la salida `DO`,
que no usamos). Eso permite analizar el espectro igual que lo hace un equipo de audio.

La cadena, que corre entera en el núcleo 1 unas 35 veces por segundo:

| Paso | Qué hace |
|---|---|
| **1. Captura** | 256 muestras a 10 kHz (25,6 ms). Mide además la frecuencia real, porque `analogRead()` no siempre tarda lo mismo |
| **2. Preparado** | Quita el punto de reposo del micrófono y aplica una ventana de Hann |
| **3. FFT** | 256 puntos → 128 frecuencias de 39 Hz cada una |
| **4. Bandas** | Agrupa en **graves** (78–234 Hz), **medios** (234–2000 Hz) y **agudos** (2–5 kHz) |
| **5. Puerta de ruido** | A cada banda se le **resta el piso de ruido** del lugar. Lo que no lo supera no es música: es el ruido del micrófono, y vale cero |
| **6. Ganancia automática** | Cada banda se normaliza contra **su propio** máximo reciente. Sin esto los agudos, que son mucho más débiles, quedarían siempre apagados |
| **7. Suavizado** | Ataque instantáneo, caída suave: el golpe se ve al toque y se apaga con elegancia |
| **8. Golpes** | Compara la energía de los graves contra el promedio del último segundo. El umbral se ajusta solo según qué tan marcados estén los golpes |

> **¿Por qué 10 kHz y 256 muestras?** 10 kHz permite analizar hasta 5 kHz, donde vive todo
> el contenido rítmico de la música — es la misma frecuencia que usa WLED, el proyecto de
> referencia en tiras reactivas al sonido. Con 512 muestras la resolución sería el doble,
> pero el refresco caería a 19 cuadros por segundo y se vería a los tirones.

**Casi nada se calibra a mano**: la ganancia automática se acomoda sola al volumen y el umbral
de los golpes se calcula a partir de la dispersión de la propia música. Lo único que depende
del lugar es el **piso de ruido** (`/piso`), porque el ruido de fondo de un taller con gente
no es el de un patio de noche. Se calibra en un minuto: `/espectro` en silencio, y se pone el
piso un poco por encima del CRUDO más alto que se vea.

> 💡 Para ver los números en vivo: **`/audio`** muestra las tres bandas en barritas, y
> **`/espectro`** el detalle completo (valores crudos, contra qué se comparan, umbral de
> golpe y frecuencia dominante). Para calibrar con datos en serio, **`/trace`** vuelca todo
> por el Monitor Serie 10 veces por segundo, y **`/onda`** la señal cruda del micrófono.

### Los cuatro efectos

| Comando | Efecto |
|---|---|
| `/efecto 1` | **ESPECTRO** — la tira en 3 zonas: graves (rojo), medios (verde), agudos (azul) |
| `/efecto 2` | **MEZCLA** — toda la tira de un color mezclado con las 3 bandas |
| `/efecto 3` | **COMETA** — recorre la tira dejando estela y rebota en cada golpe |
| `/efecto 4` | **ARCOÍRIS** — degradado que gira más rápido cuanto más fuerte suena |

Sin música, cualquiera de los cuatro pasa a una **respiración lenta** con el color derivando.

### El limitador de corriente

21 píxeles en blanco pleno consumirían 1,3 A, mucho más de lo que puede dar el USB. Por eso,
antes de mandar cada cuadro, el programa **calcula lo que va a consumir y baja el brillo si se
pasa** del presupuesto. Es lo que permite alimentar la tira del propio ESP32 sin otra fuente.

> Verificado en hardware el 2026-08-13: con `/brillo 70` y con `/brillo 100` el consumo medido
> fue **idéntico**, porque el limitador ya estaba topando en los dos casos. Funciona.

| Cómo esté alimentado el ESP32 | Comando |
|---|---|
| USB de la notebook | `/corriente 120` |
| Cargador de celular de 2A | `/corriente 500` |

`/status` muestra cuánta corriente está usando de la que tiene permitida.

---

## 📲 Comandos de Telegram

Escribile `/start` al bot para ver el menú. Comandos:

**Luces** (arrancan apagadas)
- `/luces_auto` — bailan con la música según el efecto elegido
- `/luces_on` — tira encendida fija (blanco cálido)
- `/luces_off` — tira apagada
- `/efecto 1` a `/efecto 4` — qué efecto usar en AUTO (queda guardado)
- `/brillo 70` — brillo máximo, en % (queda guardado)
- `/luces_test` — prueba la tira color por color (rojo, verde, azul, blanco)

**Calentador** (arranca apagado)
- `/calentador_auto` — que se regule solo por la temperatura
- `/calentador_on` — forzar encendido
- `/calentador_off` — forzar apagado
- `/temperatura 28` — cambiar la temperatura objetivo (queda guardada)

**Cobertor**
- `/cobertor_abrir` — destapar la pileta
- `/cobertor_cerrar` — tapar la pileta
- `/cobertor_parar` — frenar el cobertor
- `/motor_a` / `/motor_b` — prueba de taller: mueve **un** motor 2 segundos, sin mirar los
  fines de carrera. Para verificar cableado y sentido de giro con los motores desacoplados
  (ver `CABLEADO-PASO-A-PASO.md`)
- `/velocidad 35` — qué tan rápido se mueven los motores, en % (de 20 a 100). Queda guardada
  en la memoria del ESP32. Sin número, muestra la actual

**Información**
- `/status` — estado general
- `/temp` — temperatura y calentador
- `/audio` — volumen y las tres bandas, en barritas
- `/espectro` — detalle del análisis de frecuencias (para calibrar)
- `/ip` — IP del ESP32

**Ajustes finos (taller)**
- `/leds 21` — cuántos píxeles tiene la tira conectada (queda guardado)
- `/corriente 120` — presupuesto de corriente en mA (queda guardado)
- `/orden` — invierte el orden de colores GRB ↔ RGB, si los colores salen cambiados
- `/piso 12` — piso de ruido del micrófono, por banda (queda guardado). Lo que no lo supera
  vale cero. Se calibra con `/espectro` en silencio
- `/diag` — valores crudos del micrófono, más el rango de los últimos ~10 segundos
- `/trace` — prende/apaga la traza del sonido por el Monitor Serie
- `/onda` — vuelca 256 muestras crudas del micrófono por el Monitor Serie

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

## ✅ Qué está probado en hardware

- **La tira WS2812** (2026-08-13): 21 píxeles, colores correctos a la primera, limitador de
  corriente verificado y sin interferencia sobre el micrófono.
- **El análisis de espectro**, con tonos puros: 100 Hz → 78/117 Hz · 1000 Hz → 1012 Hz ·
  3000 Hz → 2998 Hz. Las tres bandas responden a lo suyo.
- **Los dos motores del cobertor** giran con `/motor_a` y `/motor_b`, a la velocidad que fija
  `/velocidad`.
- **El calentador**: ciclo completo en modo AUTO, calentó y cortó solo.

## 🚧 Pendiente

La lista completa, con prioridades y contexto, está en **[`../docs/PENDIENTES.md`](../docs/PENDIENTES.md)**.
Lo más urgente:

- **Conectar los 2 fines de carrera** del cobertor y verificar que corten el movimiento.
- **Montar el mecanismo** (rodillo + cables + lona) y definir el sentido de giro de cada motor.
- **Histéresis del calentador**: son 5 °C, mucho para una pileta. Falta decidir si baja a 2 °C.
