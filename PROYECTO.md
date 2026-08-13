# Pileta Inteligente — Descripción completa del proyecto

## Idea general
Prototipo de pileta inteligente con tres sistemas principales, todos controlables remotamente
desde un bot de Telegram. Todo corre en un solo ESP32 con el programa
`PiletaInteligente/PiletaInteligente.ino`.

---

## Sistema 1 — Calentador automático
- Sensor DS18B20 sumergible mide la temperatura del agua en tiempo real
- Relé de 1 canal controla el cartucho calefactor de 12V
- Lógica: enciende cuando la temperatura baja del umbral, apaga cuando llega al objetivo
- Histéresis de 2°C para evitar ciclos continuos: prende cuando el agua baja 2°C del
  objetivo y apaga al llegar
- Arranca APAGADO; se activa desde Telegram (auto / forzar ON / forzar OFF)
- Se apaga solo por seguridad si el sensor falla
- LCD 16x02 muestra temperatura y estado del calentador en tiempo real

## Sistema 2 — Luces al ritmo de la música
- Un micrófono KY-037 capta la música ambiente por su salida analógica
- **Análisis de espectro real**: 256 muestras a 10 kHz → FFT → separación en
  **graves (78–234 Hz)**, **medios (234–2000 Hz)** y **agudos (2–5 kHz)**, con control
  automático de ganancia por banda para que las tres se vean parejas
- **Detección de golpes** comparando la energía de los graves contra el promedio del
  último segundo, con umbral que se ajusta solo según qué tan marcado sea el ritmo
- **Puerta de ruido por banda**: lo que no supera el piso de ruido del lugar vale cero, para
  que una banda sin contenido no se llene con el ruido del micrófono (`/piso`)
- **Tira WS2812 de 21 píxeles** (70 cm, la vuelta completa a la pileta) con cuatro efectos:
  espectro, mezcla, cometa y arcoíris; sin música pasa a una respiración lenta
- **Limitador de corriente por software**: permite alimentar la tira del propio ESP32 sin
  una fuente aparte
- Arranca APAGADA; se activa desde Telegram (auto / ON / OFF)

## Sistema 3 — Cobertor automático retráctil
- Cobertor motorizado que se abre y cierra con un **lazo de hilo entre los dos ejes**: el hilo
  se enrolla en un carrete mientras se desenrolla del otro
- 2 motores DC con reductora (Pololu 6V 500 RPM) por driver L298N
- **Los dos motores giran juntos**, en la misma dirección: el lazo se traba si uno empuja y el
  otro queda suelto
- **El recorrido se mide por TIEMPO**, no por sensor, y se calibra desde Telegram
  (`/tiempo_abrir`, `/tiempo_cerrar`), con un valor propio para cada movimiento
- **Los dos motores reciben siempre la misma polaridad**: es imposible que giren uno contra el
  otro. `/cobertor_sentido` invierte el conjunto y elige cuál dirección es "abrir"
- Arrancan con una **patada al 100 %** y siguen a la velocidad de `/velocidad`; al terminar
  **frenan en seco**
- 2 fines de carrera **informan la posición** en `/status`; desde el 2026-08-13 **no cortan el
  movimiento**
- Controlable desde Telegram: /cobertor_abrir, /cobertor_cerrar, /cobertor_parar

---

## Control remoto — Bot de Telegram
Todos los sistemas se controlan desde un chat de Telegram (bot @ControlESP32Pileta_bot):
- Calentador: /calentador_auto, /calentador_on, /calentador_off
- Luces: /luces_auto, /luces_on, /luces_off
- Cobertor: /cobertor_abrir, /cobertor_cerrar, /cobertor_parar
- Prueba de taller: /motor_a, /motor_b (mueven un motor solo; `/motor_a 5` para elegir los
  segundos, 2 por omisión)
