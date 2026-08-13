# 🔧 Cableado paso a paso — cable por cable (a prueba de errores)

Esta guía dice **cada cable**: de dónde sale y a dónde llega. Si seguís la lista en orden,
no te podés equivocar. Cada línea es UN cable.

> **Símbolos:**
> - `→` significa "un cable desde ... hasta ..."
> - "pata larga" del LED = la más larga = **positivo (+)**
> - "pata corta" del LED = la más corta = **negativo (−)**
>
> **Antes de empezar:** ESP32 pinchado en el medio de la protoboard. Conviene usar las dos
> filas de los costados de la protoboard como **riel de GND (−)** y **riel de 3.3V (+)**.

> ### 🚨 Qué cambió en esta versión (2026-08-07)
>
> **Las luces son ahora una tira WS2812**, no 8 LEDs sueltos:
>
> | Qué | Antes | **Ahora** |
> |---|---|---|
> | Luces | 8 LEDs en 4 pines (GPIO16/17/14/5) | **Tira WS2812 en GPIO16**, un solo cable de datos |
> | Micrófonos | 2 (AO en GPIO34 + DO en GPIO35) | **1 solo** (AO en GPIO34). GPIO35 queda libre |
> | Cómo reacciona | Al volumen | **Al espectro**: graves, medios y agudos por separado |
>
> Y de la revisión anterior, para el **cobertor**: `ENB` se mudó de GPIO14 a **GPIO18**, y el
> **fin de carrera ABIERTO** de GPIO5 a **GPIO19** — porque esos dos pines tiran un pulso
> mientras el ESP32 arranca, y del otro lado hay motores.
>
> **Hay que cargar el sketch nuevo** (`PiletaInteligente.ino`) para que todo esto valga.

---

## 📋 Mapa rápido de pines (para chequear de un vistazo)

| Pin | Va conectado a |
|---|---|
| GPIO4 | DS18B20 (dato) + resistencia 4.7kΩ a 3.3V |
| GPIO26 | Relé — IN |
| GPIO21 / GPIO22 | LCD — SDA / SCL |
| **GPIO16** | **Tira WS2812 — DIN (con 440Ω en serie)** ⬅️ nuevo |
| GPIO34 | Micrófono — AO (módulo a 5V) |
| GPIO13 / GPIO25 / GPIO27 | L298N — IN1 / IN2 / ENA (motor A) |
| GPIO32 / GPIO33 / **GPIO18** | L298N — IN3 / IN4 / ENB (motor B) |
| GPIO23 | Fin de carrera CERRADO |
| **GPIO19** | Fin de carrera ABIERTO |
| GPIO17 · GPIO14 · GPIO5 · GPIO35 | *libres* (eran de los 8 LEDs y del 2do micrófono) |

---

## ⚡ Cómo se alimenta todo (leer antes de tocar la fuente)

Hay **tres alimentaciones independientes**. No se mezclan entre sí:

| Alimenta | A quién | Cuánto |
|---|---|---|
| **USB de la notebook** | El **ESP32** — y a través suyo el LCD, el relé, el micrófono y **la tira de luces** | 5V |
| **Fuente MASTER** | El **cartucho calefactor**, a través del relé | 12V |
| **Fuente SLAVE** | El **L298N**, y de ahí los 2 motores del cobertor | ~8V |

Lo **único** que se comparte es el **GND entre el ESP32 y el L298N**: es lo que les permite
entenderse las señales. El circuito de 12V del calefactor no comparte nada — va completamente
aparte (fuente → relé → cartucho → fuente).

```
   Notebook ──USB──► ESP32 ──señales──► L298N ──► Motor A y Motor B
                       │                  ▲
                       ├──5V y datos──► TIRA WS2812
                       │                  ▲
                       └──── GND común ───┘
                                                   ▲
                              Fuente SLAVE ~8V ────┘

   Fuente MASTER 12V ──► relé ──► cartucho calefactor ──► vuelve a la fuente
                                  (circuito cerrado sobre sí mismo, aparte de todo)
```

⚠️ **El riel de 5V está cargado.** Ese único USB alimenta al ESP32 (150–250 mA), al LCD
(~30 mA), al relé cuando está activo (~75 mA) y al micrófono (~5 mA): entre 260 y 460 mA
**antes** de sumar la tira. Un USB 2.0 entrega 500 mA. Por eso la tira va con el limitador de
corriente en `/corriente 120` mientras estés con la notebook. Con un cargador de celular de 2A
podés subirlo a 500.

💡 **Los cables de la tira, directo al pin `VIN`**, no al tramo de protoboard donde está el
micrófono. La tira pega tirones de corriente en cada cambio de brillo, y si comparte camino con
el micrófono esos tirones entran en la medición como ruido.

