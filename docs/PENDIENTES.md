# 📌 Pendientes — Pileta Inteligente

> **Fuente única de pendientes del proyecto.** Si algo queda por hacer, se anota acá, no en el
> README ni en un comentario del código. Actualizado el **2026-09-21**.
>
> 🔴 = bloquea la entrega · 🟠 = importante · 🟡 = mejora

---

## ⏸️ Estado: etapa cerrada el 2026-09-21, con el hardware esperando

Mariano pasa a un invento nuevo, así que esta etapa se cierra acá. **El proyecto no está
terminado**, y conviene decirlo con todas las letras para que quien lo retome —él o una sesión
futura— no lo lea como si lo estuviera.

**Lo que está andando y verificado:** el calentador cumple su ciclo en AUTO, la tira de 21 píxeles
sigue el ritmo con las tres bandas separadas, los dos motores del cobertor se mueven juntos con el
corte por tiempo exacto (10 s midieron 10008 ms), y el bot de Telegram responde sin quedarse mudo
ni duplicar mensajes. En 45 minutos de captura con los tres sistemas conviviendo no hubo un solo
brownout, reinicio ni watchdog.

**Lo que falta es casi todo trabajo físico o una decisión, no código:**

| Qué falta | Quién |
|---|---|
| 🔴 #1 Montar el mecanismo con la lona y anotar los tiempos y velocidades definitivos | Mariano, en el taller |
| 🔴 #7b Probar los tres sistemas a la vez **moviendo los motores con el calentador prendido** — es el único cruce que no se dio en la misma ventana | Mariano, en el taller |
| 🟠 #1b Conectar los dos fines de carrera (ya no cortan el movimiento, sólo informan) | Mariano |
| 🟠 #6 Calibrar el piso de ruido en el patio, que no es el del taller | Mariano, con `/espectro` |
| 🟠 #7 Decidir si el ESP32 se reinicia solo cuando el bot se cuelga | decisión de Mariano |
| 🟠 #7e Decidir qué hacer con los 3,5-5 s de latencia por consulta, o convivir con ellos | decisión de Mariano |
| 🟠 #7d Encontrar con la lona puesta el par de velocidades que deja el hilo parejo | Mariano, en el taller |
| 🟠 #11 **El único defecto de software abierto**: el limitador puede pintar la tira entera de negro sin avisar si `/leds` alcanza a `/corriente`. No está arreglado | código |

**El aviso eléctrico que sigue en pie:** con las luces encendidas, activar el relé del calentador
baja el brillo del LCD. Es el riel de 5 V cediendo —480 mA presupuestados contra los 500 mA de un
USB 2.0, y la bobina del relé son 75 mA—. No llegó a brownout, pero es el aviso previo. La salida
está documentada desde el 2026-08-13: **alimentar el ESP32 con un cargador de 2 A** en vez del USB
de la notebook. Hacer eso **antes** de la prueba #7b.

