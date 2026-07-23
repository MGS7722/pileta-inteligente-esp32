# PLAN LEGO — Luces v3: eliminar la FFT, ventana de 35 ms, timeout de Telegram y /temperatura

> **Autor:** Fable 5 (fase cerebro). **Ejecutor:** subagente Sonnet 5.
> **Alcance:** SOLO `PiletaInteligente/PiletaInteligente.ino` y `PiletaInteligente/README.md`. PROHIBIDO tocar cualquier otro archivo.

## Contexto (por qué)

Auditoría de raíz 2026-07-23: (1) la FFT existe solo para clasificar graves/agudos y con
la señal real del micrófono (14–30 mV) esa clasificación es ruido — se elimina; (2) la
ventana de 12,8 ms (impuesta por la FFT) pierde los beats (evidencia: /audio=16 vs
/diag=40 el mismo instante) — pasa a ~35 ms; (3) falta `client.setTimeout` y Telegram
congela el show 1–2 s por consulta; (4) la spec original pedía temperatura objetivo
configurable por Telegram y nunca se implementó.

## Cambios exactos en `PiletaInteligente.ino`

### Bloque 1 — Encabezado

**BUSCAR:**
```
//     2) LUCES DISCO  -> Sensor de sonido + 8 LEDs (4 colores x 2
//                        lados). En modo AUTO quedan prendidas y la
//                        música las va apagando al ritmo (efecto en
//                        negativo). Arrancan APAGADAS; se activan
//                        desde Telegram.
```
**REEMPLAZAR POR:**
```
//     2) LUCES DISCO  -> Sensor de sonido + 8 LEDs (4 colores x 2
//                        lados). En modo AUTO: sin música quedan
//                        prendidas, con música una "sombra" recorre
//                        los colores y cada golpe del ritmo las apaga
//                        un instante (strobe). Arrancan APAGADAS.
```

### Bloque 2 — Includes

**ELIMINAR** la línea:
```cpp
#include <arduinoFFT.h>
```
**Y AGREGAR** después de `#include <ArduinoJson.h>`:
```cpp
#include <Preferences.h>
```

### Bloque 3 — Constantes de sonido

**BUSCAR** desde la línea `#define SAMPLES 128` hasta `const float UMBRAL_CLASIFICACION = 1.3;`
(inclusive, con sus comentarios de bandas). **REEMPLAZAR TODO ese tramo POR:**
```cpp
// --- Medición del sonido (sin FFT) ---
// Con la señal débil de este micrófono, el análisis de frecuencias no aporta
// información confiable: lo único robusto es el VOLUMEN pico a pico. La ventana
// de ~35 ms es clave: un golpe de bombo dura 50-100 ms, y con ventanas más
// cortas la medición cae "entre" los picos y el beat se pierde.
const int MUESTRAS_SONIDO = 350;                 // 350 muestras a 10 kHz = ~35 ms
const unsigned long PERIODO_MUESTREO_US = 100;   // 10 kHz de muestreo
```
(Las constantes `FACTOR_GOLPE`, `GOLPE_MINIMO`, `SONIDO_MINIMO`, `DURACION_APAGON_MS` y
`REFRACTARIO_MS` que siguen quedan EXACTAMENTE como están.)

### Bloque 4 — Ajustes del calentador (temperatura configurable)

**BUSCAR:**
```cpp
const float TEMP_OBJETIVO = 23.0;   // Temperatura buscada (subir a ~30 para uso real)
```
**REEMPLAZAR POR:**
```cpp
// La temperatura objetivo se cambia desde Telegram con /temperatura y queda
// guardada en la memoria del ESP32 (sobrevive reinicios). 23.0 es el valor
// inicial de fábrica.
float tempObjetivo = 23.0;
```

### Bloque 5 — Objetos principales

**ELIMINAR** las líneas:
```cpp
double vReal[SAMPLES];
double vImag[SAMPLES];
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);
```
**Y AGREGAR** en su lugar:
```cpp
Preferences preferencias;   // Memoria no volátil (guarda la temperatura objetivo)
```

### Bloque 6 — Estado del sistema

**ELIMINAR** estas líneas (con sus comentarios):
```cpp
enum TipoMusica { MUSICA_A, MUSICA_B, SIN_SONIDO };
TipoMusica tipoMusicaActual = SIN_SONIDO;
```
```cpp
double ultimaEnergiaGraves = 0;
double ultimaEnergiaAgudos = 0;
double ultimaEnergiaTotal  = 0;
double ultimoVolumen = 0;
```
**BUSCAR:**
```cpp
unsigned long ultimoMomentoConSonido = 0;  // Para dar memoria a la clasificación A/B
```
**REEMPLAZAR POR:**
```cpp
unsigned long ultimoMomentoConSonido = 0;  // Memoria de 800 ms de "hay música"
bool hayMusica = false;                    // true mientras se detecta sonido sostenido
```
(`ultimoBrillo`, `ultimoPicoAPico`, `p2pBase` e `inicioApagon` quedan como están.)

### Bloque 7 — setup()

