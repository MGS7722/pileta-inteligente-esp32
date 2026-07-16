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

---

## PASO 0 — Preparar los rieles de la protoboard

1. **Cable** → del pin **GND** del ESP32 → a la fila azul (−) de la protoboard. *(este es tu riel de GND)*
2. **Cable** → del pin **3V3** del ESP32 → a la fila roja (+) de la protoboard. *(riel de 3.3V)*

A partir de acá, "riel GND" = fila azul, y "riel 3.3V" = fila roja.

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

14. **Cable** → del **+ (rojo)** de la fuente MASTER (12V) → a un extremo del **cartucho calefactor**.
15. **Cable** → del otro extremo del **cartucho calefactor** → al tornillo **COM** del relé.
16. **Cable** → del tornillo **NO** del relé → al **− (negro)** de la fuente MASTER (12V).

> Así el relé "abre y cierra" el circuito del calefactor. El calefactor NUNCA toca el ESP32.

---

## PASO 4 — Sensor de sonido (el de las luces)

El módulo de sonido tiene 3 pines: **VCC (o +), GND (o G), AO** (salida analógica).
⚠️ Si tiene un 4º pin **DO**, ese NO se usa.

17. **Cable** → del pin **VCC/+** del sensor → al **riel 3.3V**. *(a 3.3V, no a 5V)*
18. **Cable** → del pin **GND/G** del sensor → al **riel GND**.
19. **Cable** → del pin **AO** del sensor → al pin **GPIO34** del ESP32.

---

## PASO 5 — Las 8 luces LED (4 colores × 2 lados)

Son 8 LEDs: por cada color hay 2 (uno para cada lado de la pileta). Cada LED lleva
**su propia resistencia de 220Ω** en la pata larga (+).

**Regla para CADA LED:** la pata larga va a una resistencia 220Ω, y de la resistencia
sale un cable al pin del ESP32 que le toca por color. La pata corta va al riel GND.

### LEDs VERDES → van al GPIO16
20. LED verde lado A: **pata larga** → [resistencia 220Ω] → **GPIO16**.
21. LED verde lado A: **pata corta** → **riel GND**.
22. LED verde lado B: **pata larga** → [resistencia 220Ω] → **GPIO16** (mismo pin).
23. LED verde lado B: **pata corta** → **riel GND**.

### LEDs ROJOS → van al GPIO17
24. LED rojo lado A: **pata larga** → [220Ω] → **GPIO17**.
25. LED rojo lado A: **pata corta** → **riel GND**.
26. LED rojo lado B: **pata larga** → [220Ω] → **GPIO17**.
27. LED rojo lado B: **pata corta** → **riel GND**.

### LEDs AZULES → van al GPIO18
28. LED azul lado A: **pata larga** → [220Ω] → **GPIO18**.
29. LED azul lado A: **pata corta** → **riel GND**.
30. LED azul lado B: **pata larga** → [220Ω] → **GPIO18**.
31. LED azul lado B: **pata corta** → **riel GND**.

### LEDs BLANCOS → van al GPIO19
32. LED blanco lado A: **pata larga** → [220Ω] → **GPIO19**.
33. LED blanco lado A: **pata corta** → **riel GND**.
34. LED blanco lado B: **pata larga** → [220Ω] → **GPIO19**.
35. LED blanco lado B: **pata corta** → **riel GND**.

---

## PASO 6 — Cobertor: el driver L298N

El L298N tiene 6 pines de control (IN1, IN2, IN3, IN4, ENA, ENB) y bornes de fuerza.
⚠️ **Sacale los 2 jumpers de ENA y ENB** (los capuchones plásticos), porque usamos PWM.

### Cables de señal (del ESP32 al L298N)
36. **Cable** → **GPIO13** del ESP32 → pin **IN1** del L298N.
37. **Cable** → **GPIO25** del ESP32 → pin **IN2** del L298N.
38. **Cable** → **GPIO27** del ESP32 → pin **ENA** del L298N.
39. **Cable** → **GPIO32** del ESP32 → pin **IN3** del L298N.
40. **Cable** → **GPIO33** del ESP32 → pin **IN4** del L298N.
41. **Cable** → **GPIO14** del ESP32 → pin **ENB** del L298N.

### Alimentación del L298N (desde la fuente SLAVE, regulada a ~8V)
42. **Cable** → **+ (rojo)** de la fuente SLAVE (8V) → al borne **+12V** del L298N.
43. **Cable** → **− (negro)** de la fuente SLAVE (8V) → al borne **GND** del L298N.
44. **Cable** → del borne **GND** del L298N → al **riel GND** de la protoboard. *(GND común: MUY importante)*

### Los 2 motores del cobertor
45. **Cable** → borne **OUT1** del L298N → a un cable del **Motor A** (el del rodillo).
46. **Cable** → borne **OUT2** del L298N → al otro cable del **Motor A**.
47. **Cable** → borne **OUT3** del L298N → a un cable del **Motor B** (el que tira los cables).
48. **Cable** → borne **OUT4** del L298N → al otro cable del **Motor B**.

> Si al probar un motor gira al revés, das vuelta sus 2 cables (ej. OUT1↔OUT2). Sin drama.

---

## PASO 7 — Cobertor: los 2 fines de carrera (sensores de tope)

Cada fin de carrera es un interruptor con 2 patas. No importa el orden de las patas.

49. **Cable** → una pata del **fin de carrera CERRADO** → al pin **GPIO23** del ESP32.
50. **Cable** → la otra pata del **fin de carrera CERRADO** → al **riel GND**.
51. **Cable** → una pata del **fin de carrera ABIERTO** → al pin **GPIO5** del ESP32.
52. **Cable** → la otra pata del **fin de carrera ABIERTO** → al **riel GND**.

> No llevan resistencia: el ESP32 usa su resistencia interna (ya está en el código).

---

## PASO 8 — Alimentar el ESP32

53. **Cable USB** → del ESP32 → a la notebook. *(así lo programás y ves el Monitor Serie)*

---

## ✅ Checklist final antes de encender

- [ ] ¿Todos los **GND** están unidos? (ESP32, relé, L298N, fuente, sensores → todos al riel GND)
- [ ] ¿La resistencia de **4.7kΩ** está entre GPIO4 y 3.3V?
- [ ] ¿El sensor de sonido está en **3.3V** (no en 5V)?
- [ ] ¿Cada LED tiene **su** resistencia de 220Ω?
- [ ] ¿Sacaste los **jumpers ENA/ENB** del L298N?
- [ ] ¿La fuente **MASTER a 12V** y la **SLAVE a ~8V**, con TRACKING en **INDEP**?
- [ ] ¿La perilla **CURRENT** de la fuente empieza baja (media vuelta)?

Si todo esto está ✔️, enchufá el USB y abrí el Monitor Serie a **115200 baudios**.

---

## 🔌 Cómo configurar la fuente del laboratorio (FUENTE 3)

Es una fuente doble (MASTER + SLAVE), cada lado con perilla de VOLTAGE y de CURRENT.

1. Interruptor **TRACKING** del medio → en **INDEP** (las dos salidas independientes).
2. **MASTER**: girá **VOLTAGE** hasta que el display marque **12V**. Es para el calentador.
3. **SLAVE**: girá **VOLTAGE** hasta **~8V**. Es para los motores.
4. **CURRENT** (las dos): arrancá con la perilla como a la mitad. Si algo está mal, la
   fuente corta sola en vez de quemar algo. Es tu red de seguridad.
5. Bornes: **🔴 rojo = +** , **⚫ negro = −** , 🟢 verde = tierra (NO se usa).
