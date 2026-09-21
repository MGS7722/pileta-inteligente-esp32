# 🧰 Lista de componentes — Pileta Inteligente

Cada componente marcado con el sistema donde se usa, para saber qué queda libre.

- **S1** = Calentador · **S2** = Luces disco · **S3** = Cobertor · **GEN** = general/compartido
- **TIRA** = **está en uso en el otro proyecto**, [`../../tira-led`](../../tira-led) (la
  retroiluminación del monitor)

> ⚠️ **Esta lista es el inventario de TODO el hardware, no sólo el de la pileta** (anotado el
> 2026-09-21). No se compró hardware aparte para el proyecto de la tira LED del monitor: de acá
> salieron **el segundo ESP32 y 48 píxeles del rollo**. Lo único que se compró para aquel proyecto
> fueron el soldador, el estaño, la cinta, el módulo detector PD y los Wagos.
>
> **Consecuencia práctica:** una fila que dice «2, se usa 1» ya **no** significa que quede uno
> libre. Antes de dar por disponible cualquier componente, mirar la columna *Sistema*: lo que dice
> **TIRA** está ocupado.
>
> **Al 2026-09-21 quedan libres:** 1 DS18B20 · 1 módulo relé · 1 cartucho calefactor · 1 KY-037 ·
> 1 L298N · 1 fin de carrera · los 2 pulsadores · el pack de LEDs de 5 mm · 1 protoboard ·
> **unos 81 píxeles de tira** · la fuente de laboratorio (compartida) · y **ningún ESP32**.
> El **LCD 16×02 no está libre**: es la pantalla de estado de la pileta y está montado.

## Componentes que ya tenemos

| Componente | Cant. | Sistema | Notas |
|---|---|---|---|
| NodeMCU ESP32 38 pines (USB-C) | 2 | GEN + **TIRA** | Cerebro. **Uno acá** (toda la pileta) y **el otro en la tira LED del monitor**. ⚠️ **No queda ninguno libre** |
| Protoboard 830 puntos | 2 | GEN | Armado del circuito |
| Display LCD 16x02 + I2C (PCF8574) | 1 | GEN | Muestra estado (lo comparten todos) |
| Sensor temperatura DS18B20 | 2 | **S1** | Se usa 1 |
| Módulo relé 1 canal 5V 10A | 2 | **S1** | Se usa 1 (prende el calentador) |
| Cartucho calefactor 12V | 2 | **S1** | Se usa 1. **Medido en el taller el 2026-08-24: 4,11 Ω → 2,91 A a 12,0 V = 35 W.** La fuente tiene que quedar en **C.V a 12 V** con el límite de corriente en ~3,8 A (30 % de margen); en C.C la tensión cae y la potencia varía sola a medida que el cartucho se calienta |
| Módulo sensor de sonido KY-037 | 2 | **S2** | Se usa **1**, por su salida AO, alimentado a 5V. Su DO no se usa |
| **Tira WS2812B 5V, 30 LED/m** | rollo 5 m = 150 px | **S2** + **TIRA** | Acá se usan **70 cm = 21 píxeles** (la vuelta completa a la pileta, medida en el taller el 2026-08-13). Otros **48 están pegados al monitor** en el proyecto de la tira. **Quedan ~81 libres (2,70 m)** |
| LEDs 5mm (pack x100) | 1 | — | *Sin uso: los reemplazó la tira* |
| Resistencias 220Ω (pack x50) | 1 | **S2** | **2 en serie (440Ω)** en la línea de datos de la tira |
| Driver doble puente H L298N | 2 | **S3** | Se usa 1 (mueve los 2 motores del cobertor) |
| Fin de carrera (limit switch) | 3 | **S3** | Se usan 2 (tope abierto / tope cerrado) |
| Motor reductor Pololu 6V 500 RPM metálico | 2 | **S3** | ⬅️ NUEVO. Un motor a cada lado |
| Acople flexible 5mm × 5mm | 2 | **S3** | ⬅️ NUEVO. Agarra el eje de 3mm con el prisionero; para centrarlo bien, buje 3→5mm (opcional) |
| Botón pulsador 10mm | 2 | GEN | Control manual opcional (sin usar todavía) |

## Falta comprar

| Componente | Sistema | Para qué |
|---|---|---|
| Resistencia 4.7kΩ | S1 | Pull-up del sensor DS18B20 |
| (Opcional) Módulo reductor LM2596 | S3 | Solo si la fuente NO es regulable. Con la fuente del lab regulada a ~8V no hace falta |
| (Opcional) Buje reductor 3mm→5mm | S3 | Para centrar bien el acople de 5mm en el eje de 3mm del motor (si bambolea o patina) |

---

## 🔌 Pines del ESP32: usados y libres

### Ya ocupados (Sistemas 1 y 2)

| Pin | Sistema | Función |
|---|---|---|
| GPIO4  | S1 | DS18B20 (temperatura) |
| GPIO26 | S1 | Relé del calentador |
| GPIO21 | GEN | LCD SDA (I2C) |
| GPIO22 | GEN | LCD SCL (I2C) |
| GPIO16 | S2 | Tira WS2812 — datos (DIN), con 440Ω en serie |
| GPIO34 | S2 | Micrófono KY-037 (AO), módulo a 5V |

### Usados por el cobertor (Sistema 3)

Ya asignados en el código, **con PWM** para velocidad suave. Detalle completo de cableado
en **`CONEXIONES.md`**:

| Pin | Función |
|---|---|
| GPIO13 | L298N IN1 (motor A) |
| GPIO25 | L298N IN2 (motor A) |
| GPIO27 | L298N ENA — PWM motor A |
| GPIO32 | L298N IN3 (motor B) |
| GPIO33 | L298N IN4 (motor B) |
| GPIO18 | L298N ENB — PWM motor B |
| GPIO23 | Fin de carrera "cerrado" (pull-up interno) |
| GPIO19 | Fin de carrera "abierto" (pull-up interno) |

> Los pines del cobertor son todos "tranquilos" a propósito: no hacen nada mientras el ESP32
> arranca. Por lo mismo, la línea de datos de la tira va en GPIO16: los pines que pulsan al
> encender (GPIO5 y GPIO14) dejarían píxeles prendidos al azar hasta que arranca el programa.
> **Antes de mover cualquier pin, leer la regla 4 de `CONEXIONES.md`.**

Quedan libres: **GPIO17, GPIO14, GPIO5 y GPIO35** (eran de los 8 LEDs y del segundo
micrófono), más GPIO36 y GPIO39 (sólo entrada, sin pull-up interno: necesitan resistencia
externa si se usan para un pulsador). Los de la memoria flash GPIO6–11 **no se tocan**.
