# 📓 Changelog — Pileta Inteligente

> Qué cambió en cada versión del programa, en criollo. Las versiones numeran el **sistema de
> luces**, que es lo que más veces se rehízo; el resto del proyecto acompaña.
>
> El relato completo de cada sesión —con las mediciones, los errores y por qué se decidió cada
> cosa— está en [`../AVANCES.md`](../AVANCES.md). Acá va el resumen.

---

## v5.5 — 2026-08-20 · El envío de Telegram deja de ser de la librería, y la pantalla se cura sola

Dos problemas distintos de la misma tarde de taller, los dos de raíz.

### Arreglado
- **El bot ya no puede duplicar una respuesta.** El envío dejó de pasar por
  `UniversalTelegramBot`: ahora se arma el POST a mano, se lee el `Content-Length` que manda el
  servidor y se consumen **exactamente** esos bytes. El bug de la librería era que su lector corta
  apenas leyó el primer bloque disponible (`if (responseReceived) break;`), así que cuando los
  encabezados llegaban en un segmento TLS y el cuerpo en el siguiente se quedaba sin cuerpo, no
  podía confirmar el envío, y `sendPostMessage()` reenviaba el mismo mensaje durante 8 segundos.
  **Y lo más importante: el envío propio NO REINTENTA NUNCA.** Si algo falla queda en el Monitor
  Serie y se sigue; un mensaje no puede duplicarse porque no existe el código que lo mandaría dos
  veces. Verificado contra el servidor real antes de escribirlo: api.telegram.org responde
  HTTP/1.1 con `Content-Length`, sin `chunked`, y acepta `keep-alive`.
- **La pantalla se recupera sola.** El HD44780 trabaja en modo de 4 bits, con cada carácter
  partido en dos mitades; si un pico eléctrico le hace perder una sola mitad, todo lo que sigue se
  arma con la mitad de un carácter y la mitad del siguiente, y el display queda escribiendo
  símbolos nítidos pero sin sentido. Pasó en hardware el 2026-08-20, después de cablear el relé de
  12 V, y **no se recuperaba solo**: el programa lo inicializaba una única vez al arrancar. Ahora
  se le manda la secuencia de reenganche cada 10 segundos, justo antes de reescribir el contenido,
  así el hueco dura milisegundos y no se ve. Reengancha desde **cualquier** estado, incluso si al
  display se le corta la alimentación y vuelve en modo de 8 bits.

### Detalles que importan
- El reenganche **no usa `lcd.init()`**, aunque sea lo obvio: esa función tiene `delay(50)` y
  `delay(1000)` adentro (verificado en el código de la librería), y un segundo de loop congelado
  dejaría las luces clavadas y el sonido sordo. Los nibbles se mandan a mano al expansor PCF8574 y
  la secuencia completa cuesta unos 10 ms.
- Todo lo que se muestra pasa ahora por `pantallaMostrar()`, un solo lugar, con las líneas
  rellenadas a 16 caracteres — más barato y más seguro que un `lcd.clear()`.
- Si la librería dejó restos sin leer en el socket (su bug), el envío los detecta y rearma la
  conexión antes de mandar: leer la cola del pedido anterior como si fuera la respuesta propia
  sería el error más difícil de entender de todos.

### Todavía pendiente
- **El límite para leer la confirmación quedó corto.** Se fijó en 5 s con la red del taller, y esa
  tarde la red de la pileta (`UA-Alumnos`) se degradó a saludos TLS de **7 a 14,7 segundos**. El
  envío funciona —los mensajes llegan— pero a veces no alcanza a leer la confirmación y lo informa
  como fallo. Ver `docs/PENDIENTES.md` #15.
- **La consulta sigue siendo de la librería**, así que sigue pagando un saludo TLS por vuelta y
  pidiendo `limit=1`. Es lo que falta para que el bot conteste rápido: `docs/PENDIENTES.md` #7c.

---

## v5.4 — 2026-08-20 · Telegram: la cola vieja y el buffer (el bucle de 8 s NO quedo resuelto)

> ⚠️ **Verificado en hardware el mismo dia y el resultado fue NEGATIVO para la parte principal.**
> Subir el buffer no alcanzo: `/help` siguio llegando tres veces y el Monitor Serie midio
> `consulta 4421 ms | respuesta 16805 ms` — los dos mensajes agotaron sus 8 segundos de
> reintentos. El truncado era real y valia arreglarlo, pero **no era la causa dominante**.
> La causa de fondo esta mas abajo en la libreria y se detalla en `AVANCES.md` y en
> `docs/PENDIENTES.md` #14. Lo que si quedo resuelto de esta version es la cola vieja, la
> instrumentacion y la memoria en `/status`.