---

## PASO 0 — Preparar los rieles de la protoboard

1. **Cable** → del pin **GND** del ESP32 → a la fila azul (−) de la protoboard. *(este es tu riel de GND)*
2. **Cable** → del pin **3V3** del ESP32 → a la fila roja (+) de la protoboard. *(riel de 3.3V)*

A partir de acá, "riel GND" = fila azul, y "riel 3.3V" = fila roja.

> 💡 **Cómo funciona la protoboard (evita el 90% de las dudas):** cada fila de 5 agujeros está
> conectada por dentro, así que **los 5 agujeros de una fila son el mismo punto eléctrico**.
> Cuando un pin del ESP32 tiene que ir a varios lados —por ejemplo el **VIN (5V)**, que alimenta
> el LCD, el relé, el micrófono y la tira— no hay que amontonar cuatro cables en el mismo
> agujero: se usan los otros agujeros **de la misma fila**. Los dos rieles largos de los
> costados funcionan igual, pero a lo largo de toda la placa.

---

## PASO 1 — Sensor de temperatura DS18B20 (el del calentador)

El DS18B20 tiene 3 cables: **rojo, negro y amarillo** (o blanco).

3. **Cable** → del cable **ROJO** del sensor → al **riel 3.3V**.
4. **Cable** → del cable **NEGRO** del sensor → al **riel GND**.
5. **Cable** → del cable **AMARILLO** del sensor → al pin **GPIO4** del ESP32.
6. **Resistencia 4.7kΩ** → una pata en **GPIO4** (mismo punto que el cable amarillo) y la otra pata en el **riel 3.3V**. *(sin esto el sensor lee error)*

---

## PASO 2 — Pantalla LCD (con el módulo I2C soldado atrás)

El módulo azul detrás del LCD tiene 4 pines: **GND, VCC, SDA, SCL**.

7. **Cable** → del pin **GND** del LCD → al **riel GND**.
8. **Cable** → del pin **VCC** del LCD → al pin **VIN (5V)** del ESP32. *(el LCD necesita 5V)*
9. **Cable** → del pin **SDA** del LCD → al pin **GPIO21** del ESP32.
10. **Cable** → del pin **SCL** del LCD → al pin **GPIO22** del ESP32.

---

## PASO 3 — Módulo relé (el que prende el calentador)

El módulo relé tiene 3 pines de control: **VCC, GND, IN**.

11. **Cable** → del pin **VCC** del relé → al pin **VIN (5V)** del ESP32.
12. **Cable** → del pin **GND** del relé → al **riel GND**.
13. **Cable** → del pin **IN** del relé → al pin **GPIO26** del ESP32.

**El lado de fuerza del relé (los 3 tornillos: COM, NO, NC):**

14. **Cable** → del **+ (rojo)** de la fuente MASTER (12V) → al tornillo **COM** del relé.
15. **Cable** → del tornillo **NO** del relé → a un extremo del **cartucho calefactor**.
16. **Cable** → del otro extremo del **cartucho calefactor** → al **− (negro)** de la fuente MASTER (12V).

> Así el relé "abre y cierra" el circuito del calefactor. El calefactor NUNCA toca el ESP32.

⚠️ **Tres cosas que no se negocian en este paso:**
- **El relé va del lado del positivo** (pasos 14-15). Con el relé abierto, el cartucho queda
  sin tensión. Si se cortara el negativo, quedaría con +12V permanentes y un contacto
  accidental con masa lo encendería solo — peligroso con el cartucho sumergido.
- **NO, nunca NC.** NC está cerrado en reposo: el calefactor arrancaría prendido y seguiría
  prendido si el ESP32 se cuelga. Con NO, el estado de reposo es apagado.
- **El negro de la fuente va directo al cartucho, NO al riel GND de la protoboard.** El
  circuito de 12V se cierra sobre sí mismo; el relé ya aísla los dos mundos. Estos tres
  cables van gruesos y directos a los tornillos, nunca por la protoboard.

---

## PASO 4 — El micrófono (KY-037)

El módulo tiene 4 pines: **VCC (o +), GND (o G), DO y AO**. Usamos el **AO**; el **DO queda
sin conectar** y el potenciómetro del módulo **no hace falta tocarlo** (sólo afecta al DO).

17. **Cable** → del pin **VCC/+** del sensor → al pin **VIN (5V)** del ESP32.
    *(el módulo pide 4-6V según su especificación; su salida AO es de nivel bajo
    y no daña al GPIO34 — verificado con /diag: el Máximo debe quedar < 3000)*
18. **Cable** → del pin **GND/G** del sensor → al **riel GND**.
19. **Cable** → del pin **AO** del sensor → al pin **GPIO34** del ESP32.

