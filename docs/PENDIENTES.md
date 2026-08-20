# 📌 Pendientes — Pileta Inteligente

> **Fuente única de pendientes del proyecto.** Si algo queda por hacer, se anota acá, no en el
> README ni en un comentario del código. Actualizado el **2026-08-20**.
>
> 🔴 = bloquea la entrega · 🟠 = importante · 🟡 = mejora

---

## 🙋 Requieren a Mariano (decisiones o trabajo físico)

| # | Qué | Estado |
|---|---|---|
| 1 | 🔴 **Montar el mecanismo con la lona** y anotar los tres valores definitivos: `/velocidad`, `/tiempo_abrir` y `/tiempo_cerrar`. Con el hilo pelado hizo falta `/velocidad 100`; con la lona puesta va a costar más | pendiente |
| 1b | 🟠 **Conectar los 2 fines de carrera.** Ojo: desde el 2026-08-13 **ya no cortan el movimiento** (el recorrido se mide por tiempo). Sólo informan la posición en `/status`, que ahora muestra cada pin por separado. Se comprueban sin mover motores: apretar cada uno con el dedo y mirar `/status` | pendiente |
| 2 | ~~Definir el sentido de giro del cobertor~~ | ✅ **resuelto el 2026-08-13**: `/cobertor_sentido` invierte el conjunto desde Telegram y queda en NVS. Los dos motores reciben siempre la misma polaridad, así que no pueden girar uno contra el otro |
| 3 | ~~Decidir la histéresis del calentador~~ | ✅ **decidido el 2026-08-13**: baja de 5 °C a **2 °C**. Ya está cargado en el ESP32. **Falta verificar el ciclo con el cartucho sumergido** (no se puede probar en seco) |
| 4 | ~~Prueba visual de los cuatro efectos~~ | ✅ **hecho el 2026-08-13**: siguen el ritmo, las tres bandas se mueven por separado |
| 5 | 🟡 **Conseguir un capacitor de 500–1000 µF** (6,3 V o más) para poner entre `+5V` y `GND` de la tira. Adafruit lo especifica antes de conectar una tira a cualquier fuente. Con `/corriente 120` no es crítico; **sí conviene** si se pasa a un cargador de 2 A con `/corriente 500` | a comprar |

---

## ⚙️ Técnicos

