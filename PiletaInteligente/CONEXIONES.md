# 🔌 Conexiones completas — Pileta Inteligente

Croquis de cómo va conectado TODO al ESP32, sistema por sistema.
Es la fuente de verdad: los pines de acá son los que están en `PiletaInteligente.ino`.

> ⚠️ **Reglas de oro del cableado (leer antes de conectar):**
> 1. **Todos los GND van juntos** (ESP32, L298N, relé, fuente 12V, LM2596, sensores). Sin GND común, nada funciona bien.
> 2. **Nunca metas 5V o 12V a un pin GPIO** del ESP32: los pines son de **3.3V**. Se queman con más.
> 3. El **sensor de sonido se alimenta a 3.3V** (así su salida nunca pasa de 3.3V y no daña el GPIO34).

---

## 📋 Mapa completo de pines del ESP32

| Pin | Sistema | Va conectado a |
|---|---|---|
| GPIO4  | Calentador | DS18B20 — dato (+ resistencia 4.7kΩ a 3.3V) |
| GPIO26 | Calentador | Módulo relé — IN |
| GPIO21 | Compartido | LCD — SDA |
| GPIO22 | Compartido | LCD — SCL |
| GPIO16 | Luces | LEDs VERDES (los 2, cada uno con 220Ω) |
| GPIO17 | Luces | LEDs ROJOS (los 2, cada uno con 220Ω) |
| GPIO18 | Luces | LEDs AZULES (los 2, cada uno con 220Ω) |
| GPIO19 | Luces | LEDs BLANCOS (los 2, cada uno con 220Ω) |
| GPIO34 | Luces | Sensor de sonido — salida analógica (AO) |
| GPIO13 | Cobertor | L298N — IN1 (motor A) |
| GPIO25 | Cobertor | L298N — IN2 (motor A) |
| GPIO27 | Cobertor | L298N — ENA (PWM motor A) |
| GPIO32 | Cobertor | L298N — IN3 (motor B) |
| GPIO33 | Cobertor | L298N — IN4 (motor B) |
| GPIO14 | Cobertor | L298N — ENB (PWM motor B) |
| GPIO23 | Cobertor | Fin de carrera CERRADO (otro extremo a GND) |
| GPIO35 | Cobertor | Fin de carrera ABIERTO (otro extremo a GND + resistencia 10kΩ a 3.3V) |
| 3.3V | — | DS18B20, sensor de sonido, pull-ups |
| 5V (Vin) | — | LCD, módulo relé (lado lógico) |
| GND | — | **común a todo** |

---

## 1️⃣ Sistema Calentador

```
DS18B20 (sensor de temperatura)
   ┌─────────┐
   │  rojo   ├──── 3.3V
   │  negro  ├──── GND
   │  amar.  ├──── GPIO4
   └─────────┘
        │
   [4.7kΩ] entre GPIO4 y 3.3V   (pull-up, imprescindible)

Módulo relé
   VCC ──── 5V
   GND ──── GND
   IN  ──── GPIO26
   ── lado de potencia (12V) ──
   Fuente12V(+) ── cartucho calefactor ── relé COM
   relé NO ────────────────────────────── Fuente12V(−)
```

- El relé "corta" o "cierra" el circuito del cartucho de 12V. El cartucho **no** se conecta al ESP32.
- Sin la resistencia de 4.7kΩ, el sensor lee −127 (error) y el calentador no arranca.

## 2️⃣ Sistema Luces disco (8 LEDs = 4 colores × 2 lados)

```
Sensor de sonido
   VCC ──── 3.3V   (importante: 3.3V, NO 5V)
   GND ──── GND
   AO  ──── GPIO34  (salida ANALÓGICA, no la digital DO)

LEDs (por cada color, 2 LEDs: uno de cada lado de la pileta)
   GPIO16 ──[220Ω]──►|── GND     (LED verde  lado A)
          └─[220Ω]──►|── GND     (LED verde  lado B)
   GPIO17 ──[220Ω]──►|── GND     (LED rojo   lado A)
          └─[220Ω]──►|── GND     (LED rojo   lado B)
   GPIO18 ──[220Ω]──►|── GND     (LED azul   lado A)
          └─[220Ω]──►|── GND     (LED azul   lado B)
   GPIO19 ──[220Ω]──►|── GND     (LED blanco lado A)
          └─[220Ω]──►|── GND     (LED blanco lado B)
```

- Cada color usa **1 pin** que maneja **los 2 LEDs** (uno de cada lado) → ambos lados bailan igual.
- `►|` es el LED: la patita larga (+, ánodo) va del lado de la resistencia; la corta (−) a GND.

## 3️⃣ Sistema Cobertor (L298N + 2 motores + 2 fines de carrera)

```
L298N (control)                 L298N (potencia)
   IN1 ──── GPIO13                +12V ──── LM2596 OUT (~8V)
   IN2 ──── GPIO25                GND  ──── GND común
   ENA ──── GPIO27 (PWM)          +5V  ──── (jumper puesto: salida 5V lógica)
   IN3 ──── GPIO32
   IN4 ──── GPIO33                OUT1/OUT2 ──── Motor A (rodillo de la lona)
   ENB ──── GPIO14 (PWM)          OUT3/OUT4 ──── Motor B (tira de los cables)

   ⚠️ Quitar los jumpers de ENA y ENB (usamos PWM por pin).

Alimentación de los motores (son de 6V)
   Fuente12V(+) ── LM2596 IN+        LM2596 OUT+ ── L298N +12V
   Fuente12V(−) ── LM2596 IN− (GND)  LM2596 OUT− ── GND
   → Ajustar el LM2596 a ~7,5–8V con el tornillito ANTES de conectar los motores.

Fines de carrera (interruptores de tope)
   FC CERRADO:  GPIO23 ──── un borne ; otro borne ──── GND
   FC ABIERTO:  GPIO35 ──── un borne ; otro borne ──── GND
                + resistencia 10kΩ entre GPIO35 y 3.3V
                (GPIO35 no tiene resistencia interna, por eso la externa)
```

- **Motor A** = lado del rodillo donde se enrolla la lona. **Motor B** = lado que tira de los cables.
- El código ya hace que **cuando uno tira, el otro queda suelto** (no se traban).
- Si un motor gira al revés, se invierten sus dos cables (OUT) o se cambia HIGH/LOW en el código.

---

## ⚡ Alimentación — resumen

```
        ┌──────────── Fuente 12V ────────────┐
        │                                     │
   Relé → cartucho calefactor            LM2596 (baja a ~8V)
                                              │
                                         L298N (motores 6V)

   ESP32  → se alimenta por USB (para programar y ver el Monitor Serie)
            o por 5V. GND del ESP32 SIEMPRE unido al GND de todo lo demás.
```

- **GND común** entre fuente 12V, LM2596, L298N, relé, ESP32 y sensores. Es lo más importante.
- El ESP32 se puede dejar alimentado por el cable USB de la notebook mientras prueban.
