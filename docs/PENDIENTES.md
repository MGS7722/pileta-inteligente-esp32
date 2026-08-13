# 📌 Pendientes — Pileta Inteligente

> **Fuente única de pendientes del proyecto.** Si algo queda por hacer, se anota acá, no en el
> README ni en un comentario del código. Actualizado el **2026-08-13**.
>
> 🔴 = bloquea la entrega · 🟠 = importante · 🟡 = mejora

---

## 🙋 Requieren a Mariano (decisiones o trabajo físico)

| # | Qué | Estado |
|---|---|---|
| 1 | 🔴 **Conectar los 2 fines de carrera del cobertor** y verificar que corten el movimiento. Se comprueba sin mover motores: apretar cada uno con el dedo y mirar `/status` (ver `CABLEADO-PASO-A-PASO.md`, paso 7) | pendiente |
| 2 | 🔴 **Montar el mecanismo** (rodillo + cables + lona) y definir el sentido de giro de cada motor. Si alguno gira al revés, se invierten sus dos cables en el L298N — no se toca el código | pendiente |
| 3 | ~~Decidir la histéresis del calentador~~ | ✅ **decidido el 2026-08-13**: baja de 5 °C a **2 °C**. Ya está cargado en el ESP32. **Falta verificar el ciclo con el cartucho sumergido** (no se puede probar en seco) |
| 4 | ~~Prueba visual de los cuatro efectos~~ | ✅ **hecho el 2026-08-13**: siguen el ritmo, las tres bandas se mueven por separado |
| 5 | 🟡 **Conseguir un capacitor de 500–1000 µF** (6,3 V o más) para poner entre `+5V` y `GND` de la tira. Adafruit lo especifica antes de conectar una tira a cualquier fuente. Con `/corriente 120` no es crítico; **sí conviene** si se pasa a un cargador de 2 A con `/corriente 500` | a comprar |

---

## ⚙️ Técnicos

| # | Qué | Detalle |
|---|---|---|
| 6 | 🟠 **Calibrar el piso de ruido en el lugar definitivo** | El taller dio `/piso 12`. El patio de la pileta va a tener otro ruido de fondo: `/espectro` en silencio y ajustar. No requiere recompilar. **Ojo con la fuente de audio**: si la música sale de un celular, sus graves llegan con crudo ~11,8 y el piso de 12 los corta — ahí conviene `/piso 10` |
| 7 | 🟠 **Decisión abierta desde el 2026-08-06**: si el latido de la tarea de Telegram se detiene (bot colgado), ¿el ESP32 debe reiniciarse solo? Recupera el control remoto sin intervención, pero un reinicio deja el calentador en OFF por diseño | espera decisión de Mariano |
| 7b | 🔴 **Probar los TRES sistemas funcionando a la vez.** Hasta ahora cada uno se verificó por separado. Los motores tiran picos de más de 1 A en cada arranque y comparten el GND con el ESP32 y el micrófono: es el escenario donde puede aparecer ruido en el ADC, parpadeo en la tira o un reinicio por caída de tensión. El sistema ya tiene con qué detectarlo (aviso de brownout al arrancar, `/diag`) | pendiente |
| 8 | 🟡 **Aliasing**: no hay filtro anti-aliasing antes del ADC, así que el contenido por encima de 5 kHz se pliega dentro del rango analizado. Con música real no molestó, pero es la explicación de las frecuencias dominantes raras cuando la señal es débil | no urgente |
| 9 | 🟡 **Documentar la arquitectura viva** (los 7 pilares del estándar: `ARQUITECTURA.md` + backend, frontend, datos, integración, seguridad, infraestructura). El proyecto no los tiene | propuesto, no hecho |
| 10 | 🟡 **Ordenar `docs/`**: hay 4 planes, 3 de ellos ya cerrados (`PLAN-LUCES-ADAPTATIVAS`, `-V3-SIN-FFT`, `-V4-BICANAL`). Convendría moverlos a `docs/planes-cerrados/` para que no confundan con el vigente | **cambio estructural: avisar antes de hacerlo** |

---

## ✅ Cerrados hace poco (para no volver a abrirlos)

- **La tira WS2812 anda** — 21 píxeles, colores correctos, verificado el 2026-08-13.
- **La tira no contamina el micrófono** — medido seis veces alternando, con dos brillos.
- **El limitador de corriente funciona** — verificado comparando brillo 70 contra brillo 100.
- **La FFT separa las tres bandas** — probado con tonos puros de 100, 1000 y 3000 Hz.
- **El nivel lógico alcanza sin level shifter** — `VIN` a 4,4 V, margen de 0,22 V.
- **Los dos motores del cobertor giran** — probado el 2026-08-06 con la fuente a 8 V.
- **El calentador cumple su ciclo en AUTO** — calentó y cortó solo el 2026-08-06.
