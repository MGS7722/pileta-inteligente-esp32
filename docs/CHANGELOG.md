# 📓 Changelog — Pileta Inteligente

> Qué cambió en cada versión del programa, en criollo. Las versiones numeran el **sistema de
> luces**, que es lo que más veces se rehízo; el resto del proyecto acompaña.
>
> El relato completo de cada sesión —con las mediciones, los errores y por qué se decidió cada
> cosa— está en [`../AVANCES.md`](../AVANCES.md). Acá va el resumen.

---

## v5.1 — 2026-08-13 · La tira en el taller

Primera prueba en hardware de la tira WS2812 y recalibración del análisis de sonido con datos
medidos.

### Agregado
- **Puerta de ruido por banda** (`/piso N`, guardada en NVS): a cada banda se le resta el piso
  de ruido del lugar antes de normalizar. Lo que no lo supera vale cero. Es el *squelch* de
  WLED, y resuelve que una banda sin contenido se llenara con el ruido del micrófono.
- `/diag` ahora informa el **rango del pico a pico de los últimos ~10 segundos**, no sólo la
  foto instantánea de 25,6 ms, y avisa si el ruido de fondo supera el umbral de música.
- `/espectro` muestra la columna **UTIL** (lo que queda después del piso) y el piso activo.

### Cambiado
- **`SONIDO_MINIMO`: 45 → 90.** El valor viejo había quedado por debajo del ruido de fondo
  real del taller (32-56), así que el sistema declaraba "música detectada" en silencio. La
  música medida da 180-591.
- **La detección de golpes usa la señal útil** en vez de la cruda: el piso de ruido se suma por
  igual al golpe y al promedio, y descontarlo aumenta el contraste.
- **La tira es de 21 píxeles** (70 cm), no 15: es la vuelta completa a la pileta. Divide exacto
  en tres zonas de 7 para el efecto ESPECTRO.
- **Histéresis del calentador: 5 °C → 2 °C.** Con 5 y un objetivo de 28 °C, el agua oscilaba
  entre 23 y 28 — una diferencia que se siente al meterse. Ahora el rango es 26-28.

### Verificado en hardware
- **Los dos defectos, cerrados**: en silencio las bandas pasaron de 78/88/80 a **2/0/0**, y con
  un tono puro de 1000 Hz de 72/100/100 a **4/41/0**.
- **Los cuatro efectos siguen la música**, con las tres zonas moviéndose por separado.
- La tira enciende con los colores correctos sin `/orden` (chip GRB).
- El nivel lógico alcanza sin level shifter: `VIN` a **4,4 V** → umbral 3,08 V → 0,22 V de
  margen. Más bajo es *mejor* acá.
- **La tira no contamina el micrófono** (el riesgo que más preocupaba del plan v5).
- El limitador de corriente hace lo que promete.
- La FFT es exacta en las tres bandas (tonos de 100, 1000 y 3000 Hz).

---

## v5.0 — 2026-08-07 · Análisis de espectro real

- Sistema de luces **reescrito de cero**: FFT de 256 muestras a 10 kHz → graves / medios /
  agudos → ganancia automática por banda → suavizado → detección de golpes por energía del
  último segundo.
- **Tira WS2812** reemplaza a los 8 LEDs, con cuatro efectos (espectro, mezcla, cometa,
  arcoíris) y respiración lenta en silencio.
- **Limitador de corriente por software**: permite colgar la tira del propio ESP32 sin una
  tercera fuente.
- Comandos nuevos: `/efecto`, `/leds`, `/brillo`, `/corriente`, `/orden`, `/luces_test`,
  `/espectro`, `/onda`.
- Eliminado todo el canal DO (código para un micrófono que no está conectado) y la base
  adaptativa exponencial, que se cegaba con música sostenida.
- El arranque informa el motivo del último reinicio (brownout, watchdog, panic).

## v4 — 2026-07-23 · Detección bicanal *(nunca llegó a hardware)*

- Dos micrófonos: AO analógico + DO como detector de golpes por hardware. Se descartó al
  confirmarse que sólo hay **un** micrófono conectado.

## v3 — 2026-07-23 · Sin FFT

- FFT eliminada por completo: con el micrófono a 3,3 V daba 17 counts de pico a pico y
  clasificar frecuencias sobre eso era clasificar ruido. Medición por pico a pico.
- `client.setTimeout(2000)`: Telegram congelaba el show 1-2 s por consulta.
- `/temperatura N` guardado en NVS.

## v2 — 2026-07-16 · Todo en un programa

- Calentador + luces + cobertor + Telegram integrados en `PiletaInteligente.ino`.
- Cobertor completo: 2 motores por L298N, fines de carrera, timeout de seguridad.
- Secretos separados en `config.h`, fuera de git.

## v1 — 2026-06-25 · Calentador

- Sensor DS18B20 + relé + LCD, verificado en protoboard.
