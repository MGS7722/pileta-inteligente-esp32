# Plan LUCES v5 — Tira WS2812 + análisis de espectro real

> Escrito el 2026-08-07 para la prueba en el taller del 2026-08-08.
> **Reescribe desde cero el sistema de luces**: reemplaza los 8 LEDs por una tira WS2812B de
> 50 cm (15 píxeles) y cambia la detección "por volumen" por un **análisis de espectro real**
> con separación de graves, medios y agudos.
>
> Todo verificado contra documentación oficial, hojas de datos y el código real de las
> librerías instaladas. Las fuentes están citadas en cada decisión.

---

## 1. El hallazgo que cambia el proyecto

La bitácora tenía una pregunta abierta desde el 2026-08-06:

> *"Falta medir la forma de onda cruda del micrófono para saber si el módulo entrega audio real
> o una envolvente rectificada — si es lo segundo, no hay frecuencias que analizar."*

**Respuesta: entrega audio real.** La documentación del KY-037 es concluyente:

- El pin `AO` es la señal del micrófono electret **CMA-6542PF sin amplificar** — "pequeñas
  variaciones alrededor de un punto de polarización".
- El **LM393 de la placa es un comparador, no un amplificador**. No toca el `AO`: sólo genera
  el `DO` comparando contra el potenciómetro.
- El potenciómetro **no afecta al `AO`**, únicamente al `DO`.
- La señal está **invertida** (más sonido → menos tensión), lo cual es normal en un electret y
  no afecta a la medición de amplitud ni al espectro.

O sea: por el `AO` viaja la onda de sonido completa, con toda su información de frecuencia.

**Entonces, ¿por qué se eliminó la FFT en la v3?** Porque en ese momento el micrófono estaba
alimentado a **3,3 V** y entregaba un pico a pico de ~17 counts: clasificar frecuencias sobre 17
counts es clasificar ruido de cuantización. Esa decisión fue **correcta entonces**. Pero el
2026-08-06 el micrófono pasó a **5 V** y las mediciones reales dieron:

| Condición | Pico a pico | En milivoltios |
|---|---|---|
| Silencio | 9 – 36 | 7 – 29 mV |
| Música (valles) | 60 – 130 | 48 – 105 mV |
| Música (golpes) | 250 – 500 | 200 – 400 mV |
| Picos fuertes | 870 – 1040 | 700 – 838 mV |

Es una señal **30 veces más grande**. Con 1040 counts de recorrido hay ~10 bits efectivos: de
sobra para una FFT que sólo necesita separar tres bandas anchas. **La conclusión de la v3 quedó
vencida por un cambio de hardware, y nadie volvió a revisarla.**

---

## 2. Cómo se hace esto en la vida real

### 2.1 Las tres generaciones

| Época | Cómo | Qué logra |
|---|---|---|
| **Años 60–70** — *color organ* | Tres filtros analógicos RC (pasa-bajos, pasa-banda, pasa-altos) y una lámpara por filtro | Graves → rojo, medios → verde, agudos → azul. **Es la estética de discoteca clásica** |
| **Años 2000** — MSGEQ7 | Un chip con 7 filtros en hardware que entrega los niveles multiplexados | Espectro sin gastar CPU. Fue el estándar en Arduino |
| **Hoy** — FFT por software | Muestreo digital + transformada de Fourier + agrupación en bandas | Lo que usan WLED, Resolume y todo el software de VJ |

No tenemos MSGEQ7, pero **el ESP32 tiene potencia de sobra** para hacer la FFT por software: es
justamente lo que hace la generación actual.

### 2.2 La referencia: cómo lo hace WLED

