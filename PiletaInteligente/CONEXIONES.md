# 🔌 Conexiones completas — Pileta Inteligente

Croquis de cómo va conectado TODO al ESP32, sistema por sistema.
Es la fuente de verdad: los pines de acá son los que están en `PiletaInteligente.ino`.

> ⚠️ **Reglas de oro del cableado (leer antes de conectar):**
> 1. **Todos los GND van juntos** (ESP32, L298N, relé, fuente, sensores). Sin GND común, nada funciona bien.
> 2. **Nunca metas 5V o 12V a un pin GPIO** del ESP32: los pines son de **3.3V**. Se queman con más.
> 3. El **micrófono se alimenta a 5V/VIN**: el módulo pide 4-6V y a 3.3V daba una señal
>    demasiado débil. Verificado con `/diag`: el Máximo debe quedar **por debajo de 3000**
>    para no dañar el GPIO34. Hay **un solo micrófono**: el segundo módulo ya no se usa.
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
| GPIO16 | Luces | **Tira WS2812 — DIN** (con 440Ω en serie: dos de 220Ω) |
| GPIO34 | Luces | Micrófono KY-037 — salida analógica (AO), módulo a 5V |
| GPIO13 | Cobertor | L298N — IN1 (motor A) |
| GPIO25 | Cobertor | L298N — IN2 (motor A) |
| GPIO27 | Cobertor | L298N — ENA (PWM motor A) |
| GPIO32 | Cobertor | L298N — IN3 (motor B) |
| GPIO33 | Cobertor | L298N — IN4 (motor B) |
| GPIO18 | Cobertor | L298N — ENB (PWM motor B) |
| GPIO23 | Cobertor | Fin de carrera CERRADO (COM a GND, NO al pin) |
| GPIO19 | Cobertor | Fin de carrera ABIERTO (COM a GND, NO al pin) |
| 3.3V | — | DS18B20, pull-ups |
| 5V (Vin) | — | LCD, módulo relé (lado lógico), micrófono, **tira WS2812** |
| GND | — | **común a todo** |
| GPIO17 · GPIO14 · GPIO5 · GPIO35 | — | *libres* (eran de los 8 LEDs y del 2do micrófono) |

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

## 2️⃣ Sistema Luces disco (tira WS2812 + micrófono)

```
Micrófono KY-037 (es el ÚNICO micrófono del sistema)
   VCC ──── VIN (5V)  (el módulo pide 4-6V; su AO es de nivel bajo y es segura)
   GND ──── GND
   AO  ──── GPIO34    (salida ANALÓGICA; el DO de este módulo NO se usa)

Tira WS2812 — 21 píxeles (70 cm de una tira de 30 LED/m)
   5V  ──── VIN (5V) del ESP32
   GND ──── GND
   DIN ──── [220Ω]──[220Ω]──── GPIO16     (440Ω en serie, como pide Adafruit)
```

- **Un solo cable de datos** maneja los 21 píxeles: cada uno se queda con su color y le pasa
  el resto al siguiente. Por eso quedaron libres GPIO17, GPIO14 y GPIO5.
- **La tira tiene dirección.** El cable de datos entra por el extremo que dice `DIN`, hacia
  donde apuntan las flechas impresas. Al revés no enciende nada (pero no se rompe).
- **El cable de datos, corto** (20–30 cm): es la causa número uno de fallas al manejar una
  tira de 5V con la lógica de 3,3V del ESP32.
- **Por qué anda sin adaptador de nivel:** la tira necesita 0,7 × su alimentación para leer un
  "1". Alimentada desde el `VIN` recibe menos de 5 V (la placa tiene un diodo entre el USB y
  ese pin), así que el umbral baja y el 3,3 V del ESP32 alcanza. **Medido el 2026-08-13: 4,4 V
  → umbral 3,08 V → 0,22 V de margen.** Cuanto más baja esté esa tensión, mejor anda la señal;
  si tu placa da 5,0 V clavados, el umbral sube a 3,50 V y el margen desaparece.
- **Por qué no necesita fuente propia:** el programa lleva un **limitador de corriente**. Antes
  de mandar cada cuadro calcula el consumo y, si se pasa del presupuesto (`/corriente`), baja
  el brillo hasta que entre. Sin eso, 21 píxeles en blanco pleno pedirían 1,3 A.
