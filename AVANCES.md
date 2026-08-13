# Control de avances — Pileta Inteligente

> Programa principal: **`PiletaInteligente/PiletaInteligente.ino`** (calentador + luces + Telegram, todo en un solo archivo).

## Sistema 1 — Calentador automático

- [x] Código base (sensor DS18B20 + relé + histéresis)
- [x] LCD 16x02 mostrando temperatura y estado
- [x] Ciclo completo verificado en protoboard (2026-06-25)
- [x] Control por Telegram (auto / forzar ON / forzar OFF)
- [x] Fuente de 12V y cartucho calefactor conectados por el relé (2026-08-06)
- [x] Polaridad del relé corregida: el módulo es ACTIVO-ALTO (verificado en hardware)
- [x] **Ciclo completo probado en modo AUTO**: calentó hasta 26 °C y cortó solo (2026-08-06)
- [x] **Histéresis bajada de 5 °C a 2 °C** (2026-08-13, decisión de Mariano): con 5, un
      objetivo de 28 °C dejaba el agua oscilando entre 23 y 28. Ahora el rango es 26-28

## Sistema 2 — Luces al ritmo de la música

- [x] Micrófono leído por el ESP32 (pin GPIO34, módulo alimentado a 5V)
- [x] Sensor validado con datos reales: rango dinámico de más de 30× (2026-08-06)
- [x] Control por Telegram (auto / ON / OFF)
- [x] **v5 — Análisis de espectro real** (2026-08-07): FFT de 256 muestras a 10 kHz con
      separación en graves / medios / agudos, ganancia automática por banda y detección de
      golpes por energía del último segundo
- [x] **Tira WS2812 de 21 píxeles** reemplaza a los 8 LEDs, con 4 efectos y limitador de
      corriente por software (permite alimentarla del ESP32 sin una tercera fuente)
- [x] **Tira probada en hardware** (2026-08-13): 21 píxeles, colores correctos a la primera,
      nivel lógico OK a 4,4 V, limitador verificado y sin interferencia sobre el micrófono
- [x] **FFT verificada con tonos puros** (2026-08-13): 100 Hz → 78/117 Hz · 1000 Hz → 1012 Hz ·
      3000 Hz → 2998 Hz. Las tres bandas responden a lo suyo
- [x] **Puerta de ruido por banda** (`/piso`) + `SONIDO_MINIMO` recalibrado a 90 con datos
      reales del taller
- [x] **Los cuatro efectos verificados en vivo con música** (2026-08-13): siguen el ritmo, las
      tres bandas se mueven cada una por su lado y el destello del golpe se ve
- [ ] Recalibrar `/piso` en el lugar definitivo de la pileta (el ruido de fondo del patio no
      es el del taller)

## Sistema 3 — Cobertor automático retráctil

- [x] Motores comprados: 2× Pololu micro metal 6V 500 RPM (eje 3mm) + 2 acoples 5mm
- [x] Mecanismo definido: rodillo (motor A) + cables de tracción por guías (motor B)
- [x] Programada la apertura/cierre: lógica "un motor tira / el otro suelto", PWM y corte por fin de carrera
- [x] Integrado a Telegram: /cobertor_abrir, /cobertor_cerrar, /cobertor_parar (+ aviso al terminar)
- [x] Pines asignados y documentados en CONEXIONES.md
- [x] Alimentación resuelta: fuente regulable del laboratorio a ~8V (sin LM2596 ni resistencias extra)
- [x] Cableado auditado contra la documentación oficial y pines reasignados a los "tranquilos"
      (ENB → GPIO18, fin de carrera abierto → GPIO19) — 2026-08-06
- [x] Comandos `/motor_a` y `/motor_b` para probar un motor solo, sin fines de carrera
- [x] **Los dos motores giran** — probado en hardware con el L298N y la fuente a 8V (2026-08-06)
- [x] Velocidad ajustable en vivo desde Telegram con `/velocidad` (%, guardada en NVS)
- [ ] Conectar los 2 fines de carrera y verificar que corten
- [ ] Montar el mecanismo físico y calibrar el sentido de giro de cada motor

## Bot de Telegram

- [x] Bot creado con BotFather y token funcionando (@ControlESP32Pileta_bot)
- [x] Librerías: UniversalTelegramBot 1.3.0 + ArduinoJson **6.21.5** (la 7 no compila) + core ESP32 3.3.10
- [x] Comandos integrados para calentador, luces y cobertor
- [x] Compila OK en la máquina de Mariano (solo warnings de librerías, inofensivos)

---

## Historial de sesiones

### Sesión 2026-06-25
- Definición del proyecto (3 sistemas + Telegram)
- Calentador (sensor + LCD + LEDs + relé) armado y verificado en protoboard

