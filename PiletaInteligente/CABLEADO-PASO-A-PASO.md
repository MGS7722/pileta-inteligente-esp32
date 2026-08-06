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

> ### 🚨 Si venís a montar el COBERTOR (motores), leé esto primero
> **Cambiaron 4 pines** respecto de la versión anterior de este documento. Los motores y los
> fines de carrera se mudaron a pines que no hacen nada raro mientras el ESP32 arranca:
>
> | Qué | Antes | **Ahora** | Por qué |
> |---|---|---|---|
> | L298N **ENB** (motor B) | GPIO14 | **GPIO18** | GPIO14 tira un pulso al arrancar: podía hacer que el motor B pegue un tirón en cada encendido |
> | **Fin de carrera ABIERTO** | GPIO5 | **GPIO19** | GPIO5 es pin de arranque: la posición de la lona no debe influir en cómo arranca el chip |
> | LEDs **azules** | GPIO18 | **GPIO14** | Un LED que parpadea al arrancar no molesta a nadie |
> | LEDs **blancos** | GPIO19 | **GPIO5** | Ídem |
>
> **Hay que cargar el sketch nuevo** (`PiletaInteligente.ino`) para que estos pines valgan.

---

## 📋 Mapa rápido de pines (para chequear de un vistazo)

| Pin | Va conectado a |
|---|---|
| GPIO4 | DS18B20 (dato) + resistencia 4.7kΩ a 3.3V |
| GPIO26 | Relé — IN |
| GPIO21 / GPIO22 | LCD — SDA / SCL |
| GPIO16 | LEDs verdes · GPIO17 LEDs rojos · **GPIO14** LEDs azules · **GPIO5** LEDs blancos |
| GPIO34 | Sensor de sonido 1 — AO (módulo a 5V) |
| GPIO35 | Sensor de sonido 2 — DO (módulo a 3.3V) |
| GPIO13 / GPIO25 / GPIO27 | L298N — IN1 / IN2 / ENA (motor A) |
| GPIO32 / GPIO33 / **GPIO18** | L298N — IN3 / IN4 / ENB (motor B) |
| GPIO23 | Fin de carrera CERRADO |
| **GPIO19** | Fin de carrera ABIERTO |

---

## ⚡ Cómo se alimenta todo (leer antes de tocar la fuente)

Hay **tres alimentaciones independientes**. No se mezclan entre sí:

| Alimenta | A quién | Cuánto |
|---|---|---|
| **USB de la notebook** | El **ESP32** — y a través suyo el LCD, el lado de control del relé y los sensores, que consumen poquito | 5V |
| **Fuente MASTER** | El **cartucho calefactor**, a través del relé | 12V |
| **Fuente SLAVE** | El **L298N**, y de ahí los 2 motores del cobertor | ~8V |

Lo **único** que se comparte es el **GND entre el ESP32 y el L298N**: es lo que les permite
entenderse las señales. El circuito de 12V del calefactor no comparte nada — va completamente
aparte (fuente → relé → cartucho → fuente).

```
   Notebook ──USB──► ESP32 ──señales──► L298N ──► Motor A y Motor B
                       │                  ▲
                       └──── GND común ───┘        ▲
                                                   │
                              Fuente SLAVE ~8V ────┘

   Fuente MASTER 12V ──► relé ──► cartucho calefactor ──► vuelve a la fuente
                                  (circuito cerrado sobre sí mismo, aparte de todo)
```

---

## PASO 0 — Preparar los rieles de la protoboard

1. **Cable** → del pin **GND** del ESP32 → a la fila azul (−) de la protoboard. *(este es tu riel de GND)*
2. **Cable** → del pin **3V3** del ESP32 → a la fila roja (+) de la protoboard. *(riel de 3.3V)*

A partir de acá, "riel GND" = fila azul, y "riel 3.3V" = fila roja.

> 💡 **Cómo funciona la protoboard (evita el 90% de las dudas):** cada fila de 5 agujeros está
> conectada por dentro, así que **los 5 agujeros de una fila son el mismo punto eléctrico**.
> Cuando un pin del ESP32 tiene que ir a varios lados —por ejemplo el **VIN (5V)**, que alimenta
> el LCD, el relé y el sensor de sonido 1— no hay que amontonar tres cables en el mismo
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

## PASO 4 — Sensor de sonido 1 (volumen, canal analógico)

El módulo tiene 4 pines: **VCC (o +), GND (o G), DO y AO**. De **este** módulo usamos el **AO**;
su DO queda sin conectar.

17. **Cable** → del pin **VCC/+** del sensor → al pin **VIN (5V)** del ESP32.
    *(este módulo pide 4-6V según su especificación; su salida AO es de nivel bajo
    y no daña al GPIO34 — verificado con /diag: el Máximo debe quedar < 3000)*