> **Es un solo micrófono.** El segundo módulo (el que iba por DO al GPIO35) ya no se usa:
> el sistema analiza el espectro del micrófono analógico y de ahí saca tanto las bandas de
> frecuencia como los golpes del ritmo. GPIO35 queda libre.

---

## PASO 5 — La tira de luces WS2812 🆕

Son **3 cables**. Este paso reemplaza por completo a los 8 LEDs viejos.

### 5.1 Preparar la tira

**Cortar 21 píxeles** (70 cm en una tira de 30 LED/m: la vuelta completa a la pileta, medida
en el taller el 2026-08-13). Se corta **por las líneas de cobre** que hay entre píxel y píxel,
cada 33 mm.

> 💡 **Contá los píxeles, no midas con la regla.** Si tu tira fuera de 60 LED/m, 70 cm serían
> 42 píxeles. Lo que el programa necesita saber es la cantidad, y se la decís con `/leds N`.
>
> 💡 **Quedate con el pedazo que tiene el conector**, así no tenés que soldar nada: contá 21
> píxeles desde el conector y cortá en la línea siguiente.
>
> 💡 Si esa línea de corte está fea o soldada, **pasá a la siguiente**. El número exacto no
> importa: `/leds 22` y listo. Lo que **nunca** hay que hacer es cortar por el medio de un
> píxel.

**Fijate por dónde entran los datos.** La tira tiene dirección: un extremo dice **`DIN`** (o
`DI`) y el otro **`DO`**. Las **flechas** impresas apuntan hacia donde va la señal. El cable de
datos va del lado de **`DIN`**.
> Si la conectás al revés no enciende nada, pero **no se rompe**: se da vuelta y listo.

### 5.2 Los 3 cables

20. **Cable** → del cable **`5V` (rojo)** de la tira → al pin **VIN (5V)** del ESP32.
21. **Cable** → del cable **`GND` (blanco o negro)** de la tira → al **riel GND**.
22. **Dos resistencias de 220Ω en serie** (una después de la otra = 440Ω) entre el pin
    **GPIO16** del ESP32 y el cable **`DIN` (verde)** de la tira.

```
   GPIO16 ──[220Ω]──[220Ω]──► DIN (verde)   de la tira
   VIN 5V ─────────────────► 5V  (rojo)
   riel GND ───────────────► GND (blanco/negro)
```

⚠️ **Tres cosas de este paso:**

- **La resistencia no es opcional.** Adafruit especifica 300–500Ω entre el pin del micro y el
  DIN del primer píxel: protege esa entrada de los picos de conmutación. Dos de 220Ω en serie
  dan 440Ω, justo en el rango.
- **El cable de datos, CORTO** (20–30 cm). Es la causa número uno de fallas: cuanto más largo,
  peor le llega la señal de 3,3V a una tira de 5V.
- **Cableá con el USB desenchufado.** Adafruit especifica conectar primero GND, después +5V y
  último los datos; con todo saliendo del mismo USB, alcanza con no tener corriente encima.

### 5.3 Por qué la tira puede colgarse del ESP32 (y no necesita otra fuente)

Son dos problemas distintos y los dos están resueltos:

**El de la señal.** La tira necesita 0,7 × su alimentación para leer un "1", y el ESP32 sólo da
3,3V. Pero ese umbral **baja con la alimentación**: el pin `VIN` no entrega 5,0V sino ~4,7V,
porque la placa tiene un diodo en el medio. A 4,7V el umbral queda en **3,29V** y el ESP32 llega.

> 🔎 **Medí el `VIN` con el multímetro** (contra GND, con el USB puesto). Si da 4,6–4,8V, todo
> en orden. Si da 5,0V clavados, tu placa no tiene ese diodo: anda igual pero justo, y si ves
> parpadeo mirá el plan B del final.

**El del consumo.** 21 píxeles en blanco pleno serían 1,3 A, imposible. Por eso **el programa
lleva un limitador**: antes de mandar cada cuadro calcula lo que va a consumir y, si se pasa del
presupuesto, baja el brillo hasta que entre. La tira **no puede** pasarse.

| Cómo tengas el ESP32 | Comando |
|---|---|
| Enchufado al USB de la notebook | `/corriente 120` |
| En un cargador de celular de 2A | `/corriente 500` |

**Las fuentes del proyecto no cambian**: MASTER 12V al calefactor, SLAVE 8V a los motores.

---

## PASO 6 — Cobertor: el driver L298N

**La placa de arriba abajo** (la roja con el disipador de aluminio en el medio):

