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

### Sesión 2026-07-23
- Prueba en hardware real: WiFi, Telegram, sensor de temperatura, LCD, relé y luces ON/OFF ✔
- Problema detectado: las luces en AUTO no reaccionaban a la música. Diagnóstico con el comando
  nuevo `/diag` (señal cruda del ADC): el micrófono HW-484 a 3.3V entrega una señal debilísima
  (pico a pico ~17 ambiente / ~40 música baja / ~177 grito) que se solapa con el ruido ambiente —
  ningún umbral fijo puede separarlos
- Fix 1: volumen medido por pico a pico + DC quitado antes de la FFT (los "graves" daban falsos)
- Fix 2: luces solo prendidas/apagadas (sin PWM), para cuidar los LEDs y no ensuciar el micrófono
- Fix 3 (definitivo): **detección de ritmo adaptativa** — el sistema aprende solo el nivel
  ambiente (base) y dispara un apagón de 110 ms en cada golpe que la supera con margen
  (plan LEGO en `docs/PLAN-LUCES-ADAPTATIVAS.md`, ejecutado por Sonnet, auditado por Fable)
- Mejora de hardware pendiente de verificar: alimentar el sensor de sonido con 5V (pin VIN) en
  vez de 3.3V — el módulo pide 4-6V; verificar con `/diag` que el Máximo quede lejos de 4095
- **Auditoría de raíz** (pedida por Mariano: "algo redundante que no sirve"): confirmado — la FFT
  era complejidad muerta (solo clasificaba graves/agudos, y con 14-30 mV esa clasificación es
  ruido). Además: la ventana de 12,8 ms perdía los beats (evidencia: /audio=16 vs /diag=40 el
  mismo instante) y faltaba `client.setTimeout` (Telegram congelaba el show 1-2 s por consulta)
- **Luces v3** (plan `docs/PLAN-LUCES-V3-SIN-FFT.md`): FFT eliminada por completo (~80 líneas,
  2 KB de RAM y la librería arduinoFFT fuera del proyecto); medición por pico a pico con ventana
  de ~35 ms; efectos sin espectro: sombra rotante con música + strobe en cada golpe;
  `client.setTimeout(2000)` recuperado; nuevo comando `/temperatura N` (guardado en NVS,
  sobrevive reinicios — estaba prometido en PROYECTO.md y faltaba)
- Los compañeros ahora instalan **5 librerías** (arduinoFFT ya no hace falta)
- **Hardware identificado con exactitud** (publicaciones de compra): sensor **KY-037** (LM393,
  especificación 4–6V, DOS unidades compradas) y pack de 100 LEDs 5mm difusos (Vf 1.7–3.8V)
- **Luces v4 — detección BICANAL** (plan `docs/PLAN-LUCES-V4-BICANAL.md`): módulo 1 por AO a
  5V (VIN) con la base adaptativa + módulo 2 (el de repuesto) por DO a 3.3V en GPIO35 como
  detector de golpes por hardware (umbral con su potenciómetro, LED de placa como feedback).
  Fusión con fuente seleccionable (/sonido_mixto | /sonido_ao | /sonido_do, guardada en NVS)
  y filtro anti-pin-flotante. Redundancia para que la primera prueba en el taller funcione
- **Nuevo `PROTOCOLO-TALLER.md`**: checklist paso a paso para la visita al taller (cables,
  verificación /diag, calibración del potenciómetro a ojo, prueba de fuego y plan B)

#### Atribución por modelo (sesión 2026-07-23)
- **Opus 4.8**: /diag, pico a pico + DC removal, luces binarias, efecto en negativo (commits
  hasta `7477557`)
- **Fable 5**: investigación del sensor, diagnóstico con mediciones, plan LEGO adaptativo,
  auditoría del diff, docs
- **Sonnet 5** (subagente ejecutor): implementación del plan adaptativo (commit `b785ddc`)
