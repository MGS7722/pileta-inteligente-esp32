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
- [ ] Revisar la histéresis: 5 °C es mucha oscilación para una pileta (¿bajarla a 2 °C?)

## Sistema 2 — Luces al ritmo de la música

- [x] Sensor de sonido leído por el ESP32 (pin GPIO34, módulo alimentado a 5V)
- [x] Detección de ritmo por volumen pico a pico con base adaptativa (la FFT se eliminó
      en la v3: con esta señal no aportaba información confiable)
- [x] Sensor validado con datos reales: rango dinámico de más de 30× (2026-08-06)
- [x] 4 LEDs con efecto disco según la música
- [x] Control por Telegram (auto / ON / OFF)
- [x] Verificado funcionando (archivo "posta" de los compañeros)

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

#### Atribución por modelo (sesión 2026-07-23)
- **Opus 4.8**: /diag, pico a pico + DC removal, luces binarias, efecto en negativo (commits
  hasta `7477557`)
- **Fable 5**: investigación del sensor, diagnóstico con mediciones, plan LEGO adaptativo,
  auditoría del diff, docs
- **Sonnet 5** (subagente ejecutor): implementación del plan adaptativo (commit `b785ddc`)
