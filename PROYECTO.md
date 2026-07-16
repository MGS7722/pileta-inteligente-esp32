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
- Histéresis de 5°C para evitar ciclos continuos
- Arranca APAGADO; se activa desde Telegram (auto / forzar ON / forzar OFF)
- Se apaga solo por seguridad si el sensor falla
- LCD 16x02 muestra temperatura y estado del calentador en tiempo real

## Sistema 2 — Luces al ritmo de la música
- Sensor de sonido detecta el ritmo de la música ambiente
- Análisis FFT clasifica graves/agudos y define el patrón de luces
- 8 LEDs de colores (4 colores × 2 lados: verde, rojo, azul, blanco) al ritmo del sonido
- Arrancan APAGADAS; se activan desde Telegram (auto / ON / OFF)

## Sistema 3 — Cobertor automático retráctil
- Cobertor motorizado que se abre y cierra: rodillo que enrolla la lona de un lado +
  cables de tracción que la tiran del otro
- 2 motores DC con reductora (Pololu 6V 500 RPM) por driver L298N
- Lógica "un motor tira / el otro queda suelto" para no trabarse; velocidad por PWM
- 2 fines de carrera detectan los topes (abierto / cerrado) y frenan el motor
- Controlable desde Telegram: /cobertor_abrir, /cobertor_cerrar, /cobertor_parar

---

## Control remoto — Bot de Telegram
Todos los sistemas se controlan desde un chat de Telegram (bot @ControlESP32Pileta_bot):
- Calentador: /calentador_auto, /calentador_on, /calentador_off
- Luces: /luces_auto, /luces_on, /luces_off
- Cobertor: /cobertor_abrir, /cobertor_cerrar, /cobertor_parar
- Consultas: /status, /temp, /audio, /ip

---

## Componentes disponibles
| Componente | Cantidad | Uso |
|---|---|---|
| ESP32 38 pines | 2 | Cerebro del sistema (se usa 1) |
| Sensor DS18B20 sumergible | 2 | Temperatura del agua |
| Módulo relé 1 canal 5V/10A | 2 | Control calentador |
| Cartucho calefactor 12V | 2 | Calentador de agua |
| Display LCD 16x02 + I2C | 1 | Pantalla de estado |
| Sensor de sonido | 2 | Detección ritmo musical |
| Driver L298N doble puente H | 2 | Control motores cobertor |
| Fin de carrera (limit switch) | 3 | Posición cobertor (se usan 2) |
| Motor Pololu 6V 500 RPM metálico | 2 | Motores del cobertor |
| Acople flexible 5mm | 2 | Unir motor al eje del cobertor |
| LEDs 5mm (rojo, verde, azul, blanco) | Pack 100 | Luces disco (se usan 8) |
| Resistencias 220Ω | Pack 50 | LEDs (8 en uso) |
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
| GPIO16 | LEDs verdes (luces disco) |
| GPIO17 | LEDs rojos (luces disco) |
| GPIO18 | LEDs azules (luces disco) |
| GPIO19 | LEDs blancos (luces disco) |
| GPIO34 | Sensor de sonido (analógico, ADC1) |
| GPIO13 / GPIO25 / GPIO27 | L298N motor A: IN1 / IN2 / ENA (PWM) |
| GPIO32 / GPIO33 / GPIO14 | L298N motor B: IN3 / IN4 / ENB (PWM) |
| GPIO23 | Fin de carrera cerrado |
| GPIO5  | Fin de carrera abierto |

> El croquis de conexiones cable por cable está en `PiletaInteligente/CABLEADO-PASO-A-PASO.md`
> y `PiletaInteligente/CONEXIONES.md`.