El bot tardaba muchísimo en contestar y a veces mandaba la misma respuesta dos veces, aunque el
comando se ejecutara una sola. No era el WiFi ni el ESP32 quedándose sin memoria: era el buffer
de la librería de Telegram.

### Intentado, sin exito (el bucle de 8 segundos sigue)
- **El bucle de reenvío de 8 segundos.** La librería trae un buffer de 1500 bytes
  (`maxMessageLength`) que usa tanto para armar el mensaje como para leer la confirmación. Y
  Telegram, al confirmar un envío, **devuelve el texto entero** más unos 400 bytes con los datos
  del chat. La ayuda de `/help` son 1363 bytes de texto, así que su confirmación llegaba cortada
  por la mitad, el JSON quedaba incompleto y la librería no podía darla por buena: entraba en
  `while (millis() < sttime + 8000)` y **reenviaba el mismo mensaje durante ocho segundos**, con
  el bot sin atender nada más. Ese era el mensaje duplicado que se veía en el chat — la
  duplicación pasaba en la respuesta que salía, no en la orden que entraba. El buffer pasó a
  **4096 bytes**.
- **La ayuda de `/help` ahora son dos mensajes** en vez de uno de 1363 bytes: primero los
  comandos de todos los días (luces, calentador, cobertor), después los de consulta y los ajustes
  finos de taller. Ninguno de los dos pasa de 907 bytes.
- **La cola vieja de Telegram ya no se ejecuta al encender.** Telegram guarda hasta 24 horas los
  mensajes que el bot no confirmó, así que un ESP32 que estuvo apagado un rato arrancaba
  masticando esa cola entera, del mensaje más viejo al más nuevo y de a uno por vuelta: parecía
  trabado y, peor, podía mover el cobertor por una orden de hacía horas. Ahora al conectar se
  descarta lo pendiente y el bot arranca escuchando sólo lo nuevo.

### Agregado
- **`/status` informa la memoria**: KB libres y el mínimo histórico desde que arrancó. El mínimo
  es el número que importa: si baja sesión tras sesión hay una fuga o el heap se está
  fragmentando, y eso termina en reinicios raros. Hasta ahora el programa no medía la memoria en
  ningún lado.
- **Dos tiempos por el Monitor Serie en cada comando**: cuánto tardó la consulta y cuánto tardó
  la respuesta. Separan el problema en el acto — si el tiempo se va en *consultar* es el saludo
  TLS o la red; si se va en *responder* cerca de los 8000 ms, es el bucle de reenvío otra vez.
  Además, una consulta que pase de 3 segundos deja aviso propio.

### Todavía pendiente
- **La latencia de fondo sigue siendo de segundos** y esto no la arregla: cada vuelta abre una
  conexión TLS nueva (la librería cierra la conexión cuando no hay mensajes) y se consulta cada
  2,5 s, de a un mensaje por vez. La solución de fondo es **long polling**, que necesita prueba
  en hardware por el watchdog del núcleo 0. Ver `docs/PENDIENTES.md` #7c.

---

## v5.3 — 2026-08-13 · Los sentidos, separados y sin pisarse

Continuación de la v5.2, con el cobertor ya funcionando en hardware. Dos cambios pedidos en el
taller mientras se calibraba.

### Cambiado
- **El sentido del cobertor y el de las pruebas son ahora dos sistemas independientes.** Antes
  había un flag por motor (`/sentido_a`, `/sentido_b`) que servía para las dos cosas a la vez, y
  en la mano era un laberinto: cuatro combinaciones, dos de ellas inservibles, y tocar una sola
  cambiaba dos cosas (si los motores se acompañan **y** cuál dirección es abrir). Se podía dar
  vueltas indefinidamente sin llegar al resultado.
  - **`/cobertor_sentido`** invierte el cobertor. Los dos motores reciben **siempre la misma
    polaridad**, calculada una vez y aplicada a los dos: no existe estado del programa en el que
    puedan girar uno contra el otro. No toca las pruebas.
  - **`/sentido_a` y `/sentido_b`** ahora valen **sólo** para `/motor_a` y `/motor_b`. No
    afectan en nada al cobertor.
