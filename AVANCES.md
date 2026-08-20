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
- [x] **Mecanismo redefinido (2026-08-13)**: lazo de hilo entre los dos ejes. Los DOS motores
      giran juntos hacia el mismo lado y el recorrido se mide **por tiempo**, no por sensor
- [x] **Tiempos por Telegram** (`/tiempo_abrir`, `/tiempo_cerrar`), uno para cada movimiento,
      guardados en NVS
- [x] **Sentido de cada motor invertible desde Telegram** (`/sentido_a`, `/sentido_b`): ya no
      hace falta desatornillar cables del L298N para corregir un giro
- [x] **PWM a 8 kHz + patada de arranque al 100 %**: sin eso el motor zumbaba y no arrancaba
- [x] **Freno activo al terminar** (*fast motor stop* del L298N) en vez de dejarlo por inercia
- [x] Integrado a Telegram: /cobertor_abrir, /cobertor_cerrar, /cobertor_parar (+ aviso al terminar)
- [x] Pines asignados y documentados en CONEXIONES.md
- [x] Alimentación resuelta: fuente regulable del laboratorio a ~8V (sin LM2596 ni resistencias extra)
- [x] Cableado auditado contra la documentación oficial y pines reasignados a los "tranquilos"
      (ENB → GPIO18, fin de carrera abierto → GPIO19) — 2026-08-06
- [x] Comandos `/motor_a` y `/motor_b` para probar un motor solo, sin fines de carrera
- [x] **Los dos motores giran** — probado en hardware con el L298N y la fuente a 8V (2026-08-06)
- [x] Velocidad ajustable en vivo desde Telegram con `/velocidad` (%, guardada en NVS)
- [x] **Los dos motores moviéndose JUNTOS, verificado en hardware** (2026-08-13): movimiento de
      10 s medido en **10008 ms**, prueba de motor de 2 s medida en **2019 ms**
- [x] **Soldadura del motor B reparada** (2026-08-13): iba y venía por un contacto flojo, no por
      el firmware
- [ ] Montar el mecanismo con la lona y anotar los valores definitivos: `/velocidad`,
      `/tiempo_abrir` y `/tiempo_cerrar`
- [ ] Conectar los 2 fines de carrera (ahora sólo informan la posición en `/status`; **no**
      cortan el movimiento)

## Bot de Telegram

- [x] Bot creado con BotFather y token funcionando (@ControlESP32Pileta_bot)
- [x] Librerías: UniversalTelegramBot 1.3.0 + ArduinoJson **6.21.5** (la 7 no compila) + core ESP32 3.3.10
- [x] Comandos integrados para calentador, luces y cobertor
- [x] Compila OK en la máquina de Mariano (solo warnings de librerías, inofensivos)
- [x] **El bucle de reenvío de 8 s, resuelto** (2026-08-20): el buffer de 1500 bytes de la
      librería truncaba la confirmación de Telegram y hacía reenviar el mismo mensaje. Buffer a
      4096 y `/help` partido en dos mensajes de 907 y 486 bytes
- [x] **La cola vieja ya no se ejecuta al encender** (2026-08-20): `descartarMensajesViejos()`
- [x] **Memoria libre y mínimo histórico visibles en `/status`**, y tiempos de consulta y
      respuesta por el Monitor Serie