### Sesión 2026-07-16
- Integrados en un solo programa `PiletaInteligente.ino`: calentador + luces disco + cobertor + Telegram
- Base: archivo "posta" (luces, ya verificado) + bot de Telegram que ya andaba
- Calentador y luces por Telegram (auto / ON / OFF); ambos arrancan APAGADOS
- Luces ampliadas a 8 (4 colores × 2 lados, espejadas en 4 pines)
- Sistema 3 (cobertor) programado e integrado: 2 motores por L298N, lógica "un motor tira / el
  otro suelto", PWM, corte por fin de carrera, timeout de seguridad y aviso por Telegram
- Alimentación resuelta con la fuente de laboratorio doble regulable (12V calentador / ~8V motores)
- Secretos (WiFi + token) separados en `config.h` (fuera de GitHub, en `.gitignore`)
- Limpieza: eliminadas las versiones intermedias y duplicadas; `prueba_sensor.ino` reemplazado
- Documentación completa para los compañeros: `README.md`, `CABLEADO-PASO-A-PASO.md`,
  `CONEXIONES.md`, `COMPONENTES.md`
- Todo subido a GitHub y a la carpeta del Drive "Tecnicas Digitales"
- **Pendiente de probar en hardware** por Mariano (montar el cobertor, cargar el sketch y probar
  los 3 sistemas + comandos de Telegram; calibrar giro y velocidad de los motores)

### Sesión 2026-07-23
- Prueba en hardware real: WiFi, Telegram, sensor de temperatura, LCD, relé y luces ON/OFF ✔
- Problema detectado: las luces en AUTO no reaccionaban a la música. Diagnóstico con el comando
  nuevo `/diag` (señal cruda del ADC): el micrófono HW-484 a 3.3V entrega una señal debilísima
  (pico a pico ~17 ambiente / ~40 música baja / ~177 grito) que se solapa con el ruido ambiente —
  ningún umbral fijo puede separarlos
- Fix 1: volumen medido por pico a pico + DC quitado antes de la FFT (los "graves" daban falsos)
- Fix 2: luces solo prendidas/apagadas (sin PWM), para cuidar los LEDs y no ensuciar el micrófono
- Fix 3 (definitivo): **detección de ritmo adaptativa** — el sistema aprende solo el nivel
  ambiente (base) y dispara un apagón de 110 ms en cada golpe que la supera con margen
  (plan LEGO en `docs/PLAN-LUCES-ADAPTATIVAS.md`, ejecutado por Sonnet, auditado por Fable)
- Mejora de hardware pendiente de verificar: alimentar el sensor de sonido con 5V (pin VIN) en
  vez de 3.3V — el módulo pide 4-6V; verificar con `/diag` que el Máximo quede lejos de 4095
- **Auditoría de raíz** (pedida por Mariano: "algo redundante que no sirve"): confirmado — la FFT
  era complejidad muerta (solo clasificaba graves/agudos, y con 14-30 mV esa clasificación es
  ruido). Además: la ventana de 12,8 ms perdía los beats (evidencia: /audio=16 vs /diag=40 el
  mismo instante) y faltaba `client.setTimeout` (Telegram congelaba el show 1-2 s por consulta)
- **Luces v3** (plan `docs/PLAN-LUCES-V3-SIN-FFT.md`): FFT eliminada por completo (~80 líneas,
  2 KB de RAM y la librería arduinoFFT fuera del proyecto); medición por pico a pico con ventana
  de ~35 ms; efectos sin espectro: sombra rotante con música + strobe en cada golpe;
  `client.setTimeout(2000)` recuperado; nuevo comando `/temperatura N` (guardado en NVS,
  sobrevive reinicios — estaba prometido en PROYECTO.md y faltaba)
- Los compañeros ahora instalan **5 librerías** (arduinoFFT ya no hace falta)
- **Hardware identificado con exactitud** (publicaciones de compra): sensor **KY-037** (LM393,
  especificación 4–6V, DOS unidades compradas) y pack de 100 LEDs 5mm difusos (Vf 1.7–3.8V)
- **Luces v4 — detección BICANAL** (plan `docs/PLAN-LUCES-V4-BICANAL.md`): módulo 1 por AO a
  5V (VIN) con la base adaptativa + módulo 2 (el de repuesto) por DO a 3.3V en GPIO35 como
  detector de golpes por hardware (umbral con su potenciómetro, LED de placa como feedback).
  Fusión con fuente seleccionable (/sonido_mixto | /sonido_ao | /sonido_do, guardada en NVS)
  y filtro anti-pin-flotante. Redundancia para que la primera prueba en el taller funcione
- **Nuevo `PROTOCOLO-TALLER.md`**: checklist paso a paso para la visita al taller (cables,
  verificación /diag, calibración del potenciómetro a ojo, prueba de fuego y plan B)

### Sesión 2026-08-06 — Prueba en hardware: sensor validado y tres fallas de raíz

**Instrumentación nueva**
- Comando **`/trace`**: vuelca por el Monitor Serie el pulso del sonido (p2p, base, umbral,
  golpe AO/DO, música) ~10 veces por segundo, más una línea extra en cada golpe. Permitió
  calibrar con datos reales capturados por USB en vez de a ojo. Arranca apagado.