- **Los tiempos aceptan decimales**: `/tiempo_abrir 4.5`, con coma o punto. Por dentro se
  guardan en décimas de segundo (0,1 a 60 s) porque el recorrido no tiene por qué caer en un
  número redondo: a esta velocidad, medio segundo son varios centímetros de hilo. `/motor_a 2.5`
  también.

### Ojo al actualizar
- Los tiempos cambian de formato de guardado, así que **vuelven a 8,0 s** una vez. El resto de
  la configuración no se toca.
- Como el cobertor aplica la misma polaridad a los dos motores, **si están montados espejados**
  (uno enfrentado al otro) van a girar en sentidos visualmente opuestos, y eso ya **no se
  corrige por software**. Se invierten los dos cables de un motor en el L298N (`OUT3` ↔ `OUT4`),
  con la fuente apagada. Es el precio de que los dos sistemas no se contaminen.

---

## v5.2 — 2026-08-13 · El cobertor, rehecho

El mecanismo del cobertor cambió: ahora es un **lazo de hilo entre los dos ejes**, así que los
dos motores empujan juntos hacia el mismo lado y el recorrido se mide **por tiempo**.

### Agregado
- **`/tiempo_abrir N` y `/tiempo_cerrar N`** (1 a 60 s, en NVS): cada movimiento lleva su propio
  tiempo, porque cerrar suele costar más que abrir.
- **`/sentido_a` y `/sentido_b`**: invierten el giro de un motor **sin desatornillar cables** del
  L298N, guardado en NVS. "Que los dos giren para el mismo lado" es una condición física y
  depende de cómo queden montados.
- **`/motor_a N`**: la prueba de taller acepta cuántos segundos girar (2 por omisión). No se
  guarda en NVS a propósito: es una prueba, no un ajuste.
- Los mensajes de `/sentido_*`, `/motor_*` y `/status` dicen **hacia qué lado gira cada motor** y
  avisan si los dos quedaron alineados o cruzados.
- **Patada de arranque**: los motores parten al 100 % durante 300 ms y recién ahí bajan a la
  velocidad de régimen. Un motor con reductora necesita mucho más par para empezar a moverse.
- El Monitor Serie informa la **duración medida** de cada movimiento junto a la pedida, para
  distinguir "lo cortó el programa" de "el motor se frenó solo".

### Cambiado
- **Los dos motores giran juntos**, en la misma dirección. Antes uno tiraba y el otro quedaba
  suelto, que era lo correcto para el mecanismo anterior.
- **El recorrido termina por tiempo, no por sensor.** Los fines de carrera siguen leyéndose e
  informan la posición en `/status`, pero **ya no cortan el movimiento**.
- **PWM de los motores: 1 kHz → 8 kHz.** El valor de fábrica del core cae justo donde el oído es
  más sensible, y un motor que no llega a girar chilla a esa frecuencia.
- **Al terminar, freno en seco** (*fast motor stop* del L298N) en vez de soltar el motor: antes
  seguía girando por inercia con el carrete encima. Al encender siguen sueltos, para poder mover
  el mecanismo a mano.
- **Módulo de motores reescrito**: los dos motores pasaron a una estructura con sus pines
  adentro, con **tres funciones compartidas** en lugar de dos juegos duplicados, y **un solo
  arranque** y **un solo corte por tiempo** para los tres movimientos.

### Arreglado
- **El bot se quedaba mudo hasta dos minutos.** El saludo TLS traía un timeout de fábrica de
  120 s (`NetworkClientSecure.cpp`), y la línea que supuestamente lo protegía —`setTimeout(2000)`—
  no hacía nada: esa clase no redefine el método y terminaba en `Stream::setTimeout()`, que sólo
  gobierna las lecturas. Ahora usa **`setHandshakeTimeout(5)`**, la API correcta, en segundos.
- **El tiempo de la prueba de motor podía tomar el de la apertura**, porque había dos relojes y
  `probarMotor()` no tocaba el que usaba el corte.

### Verificado en hardware
- Movimiento de 10 s → **10008 ms medidos**. Prueba de 2 s → **2019 ms medidos**.
- Latido de Telegram: **máximo 4 s**, contra los **117 s** de antes del arreglo.
- Los dos motores moviéndose juntos, con el hilo atado.
- Aparte del firmware: el motor B tenía **una soldadura floja**, que imitaba estos síntomas.