- [ ] Cargar la v5.4 al ESP32 y verificar en vivo que `/help` no se duplica
- [ ] Long polling para bajar la latencia de fondo (ver `docs/PENDIENTES.md` #7c)

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

---

### Sesión 2026-08-13 (parte 2) — El cobertor cambia de mecanismo, y cuatro fallas de raíz

Sesión larga de taller, con el ESP32 conectado por USB y recargando firmware en vivo. Se
rediseñó el cobertor y aparecieron cuatro problemas distintos, cada uno con su causa propia.
Vale la pena leerlos por separado porque los síntomas se parecían entre sí y llevaron un rato
largo de diagnóstico.

**El cambio de fondo: el cobertor pasó a ser un lazo de hilo, movido por tiempo**

El diseño anterior era "un motor tira y el otro queda suelto": el rodillo enrollaba la lona de
un lado mientras el otro soltaba cable, y el recorrido terminaba al tocar un fin de carrera. En
el taller Mariano ató un hilo entre los dos ejes, que es un mecanismo distinto: un lazo cerrado
donde **los dos motores tienen que empujar juntos hacia el mismo lado**, y no hay topes físicos
que marquen el final del recorrido.

Decisiones tomadas (Mariano, en vivo):
- Los dos motores giran **juntos**, misma dirección, misma duración.
- El recorrido se corta **por tiempo**, no por sensor. Los fines de carrera siguen leyéndose e
  informan la posición en `/status`, pero **ya no cortan el movimiento**.
- **Abrir y cerrar llevan su propio tiempo** (`/tiempo_abrir`, `/tiempo_cerrar`), porque cerrar
  suele costar más: la lona pesa y el hilo roza.
- El sentido de cada motor se invierte **desde Telegram** (`/sentido_a`, `/sentido_b`), no
  desatornillando cables. "Que los dos giren para el mismo lado" es una condición física: según
  cómo queden montados, puede exigir sentidos eléctricos opuestos.

**Falla 1 — El motor zumbaba y no arrancaba**

Con `/cobertor_abrir` se movía un solo motor; el otro hacía ruido y quedaba quieto. Dos causas
sumadas, las dos verificadas contra el código fuente del core ESP32 3.3.10:

1. **El PWM salía a 1 kHz.** Es el valor de fábrica de `analogWrite` (`esp32-hal-ledc.c`:
   `analog_frequency = 1000`), justo en la zona donde el oído es más sensible. Un motor que
   recibe corriente pero no llega a vencer su rozamiento vibra a esa frecuencia: **el "ruido"
   que se escuchaba era literalmente el PWM**. Subido a **8 kHz**, que además es cómodo para el
   L298N, de transistores Darlington y lento para conmutar.
2. **No había pulso de arranque.** Un motor con reductora necesita mucho más par para *empezar*
   a moverse que para seguir girando: al 45 % arrancaba en vacío pero se frenaba con cualquier
   carga. Se agregó una **patada de arranque al 100 % durante 300 ms**, y recién después baja a
   la velocidad de régimen. La baja el loop, no un `delay()`, para no bloquear la tarea de
   Telegram.

**Falla 2 — "El movimiento se corta antes de tiempo" (no se cortaba)**

Con `/tiempo_abrir 10` el cobertor parecía moverse dos segundos. Se instrumentó el Monitor
Serie para informar la duración **medida** junto a la pedida, y el resultado fue terminante:

```
>>> Cobertor: ABIERTO — duro 10008 ms de los 10000 ms pedidos
```

Ocho milésimas de error. **El programa nunca tuvo el bug**: contaba los 10 segundos completos y
el motor se frenaba solo al bajar de la patada al 45 %, porque el par no alcanzaba contra la
carga del hilo. Con `/velocidad 100` el recorrido salió entero. La lección quedó en el código:
esa línea del Serie se conservó justamente para distinguir "lo cortó el programa" de "el motor
se paró solo".

**Falla 3 — El bot de Telegram se quedaba mudo**

El síntoma era "el bot está lento". El latido de la tarea mostró otra cosa: huecos de **60 y
117 segundos** con el WiFi conectado y el resto del programa funcionando perfecto. La causa
apareció en el core:

```c
// NetworkClientSecure.cpp
sslclient->handshake_timeout = 120000;   // DOS MINUTOS
```

Cuando el saludo TLS con Telegram se traba, la tarea espera los dos minutos completos. Y la
protección que el código creía tener **no existía**: `client.setTimeout(2000)` no hace nada para
esto, porque `NetworkClientSecure` no redefine ese método y termina en `Stream::setTimeout()`,
que sólo gobierna las lecturas. La API correcta es **`setHandshakeTimeout()`**, y su parámetro
va **en segundos** (el core lo multiplica por 1000). Puesto en **5 segundos**.

Medido después del cambio: **latido máximo de 4 segundos**, contra los 117 de antes.

**Falla 4 — Una soldadura floja en el motor B**

El motor B iba y venía: andaba, se trababa, dejaba de responder. Se descartó el software
—el Monitor Serie mostraba las órdenes llegando y ejecutándose— y resultó ser **un problema de
soldadura en el motor**. Vale anotarlo porque el síntoma imitaba a las fallas 1 y 2 y costó
separarlo de ellas.

**Reescritura del módulo de motores**

Con tantos síntomas parecidos, el código de motores se rehízo de cero. El problema de fondo era
**duplicación**: dos juegos de funciones casi idénticos (uno por motor) y tres arranques de
movimiento separados que repetían las mismas líneas. Toda diferencia entre esas copias era un
error esperando — de hecho, que `probarMotor()` no tocara `duracionMovimientoMs` permitía que
una prueba de 2 segundos usara los 10 de una apertura.

Ahora:
- Los motores son una **estructura con sus pines adentro** (`motorA`, `motorB`).
- **Tres funciones** son la única forma de tocar un motor, y las comparten los dos:
  `motorMover()`, `motorFrenar()`, `motorSoltar()`.
- **Un solo arranque** (`iniciarMovimiento()`) para abrir, cerrar y probar.
- **Un solo corte por tiempo** en `actualizarCobertor()`: el estado decide *qué* hacer al
  terminar, nunca *cuándo*.

**Freno activo en vez de rueda libre**

Al terminar un movimiento el código soltaba los motores (habilitación en cero), y el eje seguía
girando por inercia con el carrete de hilo encima. Ahora frena en seco usando el propio L298N
—ambas entradas en bajo con la habilitación **alta**, lo que la tabla del integrado llama *fast
motor stop*—. Como beneficio extra, el cobertor queda quieto en lugar de poder correrse solo por
el peso de la lona. Al encender, en cambio, los motores quedan sueltos, para poder mover el
mecanismo a mano.

**Comandos nuevos**

| Comando | Qué hace |
|---|---|
| `/tiempo_abrir N` · `/tiempo_cerrar N` | Duración de cada movimiento, 0,1 a 60 s, en NVS. Admite decimales (`4.5`), con coma o punto |
| `/cobertor_sentido` | Invierte el cobertor. **Sólo** abrir/cerrar |
| `/sentido_a` · `/sentido_b` | Invierte esa PRUEBA. **Sólo** `/motor_a` y `/motor_b` |
| `/motor_a N` · `/motor_b N` | Prueba de un motor N segundos (2 por omisión, **no** se guarda) |

**Dos sistemas de sentido que no se pisan.** El primer intento fue un flag por motor, y en la
mano resultó un laberinto: cuatro combinaciones, dos de ellas inservibles, y tocar una sola
cambiaba dos cosas a la vez —si los motores se acompañan **y** cuál dirección es abrir—. Mariano
lo probó un buen rato sin poder llegar al resultado, que es la señal de que el problema no era
el cálculo sino la interfaz.

Quedó así:
- **El cobertor** aplica **una sola polaridad a los dos motores**: se calcula una vez y se pasa a
  los dos `motorMover()`. No es una convención, es estructural — no existe estado del programa en
  el que puedan tirar uno contra el otro. `/cobertor_sentido` sólo elige cuál dirección es abrir.
- **Las pruebas** tienen su propio sentido por motor, y no tocan nada del cobertor.

**Consecuencia asumida**: como el cobertor da la misma polaridad a los dos, si los motores están
montados espejados van a girar en sentidos visualmente opuestos, y eso **ya no se corrige por
software**. Se invierten los dos cables de un motor en el L298N. Es el precio de que los sistemas
no se contaminen, y se eligió a conciencia: una perilla que hace una cosa y un cable bien puesto,
antes que cuatro combinaciones que se pisan.

Los mensajes de `/cobertor_sentido`, `/motor_*` y `/status` informan **hacia qué lado gira cada
motor**. Es la referencia del driver, no el eje real: si en el eje se ve al revés se lee dado
vuelta, porque lo que importa es que el hilo avance, no cómo se llame cada sentido.

**Gate**: compilado y cargado con `arduino-cli` (core esp32 3.3.10) → sin errores, **87 % de
flash y 16 % de RAM**, `Hash of data verified`. Los únicos warnings son los conocidos de
LiquidCrystal I2C y OneWire.

**Verificado en hardware**

| Prueba | Resultado |
|---|---|
| Movimiento de 10 s | **10008 ms medidos** |
| Prueba de motor de 2 s | **2019 ms medidos** |
| Latido de Telegram tras el arreglo | **máximo 4 s** (antes 117 s) |
| Los dos motores juntos | funcionan |

**Pendiente de esta sesión**: anotar los valores definitivos de calibración (velocidad y los dos
tiempos) una vez montado el mecanismo con la lona.

#### Atribución por modelo (sesión 2026-08-13, parte 2)
- **Opus 5**: rediseño del cobertor por tiempo, investigación del core ESP32 (PWM, handshake
  TLS), diagnóstico con el Monitor Serie, reescritura del módulo de motores, carga del firmware
  y documentación.

### Sesión 2026-08-20 — El bot lento: el buffer de 1500 bytes de la librería de Telegram

Mariano terminó de conectar todo en la pileta y reportó dos cosas: el bot tarda muchísimo en
responder, y **a veces manda la misma respuesta dos veces aunque el comando se ejecute una sola**.
Su pregunta fue si se le podía "limpiar el caché o la memoria" sin perder los comandos.

**La respuesta corta a esa pregunta**: el ESP32 no acumula caché. Lo que sí se acumula es la cola
de mensajes del servidor de Telegram, que guarda hasta 24 horas lo que el bot no confirmó. Y la
configuración nunca estuvo en riesgo: todo lo de `/velocidad`, `/tiempo_*`, `/brillo`, `/leds`,
`/corriente`, `/piso`, `/temperatura`, `/efecto` y los sentidos vive en NVS y sobrevive a
reinicios y a recargas de firmware.

**El hallazgo, que estaba en la librería y no en nuestro código**

`UniversalTelegramBot 1.3.0` trae un buffer de 1500 bytes (`maxMessageLength`) y lo usa para las
dos puntas: armar el mensaje que sale y leer la confirmación que vuelve. El detalle que rompe
todo es que **Telegram, al confirmar un envío, devuelve el texto entero del mensaje** más unos
400 bytes con los datos del chat.

Se midieron los mensajes del sistema con un script sobre el propio `.ino`: la ayuda de `/help`
son **1363 bytes de texto**, así que su confirmación rondaba los 1800. De ahí en adelante:

1. `readHTTPAnswer()` corta el cuerpo en 1500 (`if (ch_count < maxMessageLength)`).
2. El JSON queda incompleto y `checkForOkResponse()` no puede darlo por bueno.
3. `sendPostMessage()` entra en `while (millis() < sttime + 8000)` y **reenvía el mismo mensaje
   una y otra vez durante ocho segundos**, con la tarea de Telegram sin atender nada más.

Eso explica **exactamente** el síntoma que reportó Mariano: el mensaje llega duplicado pero el
comando se ejecuta una sola vez, porque la duplicación pasa en la respuesta que sale, no en la
orden que entra.

**La otra mitad: la cola vieja**

`last_message_received` arranca en 0, así que al encender el ESP32 pedía los updates desde el
principio: ejecutaba **los comandos viejos primero**, del más antiguo al más nuevo y de a uno por
vuelta de 2,5 s. Además de parecer trabado, es peligroso — podía mover el cobertor por una orden
de hacía horas. Ahora, al conectar, `descartarMensajesViejos()` pide el último update con
`getUpdates(-1)` y lo descarta: Telegram da por confirmada toda la cola y el bot arranca
escuchando sólo lo nuevo.

**Qué se cambió**

| Cambio | Por qué |
|---|---|
| `bot.maxMessageLength = 4096` | Que la confirmación entre entera y no se dispare el bucle de 8 s |
| `/help` partido en dos mensajes (907 y 486 bytes) | Ningún mensaje del sistema pasa ya de 907 bytes |
| `descartarMensajesViejos()` al conectar | No ejecutar la cola vieja al encender |
| Memoria libre y mínimo histórico en `/status` | El programa no medía la memoria en ningún lado |
| Tiempos de consulta y de respuesta por el Monitor Serie | Separar "tarda en preguntar" de "tarda en contestar" |

**Lo que NO arregla esto** (y quedó en `docs/PENDIENTES.md` #7c, con la cuenta hecha): la
latencia de fondo. La librería cierra la conexión cuando no hay mensajes
(`UniversalTelegramBot.cpp:433`), así que **cada vuelta paga un saludo TLS completo**, y encima
consulta cada 2,5 s pidiendo `limit=1`, un mensaje por vez. La cuenta del peor caso antes de esta
sesión era ~15 s por comando (2,5 de espera + 3 de saludo + 1,5 de lectura + 8 del bucle); sin el
bucle queda en ~7 s. La solución de fondo es **long polling**, que necesita prueba en hardware:
durante la espera la tarea no cede CPU y hay que verificar el watchdog del núcleo 0.

**Gate**: compilado con `arduino-cli` (core esp32 3.3.10) → sin errores, **87 % de flash y 16 %
de RAM**, igual que antes. El único warning es el conocido de LiquidCrystal I2C.

**Verificación en hardware el mismo día — y el resultado fue NEGATIVO**

Se cargó la v5.4 y se capturó el Monitor Serie mientras Mariano mandaba `/help` desde el celular.
El bot **siguió respondiendo tres veces** el mismo mensaje, y el log dio los números exactos:

```
----- Telegram -----
De: Mariano
Msg: /help
Tiempos: consulta 4421 ms | respuesta 16805 ms
```

Dos cosas quedaron claras de una:

1. **El comando llegó UNA sola vez** (un solo bloque `----- Telegram -----`), así que la
   duplicación es de salida, como se había deducido. Eso se confirmó.
2. **16805 ms de respuesta son 8000 + 8000**: los DOS mensajes de la ayuda agotaron su bucle de
   reintentos. Subir el buffer a 4096 **no evitó el bucle**.

Y el dato que tira abajo la hipótesis del tamaño: **la parte 2 de la ayuda son 486 bytes y también
falló**. Un mensaje de 486 bytes no puede truncarse contra un buffer de 4096. El truncado era real
y valía arreglarlo, pero **no era la causa dominante**.

**La causa que sí explica todo** está más abajo, en cómo la librería lee la respuesta HTTP:

```cpp
while (millis() - now < longPoll * 1000 + waitForResponse) {
    while (client->available()) { ...leer... }
    if (responseReceived) break;      // <-- corta apenas leyó ALGO
}
```

`responseReceived` se pone en verdadero con el primer carácter leído, así que la función corta
apenas drena el primer bloque disponible. Si los encabezados HTTP llegan en un segmento TLS y el
cuerpo JSON en el siguiente —lo normal en cuanto la respuesta crece—, se queda con los
encabezados y un cuerpo vacío o partido. `checkForOkResponse()` no puede confirmar nada y
`sendPostMessage()` entra igual en sus 8 segundos de reenvíos. Es una **carrera**, y por eso el
síntoma era intermitente ("a veces") y empeora con los mensajes largos.

No hay salida por la API de la librería: `sendSimpleMessage()` tiene exactamente el mismo bucle de
8 segundos. Acortar los mensajes tampoco sirve, porque ya falla con 486 bytes.

**Lo propuesto**: que el ENVÍO deje de pasar por la librería. Una función propia que arma el POST
a `api.telegram.org`, pide `Connection: close`, lee hasta que el servidor cierra —sin adivinar
dónde termina el cuerpo— y **no reintenta nunca**: un fallo se registra en el Monitor Serie y se
sigue, así un mensaje no puede duplicarse por diseño. `getUpdates()` seguiría siendo de la
librería, que para lo que entra funciona bien. Queda en `docs/PENDIENTES.md` #14, esperando el OK.

**Lo que SÍ quedó funcionando de la v5.4**: la cola vieja ya no se ejecuta al encender, `/status`
informa la memoria, y la instrumentación nueva — que es la que permitió medir todo esto en cinco
minutos en vez de a ciegas.

**Y un hallazgo nuevo de la instrumentación**: en la red de la pileta (`UA-Alumnos`), cada consulta
paga entre **3,0 y 6,7 segundos de saludo TLS**, medido decenas de veces. Es bastante peor que los
~3 s del taller y sube muchísimo la prioridad del long polling (`docs/PENDIENTES.md` #7c): con la
conexión abierta ese costo se pagaría una vez cada 25 s en lugar de en cada vuelta.

**Pendiente de esta sesión**: implementar el envío propio (#14) y volver a medir.

#### Atribución por modelo (sesión 2026-08-20)
- **Opus 5**: auditoría del camino completo de Telegram (nuestro código + librería 1.3.0 +
  core), medición de los mensajes, los cuatro cambios del firmware y la documentación.

### Sesión 2026-08-20 (parte 2) — El envío propio, y la pantalla que perdió el paso

**El envío de Telegram dejó de pasar por la librería**

Con la causa ya identificada (el lector de `UniversalTelegramBot` corta apenas leyó el primer
bloque disponible), se evaluaron los caminos antes de escribir una línea:

| Camino | Por qué no / por qué sí |
|---|---|
| Actualizar la librería | Se revisó el código de `master` en GitHub: **tiene el mismo bug**, mismo `break` temprano y mismo bucle de 8 s. Hay un PR abierto sin mergear |
| Acortar los mensajes | Descartado por la medición: falló con un mensaje de **486 bytes** |
| `sendSimpleMessage()` | Se leyó su implementación: trae el mismo `while (millis() - sttime < 8000ul)` |
| Cambiar a `AsyncTelegram2` | Buena librería —lee con `Content-Length` y mantiene la conexión viva—, pero es una dependencia nueva, obliga a migrar los 56 puntos de envío **igual que la opción propia**, su camino de envío lee "lo que haya disponible" con la misma debilidad, y su polling por defecto es `timeout:0`. No compra lo suficiente para justificar migrar todo el bot a otra API |
| **Envío propio** ← elegido | Treinta líneas, sin dependencias nuevas, y control total de lo único que importa acá: cuándo termina la respuesta y qué se hace si falla |

Antes de escribirlo se verificó **contra el servidor real** cómo contesta la API, en vez de
suponerlo (con un token falso, para no exponer el verdadero):

```
HTTP/1.1 429 Too Many Requests
Content-Length: 109
Connection: keep-alive
```

`Content-Length` siempre, nunca `chunked`, y `keep-alive` aceptado. Con eso, leer bien es
determinista. La doc oficial de Telegram confirmó además lo otro que usa el programa: el offset
negativo "olvida todas las actualizaciones previas" —que es lo que hace el descarte de cola al
arrancar— y que los updates se guardan 24 horas.

Tres decisiones de diseño:

1. **No hay reintento, ninguno.** Un fallo queda en el Monitor Serie y se sigue. Un mensaje no
   puede duplicarse porque no existe el código que lo mandaría dos veces.
2. **Ante cualquier problema se corta la conexión.** Media respuesta sin leer envenenaría el
   pedido siguiente.
3. **Si la librería dejó restos en el socket** —su bug—, el envío los detecta y rearma la
   conexión antes de mandar.

Los 56 puntos de envío se migraron con un transformador que cuenta paréntesis y respeta las
comillas, no con una expresión regular: las llamadas son multilínea y llevan concatenaciones
adentro. Después se realinearon las líneas de continuación, porque `telegramEnviar(` es un
carácter más corto que `bot.sendMessage(`.

**La pantalla: perdió el paso y no se podía recuperar sola**

En el medio de la sesión el LCD empezó a mostrar basura. La foto fue el diagnóstico: **símbolos
nítidos y perfectamente dibujados** de la zona alta de la tabla, con el contraste y la
retroiluminación impecables. Eso descarta contraste, alimentación y cable suelto, y es la firma de
la **desincronización de nibbles**: el HD44780 trabaja en modo de 4 bits, con cada carácter partido
en dos mitades, y si un pico eléctrico le hace perder una sola mitad, todo lo que sigue se arma con
la mitad de un carácter y la mitad del siguiente.

Se confirmó en el acto: el Monitor Serie mostraba los mismos datos **bien** (`Temp: 21.9 C |
Calentador: OFF (OFF) | ... | WiFi: OK`, sin reinicios ni brownout), así que el programa estaba
sano y era el display el que había perdido el paso. Y cuando Mariano lo desconectó y lo volvió a
conectar, volvió a mostrar todo correcto — la prueba final: sólo le faltaba reinicializarse.

**Qué lo causó**: no se puede probar después del hecho, y conviene decirlo así en vez de inventar
una causa. Lo que sí se sabe es que ocurrió justo después de cablear el relé y el circuito de 12 V
al lado del bus I2C, y que Mariano estaba con las manos en esos cables. Los candidatos, en orden:
manipular los cables del I2C mientras se cableaba (un corte de contacto de milisegundos hace
exactamente esto), el pico de conmutación del relé, el riel de 5 V cediendo con la tira ya
conectada, y el acoplamiento de ruido si los cables del I2C corren al lado de los de potencia.

**Lo que se hizo**: que el firmware sea inmune a la causa, sea cual sea. Cada 10 segundos, justo
antes de reescribir el contenido, se le manda la secuencia de reenganche (tres veces el nibble
`0x03`, que fuerza modo de 8 bits desde cualquier fase, y después `0x02`, que lo devuelve a 4
bits). Reengancha desde **cualquier** estado, incluso si al display se le corta la alimentación y
vuelve en modo de 8 bits.

⚠️ **No se usa `lcd.init()` para esto**, aunque sea lo obvio: adentro tiene `delay(50)` y
`delay(1000)` —verificado en `LiquidCrystal_I2C.cpp`—, y un segundo de loop congelado dejaría las
luces clavadas y el sonido sordo, justo lo que este proyecto ya trabajó para sacarse de encima. Los
nibbles se mandan a mano al expansor PCF8574, con el mapeo de la propia librería (P0=RS, P1=RW,
P2=Enable, P3=luz, P4-P7=datos), y la secuencia completa cuesta unos 10 ms.

**Gate**: compilado y cargado con `arduino-cli` (core esp32 3.3.10) → sin errores, **87 % de flash
y 16 % de RAM**, `Hash of data verified`.

**Pendiente**: subir `TELEGRAM_ESPERA_MS` (hoy 5 s, corto para esta red) y revisar
`HANDSHAKE_TLS_SEGUNDOS`; y la consulta propia con long polling, que es lo que falta para que el
bot conteste rápido.

#### Atribución por modelo (sesión 2026-08-20, parte 2)
- **Opus 5**: investigación de las alternativas (código de la librería en master, AsyncTelegram2,
  API real de Telegram), envío propio, diagnóstico del LCD por la foto y el reenganche de nibbles,
  carga del firmware y documentación.

### Sesión 2026-08-20 (parte 3) — Una velocidad para cada motor

Probando el cobertor con la lona puesta, Mariano necesitó poder darle **una velocidad distinta a
cada motor**. No es un capricho: es exactamente el problema anotado en `docs/PENDIENTES.md` #7d
desde el 13 de agosto. El hilo va pasando de un carrete al otro, así que el que suelta y el que
recoge no tienen el mismo diámetro; con los dos al mismo PWM, uno termina arrastrando al otro.

**Lo que había que cuidar para no romper nada**, que era el pedido explícito:

1. **El despacho de comandos usa `startsWith()`**, así que `/velocidad_a` también hace match con
   `/velocidad`. Si quedaran en ese orden, `/velocidad_a 50` le pasaría `"_a 50"` a `toInt()`, que
   devuelve 0, y el comando se rechazaría con un mensaje incomprensible. Los comandos por motor van
   **antes** que el general, con el porqué escrito al lado para que nadie los reordene.
2. **La calibración guardada no se podía perder.** Al arrancar se lee primero la clave vieja
   (`velCobertor`) y se usa como punto de partida de los dos motores; si además existen las claves
   por motor, esas mandan.
3. **Una sola regla de validación.** Los tres comandos comparten `velocidadValida()` y
   `textoVelocidadInvalida()`, así que es imposible que uno acepte lo que otro rechaza — el mismo
   criterio que llevó a unificar los motores en una sola estructura el 13 de agosto.
4. **La clave de NVS sale del nombre del motor** (`velMotor%c`), no de una lista aparte: no hay dos
   cosas que mantener en sincronía ni forma de guardar lo de un motor bajo la clave del otro.

La velocidad pasó a vivir **dentro de la estructura `Motor`**, donde ya viven sus pines. El global
`velocidadCobertor` desapareció y no quedó ninguna referencia suelta (verificado con `grep`).

**Gate**: compilado y cargado con `arduino-cli` (core esp32 3.3.10) → sin errores, **87 % de flash
y 16 % de RAM**, `Hash of data verified`.

**Falta la parte de campo**: encontrar con la lona puesta qué par de valores deja el hilo parejo de
punta a punta. La herramienta ya está; el número lo da la pileta.

#### Atribución por modelo (sesión 2026-08-20, parte 3)
- **Opus 5**: velocidad por motor, revisión de conflictos con el despacho de comandos y la
  migración de NVS, carga del firmware y documentación.

### Sesión 2026-08-20 (parte 4) — Cuatro velocidades, y la verificación del sentido de giro

Con la lona puesta apareció el resto del problema de la parte 3: **la compensación se da vuelta
entre abrir y cerrar**. Al abrir, el carrete lleno es uno; al cerrar es el otro, así que el motor
que sobra de velocidad cambia. Lo que dejaba el hilo parejo al abrir lo dejaba flojo al cerrar. Son
cuatro números, no dos.

Quedó así: `/velocidad_abrir 40 30` y `/velocidad_cerrar 45 35` (primero el A, después el B; con un
solo número los dos quedan igual), y `/velocidad 35` pone las cuatro de una. Los tres, sin
argumentos, muestran la tabla completa — que también está en `/status`.

**Se quitaron `/velocidad_a` y `/velocidad_b`**, que habían durado una sola versión: la velocidad de
un motor ahora depende del movimiento, así que por sí sola no significa nada. Y había una razón
extra, que es la trampa que enseñó esta parte: **`/velocidad_a` es prefijo de `/velocidad_abrir`**.
Con el despacho por `startsWith()` que usa el resto del programa, `/velocidad_abrir 40` habría hecho
match con `/velocidad_a` según el orden de los `if`. Por eso estos comandos **comparan la palabra
completa** (`comandoDe()`), y así agregar un comando nuevo mañana no puede robarle los mensajes a
otro.

También se dejó de confiar en `toInt()` para los argumentos: ante cualquier basura devuelve 0 en
silencio, y con velocidades un 0 que nadie pidió es un motor que no arranca y media hora buscando el
problema en la mecánica. Ahora se exige que sean dígitos de verdad.

**La verificación del sentido de giro (pedido de Mariano en el momento)**

Preguntó si algo del sentido se había cambiado, porque necesita que **los dos motores giren para el
mismo lado siempre**: horario al abrir y antihorario al cerrar. Se auditó el código entero en vez de
contestar de memoria, y el resultado es que la garantía es **estructural**:

- `motorMover()` es la ÚNICA función que toca los pines de sentido, y se la llama en cuatro lugares:
  dos en la rama de prueba (un motor cada uno, excluyentes) y dos en la del cobertor, que reciben
  **literalmente la misma variable** `avanza`.
- `motorCambiarVelocidad()` —lo único que tocaron las partes 3 y 4— escribe **sólo el pin de
  habilitación**, que es el del PWM. El sentido viaja por otros pines. **Cambiar velocidades no
  puede cambiar el giro**, por caminos separados.
- `motorFrenar()` y `motorSoltar()` ponen las dos entradas en bajo: tampoco definen sentido.

O sea: no existe estado del programa en el que el cobertor haga girar los motores en sentidos
eléctricos opuestos. Lo que sí elige el usuario es **cuál de las dos direcciones se llama "abrir"**,
con `/cobertor_sentido`, y eso mueve las dos juntas: si abrir pasa a ser horario, cerrar queda
antihorario automáticamente. `/status` lo informa en una sola línea —"los 2 motores a la derecha
(horario)"— justamente porque es un solo dato para los dos.

Y la excepción que no es del software: si los motores están **montados espejados**, la misma
polaridad eléctrica se ve como giros opuestos. Eso se corrige invirtiendo los dos cables de un motor
en el L298N (`OUT3` ↔ `OUT4`), con la fuente apagada, y está así a propósito desde el 2026-08-13.

**Gate**: compilado y cargado con `arduino-cli` (core esp32 3.3.10) → sin errores, **87 % de flash y
16 % de RAM**, `Hash of data verified`.

#### Atribución por modelo (sesión 2026-08-20, parte 4)
- **Opus 5**: las cuatro velocidades, el despacho por palabra completa, la validación de argumentos,
  la auditoría del sentido de giro, carga del firmware y documentación.

### Sesión 2026-08-20 (parte 5) — Un tiempo para cada motor, y el orden que importa

Espejo de la parte 4, ahora con los tiempos: `/tiempo_abrir 4.5 5` y `/tiempo_cerrar 5 5.5`, primero
el A y después el B, con un solo número para dejar los dos iguales. Cuatro tiempos y cuatro
velocidades, con la misma forma de comando y la misma tabla en cada respuesta.

Lo interesante no fueron los comandos sino **la máquina de estados**, que se había rehecho el 13 de
agosto justamente para que hubiera UN SOLO corte por tiempo. Ahora son dos, y había que extenderla
sin reintroducir la clase de error que ese rediseño eliminó.

Quedó así: cada motor lleva su `duracionMs` y su `enMarcha`; el que llega a su hora frena y se queda
frenado; el movimiento termina cuando no queda ninguno en marcha. **La regla de fondo no cambió**:
el estado decide QUÉ hacer al terminar, nunca CUÁNDO — el cuándo es, para cada motor, su propia
duración.

**El detalle que hay que entender del orden de las tres etapas del loop**: la hora de cada motor se
revisa ANTES del fin de la patada de arranque. Si fuera al revés, un motor con un tiempo más corto
que la patada (300 ms) frenaría correctamente y, en la misma vuelta, el fin de la patada le
volvería a levantar la habilitación: giraría de más. Está escrito en el código con el porqué, porque
es exactamente el tipo de cosa que alguien "ordena" seis meses después sin saber que la rompe.

**Verificado en hardware**, no de palabra:

```
>>> Arranca ABRIR | A 3.5 s | B 3.5 s | t=113228
>>> Fin de la patada de arranque: motor A a 40% | motor B a 80%
>>> Motor A: freno a los 3503 ms de los 3500 ms que le tocaban
>>> Motor B: freno a los 3504 ms de los 3500 ms que le tocaban
>>> Cobertor: ABIERTO - duro 3504 ms de los 3500 ms pedidos (el mas largo de los dos motores)
```

Tres y cuatro milésimas de error, cada motor con su propio reloj. Los tres formatos de comando
(`/tiempo_cerrar 4 3`, `/tiempo_cerrar 4`, `/tiempo_cerrar 3 4`) fueron aceptados.

**De paso se arregló la herramienta de medición**: la captura del puerto serie leía con
`ReadExisting()` en un bucle apretado y devolvía bloques de 32 bytes repetidos, lo que ensuciaba los
registros y en un momento hizo dudar del ESP32 cuando el problema era del lector. Ahora lee por
líneas.

#### Atribución por modelo (sesión 2026-08-20, parte 5)
- **Opus 5**: los cuatro tiempos, la extensión de la máquina de estados, la verificación en hardware
  y la documentación.