```
         ┌──────────────────────────────────────────────┐
  Motor  │ OUT1                                    OUT3 │  Motor
    A    │ OUT2        ▐▌ disipador ▐▌             OUT4 │    B
         │                                              │
         │   ▪ jumper del regulador de 5V (DEJAR)        │
         │  ┌──────┬──────┬──────┐   ▪               ▪  │
         │  │ +12V │ GND  │ +5V  │  ENA IN1 IN2 IN3 IN4 ENB
         └──┴──────┴──────┴──────┴──────────────────────┘
              ▲ alimentación (3 tornillos)   ▲ control (6 pines)
```

- **2 borneras de 2 tornillos, una a cada costado** → los motores (OUT1/OUT2 y OUT3/OUT4).
- **1 bornera de 3 tornillos** → toda la alimentación. **El del medio siempre es GND.**
- **1 tira de 6 pines** rotulada `ENA IN1 IN2 IN3 IN4 ENB` → las señales del ESP32.

⚠️ **Antes de cablear, dos jumpers:**
- **Sacá los 2 jumpers de ENA y ENB** (los capuchones plásticos). Con el jumper puesto el
  motor está siempre habilitado y no hay control de velocidad.
- **Dejá puesto el jumper del regulador de 5V** (el que está al lado de los bornes). Con él
  puesto, el módulo se fabrica sus propios 5V para la lógica. *Sólo se saca si alimentás el
  L298N con más de 12V — nosotros usamos 8V, así que va puesto.*

### Cables de señal (del ESP32 al L298N)
23. **Cable** → **GPIO13** del ESP32 → pin **IN1** del L298N.
24. **Cable** → **GPIO25** del ESP32 → pin **IN2** del L298N.
25. **Cable** → **GPIO27** del ESP32 → pin **ENA** del L298N.
26. **Cable** → **GPIO32** del ESP32 → pin **IN3** del L298N.
27. **Cable** → **GPIO33** del ESP32 → pin **IN4** del L298N.
28. **Cable** → **GPIO18** del ESP32 → pin **ENB** del L298N. ⬅️ **cambió** (antes GPIO14)

### Alimentación del L298N — la bornera de 3 tornillos

Toda la alimentación del módulo entra por **una sola bornera de 3 tornillos**. Están rotulados
en la placa y **el del medio siempre es GND**:

```
        ┌─────────┬─────────┬─────────┐
        │  +12V   │   GND   │   +5V   │   ← rotulado en la placa
        └────┬────┴────┬────┴────┬────┘
             │         │         └──  NO SE CONECTA NADA (es una SALIDA)
             │         │
             │         └──  DOS cables juntos en este mismo tornillo:
             │                 1) el − (negro) de la fuente SLAVE
             │                 2) un cable al riel GND de la protoboard
             │
             └──  el + (rojo) de la fuente SLAVE (8V)
```

**Son sólo 2 tornillos con cable, y en uno de ellos entran 2 cables.** Por eso la cuenta de
"3 tornillos = 3 cables" no cierra: el tercer tornillo (+5V) queda vacío.

29. **Cable** → **+ (rojo)** de la fuente SLAVE (8V) → al tornillo **+12V**.
    *(el tornillo se llama "+12V" pero acepta de 7 a 12V: le ponemos 8V)*
30. **Cable** → **− (negro)** de la fuente SLAVE (8V) → al tornillo **GND**.
31. **Cable** → del **mismo tornillo GND** (junto al anterior) → al **riel GND** de la
    protoboard.

Este último cable es el **GND común** y es imprescindible: sin él los motores no arrancan.

> 💡 **Cómo entran los dos cables en el tornillo:** pelá 8-10 mm de cada uno, juntá las dos
> puntas y torcelas como si fueran un solo cable, y recién ahí apretá el tornillo. Después
> tirá suave de cada uno **por separado** para confirmar que ninguno quedó sólo apoyado.
>
> Si en tu módulo no entran los dos, se puede sacar el cable 31 del **borne − de la fuente**
> en vez del tornillo: como el tornillo y el borne ya están unidos por el cable 30, son el
> mismo punto eléctrico y da exactamente lo mismo.

> ### ⚠️ El cable 31 no es de alimentación: es el que hace que se entiendan
> El ESP32 está alimentado por **USB** y el L298N por **la fuente**: cada uno tiene su propio
> cero. Cuando el ESP32 manda una señal de 3,3V, la cuenta desde SU cero; si el L298N escucha
> contando desde OTRO cero, no interpreta nada.
>
> ```
>    ESP32 ──señal de 3,3V──► L298N        ¿desde qué cero mide cada uno?
>      │                        │
>      └── USB (cero A)         └── fuente (cero B)     ← sin unir: no funciona
> ```