18. **Cable** → del pin **GND/G** del sensor → al **riel GND**.
19. **Cable** → del pin **AO** del sensor → al pin **GPIO34** del ESP32.

---

## PASO 5 — Sensor de sonido 2 (detector de golpes por hardware)

Es el módulo de sonido de repuesto (KY-037). ⚠️ Este va a **3.3V, NUNCA a 5V**
(su salida DO llega al ESP32 y más de 3.3V lo dañaría).

20. **Cable** → del pin **VCC/+** del módulo 2 → al **riel 3.3V**.
21. **Cable** → del pin **GND/G** del módulo 2 → al **riel GND**.
22. **Cable** → del pin **DO** del módulo 2 → al pin **GPIO35** del ESP32.
    *(el pin AO del módulo 2 queda SIN conectar)*

> El potenciómetro de ESTE módulo sí importa: fija el umbral del detector.
> Cómo calibrarlo está en `PROTOCOLO-TALLER.md`.

---

## PASO 6 — Las 8 luces LED ⏸️ *(en pausa: se reemplazan por la tira WS2812)*

> Los 8 LEDs están **desconectados** a propósito: los reemplaza una tira WS2812 que todavía
> no llegó. Este paso queda acá por si hay que volver a armarlos mientras tanto — **ojo que
> el azul y el blanco cambiaron de pin**.
>
> Cuando llegue la tira: **alimentación propia de 5V**, nunca desde el ESP32; sólo se comparte
> el **GND**.

Son 8 LEDs: por cada color hay 2 (uno para cada lado de la pileta). Cada LED lleva
**su propia resistencia de 220Ω** en la pata larga (+).

**Regla para CADA LED:** la pata larga va a una resistencia 220Ω, y de la resistencia
sale un cable al pin del ESP32 que le toca por color. La pata corta va al riel GND.

| Color | Pin | LEDs |
|---|---|---|
| Verdes | **GPIO16** | lado A y lado B, cada uno con su 220Ω |
| Rojos | **GPIO17** | ídem |
| Azules | **GPIO14** ⬅️ cambió | ídem |
| Blancos | **GPIO5** ⬅️ cambió | ídem |

---

## PASO 7 — Cobertor: el driver L298N

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

## PASO 8 — Cobertor: los 2 fines de carrera (sensores de tope)

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

## PASO 9 — Alimentar el ESP32

40. **Cable USB** → del ESP32 → a la notebook. *(así lo programás y ves el Monitor Serie)*

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
| `/motor_a` | Mueve **sólo el motor A** (el del rodillo) durante 2 segundos y frena solo |
| `/motor_b` | Mueve **sólo el motor B** (el que tira de los cables) durante 2 segundos y frena solo |

6. Mandá **`/motor_a`**. Tiene que girar **sólo** el motor A. Anotá para qué lado gira.
7. Mandá **`/motor_b`**. Tiene que girar **sólo** el motor B. Anotá para qué lado gira.

**Qué mirar:**
- **No gira ninguno** → revisá los jumpers de ENA/ENB (¿los sacaste?), el GND común (paso 31)
  y que la fuente SLAVE esté realmente dando 8V.
- **Gira el que no era** → tenés cruzados los cables de señal: IN1/IN2/ENA son del motor A,
  IN3/IN4/ENB son del motor B.
- **Gira muy despacio o le cuesta arrancar** → es normal: el programa los mueve al 70% de
  velocidad y el L298N se come ~2V. Si con la lona puesta no llega a arrancar, subí
  `VELOCIDAD_COBERTOR` de 180 a 255 en el `.ino`.
- **Gira para el lado equivocado** → invertí **los dos cables de ese motor** en el L298N
  (OUT1↔OUT2 para el A, OUT3↔OUT4 para el B). No toques el código.

**Cuál es el lado correcto:**
- **Motor A** tiene que girar en el sentido que **enrolla la lona en el rodillo** (= abrir).
- **Motor B** tiene que girar en el sentido que **tira de los cables** (= cerrar).

### 4. Recién ahora, el movimiento completo
8. Con los motores todavía desacoplados, probá **`/cobertor_abrir`**. Tiene que arrancar el
   motor A y quedarse hasta que aprietes el fin de carrera ABIERTO con el dedo (o hasta que
   corte solo a los 30 segundos por seguridad).
9. Apretá el fin de carrera **ABIERTO**: el motor tiene que frenar en el acto y te tiene que
   llegar el aviso *"Cobertor ABIERTO ✅"* por Telegram.
10. Lo mismo con **`/cobertor_cerrar`** y el fin de carrera **CERRADO**.
11. Probá **`/cobertor_parar`** en medio de un movimiento: tiene que frenar al instante.

Si los 4 puntos anteriores dan bien, el sistema de motores está verificado y recién ahí
conviene acoplar los motores al mecanismo.

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
- [ ] ¿El sensor de sonido **1** está en **VIN (5V)** y el **2** en **3.3V**?
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
