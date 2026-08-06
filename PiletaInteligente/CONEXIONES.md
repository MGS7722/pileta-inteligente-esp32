# 🔌 Conexiones completas — Pileta Inteligente

Croquis de cómo va conectado TODO al ESP32, sistema por sistema.
Es la fuente de verdad: los pines de acá son los que están en `PiletaInteligente.ino`.

> ⚠️ **Reglas de oro del cableado (leer antes de conectar):**
> 1. **Todos los GND van juntos** (ESP32, L298N, relé, fuente, sensores). Sin GND común, nada funciona bien.
> 2. **Nunca metas 5V o 12V a un pin GPIO** del ESP32: los pines son de **3.3V**. Se queman con más.
> 3. El **sensor de sonido 1 (AO) se alimenta a 5V/VIN**: el módulo pide 4-6V y a 3.3V daba una
>    señal demasiado débil. Verificado con `/diag`: el Máximo debe quedar **por debajo de 3000**
>    para no dañar el GPIO34. El **módulo 2 (DO), en cambio, va a 3.3V** — nunca a 5V.
> 4. **Los pines no son intercambiables.** Algunos del ESP32 hacen cosas mientras el chip
>    arranca (GPIO5 y GPIO14 emiten un pulso; GPIO5, GPIO0, GPIO2, GPIO12 y GPIO15 se leen al
>    encender para configurar el chip). Por eso el cobertor usa sólo pines "tranquilos": del
>    otro lado hay motores y un pulso suelto significa un tirón. Los LEDs, en cambio, viven
>    tranquilos en GPIO5 y GPIO14: un destello al arrancar no molesta a nadie. **Si algún día
>    hay que reasignar un pin, primero revisar esto** (referencia: pines de *strapping* y de
>    boot en la documentación de Espressif).

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
| GPIO14 | Luces | LEDs AZULES (los 2, cada uno con 220Ω) |
| GPIO5  | Luces | LEDs BLANCOS (los 2, cada uno con 220Ω) |
| GPIO34 | Luces | Sensor de sonido 1 — salida analógica (AO) |
| GPIO35 | Luces | Sensor de sonido 2 — salida digital (DO, golpes por hardware) |
| GPIO13 | Cobertor | L298N — IN1 (motor A) |
| GPIO25 | Cobertor | L298N — IN2 (motor A) |
| GPIO27 | Cobertor | L298N — ENA (PWM motor A) |
| GPIO32 | Cobertor | L298N — IN3 (motor B) |
| GPIO33 | Cobertor | L298N — IN4 (motor B) |
| GPIO18 | Cobertor | L298N — ENB (PWM motor B) |
| GPIO23 | Cobertor | Fin de carrera CERRADO (COM a GND, NO al pin) |
| GPIO19 | Cobertor | Fin de carrera ABIERTO (COM a GND, NO al pin) |
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
   Fuente12V(+) ──────────────── relé COM
   relé NO ── cartucho calefactor ── Fuente12V(−)
```

- El relé "corta" o "cierra" el circuito del cartucho de 12V. El cartucho **no** se conecta al ESP32.
- **El relé va del lado del POSITIVO** (entre los +12V y el cartucho). Así, con el relé
  abierto, el cartucho queda sin tensión en ninguno de sus extremos. Si se cortara el
  negativo, el cartucho quedaría con +12V permanentes y cualquier contacto accidental con
  masa lo encendería sin que el ESP32 pueda evitarlo — grave con el cartucho sumergido.
- **Usar NO, nunca NC**: con el relé en reposo el circuito queda abierto, así que si el
  ESP32 se cuelga o se queda sin alimentación, el calefactor queda apagado.
- ⚠️ **El negativo de los 12V NO va al riel GND de la protoboard.** El circuito del
  calefactor se cierra sobre sí mismo (fuente → relé → cartucho → fuente) y el relé ya
  aísla ambos mundos. Meter esa corriente al riel de masa corre la referencia del ADC y
  ensucia la lectura del micrófono.
- Los tres cables del lado de 12V van **gruesos y directos a los tornillos**, nunca por la
  protoboard: sus rieles no están hechos para varios amperes.
- **Este módulo relé es ACTIVO-ALTO** (verificado en hardware el 2026-08-06): se activa con
  GPIO26 en HIGH. Hay módulos activo-bajo idénticos por fuera; si se cambia el módulo, hay
  que volver a comprobarlo y ajustar `RELE_ON`/`RELE_OFF` en el `.ino` **antes** de
  conectar la carga.
- Sin la resistencia de 4.7kΩ, el sensor lee −127 (error) y el calentador no arranca.

## 2️⃣ Sistema Luces disco (8 LEDs = 4 colores × 2 lados)

```
Sensor de sonido 1 (volumen, canal analógico)
   VCC ──── VIN (5V)  (el módulo pide 4-6V; su AO es de nivel bajo y es segura)
   GND ──── GND
   AO  ──── GPIO34    (salida ANALÓGICA; el DO de este módulo queda libre)

Sensor de sonido 2 (golpes por hardware)  ⚠️ a 3.3V, NUNCA a 5V
   VCC ──── 3.3V
   GND ──── GND
   DO  ──── GPIO35    (salida DIGITAL; el AO de este módulo queda libre)