> 🚫 **Lo que NO hay que hacer: llevar el negro de la fuente al riel de la protoboard y de ahí
> al L298N.** Parece lo mismo, pero así toda la corriente de los motores (más de 1A en cada
> arranque) pasaría por el riel de la protoboard, que no está hecho para eso, y ese ruido se
> cuela en la lectura del micrófono. La corriente de los motores va **directo de la fuente al
> módulo**; el cable 31 lleva sólo la referencia.

> 🚫 **El tornillo +5V no se conecta a nada.** Es una salida que fabrica el propio módulo. Si lo
> enchufás al 5V/VIN del ESP32 mientras el ESP32 está con el USB, quedan dos fuentes de 5V
> peleándose y podés quemar el regulador de la placa. El ESP32 se alimenta por USB y punto.

### Los 2 motores del cobertor
32. **Cable** → borne **OUT1** del L298N → a un cable del **Motor A** (el del rodillo).
33. **Cable** → borne **OUT2** del L298N → al otro cable del **Motor A**.
34. **Cable** → borne **OUT3** del L298N → a un cable del **Motor B** (el que tira los cables).
35. **Cable** → borne **OUT4** del L298N → al otro cable del **Motor B**.

> No importa cuál cable del motor va a cuál borne: eso sólo decide para qué lado gira, y se
> corrige después dando vuelta los dos cables. Lo verificamos en la prueba de más abajo.
>
> 💡 Los motores tienen cables finos: apretá bien los tornillos y tirá suave de cada cable
> para confirmar que quedó agarrado. Un cable que se suelta en movimiento deja el motor
> trabado con la lona a medio camino.

---

## PASO 7 — Cobertor: los 2 fines de carrera (sensores de tope)

Los fines de carrera suelen traer **3 patas** marcadas **COM**, **NO** y **NC**. Usamos
**COM** y **NO** (la pata **NC** queda libre).

- **COM** = común · **NO** = normal abierto (se cierra al apretar la palanca) · **NC** = normal
  cerrado (se abre al apretar).
- Si tu fin de carrera tiene sólo 2 patas, es un interruptor simple y no importa el orden.

36. **Cable** → pata **COM** del **fin de carrera CERRADO** → al **riel GND**.
37. **Cable** → pata **NO** del **fin de carrera CERRADO** → al pin **GPIO23** del ESP32.
38. **Cable** → pata **COM** del **fin de carrera ABIERTO** → al **riel GND**.
39. **Cable** → pata **NO** del **fin de carrera ABIERTO** → al pin **GPIO19** del ESP32. ⬅️ **cambió** (antes GPIO5)

> No llevan resistencia: el ESP32 usa su resistencia interna (ya está en el código).
>
> ✅ **Cómo comprobar que quedaron bien**, sin mover ningún motor: mandá **`/status`** por
> Telegram y mirá la línea "Cobertor". Apretá con el dedo el fin de carrera CERRADO y volvé a
> mandar `/status`: tiene que pasar de *"parado (posición intermedia)"* a *"cerrado"*. Lo mismo
> con el de ABIERTO. **Si no cambia, están en la pata equivocada** (probablemente NC en vez de NO).

---

## PASO 8 — Alimentar el ESP32

40. **Cable USB** → del ESP32 → a la notebook. *(así lo programás y ves el Monitor Serie)*

---

# 💡 PRUEBA DE LAS LUCES (hacer esto PRIMERO, antes que los motores)

Es lo más nuevo del sistema, así que se prueba primero y solo: si algo falla, que falle sin
motores ni calefactor encima confundiendo el diagnóstico.

### 1. Antes de enchufar

1. Repasá que la tira tenga sus **3 cables** (paso 5.2) y que el de datos entre por **`DIN`**.
2. Repasá que estén las **dos resistencias de 220Ω en serie** entre GPIO16 y el `DIN`.
3. **Ahora sí**, enchufá el USB.

### 2. Medir el VIN (30 segundos, evita el 90% de los dolores de cabeza)

4. Con el multímetro en tensión continua: punta roja en **`VIN`**, punta negra en **`GND`**.

| Lectura | Qué significa |
|---|---|
| **4,2 – 4,8 V** | ✅ Perfecto. La señal de 3,3V del ESP32 alcanza con margen |
| **5,0 V clavados** | ⚠️ Tu placa no tiene el diodo. Anda igual pero justo: si después ves parpadeo, andá al plan B |
| **Menos de 4,0 V** | ❌ El USB no está dando bien. Probá otro cable u otro puerto |

> 📏 **Medido el 2026-08-13 en la placa real: 4,4 V**, y se mantuvo con las luces encendidas.
> Cuanto **más baja** esté esa tensión (dentro de lo razonable), **mejor** anda la señal: la
> tira exige 0,7 × su alimentación, así que a 4,4 V el umbral es 3,08 V y el ESP32, que da
> 3,3 V, pasa con 0,22 V de margen. A 4,7 V el umbral sería 3,29 V y el margen, apenas 0,01 V.
> **No cambies el cable USB por uno "mejor" para subir esa tensión: la empeorarías.**

