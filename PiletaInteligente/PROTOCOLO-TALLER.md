# 🛠️ Protocolo del taller — dejar las luces andando en UNA visita

Checklist en orden. Tiempo estimado: 30–40 minutos. Antes de ir: bajar el `.ino`
nuevo de GitHub y pisarlo en la carpeta de siempre (el `config.h` no se toca).

## 1) Cables (5 min, con el ESP32 DESENCHUFADO)

- [ ] **Módulo de sonido 1** (el que ya está): mover su cable **VCC de 3V3 → VIN (5V)**.
      Los otros 2 cables quedan igual (GND y AO→GPIO34).
- [ ] **Módulo de sonido 2** (el de repuesto): conectar
      **VCC → 3.3V** · **GND → riel GND** · **DO → GPIO35**. (Su AO queda libre.)
      ⚠️ Este módulo va a 3.3V, NUNCA a 5V.

## 2) Cargar y verificar la base (10 min)

- [ ] Cargar el `.ino` al ESP32. Monitor Serie a 115200: WiFi conectado + Bot listo.
- [ ] `/diag` en silencio → anotar PICO A PICO y "Promedio (DC)".
      Con el módulo 1 a 5V, el DC debería subir respecto de los ~200 de antes.
- [ ] `/diag` GRITANDO pegado al mic 1 → el campo **Maximo debe ser < 3000**.
      Si se acercara a 4095: volver el VCC del módulo 1 a 3V3 y avisar a Claude.

## 3) Calibrar el potenciómetro del módulo 2 (10 min — es lo más importante)

El módulo 2 tiene 2 LEDs: uno de encendido (queda fijo) y uno de SEÑAL.
Con un destornillador chico, girar SU potenciómetro así:

- [ ] En **silencio**: girar hasta que el LED de señal quede **APAGADO** del todo.
- [ ] Poner **música al volumen de la demo** cerca del sensor: girar de a poco en
      sentido contrario hasta que el LED de señal **titile al ritmo de la música**.
- [ ] Verificar: en silencio NO titila; con música SÍ. Ese es el punto justo.
- [ ] `/audio` con música → la línea "DO (2do mic)" debe mostrar entre 1 y 8 flancos.
      (Si muestra "ruido: pin sin conectar?", revisar el cable de GPIO35.)

## 4) La prueba de fuego (5 min)

- [ ] `/luces_auto` + música 🎵 → sombra rotante recorriendo los colores y
      apagón strobe en cada golpe.
- [ ] En silencio → todas prendidas fijas, sin parpadeos sueltos.
- [ ] `/temperatura 28` → responde "Guardada" (y `/temp` muestra el objetivo nuevo).

## 5) Si algo no convence — plan B en 30 segundos

| Síntoma | Solución |
|---|---|
| Golpes de más (parpadea sin música) | `/sonido_do` (manda solo el detector calibrado a ojo) |
| Golpes de menos | `/sonido_ao`; si sigue corto, girar el pote del módulo 2 un pelo más sensible y volver a `/sonido_mixto` |
| Nada convence | Mandar a Claude una foto de `/audio` y `/diag` CON música sonando |

## 6) Opcional si sobra tiempo (mejora visual)

Los LEDs azules y blancos brillan menos que el resto (necesitan más voltaje).
Para emparejarlos: en cada LED azul y blanco, reemplazar su resistencia de 220Ω
por **dos de 220Ω en paralelo** (mismo agujero de protoboard, una al lado de la
otra). Rojo, amarillo y verde quedan como están.