- El cálculo del golpe del canal AO se movió a `medirSonido()`, para que la traza, `/audio`
  y el efecto de luces miren exactamente el mismo dato.

**El sensor de sonido SIRVE — comprobado con 116 muestras reales**
| Condición | Pico a pico |
|---|---|
| Silencio | 9 – 36 |
| Música (valles) | 60 – 130 |
| Música (golpes) | 250 – 500 |
| Picos fuertes | **870 – 1040** |

Rango dinámico de más de 30×. Con el micrófono a 5V la señal alcanza de sobra, también
para la futura tira WS2812. (El `/diag` mostraba sólo 82 porque mide una foto de 50 ms;
la traza continua ve los picos reales.)

**Recalibración con esos números** — los umbrales venían de la señal de 3.3V (silencio ~17)
y habían quedado POR DEBAJO del ruido de fondo, así que ya no filtraban nada:
`FACTOR_GOLPE` 1.6 → **1.30**, `GOLPE_MINIMO` 32 → **50**, `SONIDO_MINIMO` 26 → **45**,
alfa de subida de la base 0.02 → **0.008**.

**Fallas encontradas (en orden de gravedad)**
1. 🔴 **Relé ACTIVO-ALTO, no activo-bajo.** `RELE_ON` estaba en LOW: el calefactor calentaba
   con el sistema en OFF y se apagaba con `/calentador_on`. Corregido a `RELE_ON = HIGH`.
   Verificado en hardware. El cableado estaba bien.
2. 🔴 **Un DS18B20 en cortocircuito** hizo hervir dos ESP32 hasta dejarlos sin arrancar.
   Ambas placas se recuperaron con un corte total de alimentación (latch-up, no daño
   permanente). Sensor reemplazado por el de repuesto.
3. ✅ **El programa se congelaba hasta 3 segundos seguidos** — RESUELTO. Culpables:
   `requestTemperatures()` bloqueaba 750 ms a 12 bits, y `getUpdates()` de Telegram otros
   ~3 s cada 2,5 s (el saludo TLS de cada conexión nueva). Era lo que hacía que el LCD y
   las luces fueran a tirones. Arreglado en dos pasos, cada uno medido con `/trace`:
   - **Sensor no bloqueante**: `setWaitForConversion(false)` + lectura en dos fases (se
     pide la conversión y el valor se recoge varias vueltas después). Eliminó los huecos
     de 750 ms.
   - **Telegram en el núcleo 0**: tarea propia con `xTaskCreatePinnedToCore` (8 KB de
     pila, `vTaskDelay` obligatorio para no despertar al watchdog), en el mismo núcleo
     donde el ESP32 maneja el WiFi. `loop()` queda libre en el núcleo 1. Como `bot` ahora
     pertenece a esa tarea, el aviso del cobertor pasa por un buzón en vez de enviarse
     desde el loop: dos núcleos por la misma conexión TLS serían un cuelgue seguro.

   | Medición (mismo método) | Antes | Después |
   |---|---|---|
   | Hueco máximo del loop | 3.301 ms | **192 ms** |
   | Hueco promedio | 856 ms | **68 ms** |
   | Huecos > 1,5 s | 21 de 70 | **0 de 588** |
   | Mediciones en ~60 s | 71 | **589** |
4. 🟠 **La base adaptativa se infla hasta cegar al sistema**: con música sostenida llegó a
   147 y dejó el umbral en 192, declarando "sin música" con música sonando. Bajar el alfa
   lo mitiga pero no lo resuelve — es estructural del filtro exponencial. **Pendiente**:
   comparar contra el promedio del último segundo (detección de ritmo por energía).
5. 🟡 **GPIO5 es pin de strapping** y lo usa el fin de carrera ABIERTO. **Pendiente**: mover
   ese fin de carrera de pin. *(Corrección del 2026-08-06, parte 2: la consecuencia anotada
   acá —"el ESP32 no vuelve a arrancar"— resultó ser INEXACTA. Según la documentación oficial
   de Espressif, GPIO5 sólo fija el "timing del esclavo SDIO", que este proyecto no usa; el
   modo de arranque lo deciden GPIO0 y GPIO2. La placa arrancaba igual. El pin se movió de
   todos modos, por los motivos reales que están en la entrada de esa sesión.)*
6. 🟡 **Los pines GPIO6–11 (SD0-SD3, CMD, CLK) son de la memoria flash.** En las placas de
   38 pines están expuestos en el header; un cable ahí y el ESP32 no arranca. Agregado al
   checklist de cableado.

**Documentación corregida**
- El relé ahora va **del lado del positivo** (`CONEXIONES.md`, `CABLEADO-PASO-A-PASO.md`):
  con el relé abierto el cartucho queda sin tensión, en vez de con +12V permanentes.
  Importante porque va sumergido.