**BUSCAR:**
```cpp
  // Sensor de temperatura
  sensores.begin();
```
**REEMPLAZAR POR:**
```cpp
  // Temperatura objetivo guardada (si nunca se configuró, queda 23.0)
  preferencias.begin("pileta", false);
  tempObjetivo = preferencias.getFloat("tempObj", 23.0);

  // Sensor de temperatura
  sensores.begin();
```

### Bloque 8 — loop()

**BUSCAR:**
```cpp
  // 1) Sonido y luces → en cada vuelta.
  muestrearAudio();
  clasificarMusica();
  actualizarLucesSegunModo();
```
**REEMPLAZAR POR:**
```cpp
  // 1) Sonido y luces → en cada vuelta.
  medirSonido();
  actualizarLucesSegunModo();
```

### Bloque 9 — Reemplazar `muestrearAudio()` + `clasificarMusica()` por `medirSonido()`

**BUSCAR** desde el comentario `// Toma SAMPLES muestras del micrófono...` hasta el `}`
que cierra `clasificarMusica()` (las dos funciones completas). **REEMPLAZAR TODO POR:**
```cpp
// Mide el sonido en una ventana de ~35 ms: pico a pico (volumen real), base
// adaptativa del ambiente y si hay sonido sostenido. Sin FFT: con la señal
// débil de este micrófono, el volumen es la única medida confiable.
void medirSonido() {
  int minimo = 4095;
  int maximo = 0;

  for (int i = 0; i < MUESTRAS_SONIDO; i++) {
    unsigned long t = micros();
    int lectura = analogRead(MIC_PIN);
    if (lectura < minimo) minimo = lectura;
    if (lectura > maximo) maximo = lectura;
    while (micros() - t < PERIODO_MUESTREO_US) {
      // espera para mantener constante la frecuencia de muestreo
    }
  }

  ultimoPicoAPico = maximo - minimo;

  // La base aprende el nivel ambiente: baja rápido y sube lento, así un golpe
  // puntual no la "infla" pero el sistema se acomoda al volumen general.
  float alfa = (ultimoPicoAPico > p2pBase) ? 0.02f : 0.06f;
  p2pBase += alfa * (ultimoPicoAPico - p2pBase);
  p2pBase = constrain(p2pBase, 12.0f, 150.0f);

  // ¿Hay música? El volumen sobresale del ambiente aprendido, con memoria de
  // 800 ms para que la detección no parpadee entre beats.
  bool haySonidoAhora = (ultimoPicoAPico >= SONIDO_MINIMO) &&
                        (ultimoPicoAPico >= p2pBase * 1.15f);
  if (haySonidoAhora) ultimoMomentoConSonido = millis();
  hayMusica = (millis() - ultimoMomentoConSonido) < 800;
}
```

### Bloque 10 — `actualizarLucesSonido()`

**BUSCAR** la función completa (desde su comentario `// Modo AUTO: luces prendidas...`
hasta su `}` de cierre). **REEMPLAZAR POR:**
```cpp
// Modo AUTO: sin música todas prendidas; con música una "sombra" recorre los
// colores; y cada GOLPE del ritmo las apaga todas un instante (strobe).
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
  ultimoBrillo = apagadas ? 0 : 255;

  if (apagadas) {
    apagarTodasLasLuces();
    return;
  }

  if (hayMusica) {
    // Sombra rotante: todos los colores prendidos menos uno, que va girando.
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
  } else {
    // Silencio: todas prendidas fijas, esperando que arranque la música.
    prenderTodasLasLuces();
  }
}
```

### Bloque 11 — `actualizarTemperatura()` y `armarTemp()`

Reemplazar **TODAS** las apariciones de `TEMP_OBJETIVO` por `tempObjetivo` (hay 2 en
`actualizarTemperatura()` y 1 en `armarTemp()`). No cambiar nada más de esas funciones.

### Bloque 12 — Timeout de Telegram (2 lugares)

**BUSCAR** (dentro de `conectarWiFiTelegram()`):
```cpp
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    telegramListo = true;
    Serial.println("Bot de Telegram listo.");
```
**REEMPLAZAR POR:**
```cpp
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    client.setTimeout(2000);   // Sin esto, una consulta lenta congela el show de luces
    telegramListo = true;
    Serial.println("Bot de Telegram listo.");
```
**BUSCAR** (dentro de `procesarTelegram()`):
```cpp
  if (!telegramListo) {
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    telegramListo = true;
  }
```
**REEMPLAZAR POR:**
```cpp
  if (!telegramListo) {
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    client.setTimeout(2000);
    telegramListo = true;
  }
```

### Bloque 13 — Comando `/temperatura`

