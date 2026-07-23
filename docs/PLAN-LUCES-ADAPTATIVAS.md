# PLAN LEGO — Detección de ritmo adaptativa para las luces

> **Autor:** Fable 5 (fase cerebro). **Ejecutor:** subagente Sonnet 5.
> **Alcance:** SOLO `PiletaInteligente/PiletaInteligente.ino` y la sección de calibración de `PiletaInteligente/README.md`. PROHIBIDO tocar cualquier otro archivo.

## Contexto (por qué)

Mediciones reales con `/diag` en el hardware (módulo HW-484 a 3.3V):

| Situación | Pico a pico |
|---|---|
| Ruido ambiente (sala con gente) | 17–26 |
| Música volumen bajo al lado del sensor | 16–40 |
| Grito pegado al sensor | 177 |

El umbral fijo actual (`UMBRAL_APAGADO = 50`) nunca se dispara con música baja, y
cualquier umbral fijo menor se solaparía con el ruido ambiente. Solución: **base
adaptativa** (el sistema aprende el nivel ambiente y detecta golpes RELATIVOS) +
**apagón con duración visible** (110 ms) y período refractario.

## Cambios exactos

### Bloque 1 — Constantes

**BUSCAR** el bloque que empieza con `// Las luces trabajan SOLO en dos estados` y
termina en `const int UMBRAL_APAGADO = 50;` (inclusive). **REEMPLAZAR TODO** por:

```cpp
// Las luces trabajan SOLO en dos estados: prendidas o apagadas (nunca a media
// luz), para no forzar los LEDs y para no meter el ruido eléctrico del PWM.
//
// --- Detección de ritmo ADAPTATIVA ---
// El sistema aprende solo el nivel de ruido ambiente (la "base") y considera
// GOLPE cuando el volumen instantáneo la supera con margen. Así reacciona igual
// con música fuerte o suave, sin depender de un umbral fijo.
const float FACTOR_GOLPE = 1.6;   // El volumen debe superar base*FACTOR para ser golpe
const int   GOLPE_MINIMO = 32;    // Piso absoluto: nunca disparar por debajo de esto
const int   SONIDO_MINIMO = 26;   // Piso de "hay sonido" (para elegir efecto A/B)
const unsigned long DURACION_APAGON_MS = 110;  // Cuánto quedan apagadas en cada golpe
const unsigned long REFRACTARIO_MS     = 150;  // Espera mínima entre golpes seguidos
```

**Además, ELIMINAR** las constantes viejas `RUIDO_DE_FONDO` y `SONIDO_MAXIMO` y su
bloque de comentarios (`// --- Sensibilidad del micrófono...` hasta la línea de
`SONIDO_MAXIMO` inclusive). `UMBRAL_CLASIFICACION` se queda.

### Bloque 2 — Variables de estado

**BUSCAR:**
```cpp
int    ultimoPicoAPico = 0;   // Volumen real medido (max - min de las muestras)
```
**REEMPLAZAR POR:**
```cpp
int    ultimoPicoAPico = 0;   // Volumen real medido (max - min de las muestras)
float  p2pBase = 20;          // Nivel de ruido ambiente aprendido (se adapta solo)
unsigned long ultimoMomentoConSonido = 0;  // Para dar memoria a la clasificación A/B
unsigned long inicioApagon = 0;            // Cuándo empezó el último apagón por golpe
```

### Bloque 3 — Actualizar la base en `muestrearAudio()`

**BUSCAR:**
```cpp
  ultimoPicoAPico = maximo - minimo;
```
**REEMPLAZAR POR:**
```cpp
  ultimoPicoAPico = maximo - minimo;

  // La base aprende el nivel ambiente: baja rápido y sube lento, así un golpe
  // puntual no la "infla" pero el sistema se acomoda al volumen general.
  float alfa = (ultimoPicoAPico > p2pBase) ? 0.02f : 0.06f;
  p2pBase += alfa * (ultimoPicoAPico - p2pBase);
  p2pBase = constrain(p2pBase, 12.0f, 150.0f);
```

### Bloque 4 — Silencio con memoria en `clasificarMusica()`

**BUSCAR:**
```cpp
  // Hay silencio o no según el VOLUMEN real (pico a pico), no según la energía
  // de dos bandas sueltas del espectro.
  if (ultimoPicoAPico < RUIDO_DE_FONDO) {
    tipoMusicaActual = SIN_SONIDO;
  } else if (energiaGraves > energiaAgudos * UMBRAL_CLASIFICACION) {
```
**REEMPLAZAR POR:**
```cpp
  // "Hay sonido" cuando el volumen sobresale del ambiente aprendido. Se le da
  // memoria de 800 ms para que la clasificación no parpadee entre beats.
  bool haySonidoAhora = (ultimoPicoAPico >= SONIDO_MINIMO) &&
                        (ultimoPicoAPico >= p2pBase * 1.15f);
  if (haySonidoAhora) ultimoMomentoConSonido = millis();

  if (millis() - ultimoMomentoConSonido > 800) {
    tipoMusicaActual = SIN_SONIDO;
  } else if (energiaGraves > energiaAgudos * UMBRAL_CLASIFICACION) {
```

### Bloque 5 — `actualizarLucesSonido()` completa