### 3. Cargar el programa

5. Abrí `PiletaInteligente.ino` y cargalo (**necesita 7 librerías**, ver el README — se sumaron
   *Adafruit NeoPixel* y *arduinoFFT*).
6. Abrí el **Monitor Serie a 115200**. Lo primero que imprime es **por qué arrancó**:
   - `Motivo del ultimo arranque: encendido normal.` → todo bien.
   - `*** CAIDA DE TENSION (brownout) ***` → la alimentación no alcanzó. Bajá `/corriente`.

### 4. Probar la tira color por color

7. Por Telegram, mandá **`/corriente 120`** (si estás con la notebook).
8. Mandá **`/luces_test`**. La tira muestra, 1,2 segundos cada uno:
   **ROJO → VERDE → AZUL → BLANCO**.

Mirá **dos cosas**:

| Qué ves | Qué significa | Qué hacer |
|---|---|---|
| Los 4 colores, en ese orden, en todos los píxeles | ✅ Todo bien | Seguí al punto 9 |
| Los colores no coinciden (dice ROJO y ves VERDE) | Tu tira usa otro orden de bytes | `/orden` y repetí `/luces_test` |
| Se encienden menos píxeles de los que cortaste | El programa cree que hay otra cantidad | `/leds 21` (o los que tengas) y repetí |
| **No enciende nada** | Casi seguro el cable de datos está en `DO` en vez de `DIN` | Dá vuelta la tira |
| Sólo el primer píxel, o colores al azar | La señal de datos llega mal | Acortá el cable de datos; revisá los 440Ω y el GND |
| El ESP32 se reinicia | Consumo excesivo | `/corriente 80` y probá de nuevo |

9. Mandá **`/leds 21`** (o la cantidad real que hayas cortado) y repetí `/luces_test` para
   confirmar que responden todos.

### 5. Verificar que la tira no ensucie el micrófono

Este es el chequeo que más me importa: la tira y el micrófono comparten el riel de 5V.

10. Con **silencio** en el taller y las luces apagadas (`/luces_off`), mandá **`/diag`**.
    Anotá el número de **PICO A PICO** (debería estar entre 9 y 36).
11. Mandá **`/luces_on`** y, con el mismo silencio, mandá **`/diag`** otra vez.

| Resultado | Qué significa |
|---|---|
| El pico a pico casi no cambió | ✅ Perfecto |
| Subió a más de 60 | ⚠️ La tira está contaminando la medición: pasá sus cables directo al pin `VIN`, separados del micrófono |

### 6. Calibrar el piso de ruido (una vez por lugar, 1 minuto)

Es el único ajuste que depende de dónde esté instalada la pileta: el ruido de fondo de un
taller con gente hablando no es el de un patio de noche. Sin esto, las bandas sin música se
llenan con el ruido del micrófono y la tira baila sola.

12. Con el lugar **en silencio**, mandá **`/espectro`** y mirá la columna **CRUDO**.
13. Mandá **`/piso N`** con un número **un poco por encima del CRUDO más alto** que hayas
    visto. *(En el taller, en silencio, las tres bandas daban 7,9 – 9,4 → `/piso 12`.)*
14. Verificá: **`/audio`** en silencio tiene que mostrar las tres bandas en **0 o casi**, y
    arriba decir **"silencio"**. Si alguna sigue alta, subí el piso de a 2.

> El otro umbral, el de "hay música" (pico a pico 90), está fijo en el código. `/diag` te dice
> si quedó bien: si el **rango de los últimos 10 segundos** en silencio ya lo supera, avisa solo.

### 7. La fiesta

15. **`/luces_auto`** y poné música.
16. Probá los cuatro efectos y quedate con el que más te guste:

| Comando | Qué hace |
|---|---|
| `/efecto 1` | **ESPECTRO** — la tira en 3 zonas: graves (rojo), medios (verde), agudos (azul). Cada zona sube y baja con su banda |
| `/efecto 2` | **MEZCLA** — toda la tira de un color mezclado con las 3 bandas: mucho bajo = rojo, platos = celeste |
| `/efecto 3` | **COMETA** — recorre la tira dejando estela y rebota en cada golpe |
| `/efecto 4` | **ARCOÍRIS** — degradado que gira más rápido cuanto más fuerte suena |

17. Si te parece que reacciona poco o de más, mandá **`/espectro`** con la música sonando: te
    dice el nivel de cada banda, contra qué se está comparando y a partir de qué valor dispara
    los golpes. **`/audio`** te muestra lo mismo en barritas, más rápido de leer.