**BUSCAR:**
```cpp
  else if (text == "/audio") {
    bot.sendMessage(chat_id, armarAudio(), "");
  }
```
**REEMPLAZAR POR:**
```cpp
  else if (text.startsWith("/temperatura")) {
    String arg = text.substring(12);   // lo que viene después de "/temperatura"
    arg.trim();
    float nueva = arg.toFloat();
    if (arg.length() == 0) {
      bot.sendMessage(chat_id, "Temperatura objetivo actual: " + String(tempObjetivo, 1) +
                               " C.\nPara cambiarla: /temperatura 28", "");
    } else if (nueva < 15 || nueva > 35) {
      bot.sendMessage(chat_id, "Valor invalido. Usa un numero entre 15 y 35. Ej: /temperatura 28", "");
    } else {
      tempObjetivo = nueva;
      preferencias.putFloat("tempObj", tempObjetivo);
      bot.sendMessage(chat_id, "Temperatura objetivo: " + String(tempObjetivo, 1) +
                               " C. Guardada: sobrevive reinicios.", "");
    }
  }
  else if (text == "/audio") {
    bot.sendMessage(chat_id, armarAudio(), "");
  }
```

### Bloque 14 — Ayuda del bot

**BUSCAR:**
```cpp
  s += "/calentador_off - forzar apagado\n\n";
```
**REEMPLAZAR POR:**
```cpp
  s += "/calentador_off - forzar apagado\n";
  s += "/temperatura 28 - cambiar la temperatura objetivo\n\n";
```

### Bloque 15 — `armarStatus()`, `armarAudio()` y `tipoMusicaTexto()`

En `armarStatus()`, **BUSCAR:**
```cpp
  s += "Música: " + tipoMusicaTexto() + "\n";
```
**REEMPLAZAR POR:**
```cpp
  s += "Sonido: ";
  s += (hayMusica ? "musica detectada" : "silencio");
  s += "\n";
```

**REEMPLAZAR la función `armarAudio()` completa POR:**
```cpp
String armarAudio() {
  String s = "AUDIO\n";
  s += "Sonido: ";
  s += (hayMusica ? "musica detectada" : "silencio");
  s += "\n";
  s += "VOLUMEN (pico a pico): " + String(ultimoPicoAPico) + "\n";
  s += "Base ambiente (aprendida): " + String(p2pBase, 1) + "\n";
  int umbralGolpe = max((int)(p2pBase * FACTOR_GOLPE), GOLPE_MINIMO);
  s += "Golpe a partir de: " + String(umbralGolpe) + "\n";
  s += "Luces ahora: ";
  s += (ultimoBrillo > 0 ? "PRENDIDAS" : "APAGADAS");
  s += "\n";
  s += "Modo luces: " + modoLucesTexto();
  return s;
}
```

**ELIMINAR** la función `tipoMusicaTexto()` completa.

## Cambios exactos en `README.md`

### Bloque 16 — Tabla de librerías

Quitar la fila `| arduinoFFT | **2.0.4** | Enrique Condes |` de la tabla y cambiar el
texto "instalá estas 6 con **estas versiones**" por "instalá estas 5 con **estas
versiones**".

### Bloque 17 — Tabla de bloques del código

**BUSCAR:**
```
| **AJUSTES DE LUCES / SONIDO** | Parámetros del análisis de sonido (FFT) y bandas de frecuencia |
```
**REEMPLAZAR POR:**
```
| **AJUSTES DE LUCES / SONIDO** | Parámetros de la medición de sonido y la detección de ritmo |
```

### Bloque 18 — Comandos

**BUSCAR:**
```
- `/calentador_off` — forzar apagado
```
**REEMPLAZAR POR:**
```
- `/calentador_off` — forzar apagado
- `/temperatura 28` — cambiar la temperatura objetivo (queda guardada)
```

## Verificación obligatoria (gate del lote, sincrónico)

Correr en `PiletaInteligente/`:
1. `grep -c "FFT\|vReal\|vImag" PiletaInteligente.ino` → **0**
2. `grep -c "MUSICA_A\|MUSICA_B\|SIN_SONIDO\|tipoMusica\|BANDA_\|UMBRAL_CLASIFICACION\|ultimaEnergia\|ultimoVolumen" PiletaInteligente.ino` → **0**
3. `grep -c "SAMPLES\|SAMPLING_FREQUENCY" PiletaInteligente.ino` → **0**
4. `grep -c "TEMP_OBJETIVO" PiletaInteligente.ino` → **0**
5. `grep -c "medirSonido" PiletaInteligente.ino` → **2** · `grep -c "hayMusica"` → **≥ 4** · `grep -c "tempObjetivo"` → **≥ 6** · `grep -c "Preferences\|preferencias"` → **≥ 4** · `grep -c "setTimeout" ` → **2**
6. `grep -c "arduinoFFT" README.md` → **0**
7. Leer con Read las funciones `medirSonido()` y `actualizarLucesSonido()` completas y confirmar llaves balanceadas.

(Recordatorio: `grep -c` sale con código 1 cuando da 0 — no es error si el esperado era 0; encadenar con `;`.)

## Entrega

- **UN commit**: `feat: luces v3 sin FFT, ventana 35ms, timeout Telegram y /temperatura configurable`, cuerpo breve con los 4 motivos, y trailer EXACTO `Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>`.
- **NO pushear.**
- Devolver resumen compacto: archivos:líneas, resultados numéricos de los 7 checks, desviaciones (idealmente ninguna), hash.