---

## v5.1 — 2026-08-13 · La tira en el taller

Primera prueba en hardware de la tira WS2812 y recalibración del análisis de sonido con datos
medidos.

### Agregado
- **Puerta de ruido por banda** (`/piso N`, guardada en NVS): a cada banda se le resta el piso
  de ruido del lugar antes de normalizar. Lo que no lo supera vale cero. Es el *squelch* de
  WLED, y resuelve que una banda sin contenido se llenara con el ruido del micrófono.
- `/diag` ahora informa el **rango del pico a pico de los últimos ~10 segundos**, no sólo la
  foto instantánea de 25,6 ms, y avisa si el ruido de fondo supera el umbral de música.
- `/espectro` muestra la columna **UTIL** (lo que queda después del piso) y el piso activo.

### Cambiado
- **`SONIDO_MINIMO`: 45 → 90.** El valor viejo había quedado por debajo del ruido de fondo
  real del taller (32-56), así que el sistema declaraba "música detectada" en silencio. La
  música medida da 180-591.
- **La detección de golpes usa la señal útil** en vez de la cruda: el piso de ruido se suma por
  igual al golpe y al promedio, y descontarlo aumenta el contraste.
- **La tira es de 21 píxeles** (70 cm), no 15: es la vuelta completa a la pileta. Divide exacto
  en tres zonas de 7 para el efecto ESPECTRO.
- **Histéresis del calentador: 5 °C → 2 °C.** Con 5 y un objetivo de 28 °C, el agua oscilaba
  entre 23 y 28 — una diferencia que se siente al meterse. Ahora el rango es 26-28.

### Verificado en hardware
- **Los dos defectos, cerrados**: en silencio las bandas pasaron de 78/88/80 a **2/0/0**, y con
  un tono puro de 1000 Hz de 72/100/100 a **4/41/0**.
- **Los cuatro efectos siguen la música**, con las tres zonas moviéndose por separado.
- La tira enciende con los colores correctos sin `/orden` (chip GRB).
- El nivel lógico alcanza sin level shifter: `VIN` a **4,4 V** → umbral 3,08 V → 0,22 V de
  margen. Más bajo es *mejor* acá.
- **La tira no contamina el micrófono** (el riesgo que más preocupaba del plan v5).
- El limitador de corriente hace lo que promete.
- La FFT es exacta en las tres bandas (tonos de 100, 1000 y 3000 Hz).

---

## v5.0 — 2026-08-07 · Análisis de espectro real

- Sistema de luces **reescrito de cero**: FFT de 256 muestras a 10 kHz → graves / medios /
  agudos → ganancia automática por banda → suavizado → detección de golpes por energía del
  último segundo.
- **Tira WS2812** reemplaza a los 8 LEDs, con cuatro efectos (espectro, mezcla, cometa,
  arcoíris) y respiración lenta en silencio.
- **Limitador de corriente por software**: permite colgar la tira del propio ESP32 sin una
  tercera fuente.
- Comandos nuevos: `/efecto`, `/leds`, `/brillo`, `/corriente`, `/orden`, `/luces_test`,
  `/espectro`, `/onda`.
- Eliminado todo el canal DO (código para un micrófono que no está conectado) y la base
  adaptativa exponencial, que se cegaba con música sostenida.
- El arranque informa el motivo del último reinicio (brownout, watchdog, panic).

## v4 — 2026-07-23 · Detección bicanal *(nunca llegó a hardware)*

- Dos micrófonos: AO analógico + DO como detector de golpes por hardware. Se descartó al
  confirmarse que sólo hay **un** micrófono conectado.

## v3 — 2026-07-23 · Sin FFT

- FFT eliminada por completo: con el micrófono a 3,3 V daba 17 counts de pico a pico y
  clasificar frecuencias sobre eso era clasificar ruido. Medición por pico a pico.
- `client.setTimeout(2000)`: Telegram congelaba el show 1-2 s por consulta.
- `/temperatura N` guardado en NVS.

## v2 — 2026-07-16 · Todo en un programa

- Calentador + luces + cobertor + Telegram integrados en `PiletaInteligente.ino`.
- Cobertor completo: 2 motores por L298N, fines de carrera, timeout de seguridad.
- Secretos separados en `config.h`, fuera de git.

## v1 — 2026-06-25 · Calentador

- Sensor DS18B20 + relé + LCD, verificado en protoboard.