- **Verificado en hardware el 2026-08-13**: con la tira encendida, el pico a pico del micrófono
  NO subió (promedios: 47 con luces apagadas, 38 con luces encendidas) ni con brillo 70 ni con
  brillo 100. La tira **no** contamina la medición del sonido.
- ⚠️ Los cables de 5V y GND de la tira van **directo al ESP32**, no por el tramo de protoboard
  donde está el micrófono: los tirones de corriente de la tira entran en el ADC como ruido.

## 3️⃣ Sistema Cobertor (L298N + 2 motores + 2 fines de carrera)

```
L298N (control)                 L298N (potencia)
   IN1 ──── GPIO13                OUT1/OUT2 ──── Motor A (rodillo de la lona)
   IN2 ──── GPIO25                OUT3/OUT4 ──── Motor B (tira de los cables)
   ENA ──── GPIO27 (PWM)
   IN3 ──── GPIO32                Bornera de 3 tornillos (GND es el del medio):
   IN4 ──── GPIO33                  +12V ──── + de la fuente SLAVE (~8V)
   ENB ──── GPIO18 (PWM)            GND  ──── DOS cables: − de la fuente SLAVE
                                             + un cable al riel GND de la protoboard
                                    +5V  ──── SIN CONECTAR (es una salida del módulo)

   Ese segundo cable del GND es el GND común y es imprescindible: une el cero del
   ESP32 (que se alimenta por USB) con el cero del L298N. Sin él, el módulo no
   interpreta las señales de 3,3V y los motores no arrancan. Si no entraran los
   dos cables en el tornillo, puede salir del borne − de la fuente: es el mismo
   punto eléctrico.

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

> ⚠️ **Desde el 2026-08-13 los fines de carrera NO cortan el movimiento.** El mecanismo pasó a
> ser un lazo de hilo sin topes físicos, así que el recorrido se mide por tiempo
> (`/tiempo_abrir`, `/tiempo_cerrar`). Los dos sensores siguen leyéndose y **sólo informan la
> posición** en `/status`.

- **Verificación sin mover motores:** mandá `/status` y mirá la línea "Fines de carrera", que
  muestra **cada pin por separado**: `cerrado libre | abierto libre`. Apretando cada uno con el
  dedo, el suyo tiene que pasar a `TOCADO`. Se informan sueltos y no resumidos en una palabra a
  propósito: si un cable quedara en la pata **NC** en vez de la NO, el pin se lee TOCADO en
  reposo, y resumido eso se disfraza de posición válida y la verificación da un falso OK.

**Cómo trabajan los dos motores**

- Cada motor mueve **su carrete**: el hilo se enrolla en uno mientras se desenrolla del otro.
- **Los dos giran juntos, en la misma dirección.** Si uno empujara y el otro quedara suelto, el
  lazo se destensaría de un lado y se trabaría del otro.
- **"El mismo lado" es físico, no eléctrico**: según cómo queden montados los motores
  (enfrentados o alineados), mover el hilo en la misma dirección puede exigir que uno gire
  horario y el otro antihorario.
- Si un motor gira para el lado equivocado, **no se tocan los cables**: se manda `/sentido_a` o
  `/sentido_b` y queda guardado en NVS. `/status` informa hacia dónde gira cada uno.
- Arrancan con una **patada al 100 % durante 300 ms** —un motor con reductora necesita mucho más
  par para empezar a moverse que para seguir girando— y al terminar **frenan en seco**.

---

## ⚡ Alimentación — resumen

Son **tres alimentaciones independientes**; no se mezclan entre sí:

| Alimenta | A quién | Cuánto |
|---|---|---|
| USB de la notebook | El ESP32 — y por su intermedio el LCD, el lado de control del relé, el micrófono y **la tira de luces** | 5V |
| Fuente MASTER | El cartucho calefactor, a través del relé | 12V |
| Fuente SLAVE | El L298N y los 2 motores del cobertor | ~8V |

⚠️ **El riel de 5V está bastante cargado**: el ESP32 (150–250 mA), el LCD (~30 mA), el relé
cuando está activo (~75 mA) y el micrófono (~5 mA) suman entre 260 y 460 mA, y un USB 2.0
entrega 500 mA. Por eso la tira va con el limitador en `/corriente 120` mientras el ESP32 esté
enchufado a la notebook; con un cargador de celular de 2A se puede subir a 500.

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
