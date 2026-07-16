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
- [x] Alimentación resuelta: fuente regulable del laboratorio a ~8V (sin LM2596 ni resistencias extra)
- [ ] Montar el mecanismo físico y conectar L298N + motores + fines de carrera
- [ ] Probar en hardware (calibrar velocidad y direcciones de giro)

## Bot de Telegram

- [x] Bot creado con BotFather y token funcionando (@ControlESP32Pileta_bot)
- [x] Librerías: UniversalTelegramBot 1.3.0 + ArduinoJson **6.21.5** (la 7 no compila) + core ESP32 3.3.10
- [x] Comandos integrados para calentador, luces y cobertor
- [x] Compila OK en la máquina de Mariano (solo warnings de librerías, inofensivos)

---

## Historial de sesiones

### Sesión 2026-06-25
- Definición del proyecto (3 sistemas + Telegram)
- Calentador (sensor + LCD + LEDs + relé) armado y verificado en protoboard

### Sesión 2026-07-16
- Integrados en un solo programa `PiletaInteligente.ino`: calentador + luces disco + cobertor + Telegram
- Base: archivo "posta" (luces, ya verificado) + bot de Telegram que ya andaba
- Calentador y luces por Telegram (auto / ON / OFF); ambos arrancan APAGADOS
- Luces ampliadas a 8 (4 colores × 2 lados, espejadas en 4 pines)
- Sistema 3 (cobertor) programado e integrado: 2 motores por L298N, lógica "un motor tira / el
  otro suelto", PWM, corte por fin de carrera, timeout de seguridad y aviso por Telegram
- Alimentación resuelta con la fuente de laboratorio doble regulable (12V calentador / ~8V motores)
- Secretos (WiFi + token) separados en `config.h` (fuera de GitHub, en `.gitignore`)
- Limpieza: eliminadas las versiones intermedias y duplicadas; `prueba_sensor.ino` reemplazado
- Documentación completa para los compañeros: `README.md`, `CABLEADO-PASO-A-PASO.md`,
  `CONEXIONES.md`, `COMPONENTES.md`
- Todo subido a GitHub y a la carpeta del Drive "Tecnicas Digitales"
- **Pendiente de probar en hardware** por Mariano (montar el cobertor, cargar el sketch y probar
  los 3 sistemas + comandos de Telegram; calibrar giro y velocidad de los motores)