- El negativo de los 12V **no va al riel GND** de la protoboard (corre la referencia del
  ADC y ensucia el micrófono).
- Micrófono 1 a 5V/VIN (estaba documentado a 3.3V); `PROYECTO.md` ya no menciona la FFT y
  suma GPIO35 y los comandos nuevos.

**Decisión abierta**: Mariano pide efecto de discoteca real, con las luces separadas por
graves / medios / agudos. Falta medir la forma de onda cruda del micrófono para saber si
el módulo entrega audio real o una envolvente rectificada — si es lo segundo, no hay
frecuencias que analizar. Los 8 LEDs se desconectaron: se reemplazan por la tira WS2812.

#### Atribución por modelo (sesión 2026-08-06)
- **Opus 5**: toda la sesión — instrumentación `/trace`, captura y análisis de los datos
  del sensor, recalibración, diagnóstico de las 6 fallas, corrección del relé y los docs.

### Sesión 2026-08-06 (parte 2) — Auditoría del cableado de motores, antes de conectarlos

Objetivo: dejar el sistema del cobertor listo para conectar y probar en el taller. Se auditó
`CABLEADO-PASO-A-PASO.md` completo contra la documentación oficial de Espressif, la del
L298N y las especificaciones de los motores.

**Reasignación de 4 pines (sin componentes nuevos ni funciones perdidas)**

Los pines del cobertor no podían quedar en pines que el ESP32 usa mientras arranca: del otro
lado hay motores. Se intercambiaron con dos colores de LED (que hoy están desconectados,
porque los reemplaza la tira WS2812):

| Señal | Antes | Ahora | Motivo |
|---|---|---|---|
| L298N ENB (motor B) | GPIO14 | **GPIO18** | GPIO14 emite un pulso al arrancar → habilita el puente H antes de que el programa ponga orden: tirón del motor en cada encendido |
| Fin de carrera ABIERTO | GPIO5 | **GPIO19** | GPIO5 es pin de arranque (strapping) y además pulsa al encender: la posición de la lona no debe influir en la configuración del chip, ni el pulso descargar contra un contacto a masa |
| LEDs azules | GPIO18 | **GPIO14** | Un LED que destella al arrancar es inofensivo |
| LEDs blancos | GPIO19 | **GPIO5** | Ídem (el blanco tiene tensión de encendido alta: no altera la lectura del pin al arrancar) |

**Dato corregido**: la bitácora afirmaba que con el fin de carrera en GPIO5 presionado "el
ESP32 no vuelve a arrancar". La documentación oficial de Espressif lo desmiente: GPIO5 sólo
elige el *timing del esclavo SDIO* (no usado acá); el modo de arranque lo deciden GPIO0 y
GPIO2, y GPIO12 es el verdaderamente peligroso (fija la tensión de la flash). El cambio se
hizo igual, pero por los motivos correctos.

**Dos bugs de concurrencia encontrados al auditar el código del cobertor** — nacieron cuando
Telegram se mudó al núcleo 0 y sólo se disparan con el cobertor en uso:
1. 🔴 `cobertorAbrir()` y `cobertorCerrar()` escribían el estado ANTES del reloj del
   movimiento. El loop, corriendo en el otro núcleo, podía ver el estado nuevo junto al reloj
   del movimiento anterior y cortar en el acto "por seguridad". Ahora el estado se escribe
   último: cuando el loop lo ve, ya está todo listo.
2. 🔴 `chatCobertor`, `avisoTexto` y `avisoChat` eran `String` compartidos entre los dos
   núcleos — dos núcleos tocando el heap del mismo String corrompen memoria y provocan
   reinicios inexplicables. Pasaron a buffers de tamaño fijo con un único escritor por
   variable. Las variables compartidas del cobertor quedaron `volatile`.

**Comandos nuevos `/motor_a` y `/motor_b`**: mueven UN motor durante 2 s ignorando los fines
de carrera, para verificar cableado y sentido de giro con los motores desacoplados, antes de
montar el mecanismo.

**Correcciones del documento de cableado** (además de los pines):
- Los fines de carrera se cableaban "cualquier pata": ahora se especifica **COM a GND y NO al
  pin** (con NC, el sistema arranca creyendo que está en el tope y no se mueve nunca), más un
  método para verificarlos con `/status` sin mover motores.
- Se agregó que el **jumper del regulador de 5V del L298N va PUESTO** (sólo se saca con más
  de 12V) — antes sólo se hablaba de los de ENA/ENB.
- Se prohibió explícitamente conectar el **borne +5V del L298N** al 5V/VIN del ESP32: con el
  USB puesto quedan dos fuentes enfrentadas.
- **Límite de corriente concreto** para la prueba (~1A en la fuente SLAVE) en vez de "la
  perilla a la mitad": los motores pueden llegar a 1,6A cada uno con el eje trabado.
- Protocolo de prueba completo, del orden correcto: fuente → sketch → un motor por vez →
  fines de carrera a dedo → movimiento completo → recién ahí acoplar el mecanismo.
