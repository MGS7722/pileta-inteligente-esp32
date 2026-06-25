# Control de avances — Pileta Inteligente

## Sistema 1 — Calentador automático

- [x] Código base escrito (`prueba_sensor.ino`)
- [x] Sensor DS18B20 conectado y leyendo temperatura
- [x] LCD 16x02 con I2C conectado y mostrando datos
- [x] LED verde (calentador ON) y LED rojo (calentador OFF) conectados
- [x] Módulo relé conectado al ESP32 (GPIO26)
- [x] Probar ciclo completo: sensor en agua fría → relé hace click → LED cambia ✓ (2026-06-25)
- [ ] Conseguir fuente 12V y conectar cartucho calefactor al relé
- [ ] Probar sistema completo con calentador real
- [ ] Integrar control por Telegram (encender/apagar, cambiar temperatura objetivo)

## Sistema 2 — Cobertor automático retráctil

- [ ] Conseguir motores
- [ ] Definir tipo de motor y mecanismo del cobertor
- [ ] Conectar L298N al ESP32 y a los motores
- [ ] Conectar fin de carrera para detectar posición abierto/cerrado
- [ ] Programar lógica de apertura y cierre
- [ ] Integrar control por Telegram (abrir/cerrar)

## Sistema 3 — Luces al ritmo de la música

- [ ] Conectar sensor de sonido al ESP32
- [ ] Programar detección de picos de sonido
- [ ] Conectar LEDs de colores
- [ ] Programar efecto disco (LEDs al ritmo del sonido)
- [ ] Integrar control por Telegram (encender/apagar)

## Bot de Telegram

- [ ] Crear bot en Telegram con BotFather y obtener token
- [ ] Integrar librería UniversalTelegramBot en el ESP32
- [ ] Programar comandos para los 3 sistemas
- [ ] Probar control remoto completo

---

## Historial de sesiones

### Sesión 2026-06-25
- Definición completa del proyecto (3 sistemas + Telegram)
- Escritura del código base para sensor + LCD + LEDs + relé (`prueba_sensor.ino`)
- Conexión física del DS18B20, LCD, LEDs y relé en protoboard
- Sistema de calentador verificado completo: ciclo frío/calor, click del relé, LEDs y LCD funcionando
- Pines asignados: GPIO4 (sensor), GPIO21/22 (LCD), GPIO26 (relé), GPIO14/27 (LEDs)
- Pendiente próxima sesión: conectar sensor de sonido y programar luces disco (Sistema 3)