> **Dónde vive el proyecto desde el 2026-09-21:**
> `C:\Users\salam\Downloads\claude\MGS-Inventos\pileta-inteligente` (antes `claude\ESP32-proyecto`).
> Al lado está `MGS-Inventos\tira-led`, que usa el segundo ESP32 y parte de la misma tira. Son dos
> repositorios distintos; `MGS-Inventos\` es una carpeta del disco, sin git.
>
> **El inventario de hardware es UNO SOLO para los dos proyectos** (anotado el 2026-09-21): no se
> compró nada aparte para la tira del monitor, así que **no queda ningún ESP32 libre** y del rollo
> de tira quedan unos 81 píxeles. La cuenta al día está en
> [`../PiletaInteligente/COMPONENTES.md`](../PiletaInteligente/COMPONENTES.md).
>
> **Nada pendiente de subir.** Verificado el 2026-09-21 con `git ls-remote origin master`: el remoto
> está en `e27d668`, el mismo commit que la rama local. La nota de `AVANCES.md` que decía que
> quedaba un commit sin subir quedó corregida.

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
| 7c | ❌ **Long polling: PROBADO EN HARDWARE Y DESCARTADO el 2026-08-24** | La sospecha era correcta y peor de lo previsto. Con `bot.longPoll = 25` la placa entra en **bucle de reinicio**: 4 reinicios en 130 s, con `task_wdt: IDLE0 (CPU 0) did not reset the watchdog in time` y `CPU 0: telegram`. La causa está en la librería, `readHTTPAnswer()` (`UniversalTelegramBot.cpp:106`): el bucle `while (millis() - now < longPoll * 1000 + waitForResponse)` **gira en vacío sin ceder el CPU** mientras no llegan datos, y 33 s de giro dejan sin correr a IDLE0. No se arregla desde afuera — habría que modificar la dependencia. **Corolario que sirve igual: `waitForResponse` tampoco puede subirse libremente**, porque el giro dura eso mismo; se dejó en 3000 ms (de 1500 de fábrica), por debajo de los 5 s del watchdog | **cerrado con resultado negativo** |
| 7e | 🟠 **La latencia sigue: ~3,5 a 5 s por consulta** | Con el long polling descartado, cada vuelta sigue pagando un saludo TLS entero. Medido el 2026-08-24 tras la reversión: 3548 / 4548 / 4558 / 4653 ms. Los caminos que quedan, ninguno trivial: (a) mantener viva la conexión a mano, sin la librería —el envío propio ya demostró que se puede—; (b) migrar a `AsyncTelegram2`, evaluada y descartada el 2026-08-20 por costo de migración; (c) convivir con los 5 s. **Requiere decisión de Mariano antes de invertir en ello** | abierto |
| 7d | 🟠 **El diámetro cambia a medida que el hilo pasa de un carrete al otro**: al principio del recorrido el que suelta tiene más diámetro que el que recoge y el hilo se afloja; al final pasa lo contrario y se tensa. **Desde el 2026-08-20 hay con qué corregirlo**: `/velocidad_a` y `/velocidad_b` dan una velocidad propia a cada motor (v5.6). Falta la parte de campo: encontrar con la lona puesta qué par de valores deja el hilo parejo de punta a punta | herramienta lista, falta calibrar |
| 7b | 🔴 **Probar los TRES sistemas funcionando a la vez.** Hasta ahora cada uno se verificó por separado. Los motores tiran picos de más de 1 A en cada arranque y comparten el GND con el ESP32 y el micrófono: es el escenario donde puede aparecer ruido en el ADC, parpadeo en la tira o un reinicio por caída de tensión. El sistema ya tiene con qué detectarlo (aviso de brownout al arrancar, `/diag`) | pendiente |
| 8 | 🟡 **Aliasing**: no hay filtro anti-aliasing antes del ADC, así que el contenido por encima de 5 kHz se pliega dentro del rango analizado. Con música real no molestó, pero es la explicación de las frecuencias dominantes raras cuando la señal es débil | no urgente |
| 9 | 🟡 **Documentar la arquitectura viva** (los 7 pilares del estándar: `ARQUITECTURA.md` + backend, frontend, datos, integración, seguridad, infraestructura). El proyecto no los tiene | propuesto, no hecho |
| 10 | 🟡 **Ordenar `docs/`**: hay 4 planes, 3 de ellos ya cerrados (`PLAN-LUCES-ADAPTATIVAS`, `-V3-SIN-FFT`, `-V4-BICANAL`). Convendría moverlos a `docs/planes-cerrados/` para que no confundan con el vigente | **cambio estructural: avisar antes de hacerlo** |
| 11 | 🟠 **El limitador de corriente puede apagar la tira entera sin avisar** | Cada píxel consume 1 mA aunque esté apagado, y el limitador resta ese consumo fijo del presupuesto de `/corriente`. Si `/leds` alcanza a `/corriente` (por ejemplo `/leds 120` con `/corriente 120`), no queda nada para el color: **la tira se pinta negra en todos los modos, incluso `/luces_on`**, y no se informa en ningún lado. `/leds` y `/corriente` deberían rechazar la combinación imposible, y `/status` avisar cuando el reposo se come el presupuesto | detectado el 2026-08-20, no hecho |
| 12 | ✅ **v5.4 cargada y verificada el 2026-08-20** | Arranque limpio, sin brownout. La cola vieja y la instrumentacion funcionan. **La verificacion de `/help` dio NEGATIVA**: sigue llegando por triplicado, medido `respuesta 16805 ms` (dos mensajes x 8 s de reintentos). Ver #14 | cerrado con resultado negativo |
| 13 | 🟡 **Límite conocido de la librería 1.3.0** | `readHTTPAnswer()` corta la lectura apenas hay datos disponibles (`if (responseReceived) break;`), así que una respuesta partida en varios segmentos TCP puede leerse incompleta. No se puede arreglar sin tocar la librería —que es dependencia oficial y no se modifica—; se mitiga manteniendo todos los mensajes cortos (hoy ninguno pasa de 907 bytes) | documentado, sin acción |
| 14 | ✅ **Resuelto el 2026-08-20 (v5.5)**: el envío de Telegram dejó de pasar por la librería. Se arma el POST a mano, se lee el `Content-Length` y se consumen exactamente esos bytes, y **no se reintenta nunca**, así que un mensaje no puede duplicarse | cerrado |
| 15 | ✅ **Resuelto el 2026-08-20**: `TELEGRAM_ESPERA_MS` pasó de 5 s a **15 s**, con la medición que lo justifica (consultas de 7 a 14,7 s en la red de la pileta). `HANDSHAKE_TLS_SEGUNDOS` se dejó en 5 a propósito: no hay evidencia de que esté cortando saludos, y subirlo devolvería el riesgo del bot mudo | cerrado |
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