- El paso de los 8 LEDs quedó marcado como **en pausa** (están desconectados) con los pines
  nuevos, por si hay que rearmarlos antes de que llegue la tira.
- **La alimentación del L298N no se entendía** (lo levantó Mariano con el módulo en la mano):
  el documento pedía 3 cables y la bornera tiene 3 tornillos, pero uno (`+5V`) va vacío y en
  `GND` entran **dos** cables. Se agregó el croquis de la bornera (`+12V | GND | +5V`, GND
  siempre al medio), cómo meter dos cables en un tornillo, un croquis de la placa entera y un
  mapa de las **tres alimentaciones independientes** (USB → ESP32 · MASTER 12V → calefactor ·
  SLAVE 8V → L298N), que es lo único que comparten: el GND entre ESP32 y L298N. También se
  explicó que las filas de la protoboard son un mismo punto eléctrico, que era la raíz de la
  confusión (el VIN de 5V alimenta tres cosas).

**Gate**: compilado con `arduino-cli` (core esp32 3.3.10) → **sin errores**, 83% de flash y
15% de RAM. El único warning es el conocido de LiquidCrystal I2C (declara arquitectura AVR).

**Prueba en hardware — los motores andan** (misma tarde, con el L298N cableado y la fuente
SLAVE a 8V): `/motor_a` y `/motor_b` mueven cada motor por separado. Verificado además por el
Monitor Serie leído desde la PC: WiFi conectado, `Bot de Telegram listo`, sensor en 20,5 °C y
el loop corriendo parejo. Los fines de carrera todavía no están conectados — con los pines al
aire el pull-up interno los deja en "no tocado", así que su cableado queda por verificar.

**Velocidad ajustable en vivo** (`/velocidad N`, en porcentaje, guardada en NVS): los motores
iban demasiado rápido para un cobertor. En vez de dejar el número fijo en el código, se hizo
configurable desde Telegram para calibrarla en el taller sin recompilar. El valor de fábrica
bajó de 180/255 (70%) a **115/255 (45%)** y el mínimo aceptado es 20%: por debajo, el motor
zumba y no vence su propio rozamiento.

**🔴 La tarea de Telegram se colgó en pleno taller** (mismo día, después de varias pruebas de
motores). Síntoma: el bot dejó de responder mientras **todo lo demás seguía perfecto**. El
Monitor Serie lo demostró: el loop imprimía la temperatura cada 2,7 s, **cero reinicios**
(`rst:` = 0), cero panics, cero watchdog, cero brownout. Al resetear la placa, Telegram
entregó de golpe los comandos encolados (tres `/motor_a` seguidos, que movieron el motor).

Causa: cada consulta a Telegram abre una conexión TLS nueva, y ese saludo puede quedarse
esperando indefinidamente si la red se pone rara — la del taller es `UA-Alumnos`, una red
institucional. `client.setTimeout(2000)` cubre las lecturas, **no** el establecimiento de la
conexión. La tarea del núcleo 0 queda bloqueada y el loop del núcleo 1 sigue como si nada.

Lo peor era que **el cuelgue es invisible desde afuera**: nada en el log decía que Telegram
había dejado de latir. Se agregó:
- **Latido de la tarea de Telegram** (`ultimoLatidoTelegram`), que la tarea actualiza en cada
  vuelta y el loop publica en el Monitor Serie: `WiFi: OK | Telegram late hace 0s`. Si ese
  número empieza a crecer sin parar, la tarea se colgó.
- **Reconexión de WiFi forzada** cada 20 s mientras esté caído, en vez de confiar sólo en el
  reintento automático del core.

**Pendiente**: conectar los fines de carrera y verificar que corten; definir el sentido de
giro de cada motor una vez montado el mecanismo. **Decisión para Mariano**: si el latido se
detiene (Telegram colgado), ¿el ESP32 debe reiniciarse solo para recuperar el bot? Recupera
el control remoto sin intervención, pero un reinicio deja el calentador en OFF por diseño.

#### Atribución por modelo (sesión 2026-08-06, parte 2)
- **Opus 5**: auditoría del cableado, investigación en documentación oficial, reasignación de
  pines, arreglo de los dos bugs de concurrencia, comandos de prueba y reescritura de los
  documentos.

### Sesión 2026-08-07 — Luces v5: la tira WS2812 y el análisis de espectro

Llegó la tira WS2812 (rollo de 5 m, 30 LED/m) y Mariano pidió reemplazar los 8 LEDs, dejando
todo listo para conectar y probar en una sola visita al taller. Pidió además reinvestigar el
sistema de luces entero —"siento que hay mucha basura"— y reescribirlo desde cero.

**El hallazgo que cambió el plan: el micrófono SÍ entrega audio real**

