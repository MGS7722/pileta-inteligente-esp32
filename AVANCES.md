# Control de avances — Pileta Inteligente

> Programa principal: **`PiletaInteligente/PiletaInteligente.ino`** (calentador + luces + Telegram, todo en un solo archivo).

## Sistema 1 — Calentador automático

- [x] Código base (sensor DS18B20 + relé + histéresis)
- [x] LCD 16x02 mostrando temperatura y estado
- [x] Ciclo completo verificado en protoboard (2026-06-25)
- [x] Control por Telegram (auto / forzar ON / forzar OFF)
- [ ] Conseguir fuente 12V y conectar el cartucho calefactor real
- [ ] Probar el sistema completo con el calentador real

## Sistema 2 — Luces al ritmo de la música

- [x] Sensor de sonido leído por el ESP32 (pin GPIO34)
- [x] Detección de graves/agudos por FFT (arduinoFFT)
- [x] 4 LEDs con efecto disco según la música
- [x] Control por Telegram (auto / ON / OFF)
- [x] Verificado funcionando (archivo "posta" de los compañeros)

## Sistema 3 — Cobertor automático retráctil

- [x] Motores comprados: 2× Pololu micro metal 6V 500 RPM (eje 3mm) + 2 acoples 5mm
- [x] Mecanismo definido: rodillo (motor A) + cables de tracción por guías (motor B)
- [x] Programada la apertura/cierre: lógica "un motor tira / el otro suelto", PWM y corte por fin de carrera
- [x] Integrado a Telegram: /cobertor_abrir, /cobertor_cerrar, /cobertor_parar (+ aviso al terminar)
- [x] Pines asignados y documentados en CONEXIONES.md
- [ ] Conseguir LM2596 + resistencia 10kΩ (pull-up del FC abierto)
- [ ] Montar el mecanismo físico y conectar L298N + motores + fines de carrera
- [ ] Probar en hardware (calibrar velocidad y direcciones de giro)

## Bot de Telegram

- [x] Bot creado con BotFather y token funcionando
- [x] Librerías: UniversalTelegramBot 1.3.0 + ArduinoJson **6.x** (la 7 no compila)
- [x] Comandos integrados para calentador y luces
- [ ] Comandos para el cobertor (cuando exista el Sistema 3)

---

## Historial de sesiones

### Sesión 2026-06-25
- Definición del proyecto (3 sistemas + Telegram)
- Calentador (sensor + LCD + LEDs + relé) armado y verificado en protoboard

### Sesión 2026-07-16
- Integrados en un solo programa `PiletaInteligente.ino`: calentador + luces disco + Telegram
- Base: archivo "posta" (luces, ya verificado) + bot de Telegram que ya andaba
- Agregado control del calentador por Telegram (auto / ON / OFF), con apagado por seguridad si falla el sensor
- Secretos (WiFi + token) separados en `config.h` (fuera de GitHub)
- Limpieza: eliminadas las versiones intermedias y duplicadas; `prueba_sensor.ino` reemplazado por el programa unificado
- Documentación para los compañeros: `README.md` + `COMPONENTES.md`
- **Pendiente de probar en hardware** por Mariano (compilar con ArduinoJson 6.21.5 y cargar al ESP32)
