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
| Módulo sensor de sonido | 2 | **S2** | Se usa 1 |
| LEDs 5mm (pack x100) | 1 | **S2** | Se usan 8: 4 colores × 2 lados (verde, rojo, azul, blanco) |
| Resistencias 220Ω (pack x50) | 1 | **S2** | 8, una por cada LED de las luces |
| Driver doble puente H L298N | 2 | **S3** | Se usa 1 (mueve los 2 motores del cobertor) |
| Fin de carrera (limit switch) | 3 | **S3** | Se usan 2 (tope abierto / tope cerrado) |
| Motor reductor Pololu 6V 500 RPM metálico | 2 | **S3** | ⬅️ NUEVO. Un motor a cada lado |
| Acople flexible 5mm × 5mm | 2 | **S3** | ⬅️ NUEVO. Agarra el eje de 3mm con el prisionero; para centrarlo bien, buje 3→5mm (opcional) |
| Botón pulsador 10mm | 2 | GEN | Control manual opcional (sin usar todavía) |

## Falta comprar

| Componente | Sistema | Para qué |
|---|---|---|
| Resistencia 4.7kΩ | S1 | Pull-up del sensor DS18B20 |
| Fuente de alimentación 12V | S1 + S3 | Alimenta el calentador y (vía LM2596) los motores |
| Módulo reductor LM2596 | S3 | Bajar los 12V a ~7-8V para los motores de 6V |
| Resistencia 10kΩ | S3 | Pull-up del fin de carrera "abierto" (GPIO35 no tiene interno) |
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
| GPIO16 | S2 | LED 1 |
| GPIO17 | S2 | LED 2 |
| GPIO18 | S2 | LED 3 |
| GPIO19 | S2 | LED 4 |
| GPIO34 | S2 | Sensor de sonido |

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
| GPIO14 | L298N ENB — PWM motor B |
| GPIO23 | Fin de carrera "cerrado" (pull-up interno) |
| GPIO35 | Fin de carrera "abierto" (resistencia 10kΩ externa a 3.3V) |

Quedan libres de sobra: GPIO5, GPIO36, GPIO39 y más.