[WLED sound-reactive](https://kno.wled.ge/advanced/audio-reactive/) es el estándar de facto en
tiras LED reactivas al sonido. Su receta, verificada en la documentación oficial:

| Qué hace | Valor en WLED | Qué adoptamos |
|---|---|---|
| Frecuencia de muestreo | **10240 Hz** | 10 kHz — el que ya usamos y está validado |
| Bandas de salida | 16 | **3** (graves/medios/agudos) — es lo que pide el efecto y lo que 15 píxeles pueden mostrar |
| Escalado de magnitudes | Raíz cuadrada (default) o logarítmico | **Raíz cuadrada** |
| Reparto de bandas | Logarítmico | Logarítmico |
| **AGC** (ganancia automática) | Sí — sigue el volumen de la música | **Sí, y por banda** |
| Suavizado | *Dynamics limiter*: caída lenta | Ataque instantáneo / caída suave |

Que WLED muestree a 10240 Hz confirma nuestro número: es la frecuencia correcta para música
(permite analizar hasta 5 kHz, donde ya vive todo el contenido rítmico).

### 2.3 Los dos análisis que hay que hacer, y no son el mismo

Un sistema de luces bueno combina **dos cosas distintas** que suelen confundirse:

1. **ESPECTRO** — *qué* frecuencias están sonando ahora. Da los **colores y las zonas**.
   Se obtiene con la FFT.
2. **BEAT** — *cuándo* pega el golpe. Da los **destellos y los cambios**.
   No se obtiene de la FFT: se obtiene comparando la energía de los graves contra el promedio
   del último segundo ([Beat Detection Algorithms, flipcode/GameDev](https://www.flipcode.com/misc/BeatDetectionAlgorithms.pdf)).

El sistema actual sólo tiene una versión pobre del segundo (volumen total contra una base que se
ciega). Le falta el primero por completo. **La v5 hace los dos.**

---

## 3. El sistema nuevo: cadena de procesamiento

Todo corre en el **núcleo 1** (el `loop()`), porque el núcleo 0 es de Telegram y del WiFi.

```
[1] CAPTURA     256 muestras a 10 kHz  ................  25,6 ms
[2] PREPARADO   quitar el DC + ventana de Hann  .......   0,3 ms
[3] FFT         256 puntos -> 128 bins de 39 Hz  ......   1,5 ms
[4] BANDAS      agrupar en graves / medios / agudos  ..   0,1 ms
[5] AGC         normalizar cada banda por su máximo  ..   0,1 ms
[6] SUAVIZADO   ataque instantáneo, caída suave  ......   0,1 ms
[7] BEAT        graves vs promedio del último segundo     0,1 ms
[8] EFECTO      pintar el cuadro en memoria  ..........   0,2 ms
[9] VOLCADO     límite de corriente + gamma -> show()     0,5 ms
                                              TOTAL  ~  28,5 ms  ->  35 cuadros/s
```

**35 cuadros por segundo**, mejor que los 28 de hoy (que gastaba 35 ms sólo en muestrear). Es
suficiente para que el ojo lo vea fluido.

### 3.1 Captura — por qué 256 muestras a 10 kHz

| Parámetro | Valor | Por qué |
|---|---|---|
| Frecuencia de muestreo | 10 kHz | Permite analizar hasta 5 kHz (Nyquist). Coincide con WLED |
| Muestras por ventana | 256 | Potencia de 2 (la FFT lo exige) |
| Duración de la ventana | 25,6 ms | Un ciclo completo de 40 Hz entra justo; un bombo de 60 Hz entra 1,5 veces |
| Resolución en frecuencia | **39,06 Hz por bin** | Comparable a los ~43 Hz de WLED |

Con 512 muestras la resolución sería el doble, pero la ventana pasaría a 51 ms y el refresco
caería a 19 cuadros/s: se vería a los tirones. **256 es el punto justo.**

> **Frecuencia real, no nominal.** `analogRead()` no tarda siempre lo mismo, así que la captura
> mide cuánto tardó de verdad y calcula la frecuencia efectiva con ese número. Los cortes entre
> bandas se computan sobre la frecuencia real. Sin esto, si el muestreo sale a 9,2 kHz en vez de
> 10, todas las bandas quedan corridas un 8 % y nadie se entera.

### 3.2 Preparado de la señal

1. **Quitar el DC** (`dcRemoval()` de la librería): la señal viene montada sobre el punto de
   polarización del micrófono (~1,6 V). Sin quitarlo, ese offset aparece como un pico gigante en
   el bin 0 y contamina los graves. **Este era uno de los errores de la FFT vieja.**
2. **Ventana de Hann** (`windowing()`): sin ventana, los bordes abruptos de cada bloque de
   muestras generan frecuencias falsas repartidas por todo el espectro (*spectral leakage*). Hann
   es la ventana estándar para análisis musical.

### 3.3 Las tres bandas

Los cortes siguen la división clásica del audio:

| Banda | Frecuencias | Bins | Qué instrumentos viven ahí |
|---|---|---|---|
| **GRAVES** | 78 – 234 Hz | 2 – 6 | Bombo, bajo, tom |
| **MEDIOS** | 234 – 2000 Hz | 6 – 51 | Voz, guitarra, piano, caja |
| **AGUDOS** | 2000 – 5000 Hz | 51 – 128 | Platos, hi-hat, brillo |

> **Por qué los graves arrancan en 78 Hz y no en 40.** El bin 1 cubre de 39 a 78 Hz, que es
> exactamente donde cae el **zumbido de 50 Hz de la red eléctrica** que cualquier micrófono sin
> blindaje capta de la fuente. Incluirlo sería alimentar los graves con el ruido de la instalación
> en vez de con la música. El bombo se detecta igual: su fundamental está en 50–100 Hz pero sus
> armónicos (100–200 Hz) caen de lleno en la banda. **El comando `/espectro` permite verificar en
> el taller si ese zumbido existe y cuánto vale.**

Las magnitudes se comprimen con **raíz cuadrada** antes de usarlas (como WLED): el rango dinámico
de una FFT es enorme y sin comprimir sólo se vería el pico más fuerte.

### 3.4 AGC — la pieza que hace que se vea bien

Sin control automático de ganancia, el sistema anda con un volumen y con otro no. Y hay un
problema adicional: en música real **los graves son naturalmente 10 o 20 veces más fuertes que
los agudos**, así que una escala común dejaría la banda de agudos siempre apagada.

Solución: **cada banda se normaliza contra su propio máximo reciente.**

```
si (nivel > maximoBanda)  maximoBanda = nivel          // sube al instante
sino                      maximoBanda *= 0,999         // baja de a poco (~8 s)
maximoBanda = max(maximoBanda, PISO)                   // nunca por debajo del piso
nivelNormalizado = nivel / maximoBanda                 // queda entre 0 y 1
```

El **piso es imprescindible**: sin él, en silencio el AGC amplifica el ruido de fondo hasta el
tope y la tira baila sola con el zumbido del ambiente. Es el error clásico de los AGC caseros.

### 3.5 Suavizado — ataque instantáneo, caída suave

```
si (nuevo > mostrado)  mostrado = nuevo                       // el golpe se ve YA
sino                   mostrado += (nuevo - mostrado) × 0,25  // se apaga con elegancia
```

Es lo que hace que se vea "musical" en vez de epiléptico, y es lo que WLED llama *dynamics
limiter*. Sin esto, con 35 cuadros por segundo la tira parpadea de forma desagradable.

### 3.6 Detección de beat

Sobre la **energía de la banda de graves** (que es donde está el pulso de la música), con el
algoritmo clásico de energía sonora:

- Historial circular de **32 ventanas ≈ 0,9 segundos**.
- Es golpe cuando la energía instantánea supera `C × promedio del historial`.
- **`C` se calcula solo** a partir de la dispersión del historial: si los golpes están muy
  marcados se puede ser menos exigente; si la señal es plana hay que exigir más para no disparar
  con ruido. Como las constantes del artículo original están en una escala de energías distinta
  a la nuestra, se usa el **coeficiente de variación** (desvío ÷ promedio), que es adimensional:

  ```
  C = 1,50 − 0,50 × min(1 ; desvío/promedio)     acotado a [1,15 ; 1,50]
  ```

  Música muy marcada → C ≈ 1,20. Señal plana → C ≈ 1,45. El factor fijo de hoy es 1,30, o sea que
  el rango nuevo queda **centrado en el valor ya validado con datos reales del taller**.
- **Refractario de 150 ms**: sin él, un solo golpe de bombo dispara tres veces seguidas.

**Por qué esto reemplaza a la base adaptativa.** La base actual es un filtro exponencial que con
música sostenida se infla hasta tapar los golpes que debería detectar (llegó a 147, dejando el
umbral en 192, con la música sonando). El promedio de una ventana deslizante **no puede dispararse
sin techo** y se recupera en un segundo exacto.

---

## 4. Los efectos

Cuatro efectos con identidad propia, elegibles con `/efecto N` y guardados en memoria.

### 4.1 `/efecto 1` — ESPECTRO *(el que pediste)*

La tira se divide en **tres zonas de 5 píxeles**. Cada zona es una barra de nivel de su banda:

```
[ GRAVES ][ MEDIOS ][ AGUDOS ]
  rojo      verde      azul
 ●●●●○     ●●○○○      ●●●●●
```

Es el analizador de espectro clásico: se ve **exactamente** qué está haciendo la música. Cada
banda tiene su color y su barra crece desde el borde de su zona. El último píxel de cada barra
se enciende a brillo parcial en vez de saltar de golpe: con sólo 5 píxeles por banda, ese
detalle es la diferencia entre una barra que se mueve con la música y una que va a escalones.

### 4.2 `/efecto 2` — MEZCLA

Toda la tira toma **un solo color, mezclado a partir de las tres bandas**: rojo = graves,
verde = medios, azul = agudos. Una canción con mucho bajo se ve roja; un pasaje de platos, celeste.
El brillo sigue el volumen total y cada beat mete un destello blanco que se abre desde el centro.

Es el más orgánico: la tira "respira" el color de la música.

### 4.3 `/efecto 3` — COMETA

Un cometa recorre la tira dejando estela. **Su velocidad la fija el volumen** y **su color, la
banda dominante**. En cada beat rebota al otro extremo y cambia de tono. El más vistoso cuando la
tira está estirada a lo largo.

### 4.4 `/efecto 4` — ARCOÍRIS

Degradado completo que gira sobre la tira. La velocidad de giro sigue el volumen y cada beat le
pega un salto de fase más un pico de brillo. La "discoteca clásica".

### 4.5 El destello del golpe (común a los cuatro)

En cada beat sale una **onda blanca desde el centro hacia los dos extremos**, que se abre y se
apaga en 110 ms. Se suma encima del efecto que esté corriendo en vez de pisarlo, así los cuatro
laten igual y el golpe se lee siempre, sea cual sea el efecto elegido.

### 4.6 En silencio (los cuatro)

Respiración lenta con el tono derivando por la rueda de color. La tira va a pasar la mayor parte
del tiempo así, con lo cual **tiene que verse linda también en silencio**. Con corrección de gamma
para que la respiración sea pareja y no un salto brusco.

---

## 5. Hardware

### 5.1 La tira

- **Modelo**: WS2812B 5 V, 30 LED/m, rollo de 5 m (150 píxeles).
- **A usar**: **50 cm = 15 píxeles**.
- **Dónde cortar**: por las líneas de cobre entre píxeles (una cada 33 mm). **Cortá del extremo
  que trae el conector de entrada**, así no hay que soldar nada: se aprovechan los cables que la
  tira ya trae.
- **Cuál es el extremo de entrada**: el que dice **`DIN`** (o `DI`) en la serigrafía, hacia donde
  apuntan las **flechas** impresas. El otro dice `DO` y ahí la señal no entra. Si se conecta al
  revés no enciende nada — es el error más común y **no rompe nada**.

### 5.2 Los tres cables

| Cable de la tira | Va a | Detalle |
|---|---|---|
| **`5V`** (rojo) | Pin `VIN` / `5V` del ESP32 | Los ~4,7 V que ya llegan del USB |
| **`GND`** (blanco o negro) | Riel `GND` de la protoboard | El mismo GND de todo el sistema |
| **`DIN`** (verde) | **GPIO16**, con **440 Ω** en serie | Dos resistencias de 220 Ω **en serie** |

**Sobre la resistencia**: Adafruit especifica *"300 to 500 Ohm resistor between the Arduino data
output pin and the input to the first NeoPixel"*, para proteger esa primera entrada de los picos
de conmutación. Verifiqué que 440 Ω no degrada la señal: con la capacidad de entrada del chip más
20 cm de cable (~25 pF), la constante de tiempo es de 11 ns — despreciable frente a los ~300 ns
que dura el flanco de un bit.

**El cable de datos, corto** (20–30 cm). Es la causa número uno de fallas al manejar una tira de
5 V con lógica de 3,3 V.

### 5.3 Por qué funciona sin level shifter (resuelto gratis)

El WS2812B exige **0,7 × su tensión de alimentación** para leer un "1". El ESP32 entrega 3,3 V.
La clave: ese umbral **no es fijo**, baja con la alimentación de la tira.

| Alimentación de la tira | Umbral que exige | ESP32 da 3,3 V |
|---|---|---|
| 5,0 V (cargador de celular) | 3,50 V | ❌ marginal — falla intermitente |
| **4,7 V (pin `VIN` del ESP32)** | **3,29 V** | ✅ **alcanza** |
| 4,5 V (fuente regulable) | 3,15 V | ✅ con margen |

Las placas DevKit llevan un **diodo Schottky** entre los 5 V del USB y el pin `VIN`, para que la
placa no devuelva corriente a la PC cuando se la alimenta por fuera. Ese diodo cae ~0,3 V, y de
ahí salen los ~4,7 V. **El level shifter que no compramos lo hace un diodo que la placa ya tiene.**

> ⚠️ **Verificar con el multímetro**: `VIN` contra `GND`, con el USB puesto. 4,6–4,8 V → en orden.
> Si mide 5,0 V clavados (hay placas sin ese diodo), el margen desaparece → plan B en §8.

### 5.4 Corriente: las cuentas reales

**El riel `VIN` no está libre.** Según `CONEXIONES.md` ya alimenta al LCD, al lado lógico del relé
y al micrófono, además del propio ESP32 por su regulador:

| Consumidor | Corriente |
|---|---|
| ESP32 con WiFi activo | 150 – 250 mA (picos de ~400 mA al transmitir) |
| LCD 16×2 con retroiluminación | ~30 mA |
| Módulo relé (con el calentador encendido) | ~75 mA |
| Micrófono KY-037 | ~5 mA |
| **Total actual, sin la tira** | **260 – 460 mA** |

Un USB 2.0 de notebook entrega 500 mA nominales: **el sistema hoy ya está cerca del límite**, y
funciona. Ese es el presupuesto real.

Cada píxel WS2812B consume hasta 60 mA (20 mA por canal) en blanco pleno; 15 píxeles a full serían
900 mA. **Por eso el limitador de corriente por software no es un adorno: es lo que hace viable no
usar una tercera fuente.**

| Alimentación del ESP32 | `/corriente` | Qué se ve con 15 píxeles |
|---|---|---|
| **USB de la notebook** (Monitor Serie disponible) | **120 mA** | Colores saturados brillantes; blanco tenue |
| **Cargador de celular de 2 A** | **500 mA** | Todo a full, blanco incluido |

Consecuencia de diseño, y es buena noticia estética: **los efectos privilegian colores saturados
(uno o dos canales) por sobre el blanco**, que consume el triple. Una discoteca se ve mejor en
color saturado que en blanco, así que la restricción eléctrica empuja hacia el buen gusto. El
blanco queda para los destellos de beat, que duran 100 ms.

**Las fuentes del proyecto no se tocan**: MASTER 12 V al calefactor, SLAVE 8 V a los motores, la
tira colgada del USB que ya estaba. **Siguen siendo dos fuentes + el USB, igual que hoy.**

### 5.5 Pines

| Pin | Antes | Ahora |
|---|---|---|
| **GPIO16** | LEDs verdes | **DATOS de la tira** |
| GPIO17 | LEDs rojos | *libre* |
| GPIO14 | LEDs azules | *libre* |
| GPIO5 | LEDs blancos | *libre* |
| GPIO34 | Micrófono AO | **sigue igual** (único micrófono, a 5 V) |
| GPIO35 | Micrófono 2 (DO) | *libre* — **ese módulo no está conectado** |

GPIO16 se elige a propósito: GPIO5 y GPIO14 emiten un pulso mientras el ESP32 arranca, y un pulso
en la línea de datos deja píxeles encendidos en colores al azar hasta que el programa toma el
control. GPIO16 y GPIO17 están tranquilos; se usa el 16 y queda el 17 de reserva.

---

## 6. Qué se borra del código (la "basura")

El sistema de luces acumuló tres capas de parches. Se reescribe entero:

| Se elimina | Por qué |
|---|---|
| **Todo el canal DO** (`MIC_DO_PIN`, `golpeDO`, `ultimosFlancosDO`, `FLANCOS_DO_MAXIMO`) | **El segundo micrófono no está conectado.** Es código para hardware que no existe |
| `fuenteGolpe`, `usaAO()`, `usaDO()`, `/sonido_mixto`, `/sonido_ao`, `/sonido_do` | Sólo tenían sentido con dos micrófonos |
| `p2pBase`, `ALFA_BASE_SUBE/BAJA`, `FACTOR_GOLPE` | La base exponencial se ciega (§3.6). La reemplaza el promedio del último segundo |
| `LED_VERDE/ROJO/AZUL/BLANCO`, `prenderTodasLasLuces()`, `apagarTodasLasLuces()` | Los 8 LEDs se van |
| La "sombra rotante" y el strobe binario | Efectos limitados por tener sólo 4 pines de encendido/apagado |
| `armarDiagnosticoMicrofono()` con su propio muestreo | Lee el ADC desde el núcleo equivocado (§7, H1) |

**Red de seguridad**: los 8 LEDs siguen existiendo en el commit anterior de git. Si mañana la tira
no anda, se recupera el sistema viejo en un minuto.

---

## 7. Auditoría del código actual — 5 hallazgos

Revisando el `.ino` completo para saber qué se rompe al sumar la tira:

### 🔴 H1 — El ADC se lee desde los dos núcleos a la vez
`/diag` ejecuta `analogRead()` **500 veces desde la tarea de Telegram (núcleo 0)** mientras
`medirSonido()` lee el mismo ADC1 desde el `loop()` (núcleo 1). Los periféricos del ESP32 deben
tener un solo dueño o estar protegidos por un mutex; compartir el ADC entre núcleos da lecturas
corrompidas y puede colgar el driver.
**Arreglo**: `/diag` deja de medir por su cuenta y **reporta** lo que ya calculó el núcleo 1.

### 🔴 H2 — Los comandos de Telegram encienden luces desde el núcleo 0
`/luces_on` y `/luces_off` llaman a las funciones de LEDs desde la tarea de Telegram. Con LEDs
simples es inofensivo (`digitalWrite` es atómico). **Con la tira sería fatal**: mandar datos por el
RMT desde el núcleo del WiFi, posiblemente pisando un envío en curso del núcleo 1.
**Arreglo**: los comandos sólo cambian `modoLuces`; el loop dibuja 28 ms después. Es además lo que
el propio README dice que el programa hace — el código se había desviado de su propia regla.

### 🟠 H3 — Variables compartidas entre núcleos sin `volatile`
`modoLuces`, `modoCalentador`, `trazaSonidoActiva` y `tempObjetivo` se escriben en el núcleo 0 y se
leen en el núcleo 1. Sin `volatile`, el compilador puede guardarlas en un registro y no releerlas:
un comando podría no surtir efecto hasta un reinicio. Hoy no se manifiesta por casualidad del
optimizador, no por diseño. Las del cobertor ya se corrigieron; faltaron éstas.

### 🟠 H4 — La base adaptativa se ciega con música sostenida
Ya diagnosticado en la bitácora. **Arreglo en §3.6.**

### 🟡 H5 — Un reinicio por caída de tensión sería invisible
Si la tira consume de más y hace caer los 5 V, el ESP32 se reinicia por *brownout* y arranca como
si nada: calentador en OFF, luces en OFF, y nadie sabe por qué. Es el mismo problema que la tarea
de Telegram colgada, y se resuelve igual: **haciéndolo visible**.
**Arreglo**: leer `esp_reset_reason()` al arrancar e informar por Monitor Serie y en `/status` si
el último reinicio fue por caída de tensión, watchdog o panic.

---

## 8. Riesgos y plan B

| # | Riesgo | Probab. | Mitigación |
|---|---|---|---|
| 1 | Colores cambiados (chip RGB en vez de GRB) | Media | `/orden` lo corrige desde Telegram |
| 2 | Nivel lógico justo (placa con `VIN` a 5,0 V) | Baja | Parpadeo o píxeles al azar → plan B |
| 3 | **La tira ensucia la señal del micrófono** | **Media** | Ver abajo |
| 4 | Reinicio por caída de tensión | Media | El limitador lo previene; H5 lo hace visible |
| 5 | Parpadeo por interferencia WiFi ↔ RMT | Baja | La tira se dibuja sólo en el núcleo 1 |
| 6 | El zumbido de 50 Hz contamina los graves | Media | Los graves arrancan en 78 Hz; `/espectro` lo verifica |
| 7 | Si la tira no anda, quedamos sin luces | Baja | El commit anterior tiene los 8 LEDs |

**Riesgo 3 en detalle.** El micrófono es analógico y se alimenta **del mismo riel de 5 V que la
tira**. Cada cambio de brillo es un tirón de corriente que hace caer la tensión del riel, y esa
caída entra en el ADC como ruido. Contramedidas:
- Los cables de la tira salen **directo del pin `VIN`**, no del tramo de protoboard donde está el
  micrófono (topología en estrella).
- Presupuesto de corriente bajo: menos corriente, menos caída.
- **Verificación concreta**: `/diag` con la tira apagada y otra vez con `/luces_on` a full. Si el
  pico a pico en silencio salta de ~30 a más de 60, la tira está contaminando y hay que separar la
  alimentación.

**Plan B, en orden de probabilidad:**
1. **Los colores no coinciden** → `/orden`. No es falla: es otro chip.
2. **No enciende nada** → el cable de datos está en `DO` en vez de `DIN`. Se da vuelta la tira.
3. **Sólo enciende el primer píxel o hay colores al azar** → acortar el cable de datos, verificar
   los 440 Ω y que el `GND` de la tira sea el mismo punto que el del ESP32.
4. **Parpadeo aleatorio** → nivel lógico al límite: pasar la alimentación de la tira del `VIN` al
   canal **SLAVE de la fuente regulado a 4,5 V** (los motores quedan sin alimentación mientras
   dure la prueba; ya están probados). A 4,5 V el umbral cae a 3,15 V.
5. **El ESP32 se reinicia al encender las luces** → bajar `/corriente`. El aviso de H5 lo confirma.
6. **Nada de lo anterior** → volver al commit de los 8 LEDs y probar el resto del sistema.

---

## 9. Comandos de Telegram nuevos

Todos guardados en NVS (sobreviven reinicios), como `/temperatura` y `/velocidad`: **calibrar en
el taller sin recompilar ni volver a cargar**.

| Comando | Para qué | Rango |
|---|---|---|
| `/efecto N` | 1 espectro · 2 mezcla · 3 cometa · 4 arcoíris | 1–4 |
| `/leds N` | Cuántos píxeles tiene la tira conectada | 1–150 |
| `/brillo N` | Brillo máximo, en % | 0–100 |
| `/corriente N` | Presupuesto de corriente, en mA | 50–2000 |
| `/luces_test` | Enciende rojo, verde, azul y blanco uno por uno, anunciando cuál toca | — |
| `/orden` | Alterna GRB ↔ RGB si los colores salen cambiados | — |
| `/espectro` | Muestra el nivel de las 3 bandas y la frecuencia dominante | — |
| `/onda` | Vuelca 256 muestras crudas del micrófono por el Monitor Serie | — |

**`/luces_test` es la herramienta clave de mañana**: anuncia por Telegram qué color está mostrando.
Si dice "ROJO" y se ve verde, el chip usa otro orden de bytes → `/orden`, en el momento y sin tocar
la PC. También confirma cuántos píxeles responden de verdad.

**`/espectro` es la segunda**: dice si el micrófono está entregando espectro útil, si hay zumbido de
50 Hz y si el AGC está trabajando bien. Es el `/diag` de la era del espectro.

---

## 10. Lo que NO entra en esta carga

- **Histéresis del calentador (pendiente 4)**: es un cambio independiente y mañana ya hay muchas
  variables nuevas. Queda anotado en PENDIENTES para la carga siguiente.
- **Los fines de carrera del cobertor**: siguen pendientes de conectar, sin relación con las luces.
- **Capacitor de 1000 µF en la tira**: mejora recomendada por Adafruit, no crítica con 15 píxeles.
  A PENDIENTES.