LEDs (por cada color, 2 LEDs: uno de cada lado de la pileta)
   GPIO16 ──[220Ω]──►|── GND     (LED verde  lado A)
          └─[220Ω]──►|── GND     (LED verde  lado B)
   GPIO17 ──[220Ω]──►|── GND     (LED rojo   lado A)
          └─[220Ω]──►|── GND     (LED rojo   lado B)
   GPIO14 ──[220Ω]──►|── GND     (LED azul   lado A)
          └─[220Ω]──►|── GND     (LED azul   lado B)
   GPIO5  ──[220Ω]──►|── GND     (LED blanco lado A)
          └─[220Ω]──►|── GND     (LED blanco lado B)
```

- Cada color usa **1 pin** que maneja **los 2 LEDs** (uno de cada lado) → ambos lados bailan igual.
- `►|` es el LED: la patita larga (+, ánodo) va del lado de la resistencia; la corta (−) a GND.

## 3️⃣ Sistema Cobertor (L298N + 2 motores + 2 fines de carrera)

```
L298N (control)                 L298N (potencia)
   IN1 ──── GPIO13                OUT1/OUT2 ──── Motor A (rodillo de la lona)
   IN2 ──── GPIO25                OUT3/OUT4 ──── Motor B (tira de los cables)
   ENA ──── GPIO27 (PWM)
   IN3 ──── GPIO32                Bornera de 3 tornillos (GND es el del medio):
   IN4 ──── GPIO33                  +12V ──── + de la fuente SLAVE (~8V)
   ENB ──── GPIO18 (PWM)            GND  ──── − de la fuente SLAVE
                                    +5V  ──── SIN CONECTAR (es una salida del módulo)

   GND común (imprescindible): del borne − de la fuente SLAVE sale un segundo
   cable al riel GND de la protoboard. Une el cero del ESP32 (que se alimenta
   por USB) con el cero del L298N; sin él, el módulo no interpreta las señales.
   Puede salir del borne de la fuente o del tornillo GND del módulo: es el mismo
   punto eléctrico. Del borne es más cómodo (entran dos cables sin apretujar).

   ⚠️ Quitar los jumpers de ENA y ENB (usamos PWM por pin).
   ⚠️ DEJAR puesto el jumper del regulador de 5V (sólo se saca con más de 12V).
   🚫 El borne +5V del L298N NO se conecta: es una salida. Enchufarlo al 5V/VIN del
      ESP32 con el USB puesto enfrenta dos fuentes y quema el regulador de la placa.

Alimentación de los motores (son de 6V)
   ► OPCIÓN A (fuente de laboratorio REGULABLE — lo más simple):
     Regulá la fuente a ~7,5–8V y conectala directo al L298N (+12V y GND).
     El L298N pierde ~2V y deja ~6V en los motores. NO hace falta el LM2596.
     (7,5–8V porque el driver "se come" ~2V; si tu fuente puede dar más
      corriente, mejor: los 2 motores juntos piden < 1,5A.)

   ► OPCIÓN B (fuente fija de 12V + módulo LM2596):
     Fuente12V(+) ── LM2596 IN+        LM2596 OUT+ ── L298N +12V
     Fuente12V(−) ── LM2596 IN− (GND)  LM2596 OUT− ── GND
     → Ajustar el LM2596 a ~7,5–8V con el tornillito ANTES de conectar los motores.

Fines de carrera (interruptores de tope) — usar las patas COM y NO
   FC CERRADO:  COM ──── GND  ;  NO ──── GPIO23
   FC ABIERTO:  COM ──── GND  ;  NO ──── GPIO19
                (la pata NC queda libre. Ambos usan la resistencia pull-up
                 INTERNA del ESP32: no hace falta ninguna externa)
```

- **Verificación sin mover motores:** mandá `/status` y mirá la línea "Cobertor". Apretando
  cada fin de carrera con el dedo tiene que pasar de "parado (posición intermedia)" a
  "cerrado" o "abierto". Si no cambia, el cable está en la pata NC en vez de la NO.

- **Motor A** = lado del rodillo donde se enrolla la lona. **Motor B** = lado que tira de los cables.
- El código ya hace que **cuando uno tira, el otro queda suelto** (no se traban).
- Si un motor gira al revés, se invierten sus dos cables (OUT) o se cambia HIGH/LOW en el código.

---

## ⚡ Alimentación — resumen

Son **tres alimentaciones independientes**; no se mezclan entre sí:

| Alimenta | A quién | Cuánto |
|---|---|---|
| USB de la notebook | El ESP32 — y por su intermedio el LCD, el lado de control del relé y los sensores | 5V |
| Fuente MASTER | El cartucho calefactor, a través del relé | 12V |
| Fuente SLAVE | El L298N y los 2 motores del cobertor | ~8V |

```
   Notebook ──USB──► ESP32 ──señales──► L298N ──► Motor A y Motor B
                       │                  ▲
                       └──── GND común ───┘        ▲
                                                   │
                              Fuente SLAVE ~8V ────┘

   Fuente MASTER 12V ──► relé ──► cartucho calefactor ──► vuelve a la fuente
                                  (circuito cerrado sobre sí mismo, aparte de todo)
```

- **GND común** entre la fuente, el L298N, el relé, el ESP32 y los sensores. Es lo más importante.
- El ESP32 se puede dejar alimentado por el cable USB de la notebook mientras prueban.