**BUSCAR** la función completa `void actualizarLucesSonido() { ... }` (desde su
comentario `// Modo AUTO: patrón de luces...` hasta su `}` de cierre).
**REEMPLAZAR POR:**

```cpp
// Modo AUTO: luces prendidas; cada GOLPE del ritmo las apaga un instante.
// Siempre prendido o apagado pleno, nunca a media luz.
void actualizarLucesSonido() {
  unsigned long ahora = millis();

  // ¿Golpe? El volumen superó a la base con margen y pasó el período refractario.
  bool superaUmbral = (ultimoPicoAPico >= p2pBase * FACTOR_GOLPE) &&
                      (ultimoPicoAPico >= GOLPE_MINIMO);
  if (superaUmbral && (ahora - inicioApagon) >= (DURACION_APAGON_MS + REFRACTARIO_MS)) {
    inicioApagon = ahora;   // Arranca un apagón nuevo
  }

  // Las luces quedan apagadas mientras dura el apagón del último golpe.
  bool apagadas = (ahora - inicioApagon) < DURACION_APAGON_MS;

  ultimoVolumen = ultimoPicoAPico;
  ultimoBrillo  = apagadas ? 0 : 255;

  if (apagadas) {
    apagarTodasLasLuces();
    return;
  }

  switch (tipoMusicaActual) {

    case MUSICA_B: {
      // Agudos/voz: una "sombra" recorre los colores (todos prendidos menos uno).
      static int posicion = 0;
      static unsigned long ultimoPaso = 0;
      if (ahora - ultimoPaso > 100) {
        ultimoPaso = ahora;
        posicion = (posicion + 1) % 4;
      }
      int colores[4] = {LED_VERDE, LED_ROJO, LED_AZUL, LED_BLANCO};
      for (int i = 0; i < 4; i++) {
        digitalWrite(colores[i], (i == posicion) ? LOW : HIGH);
      }
      break;
    }

    case MUSICA_A:
    case SIN_SONIDO:
    default: {
      // Graves o silencio: prendidas fijas, esperando el próximo golpe.
      prenderTodasLasLuces();
      break;
    }
  }
}
```

### Bloque 6 — `armarAudio()` con los datos nuevos

**BUSCAR** dentro de `armarAudio()`:
```cpp
  s += "VOLUMEN (pico a pico): " + String(ultimoPicoAPico) + "\n";
  s += "  (silencio<" + String(RUIDO_DE_FONDO);
  s += " / se apagan en " + String(UMBRAL_APAGADO) + ")\n";
```
**REEMPLAZAR POR:**
```cpp
  s += "VOLUMEN (pico a pico): " + String(ultimoPicoAPico) + "\n";
  s += "Base ambiente (aprendida): " + String(p2pBase, 1) + "\n";
  int umbralGolpe = max((int)(p2pBase * FACTOR_GOLPE), GOLPE_MINIMO);
  s += "Golpe a partir de: " + String(umbralGolpe) + "\n";
```

### Bloque 7 — README (sección de calibración)

En `PiletaInteligente/README.md`, **BUSCAR** la sección `### Ajustar la sensibilidad
de las luces` completa (hasta la línea del blockquote de `/diag` inclusive) y
**REEMPLAZAR POR:**

```markdown
### Ajustar la sensibilidad de las luces

Las luces trabajan **solo prendidas o apagadas** (nunca a media luz). La detección de
ritmo es **adaptativa**: el sistema aprende solo el nivel de ruido ambiente (la "base")
y apaga las luces un instante en cada golpe que sobresale de esa base. No hay que
calibrar según el volumen: se acomoda solo.

Perillas disponibles en el código, por si hiciera falta afinar:

| Constante | Valor | Qué hace |
|---|---|---|
| `FACTOR_GOLPE` | 1.6 | Cuánto debe sobresalir el golpe sobre el ambiente (bajar = más sensible) |
| `GOLPE_MINIMO` | 32 | Piso absoluto para disparar (evita falsos golpes en silencio) |
| `DURACION_APAGON_MS` | 110 | Cuántos milisegundos quedan apagadas en cada golpe |

> 💡 Para ver los números en vivo: mandá `/audio` con música sonando. Muestra el
> volumen actual, la base aprendida y a partir de qué valor se dispara el golpe.
```

## Verificación obligatoria (gate del lote, sincrónico)

1. `grep -c "RUIDO_DE_FONDO\|SONIDO_MAXIMO\|UMBRAL_APAGADO" PiletaInteligente/PiletaInteligente.ino` → **0**
2. `grep -c "p2pBase" PiletaInteligente/PiletaInteligente.ino` → **≥ 5** (declaración + 4 usos)
3. `grep -c "analogWrite(LED_\|analogWrite(colores" PiletaInteligente/PiletaInteligente.ino` → **0** (las luces siguen binarias)
4. `grep -c "DURACION_APAGON_MS" PiletaInteligente/PiletaInteligente.ino` → **≥ 3**
5. Revisar a mano que no queden llaves desbalanceadas en `actualizarLucesSonido` (la función nueva termina con un único `}` de cierre tras el `switch`).

## Entrega

- **UN commit** con mensaje Conventional Commits (`feat: deteccion de ritmo adaptativa en las luces (base + golpes relativos)`) con trailer `Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>`.
- **NO pushear.**
- Devolver resumen compacto: archivos:líneas tocadas, resultados de los 5 checks, hash del commit.