| # | Qué | Detalle |
|---|---|---|
| 6 | 🟠 **Calibrar el piso de ruido en el lugar definitivo** | El taller dio `/piso 12`. El patio de la pileta va a tener otro ruido de fondo: `/espectro` en silencio y ajustar. No requiere recompilar. **Ojo con la fuente de audio**: si la música sale de un celular, sus graves llegan con crudo ~11,8 y el piso de 12 los corta — ahí conviene `/piso 10` |
| 7 | 🟠 **Decisión abierta desde el 2026-08-06**: si el latido de la tarea de Telegram se detiene (bot colgado), ¿el ESP32 debe reiniciarse solo? Recupera el control remoto sin intervención, pero un reinicio deja el calentador en OFF por diseño. **Menos urgente desde el 2026-08-13**: la causa principal de los cuelgues era el timeout del saludo TLS, ya bajado de 120 s a 5 s | espera decisión de Mariano |
| 7c | 🟠 **Latencia de fondo del bot: long polling** | Medido el 2026-08-20: cada vuelta paga un saludo TLS completo, porque la librería cierra la conexión cuando no hay mensajes (`UniversalTelegramBot.cpp:433`), y encima consulta cada 2,5 s pidiendo `limit=1` — un mensaje por vez. Peor caso antes de la v5.4: ~15 s por comando; ahora ~7 s. La solución de fondo es **long polling** (`bot.longPoll`), que deja la consulta abierta y contesta apenas llega el mensaje. ⚠️ Necesita prueba en hardware: durante la espera la tarea no cede CPU y hay que verificar el watchdog del núcleo 0, y el latido de `ultimoLatidoTelegram` se tiene que ajustar o va a parecer colgado | propuesto, no hecho |
| 7d | 🟡 **El diámetro cambia a medida que el hilo pasa de un carrete al otro**: al principio del recorrido el que suelta tiene más diámetro que el que recoge y el hilo se afloja; al final pasa lo contrario y se tensa. Con motores rígidamente sincronizados es inevitable (en un cassette se resuelve con embrague). Si aparece, la salida simple es que el motor que desenrolla vaya más despacio | a observar con la lona puesta |
| 7b | 🔴 **Probar los TRES sistemas funcionando a la vez.** Hasta ahora cada uno se verificó por separado. Los motores tiran picos de más de 1 A en cada arranque y comparten el GND con el ESP32 y el micrófono: es el escenario donde puede aparecer ruido en el ADC, parpadeo en la tira o un reinicio por caída de tensión. El sistema ya tiene con qué detectarlo (aviso de brownout al arrancar, `/diag`) | pendiente |
| 8 | 🟡 **Aliasing**: no hay filtro anti-aliasing antes del ADC, así que el contenido por encima de 5 kHz se pliega dentro del rango analizado. Con música real no molestó, pero es la explicación de las frecuencias dominantes raras cuando la señal es débil | no urgente |
| 9 | 🟡 **Documentar la arquitectura viva** (los 7 pilares del estándar: `ARQUITECTURA.md` + backend, frontend, datos, integración, seguridad, infraestructura). El proyecto no los tiene | propuesto, no hecho |
| 10 | 🟡 **Ordenar `docs/`**: hay 4 planes, 3 de ellos ya cerrados (`PLAN-LUCES-ADAPTATIVAS`, `-V3-SIN-FFT`, `-V4-BICANAL`). Convendría moverlos a `docs/planes-cerrados/` para que no confundan con el vigente | **cambio estructural: avisar antes de hacerlo** |
| 11 | 🟠 **El limitador de corriente puede apagar la tira entera sin avisar** | Cada píxel consume 1 mA aunque esté apagado, y el limitador resta ese consumo fijo del presupuesto de `/corriente`. Si `/leds` alcanza a `/corriente` (por ejemplo `/leds 120` con `/corriente 120`), no queda nada para el color: **la tira se pinta negra en todos los modos, incluso `/luces_on`**, y no se informa en ningún lado. `/leds` y `/corriente` deberían rechazar la combinación imposible, y `/status` avisar cuando el reposo se come el presupuesto | detectado el 2026-08-20, no hecho |
| 12 | ✅ **v5.4 cargada y verificada el 2026-08-20** | Arranque limpio, sin brownout. La cola vieja y la instrumentacion funcionan. **La verificacion de `/help` dio NEGATIVA**: sigue llegando por triplicado, medido `respuesta 16805 ms` (dos mensajes x 8 s de reintentos). Ver #14 | cerrado con resultado negativo |
| 13 | 🟡 **Límite conocido de la librería 1.3.0** | `readHTTPAnswer()` corta la lectura apenas hay datos disponibles (`if (responseReceived) break;`), así que una respuesta partida en varios segmentos TCP puede leerse incompleta. No se puede arreglar sin tocar la librería —que es dependencia oficial y no se modifica—; se mitiga manteniendo todos los mensajes cortos (hoy ninguno pasa de 907 bytes) | documentado, sin acción |
| 14 | ✅ **Resuelto el 2026-08-20 (v5.5)**: el envío de Telegram dejó de pasar por la librería. Se arma el POST a mano, se lee el `Content-Length` y se consumen exactamente esos bytes, y **no se reintenta nunca**, así que un mensaje no puede duplicarse | cerrado |
| 15 | 🟠 **El límite para leer la confirmación de Telegram quedó corto** | Se fijó en 5 s (`TELEGRAM_ESPERA_MS`) con los tiempos del taller, y la red de la pileta se degradó a saludos TLS de 7 a 14,7 s. El envío funciona —los mensajes llegan— pero a veces no alcanza a leer la confirmación y lo reporta como fallo. Subirlo a ~15 s, y de paso revisar `HANDSHAKE_TLS_SEGUNDOS` (hoy 5), que con esta red puede estar abortando saludos que iban a completarse | pendiente, de una línea |
| 16 | 🟡 **Cuánto cuesta refrescar la pantalla** | Cada carácter viaja al PCF8574 en dos nibbles y cada nibble son tres transacciones I2C: refrescar las dos filas serían unos 44 ms de loop bloqueado cada 2 s. Es una CUENTA, no una medición — hay que medirlo antes de decidir nada. Si se confirma, la salida es escribir sólo los caracteres que cambiaron | a medir |

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