18. **`/brillo 90`** si querés más luz (fijate que `/status` te dice cuánta corriente está
    usando de la que tiene permitida).

### 8. Para afinar después (opcional, con la música sonando)

19. **`/trace`** vuelca por el Monitor Serie todo el análisis, 10 veces por segundo. Dejalo
    correr un minuto con música y guardá la salida: con esos datos se termina de calibrar.
20. **`/onda`** vuelca 256 muestras crudas del micrófono. Sirve para verificar la calidad de la
    señal y si hay zumbido de la red eléctrica metiéndose.

---

# 🧪 PRUEBA DE LOS MOTORES (hacer esto ANTES de montar el mecanismo)

El objetivo es verificar el cableado y **para qué lado gira cada motor** con los motores
**sueltos, desacoplados de la lona y del rodillo**. Si el mecanismo ya está armado y algo está
al revés, el cobertor tira para el lado equivocado y se traba.

### 1. Preparar la fuente (red de seguridad)
1. **TRACKING** en **INDEP**.
2. **SLAVE**: **8V**.
3. **CURRENT** de la SLAVE: bajala hasta casi el mínimo y después subila hasta que marque
   **~1A**. En vacío los dos motores juntos consumen menos de 0,3A, así que 1A es de sobra.
   **Si la fuente entra en límite de corriente** (se prende la luz de "CC" y la tensión se cae),
   **algo está trabado o en corto: cortá y revisá**. Esa es la protección que evita quemar el
   L298N o un motor.

### 2. Cargar el programa
4. Abrí `PiletaInteligente.ino` y cargalo al ESP32 (los pines nuevos vienen en esta versión).
5. Abrí el **Monitor Serie a 115200** para ir viendo lo que informa.

### 3. Probar cada motor por separado
Por Telegram:

| Comando | Qué hace |
|---|---|
| `/motor_a` | Mueve **sólo el motor A** durante 2 segundos y frena solo |
| `/motor_b` | Mueve **sólo el motor B** durante 2 segundos y frena solo |
| `/motor_a 5` | Lo mismo, pero eligiendo los segundos (de 1 a 60) |

6. Mandá **`/motor_a`**. Tiene que girar **sólo** el motor A. Anotá para qué lado gira.
7. Mandá **`/motor_b`**. Tiene que girar **sólo** el motor B. Anotá para qué lado gira.

> ⚠️ **Hacé esto con el hilo DESATADO.** Estos dos comandos mueven un motor y dejan el otro
> suelto; con el lazo puesto, el suelto frena por su propia reductora y traba la prueba. Peor:
> mientras los sentidos no estén alineados, cada intento pone a los motores a tirar uno contra
> el otro y el hilo se tensa hasta trabarse. Primero se alinean los sentidos, después se ata.
>
> Con el mecanismo ya armado, la prueba válida es `/tiempo_abrir 2` + `/cobertor_abrir`, que
> mueve los dos juntos.

**Qué mirar:**
- **No gira ninguno** → revisá los jumpers de ENA/ENB (¿los sacaste?), el GND común (paso 31)
  y que la fuente SLAVE esté realmente dando 8V.
- **Gira el que no era** → tenés cruzados los cables de señal: IN1/IN2/ENA son del motor A,
  IN3/IN4/ENB son del motor B.
- **Gira muy despacio o le cuesta arrancar** → el programa los mueve al **45%** de velocidad
  y el L298N se come ~2V. Se ajusta desde Telegram con **`/velocidad 60`** (en %, de 20 a
  100) y queda guardado en el ESP32: no hay que recompilar nada. Con la lona puesta hace
  falta más fuerza, así que puede que tengas que subirlo.
- **Zumba pero no gira** → la velocidad quedó demasiado baja: por debajo del 20% el motor no
  vence su propio rozamiento. Subila con `/velocidad`.
- **Gira para el lado equivocado** → **`/sentido_a`** o **`/sentido_b`**, y volvé a probar.
  Queda guardado en el ESP32. **No hay que tocar ningún cable ni recompilar.**

**Cuál es el lado correcto:** los dos motores tienen que mover el hilo **en la misma
dirección**. Ojo: eso es una condición *física*. Si los motores quedaron enfrentados, "el mismo
lado" para el hilo significa que uno gira horario y el otro antihorario — por eso se calibra
mirando el hilo, no el eje. `/status` te muestra los dos juntos:

```
Al ABRIR: motor A a la derecha (horario)
          motor B a la derecha (horario)
```

### 4. Recién ahora, el movimiento completo
8. **Atá el hilo** entre los dos ejes, ahora que los sentidos están alineados.
9. Poné un tiempo corto: **`/tiempo_abrir 2`**, y mandá **`/cobertor_abrir`**. Tienen que
   arrancar **los dos motores juntos** y frenar solos a los 2 segundos.
