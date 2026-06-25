# Pileta Inteligente — Descripción completa del proyecto

## Idea general
Prototipo de pileta inteligente con tres sistemas principales, todos controlables remotamente
desde un bot de Telegram.

---

## Sistema 1 — Calentador automático
- Sensor DS18B20 sumergible mide la temperatura del agua en tiempo real
- Relé de 1 canal controla el cartucho calefactor de 12V
- Lógica: enciende cuando la temperatura baja del umbral, apaga cuando llega al objetivo
- Histéresis de 5°C para evitar ciclos continuos
- Temperatura objetivo configurable desde Telegram
- LCD 16x02 muestra temperatura y estado del calentador en tiempo real
- LED verde = calentador encendido / LED rojo = calentador apagado

## Sistema 2 — Cobertor automático retráctil
- Cobertor motorizado que se abre y cierra automáticamente
- Motores a comprar (pendiente)
- Driver L298N (ya disponible) para controlar los motores
- Fin de carrera (limit switch) para detectar posición abierto/cerrado
- Controlable desde Telegram: abrir / cerrar

## Sistema 3 — Luces al ritmo de la música
- Sensor de sonido detecta el ritmo de la música ambiente
- LEDs de colores parpadean al ritmo del sonido (efecto discoteca)
- Encendido/apagado controlable desde Telegram

---

## Control remoto — Bot de Telegram
Todos los sistemas se controlan desde un chat de Telegram con comandos:
- Abrir / cerrar el cobertor
- Encender / apagar el calentador
- Cambiar la temperatura objetivo del calentador
- Encender / apagar las luces de música

---

## Componentes disponibles
| Componente | Cantidad | Uso |
|---|---|---|
| ESP32 38 pines | 2 | Cerebro del sistema |
| Sensor DS18B20 sumergible | 2 | Temperatura del agua |
| Módulo relé 1 canal 5V/10A | 2 | Control calentador |
| Cartucho calefactor 12V | 2 | Calentador de agua |
| Display LCD 16x02 + I2C | 1 | Pantalla de estado |
| Sensor de sonido | 2 | Detección ritmo musical |
| Driver L298N doble puente H | 2 | Control motores cobertor |
| Fin de carrera (limit switch) | 3 | Posición cobertor |
| LEDs 5mm (rojo, verde, azul, etc.) | Pack 100 | Luces disco + indicadores |
| Resistencias 220Ω | Pack 50 | LEDs |
| Resistencia 4.7kΩ | 1 | Pull-up DS18B20 |
| Protoboard 830 puntos | 2 | Circuito |
| Botones pulsadores | 2 | Control manual |

## Componentes a conseguir
| Componente | Para qué |
|---|---|
| Motores (a definir tipo) | Cobertor retráctil |
| Fuente de alimentación 12V 5A | Cartucho calefactor |

---

## Pines ESP32 asignados (sistema actual)
| Pin GPIO | Función |
|---|---|
| GPIO4  | DS18B20 DATA |
| GPIO21 | LCD SDA (I2C) |
| GPIO22 | LCD SCL (I2C) |
| GPIO26 | Relé IN |
| GPIO14 | LED verde (calentador ON) |
| GPIO27 | LED rojo (calentador OFF) |
