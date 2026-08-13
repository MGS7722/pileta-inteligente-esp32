# 📌 Pendientes — Pileta Inteligente

> **Fuente única de pendientes del proyecto.** Si algo queda por hacer, se anota acá, no en el
> README ni en un comentario del código. Actualizado el **2026-08-13**.
>
> 🔴 = bloquea la entrega · 🟠 = importante · 🟡 = mejora

---

## 🙋 Requieren a Mariano (decisiones o trabajo físico)

| # | Qué | Estado |
|---|---|---|
| 1 | 🔴 **Montar el mecanismo con la lona** y anotar los tres valores definitivos: `/velocidad`, `/tiempo_abrir` y `/tiempo_cerrar`. Con el hilo pelado hizo falta `/velocidad 100`; con la lona puesta va a costar más | pendiente |
| 1b | 🟠 **Conectar los 2 fines de carrera.** Ojo: desde el 2026-08-13 **ya no cortan el movimiento** (el recorrido se mide por tiempo). Sólo informan la posición en `/status`, que ahora muestra cada pin por separado. Se comprueban sin mover motores: apretar cada uno con el dedo y mirar `/status` | pendiente |
| 2 | ~~Definir el sentido de giro de cada motor~~ | ✅ **resuelto el 2026-08-13 sin tocar cables**: `/sentido_a` y `/sentido_b` invierten cada motor desde Telegram y quedan en NVS |
| 3 | ~~Decidir la histéresis del calentador~~ | ✅ **decidido el 2026-08-13**: baja de 5 °C a **2 °C**. Ya está cargado en el ESP32. **Falta verificar el ciclo con el cartucho sumergido** (no se puede probar en seco) |
| 4 | ~~Prueba visual de los cuatro efectos~~ | ✅ **hecho el 2026-08-13**: siguen el ritmo, las tres bandas se mueven por separado |
| 5 | 🟡 **Conseguir un capacitor de 500–1000 µF** (6,3 V o más) para poner entre `+5V` y `GND` de la tira. Adafruit lo especifica antes de conectar una tira a cualquier fuente. Con `/corriente 120` no es crítico; **sí conviene** si se pasa a un cargador de 2 A con `/corriente 500` | a comprar |

---

## ⚙️ Técnicos

| # | Qué | Detalle |
|---|---|---|
| 6 | 🟠 **Calibrar el piso de ruido en el lugar definitivo** | El taller dio `/piso 12`. El patio de la pileta va a tener otro ruido de fondo: `/espectro` en silencio y ajustar. No requiere recompilar. **Ojo con la fuente de audio**: si la música sale de un celular, sus graves llegan con crudo ~11,8 y el piso de 12 los corta — ahí conviene `/piso 10` |
| 7 | 🟠 **Decisión abierta desde el 2026-08-06**: si el latido de la tarea de Telegram se detiene (bot colgado), ¿el ESP32 debe reiniciarse solo? Recupera el control remoto sin intervención, pero un reinicio deja el calentador en OFF por diseño. **Menos urgente desde el 2026-08-13**: la causa principal de los cuelgues era el timeout del saludo TLS, ya bajado de 120 s a 5 s | espera decisión de Mariano |
| 7c | 🟡 **Latencia del bot**: responde con hasta ~2,5 s de demora porque consulta a intervalos (`INTERVALO_TELEGRAM`). La mejora de fondo es **long polling** —que Telegram mantenga la consulta abierta y conteste apenas llega un mensaje—, lo que además haría muchas menos conexiones. Cambio acotado a la tarea de Telegram | propuesto, no hecho |
| 7d | 🟡 **El diámetro cambia a medida que el hilo pasa de un carrete al otro**: al principio del recorrido el que suelta tiene más diámetro que el que recoge y el hilo se afloja; al final pasa lo contrario y se tensa. Con motores rígidamente sincronizados es inevitable (en un cassette se resuelve con embrague). Si aparece, la salida simple es que el motor que desenrolla vaya más despacio | a observar con la lona puesta |
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
- **Los dos motores se mueven JUNTOS y el corte por tiempo es exacto** — 2026-08-13: un
  movimiento de 10 s midió 10008 ms y una prueba de 2 s midió 2019 ms.
- **El bot ya no se queda mudo** — el saludo TLS tenía un timeout de fábrica de 120 s; con
  `setHandshakeTimeout(5)` el latido bajó de 117 s a un máximo de 4 s.
- **El motor zumbaba y no arrancaba** — era el PWM a 1 kHz (valor de fábrica del core) más la
  falta de par de arranque. Resuelto con 8 kHz y patada al 100 % durante 300 ms.
- **El motor B tenía una soldadura floja** — 2026-08-13. No era el firmware: imitaba las otras
  fallas y costó separarlo de ellas.
