# 🧰 Lista de componentes — Pileta Inteligente

Cada componente marcado con el sistema donde se usa, para saber qué queda libre.

- **S1** = Calentador · **S2** = Luces disco · **S3** = Cobertor · **GEN** = general/compartido

## Componentes que ya tenemos

| Componente | Cant. | Sistema | Notas |
|---|---|---|---|
| NodeMCU ESP32 38 pines (USB-C) | 2 | GEN | Cerebro. Se usa 1 para toda la pileta |
| Protoboard 830 puntos | 2 | GEN | Armado del circuito |
| Display LCD 16x02 + I2C (PCF8574) | 1 | GEN | Muestra estado (lo comparten todos) |
| Sensor temperatura DS18B20 | 2 | **S1** | Se usa 1 |
| Módulo relé 1 canal 5V 10A | 2 | **S1** | Se usa 1 (prende el calentador) |
| Cartucho calefactor 12V | 2 | **S1** | Se usa 1 |
| Módulo sensor de sonido KY-037 | 2 | **S2** | Se usa **1**, por su salida AO, alimentado a 5V. Su DO no se usa |
| **Tira WS2812B 5V, 30 LED/m** | rollo 5 m | **S2** | Se usan **70 cm = 21 píxeles** (la vuelta completa a la pileta, medida en el taller el 2026-08-13). Reemplaza a los 8 LEDs |
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