La bitácora del 2026-08-06 dejó abierta la pregunta de si el módulo entregaba audio o una
envolvente rectificada. La documentación del KY-037 la responde: el pin `AO` es la señal
**cruda del electret CMA-6542PF, sin amplificar**; el LM393 de la placa **es un comparador,
no un amplificador**, y sólo maneja la salida `DO`. El potenciómetro tampoco toca el `AO`.

O sea que por el `AO` viaja la onda completa, con toda su información de frecuencia. **La FFT
se puede hacer.** Se había eliminado en la v3 con razón —el micrófono estaba a 3,3 V y daba 17
counts de pico a pico— pero desde que pasó a 5 V da hasta 1040: treinta veces más señal. *La
conclusión de la v3 había quedado vencida por un cambio de hardware y nadie la revisó.*

**Cómo se hace en la vida real (investigado antes de diseñar)**

Se tomó como referencia [WLED sound-reactive](https://kno.wled.ge/advanced/audio-reactive/),
que es el estándar de facto: digitaliza a **10240 Hz**, reparte en bandas con escalado
logarítmico o raíz cuadrada, aplica **control automático de ganancia** y suaviza con caída
lenta. Los 10 kHz que ya usábamos coinciden con esa referencia. Para los golpes se usó el
algoritmo clásico de energía sonora (flipcode/GameDev): energía instantánea contra el promedio
del último segundo, con el umbral derivado de la dispersión del propio historial.

**Lo que se hizo**

- **Sistema de luces reescrito de cero.** Cadena nueva: captura de 256 muestras a 10 kHz →
  quitar DC + ventana de Hann → FFT → tres bandas (graves 78–234 Hz, medios 234–2000 Hz,
  agudos 2–5 kHz) → ganancia automática **por banda** → suavizado → detección de golpes.
  Unos 35 cuadros por segundo, mejor que los 28 de antes.
- **Cuatro efectos** con identidad propia: espectro (3 zonas), mezcla (color por bandas),
  cometa (con estela) y arcoíris. Sin música, respiración lenta. Corrección de gamma para que
  los degradados no salgan escalonados.
- **Limitador de corriente por software**: antes de cada cuadro se estima el consumo y, si se
  pasa del presupuesto, se atenúa todo el cuadro. Es lo que permite colgar la tira del propio
  ESP32 sin una tercera fuente. Se calcula sobre el color **post-gamma**, que es el que el LED
  muestra de verdad.
- **Nivel lógico resuelto sin comprar nada**: el WS2812B exige 0,7 × su alimentación, y el pin
  `VIN` no da 5,0 V sino ~4,7 V por el diodo Schottky de la placa. El umbral baja a 3,29 V y
  el 3,3 V del ESP32 alcanza. **Hay que medirlo en el taller**: si la placa diera 5,0 V
  clavados, el margen desaparece.
- **Comandos nuevos**: `/efecto`, `/leds`, `/brillo`, `/corriente`, `/orden`, `/luces_test`,
  `/espectro`, `/onda`. Todos guardados en NVS, para calibrar en el taller sin recompilar.

**Código eliminado (la "basura" que Mariano detectó)**

Mariano avisó que **sólo hay un micrófono conectado**, no dos. Todo el canal DO era código
para hardware inexistente: `MIC_DO_PIN`, `golpeDO`, conteo de flancos, `FLANCOS_DO_MAXIMO`,
`fuenteGolpe` con sus tres comandos `/sonido_*`. Se fue también la base adaptativa
exponencial, los 4 pines de LED y la sombra rotante. Se borró `PROTOCOLO-TALLER.md`, que
quedaba contradictorio (calibraba un potenciómetro de un módulo que no está conectado).

**Cinco hallazgos de la auditoría previa**

1. 🔴 **El ADC se leía desde los dos núcleos.** `/diag` hacía 500 `analogRead()` desde la tarea
   de Telegram (núcleo 0) mientras `medirSonido()` leía el mismo ADC1 en el loop (núcleo 1).
   Los periféricos del ESP32 necesitan un solo dueño. Ahora `/diag` **reporta** lo que midió el
   núcleo 1 en vez de medir por su cuenta.
2. 🔴 **Los comandos encendían luces desde el núcleo 0.** Inofensivo con `digitalWrite`, fatal
   con la tira: sería mandar datos por el RMT desde el núcleo del WiFi, pisando un envío en
   curso. Ahora los comandos sólo cambian el modo, como el README ya decía que hacían.
3. 🟠 **Variables compartidas entre núcleos sin `volatile`** (`modoLuces`, `modoCalentador`,
   `tempObjetivo`, `trazaSonidoActiva`): funcionaban por casualidad del optimizador.
4. 🟠 La base adaptativa que se cegaba con música sostenida (ya diagnosticada) — resuelta.
5. 🟡 **Un reinicio por caída de tensión era invisible.** Ahora el arranque informa el motivo
   (`esp_reset_reason`): brownout, watchdog o panic. Si la tira hace caer los 5 V, se ve.

**Decisión de alimentación**: la tira se cuelga del `VIN` del ESP32. **Se descubrió que ese
riel ya está bastante cargado** —ESP32 150–250 mA + LCD 30 mA + relé 75 mA + micrófono 5 mA =
260 a 460 mA, contra los 500 mA de un USB 2.0—, así que el presupuesto de fábrica quedó en
**120 mA** y el limitador es el que lo garantiza. Las dos fuentes del proyecto no se tocan.

**Gate**: compilado con `arduino-cli` (core esp32 3.3.10) → **sin errores**, 86 % de flash y
16 % de RAM. El único warning es el conocido de LiquidCrystal I2C.

**Plan completo**: `docs/PLAN-LUCES-V5-ESPECTRO.md`. **Guía de conexión y prueba**:
`PiletaInteligente/CABLEADO-PASO-A-PASO.md` (pasos 4 y 5 + la sección "PRUEBA DE LAS LUCES").

**Pendiente para el taller**: conectar la tira y probar. Nada de esto se verificó en hardware.

#### Atribución por modelo (sesión 2026-08-07)
- **Opus 5**: investigación (KY-037, WS2812B, WLED, Adafruit NeoPixel, algoritmos de beat
  detection), auditoría del código, plan v5, reescritura completa del sistema de luces y
  actualización de toda la documentación.

### Sesión 2026-08-13 — La tira en el taller: todo verificado en hardware

Primera visita al taller con la tira. Se conectó, se probó cada pieza del sistema de luces y se
recalibró el análisis de sonido con datos medidos, no supuestos.

**La tira anduvo a la primera**
- **21 píxeles, no 15**: Mariano midió el perímetro real de la pileta con el recipiente en la
  mano y hacían falta 70 cm. 21 divide exacto en tres zonas de 7 para el efecto ESPECTRO.
- Los cuatro colores de `/luces_test` salieron **en el orden correcto sin tocar `/orden`**: el
  chip es GRB, como asumía el código, y el extremo del conector era el `DIN`.
- **Nivel lógico**: el `VIN` midió **4,4 V**, más bajo que los ~4,7 V que estimaba el plan. Es
  *mejor*: el umbral del WS2812B (0,7 × alimentación) queda en 3,08 V y el ESP32 pasa con
  0,22 V de margen, contra los 0,01 V que habría a 4,7 V. La documentación quedó corregida —
  y con la advertencia de NO "mejorar" el cable USB, porque subiría la tensión y achicaría el
  margen.

**Tres verificaciones que cierran riesgos abiertos del plan v5**

1. **La tira NO contamina el micrófono** (riesgo 3 del plan, el que más preocupaba). Se midió
   alternando seis veces con `/diag`: promedio **47** con las luces apagadas y **38** con las
   luces encendidas — el ruido ambiente de las conversaciones pesa más que la tira. Repetido
   con `/brillo 100`: sin cambios. Descartado con evidencia.
2. **El limitador de corriente funciona**, verificado sin querer: con brillo 70 el punto de
   reposo del micrófono quedó en 176 y con brillo 100 también en 176-177. Idéntico, porque el
   limitador ya topaba en 120 mA en los dos casos.
3. **La FFT es exacta**, probada con tonos puros generados desde el celular:

   | Tono | Detectó | Banda |
   |---|---|---|
   | 100 Hz | 78 / 117 Hz (los dos bins que rodean a 100) | graves |
   | 1000 Hz | 1012 Hz | medios |
   | 3000 Hz | 2998 Hz | agudos |

   Era la apuesta grande del rediseño de la v5 y quedó confirmada de punta a punta.

**El defecto que encontraron esas mediciones: el AGC amplificaba el silencio**

Con un tono **puro** de 1000 Hz sonando, las tres bandas marcaban **72 / 100 / 100**. Las dos
bandas sin absolutamente nada de contenido mostraban el máximo.

La culpa no era del AGC sino de cómo se lo usaba: al normalizar contra el máximo reciente, una
banda sin señal ve caer ese máximo hasta el piso y su propio ruido pasa a valer 100 %. **Acotar
el máximo no alcanza** —con un piso de 12 y un ruido de 9 la banda seguiría mostrando 75 %—:
hay que **restar** el ruido antes de normalizar. Es el *squelch* de WLED.

Se agregó una **puerta de ruido por banda**: `util = max(0, crudo − piso)`, y el AGC pasa a
trabajar sobre `util`. La detección de golpes también usa `util`, porque el piso se suma por
igual al golpe y al promedio y achata el contraste entre los dos.

**Y el umbral de música estaba por debajo del ruido de fondo**

`SONIDO_MINIMO` valía 45, calibrado el 2026-08-06 cuando el silencio daba 9-36. El ruido real
del taller (conversaciones) resultó ser **32-56**, así que el sistema declaraba "música
detectada" sin que sonara nada. Con música medida entre **180 y 591**, el umbral nuevo es **90**:
casi el doble del ruido máximo y la mitad de la música más floja.

**Cambios de código**

| Qué | Antes | Ahora |
|---|---|---|
| `SONIDO_MINIMO` | 45 | **90** |
| Puerta de ruido por banda | no existía | **`pisoRuidoBanda = 12`**, ajustable con `/piso` y guardada en NVS |
| Detección de golpes | sobre el valor crudo | sobre el **útil** (mejor contraste) |
| `/diag` | una foto de 25,6 ms | + **rango de los últimos ~10 s** y aviso si el ruido supera el umbral |
| `/espectro` | CRUDO · TOPE · NIVEL | + columna **UTIL** y el piso activo |

> El `/diag` de una sola ventana costó **quince mediciones** para estimar un piso de ruido.
> Ahora es un comando. Es la clase de detalle que sólo aparece usando la herramienta en serio.

**Gate**: compilado y **cargado al ESP32** con `arduino-cli` (core esp32 3.3.10) → sin errores,
86 % de flash y 16 % de RAM, `Hash of data verified`. El único warning es el conocido de
LiquidCrystal I2C.

**Verificado después de recargar el firmware** — los dos defectos, cerrados con evidencia:

| Prueba | Antes | Después |
|---|---|---|
| `/audio` en silencio | Graves 78 · Medios 88 · Agudos 80, "música detectada" | **2 / 0 / 0, "silencio"** ✅ |
| Tono puro de 1000 Hz | 72 / **100** / 100 | **4 / 41 / 0**, dominante 1013 Hz ✅ |
| Tono puro de 3000 Hz | — | **0 / 0 / 27**, dominante 2996 Hz ✅ |
| `/espectro` en silencio | — | CRUDO 7,5 / 7,5 / 6,8 → **UTIL 0 / 0 / 0** ✅ |

**Y los cuatro efectos funcionan con música**: las tres zonas se mueven cada una por su lado y
el destello del golpe se ve. El sistema de luces queda **terminado y verificado en hardware**.

**Un hallazgo para tener en cuenta al instalar**: con la música saliendo del parlante de un
celular, el crudo de la banda de graves llega a **11,8** — justo por debajo del piso de 12, con
lo cual la banda se apagaba entera. Es la fuente, no el sistema: un parlante de 10-15 mm tiene
su resonancia cerca de 1 kHz y por debajo casi no emite (se ve en los volúmenes medidos: 194
con el tono de 3 kHz, 124 con el de 1 kHz, 39-60 con el de 100 Hz). Con esa fuente conviene
**`/piso 10`**, que deja el silencio igual en cero (el ruido crudo es 7,5) y le devuelve señal
a los graves. Con un parlante decente, 12 está bien.

**Calibración final con música real** (*Queen — "Another One Bites the Dust"*, elegida porque su
riff de bajo está en Mi (82 Hz) con armónicos fuertes en 165 y 247 Hz, que un parlante chico sí
reproduce). Con `/piso 10`, cuatro lecturas seguidas de `/espectro`:

| Momento | Graves ÚTIL | Medios ÚTIL | Agudos ÚTIL | Dominante |
|---|---|---|---|---|
| mezcla | 4,2 | 3,6 | 0,0 | 857 Hz |
| hi-hats | 6,9 | 4,5 | 9,5 | 4362 Hz |
| **riff de bajo solo** | **6,9** | **0,0** | **0,0** | **195 Hz** |
| voz y palmas | 1,1 | 12,6 | 10,7 | 2454 Hz |

Las tres bandas se mueven de forma **independiente y completa**: en el momento del riff las
otras dos caen a cero absoluto, y en el de la voz pasa lo contrario. La dominante de 195 Hz cae
en el bin 5, dentro de la banda de graves: captó el bajo con sus armónicos. La detección de
golpes trabaja con promedio 2,7-5,5 y umbral 3,1-6,9, y los picos de graves lo cruzan.

**`/piso 10` quedó guardado en el ESP32.** El valor de fábrica sigue en 12 a propósito: en un
lugar desconocido conviene el conservador, y bajarlo cuesta un comando.

**Pendiente de esta sesión**: nada del sistema de luces. Queda lo del cobertor (fines de
carrera y mecanismo) y la decisión de la histéresis — todo en `docs/PENDIENTES.md`.

#### Atribución por modelo (sesión 2026-08-13)
- **Opus 5**: guía de conexión en vivo, diagnóstico de las mediciones, investigación de
  consumo y nivel lógico (Adafruit, PJRC, datasheet WS2812B, USB-IF), puerta de ruido,
  recalibración, carga del firmware y actualización de la documentación.

#### Atribución por modelo (sesión 2026-07-23)
- **Opus 4.8**: /diag, pico a pico + DC removal, luces binarias, efecto en negativo (commits
  hasta `7477557`)
- **Fable 5**: investigación del sensor, diagnóstico con mediciones, plan LEGO adaptativo,
  auditoría del diff, docs
- **Sonnet 5** (subagente ejecutor): implementación del plan adaptativo (commit `b785ddc`)