10. Mirá hacia dónde se movió el hilo. Si va al revés de lo que querés que sea "abrir", mandá
    **`/sentido_a` y `/sentido_b`** —los dos— para dar vuelta el conjunto entero.
11. Subí de a poco `/tiempo_abrir` hasta que el recorrido quede completo, y hacé lo mismo con
    **`/tiempo_cerrar`**. Cerrar suele necesitar un poco más: la lona pesa y el hilo roza.
12. Probá **`/cobertor_parar`** en medio de un movimiento: tiene que frenar al instante.

> 💡 **Si el motor zumba y no arranca, o arranca y se frena a mitad de camino**, es falta de
> par: subí `/velocidad`. Con el hilo pelado hizo falta llegar a `/velocidad 100`. El Monitor
> Serie te dice si el programa cortó o si el motor se frenó solo:
> `>>> Cobertor: ABIERTO — duro 10008 ms de los 10000 ms pedidos`.

⚠️ **Ya no hay corte por sensor.** Si el hilo se traba, los motores empujan hasta que se cumpla
el tiempo. La protección real es **el límite de corriente de la fuente** (~1 A) y
`/cobertor_parar`. Por eso se calibra con tiempos cortos y se va subiendo.

---

## ✅ Checklist final antes de encender

- [ ] ¿Los **GND de la lógica** están unidos? (ESP32, L298N, relé *lado control*, sensores)
      **El negativo de los 12V del calefactor NO** — ese circuito va aparte.
- [ ] ¿Está el **cable 31** (el GND común entre el mundo del ESP32 y el de la fuente SLAVE)?
      Es el que más se olvida y sin él los motores no arrancan.
      **Con téster** (todo apagado, en modo continuidad 🔊): una punta en el pin **GND del
      ESP32** y la otra en el **tornillo GND del L298N** → **tiene que pitar**. Si no pita,
      falta ese cable o está flojo.
- [ ] ¿La resistencia de **4.7kΩ** está entre GPIO4 y 3.3V?
- [ ] ¿El **micrófono** está alimentado desde **VIN (5V)** y su **AO** en **GPIO34**?
- [ ] **Tira:** ¿el cable de datos entra por **`DIN`** (el lado de las flechas) y no por `DO`?
- [ ] **Tira:** ¿están las **dos resistencias de 220Ω en serie** entre GPIO16 y el `DIN`?
- [ ] **Tira:** ¿el cable de datos es **corto** (20–30 cm)?
- [ ] **Tira:** ¿sus cables de 5V y GND salen **directo del ESP32**, sin compartir camino con
      el micrófono?
- [ ] ¿Mediste el **`VIN`** con el multímetro? (esperado 4,2–4,8 V; en la placa de Mariano: 4,4 V)
- [ ] ¿Ningún cable en los pines **SD0, SD1, SD2, SD3, CMD, CLK** (GPIO6–11)? Son de la
      memoria flash: un solo cable ahí y **el ESP32 no arranca**.
- [ ] ¿Sacaste los **jumpers ENA/ENB** del L298N y **dejaste** el jumper del regulador de 5V?
- [ ] ¿El borne **+5V del L298N** quedó **sin conectar**?
- [ ] ¿El **ENB** está en **GPIO18** y el **fin de carrera ABIERTO** en **GPIO19**? *(pines nuevos)*
- [ ] ¿Los fines de carrera están en las patas **COM** y **NO**?
- [ ] ¿La fuente **MASTER a 12V** y la **SLAVE a ~8V**, con TRACKING en **INDEP**?
- [ ] ¿La perilla **CURRENT** de la SLAVE limitada a **~1A** para la prueba de motores?
- [ ] ¿Los motores están **desacoplados** del mecanismo para la primera prueba?

Si todo esto está ✔️, enchufá el USB y abrí el Monitor Serie a **115200 baudios**.

---

## 🔌 Cómo configurar la fuente del laboratorio (FUENTE 3)

Es una fuente doble (MASTER + SLAVE), cada lado con perilla de VOLTAGE y de CURRENT.

1. Interruptor **TRACKING** del medio → en **INDEP** (las dos salidas independientes).
2. **MASTER**: girá **VOLTAGE** hasta que el display marque **12V**. Es para el calentador.
3. **SLAVE**: girá **VOLTAGE** hasta **~8V**. Es para los motores.
4. **CURRENT**: para la prueba de motores, limitá la SLAVE a **~1A** (ver arriba). Si algo
   está mal, la fuente corta sola en vez de quemar algo. Es tu red de seguridad.
5. Bornes: **🔴 rojo = +** , **⚫ negro = −** , 🟢 verde = tierra (NO se usa).