- Efectos de luces: /efecto 1 (espectro) | 2 (mezcla) | 3 (cometa) | 4 (arcoíris)
- Ajustes: /temperatura 28 (objetivo), /velocidad 35 (motores, en %), /tiempo_abrir 4.5 y
  /tiempo_cerrar 5 (duración del recorrido, admite decimales), /cobertor_sentido (invertir el
  cobertor), /sentido_a y /sentido_b (invertir sólo esa prueba), /brillo 70, /leds 21,
  /corriente 120, /orden, /piso 12 — todos guardados en NVS
- Consultas: /status, /temp, /audio, /espectro, /diag, /trace, /onda, /luces_test, /ip

---

## Componentes disponibles
| Componente | Cantidad | Uso |
|---|---|---|
| ESP32 38 pines | 2 | Cerebro del sistema (se usa 1) |
| Sensor DS18B20 sumergible | 2 | Temperatura del agua |
| Módulo relé 1 canal 5V/10A | 2 | Control calentador |
| Cartucho calefactor 12V | 2 | Calentador de agua |
| Display LCD 16x02 + I2C | 1 | Pantalla de estado |
| **Tira WS2812B 5V, 30 LED/m** | rollo de 5 m | **Luces disco (se usan 70 cm = 21 píxeles)** |
| Sensor de sonido KY-037 | 2 | Detección del ritmo (se usa **1**) |
| Driver L298N doble puente H | 2 | Control motores cobertor |
| Fin de carrera (limit switch) | 3 | Posición cobertor (se usan 2) |
| Motor Pololu 6V 500 RPM metálico | 2 | Motores del cobertor |
| Acople flexible 5mm | 2 | Unir motor al eje del cobertor |
| LEDs 5mm (rojo, verde, azul, blanco) | Pack 100 | *sin uso: los reemplazó la tira* |
| Resistencias 220Ω | Pack 50 | 2 en serie (440Ω) en la línea de datos de la tira |
| Protoboard 830 puntos | 2 | Circuito |
| Botones pulsadores | 2 | Control manual (sin usar todavía) |
| Fuente de laboratorio doble regulable | 1 | 12V (calentador) + ~8V (motores) |

## Componentes a conseguir
| Componente | Para qué |
|---|---|
| Resistencia 4.7kΩ | Pull-up del DS18B20 (si no la tienen ya) |

> La lista detallada con la columna de qué sistema usa cada cosa está en
> `PiletaInteligente/COMPONENTES.md`.

---

## Pines ESP32 asignados (programa `PiletaInteligente.ino`)
| Pin GPIO | Función |
|---|---|
| GPIO4  | DS18B20 DATA (pull-up 4.7kΩ a 3.3V) |
| GPIO26 | Relé IN (calentador) |
| GPIO21 | LCD SDA (I2C) |
| GPIO22 | LCD SCL (I2C) |
| GPIO16 | Tira WS2812 — entrada DIN (con 440Ω en serie) |
| GPIO34 | Micrófono KY-037 — salida AO (analógica, ADC1). Módulo alimentado a 5V/VIN |
| GPIO13 / GPIO25 / GPIO27 | L298N motor A: IN1 / IN2 / ENA (PWM) |
| GPIO32 / GPIO33 / GPIO18 | L298N motor B: IN3 / IN4 / ENB (PWM) |
| GPIO23 | Fin de carrera cerrado (COM a GND, NO al pin) |
| GPIO19 | Fin de carrera abierto (COM a GND, NO al pin) |

> **Pines libres**: GPIO17, GPIO14, GPIO5 y GPIO35 (eran de los 8 LEDs y del segundo micrófono).
>
> Los pines del cobertor evitan a propósito los que el ESP32 usa al arrancar (GPIO5 y GPIO14
> emiten un pulso al encender; con un driver de motor del otro lado eso es un tirón). Por lo
> mismo, la tira de datos va en GPIO16 y no en esos dos: un pulso en la línea de datos deja
> píxeles encendidos al azar hasta que el programa arranca. Detalle en la regla 4 de
> `PiletaInteligente/CONEXIONES.md`.

> El croquis de conexiones cable por cable está en `PiletaInteligente/CABLEADO-PASO-A-PASO.md`
> y `PiletaInteligente/CONEXIONES.md`.
