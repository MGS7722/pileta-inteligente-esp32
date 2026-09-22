# 🧰 Lista de componentes — Pileta Inteligente

Cada componente marcado con el sistema donde se usa, para saber qué queda libre.

- **TIRA** = **está en uso en el otro proyecto**, [`../../tira-led`](../../tira-led) (la
  retroiluminación del monitor). Es lo único ocupado.
- **—** = libre y disponible. Desde el desmontaje del 2026-09-21 es casi todo.
- Los sistemas de la pileta —**S1** calentador · **S2** luces disco · **S3** cobertor— ya no
  ocupan nada: se citan en el resto del documento para decir dónde iba cada pieza.

> ⚠️ **Esta lista es el inventario de TODO el hardware, no sólo el de la pileta** (anotado el
> 2026-09-21). No se compró hardware aparte para el proyecto de la tira LED del monitor: de acá
> salieron **el segundo ESP32 y 48 píxeles del rollo**. Lo único que se compró para aquel proyecto
> fueron el soldador, el estaño, la cinta, el módulo detector PD y los Wagos.
>
> 📦 **La pileta se desmontó el 2026-09-21 y su hardware volvió a la caja.** Lo dijo Mariano ese
> mismo día, al cerrar el proyecto para pasar a un invento nuevo. El aparato ya no existe armado:
> el ESP32, el LCD, los dos motores, el L298N, el relé, el cartucho, el DS18B20, el KY-037, los
> fines de carrera y los 21 píxeles del anillo **están otra vez disponibles**.
>
> **Consecuencia práctica:** de esta lista hoy está ocupado **sólo lo que está pegado al monitor**,
> que es lo marcado **TIRA**. Todo lo demás se puede usar.
>
> **Libre al 2026-09-21, después del desmontaje:** **1 ESP32** · el LCD 16×02 con su I2C ·
> 2 DS18B20 · 2 módulos relé · 2 cartuchos calefactores · 2 KY-037 · 2 L298N · 3 fines de carrera ·
> 2 motores Pololu 6 V 500 RPM con sus 2 acoples · 2 pulsadores · 2 protoboards · el pack de LEDs
> de 5 mm · las resistencias · **unos 102 píxeles de tira** (81 sueltos del rollo más el anillo de
> 21, ya cortado y con sus cables soldados) · y la fuente de laboratorio.
>
> **Lo único NO disponible:** el otro ESP32 y los 48 píxeles pegados al monitor, que siguen
> funcionando en [`../../tira-led`](../../tira-led).

## Componentes que ya tenemos

| Componente | Cant. | Sistema | Notas |
|---|---|---|---|
| NodeMCU ESP32 38 pines (USB-C) | 2 | **TIRA** | Cerebro. Uno **quedó libre** al desmontarse la pileta el 2026-09-21; el otro está en la tira LED del monitor |
| Protoboard 830 puntos | 2 | — | Armado del circuito. Las 2 libres desde el desmontaje |
| Display LCD 16x02 + I2C (PCF8574) | 1 | — | Era la pantalla de estado de la pileta. **Libre desde el desmontaje** |
| Sensor temperatura DS18B20 | 2 | — | Se usaba 1. Los 2 libres desde el desmontaje |
| Módulo relé 1 canal 5V 10A | 2 | — | Se usaba 1 (prendía el calentador). Los 2 libres desde el desmontaje |
| Cartucho calefactor 12V | 2 | **S1** | Se usa 1. **Medido en el taller el 2026-08-24: 4,11 Ω → 2,91 A a 12,0 V = 35 W.** La fuente tiene que quedar en **C.V a 12 V** con el límite de corriente en ~3,8 A (30 % de margen); en C.C la tensión cae y la potencia varía sola a medida que el cartucho se calienta |
| Módulo sensor de sonido KY-037 | 2 | — | Se usaba **1**, por su salida AO, alimentado a 5V (su DO no sirve). Los 2 libres desde el desmontaje |
| **Tira WS2812B 5V, 30 LED/m** | rollo 5 m = 150 px | **TIRA** | **48 están pegados al monitor** y siguen en uso. Los **21 del anillo de la pileta** (70 cm, medidos en el taller el 2026-08-13) volvieron a la caja al desmontarse, cortados y con sus cables ya soldados. **Quedan ~102 libres (3,40 m)** |
| LEDs 5mm (pack x100) | 1 | — | *Sin uso: los reemplazó la tira* |
| Resistencias 220Ω (pack x50) | 1 | **TIRA** | **2 en serie (440Ω)** en la línea de datos de una tira. Del pack de 50 quedan de sobra |
| Driver doble puente H L298N | 2 | — | Se usaba 1 (movía los 2 motores del cobertor). Los 2 libres desde el desmontaje |
| Fin de carrera (limit switch) | 3 | — | Se usaban 2 (tope abierto / tope cerrado). Los 3 libres desde el desmontaje |
| Motor reductor Pololu 6V 500 RPM metálico | 2 | — | Movían el cobertor, uno a cada lado. **Los 2 libres desde el desmontaje**: son lo más caro de la caja |
| Acople flexible 5mm × 5mm | 2 | — | Agarra el eje de 3mm con el prisionero; para centrarlo bien, buje 3→5mm (opcional). Los 2 libres desde el desmontaje |
| Botón pulsador 10mm | 2 | — | Nunca se llegaron a usar. Libres |

## Falta comprar

> Esta tabla quedó **congelada el 2026-09-21**, cuando la pileta se desmontó: nada de acá se
> compró, y nada de acá hace falta comprar ya, salvo que el aparato se vuelva a armar.

| Componente | Sistema | Para qué |
|---|---|---|
| Resistencia 4.7kΩ | S1 | Pull-up del sensor DS18B20 |
| (Opcional) Módulo reductor LM2596 | S3 | Solo si la fuente NO es regulable. Con la fuente del lab regulada a ~8V no hace falta |
| (Opcional) Buje reductor 3mm→5mm | S3 | Para centrar bien el acople de 5mm en el eje de 3mm del motor (si bambolea o patina) |

---

## 🔌 Pines del ESP32: usados y libres

> 📦 **Desde el desmontaje del 2026-09-21 ningún pin está ocupado**: el circuito ya no existe
> armado. Lo que sigue es el **mapa de cómo estaba cableado**, que vale para dos cosas: volver a
> armarlo tal cual, y —sobre todo— la regla del final, que explica **por qué** cada pin es el que
> es. Esa parte no vence y se aplica a cualquier proyecto nuevo con este mismo ESP32.

### Cómo estaban asignados (Sistemas 1 y 2)

| Pin | Sistema | Función |
|---|---|---|
| GPIO4  | S1 | DS18B20 (temperatura) |
| GPIO26 | S1 | Relé del calentador |
| GPIO21 | GEN | LCD SDA (I2C) |
| GPIO22 | GEN | LCD SCL (I2C) |
| GPIO16 | S2 | Tira WS2812 — datos (DIN), con 440Ω en serie |
| GPIO34 | S2 | Micrófono KY-037 (AO), módulo a 5V |

### Cómo estaban asignados en el cobertor (Sistema 3)

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
