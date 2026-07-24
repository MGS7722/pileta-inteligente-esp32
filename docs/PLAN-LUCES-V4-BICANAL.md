# PLAN LEGO — Luces v4: detección BICANAL (AO adaptativo + DO por hardware)

> **Autor:** Fable 5 (fase cerebro). **Ejecutor:** subagente Sonnet 5.
> **Alcance:** `PiletaInteligente/PiletaInteligente.ino`, `PiletaInteligente/README.md`,
> `PiletaInteligente/CABLEADO-PASO-A-PASO.md`, `PiletaInteligente/CONEXIONES.md`,
> `PiletaInteligente/COMPONENTES.md` y crear `PiletaInteligente/PROTOCOLO-TALLER.md`.
> PROHIBIDO tocar cualquier otro archivo.

## Contexto (por qué)

Hardware identificado: sensor KY-037 (LM393, especificación 4–6V; hay DOS módulos
comprados y solo uno en uso). La comunidad usa el DO (salida digital con umbral por
potenciómetro, con LED de placa como feedback visual) como detector de golpes por
hardware — método a prueba de fallos. Mariano no puede probar por ~2 semanas: se
implementa REDUNDANCIA bicanal para que la primera prueba funcione sí o sí:
- Módulo 1 a 5V (VIN): AO → GPIO34 (volumen adaptativo, ya implementado).
- Módulo 2 a 3.3V: DO → GPIO35 (golpes por hardware; a 3.3V el nivel HIGH es seguro
  para el ESP32 — NUNCA alimentar este módulo 2 con 5V mientras su DO esté en GPIO35).
- Fusión: golpe si dispara cualquiera de los dos canales; fuente seleccionable por
  Telegram y guardada en NVS; filtro anti-pin-flotante para cuando el módulo 2 no está.

## Cambios exactos en `PiletaInteligente.ino`

### Bloque 1 — Pin nuevo

**BUSCAR:**
```cpp
#define MIC_PIN     34    // Sensor de sonido. ADC1 (no usar ADC2: choca con WiFi).
```
**REEMPLAZAR POR:**
```cpp
#define MIC_PIN     34    // Sensor de sonido 1, salida AO. ADC1 (no usar ADC2: choca con WiFi).
#define MIC_DO_PIN  35    // Sensor de sonido 2, salida DO (golpes por hardware).
                          // El módulo 2 se alimenta a 3.3V: así su DO nunca supera
                          // los 3.3V que tolera el ESP32. NO alimentarlo con 5V.
```

### Bloque 2 — Constante del filtro anti-flotante

**BUSCAR:**
```cpp
const unsigned long REFRACTARIO_MS     = 150;  // Espera mínima entre golpes seguidos
```
**REEMPLAZAR POR:**
```cpp
const unsigned long REFRACTARIO_MS     = 150;  // Espera mínima entre golpes seguidos

// Canal DO (2do micrófono): un golpe real produce POCOS cambios de estado por
// ventana; un pin desconectado (flotante) produce muchísimos. Por encima de este
// tope, la lectura del DO se descarta como ruido.
const int FLANCOS_DO_MAXIMO = 8;
```

### Bloque 3 — Estado nuevo

**BUSCAR:**
```cpp
float  p2pBase = 20;          // Nivel de ruido ambiente aprendido (se adapta solo)
```
**REEMPLAZAR POR:**
```cpp
float  p2pBase = 20;          // Nivel de ruido ambiente aprendido (se adapta solo)

// Fuente de detección de golpes: MIXTA usa ambos micrófonos (el que dispare
// primero); AO solo el análogo; DO solo el detector por hardware del módulo 2.
enum FuenteGolpe { FUENTE_MIXTA, FUENTE_AO, FUENTE_DO };
FuenteGolpe fuenteGolpe = FUENTE_MIXTA;
int  ultimosFlancosDO = 0;    // Cambios de estado del DO en la última ventana
bool golpeDO = false;         // true si el módulo 2 detectó golpe en la última ventana
```

### Bloque 4 — setup()

**BUSCAR:**
```cpp
  // Sensor de sonido
  pinMode(MIC_PIN, INPUT);
```
**REEMPLAZAR POR:**
```cpp
  // Sensores de sonido (1: AO análogo; 2: DO golpes por hardware)
  pinMode(MIC_PIN, INPUT);
  pinMode(MIC_DO_PIN, INPUT);   // GPIO35 es solo-entrada; el módulo 2 trae su pull-up
```
**Y BUSCAR:**
```cpp
  preferencias.begin("pileta", false);
  tempObjetivo = preferencias.getFloat("tempObj", 23.0);
```
**REEMPLAZAR POR:**
```cpp
  preferencias.begin("pileta", false);
  tempObjetivo = preferencias.getFloat("tempObj", 23.0);
  fuenteGolpe  = (FuenteGolpe)preferencias.getUChar("fuenteGolpe", FUENTE_MIXTA);
```

### Bloque 5 — `medirSonido()`: leer también el DO en la misma ventana

**BUSCAR:**
```cpp
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
```
**REEMPLAZAR POR:**
```cpp
  int minimo = 4095;
  int maximo = 0;
  int flancos = 0;
  bool estadoDO = digitalRead(MIC_DO_PIN);

  for (int i = 0; i < MUESTRAS_SONIDO; i++) {
    unsigned long t = micros();

    int lectura = analogRead(MIC_PIN);
    if (lectura < minimo) minimo = lectura;
    if (lectura > maximo) maximo = lectura;

    // Canal DO: contar cambios de estado (golpes que superan el umbral del
    // potenciómetro del módulo 2). Se cuentan flancos y no niveles para que
    // funcione igual con módulos de polaridad invertida.
    bool d = digitalRead(MIC_DO_PIN);
    if (d != estadoDO) {
      flancos++;
      estadoDO = d;
    }

    while (micros() - t < PERIODO_MUESTREO_US) {
      // espera para mantener constante la frecuencia de muestreo
    }
  }

  // Un pin flotante (módulo 2 desconectado) genera decenas de flancos por
  // ventana: en ese caso la lectura del DO se descarta.
  ultimosFlancosDO = flancos;
  golpeDO = (flancos > 0) && (flancos <= FLANCOS_DO_MAXIMO);
```

### Bloque 6 — `medirSonido()`: el DO también cuenta como "hay música"

**BUSCAR:**
```cpp
  bool haySonidoAhora = (ultimoPicoAPico >= SONIDO_MINIMO) &&
                        (ultimoPicoAPico >= p2pBase * 1.15f);
  if (haySonidoAhora) ultimoMomentoConSonido = millis();
```
**REEMPLAZAR POR:**
```cpp
  bool haySonidoAhora = (ultimoPicoAPico >= SONIDO_MINIMO) &&
                        (ultimoPicoAPico >= p2pBase * 1.15f);
  if (usaDO() && golpeDO) haySonidoAhora = true;
  if (haySonidoAhora) ultimoMomentoConSonido = millis();
```

### Bloque 7 — Fusión de canales en `actualizarLucesSonido()`

**BUSCAR:**
```cpp
  // ¿Golpe? El volumen superó a la base con margen y pasó el período refractario.
  bool superaUmbral = (ultimoPicoAPico >= p2pBase * FACTOR_GOLPE) &&
                      (ultimoPicoAPico >= GOLPE_MINIMO);
```
**REEMPLAZAR POR:**
```cpp
  // ¿Golpe? Según la fuente elegida: el canal AO (volumen supera a la base con
  // margen), el canal DO (el módulo 2 lo detectó por hardware), o cualquiera.
  bool golpeAO = (ultimoPicoAPico >= p2pBase * FACTOR_GOLPE) &&
                 (ultimoPicoAPico >= GOLPE_MINIMO);
  bool superaUmbral = (usaAO() && golpeAO) || (usaDO() && golpeDO);
```

### Bloque 8 — Funciones auxiliares de fuente (agregar ANTES de `actualizarLucesSegunModo()`)

**BUSCAR:**
```cpp
// Aplica el modo de luces elegido (AUTO / ON / OFF).
void actualizarLucesSegunModo() {
```
**REEMPLAZAR POR:**
```cpp
// ¿La fuente elegida incluye a cada canal?
bool usaAO() { return fuenteGolpe == FUENTE_MIXTA || fuenteGolpe == FUENTE_AO; }
bool usaDO() { return fuenteGolpe == FUENTE_MIXTA || fuenteGolpe == FUENTE_DO; }

String fuenteGolpeTexto() {
  switch (fuenteGolpe) {
    case FUENTE_MIXTA: return "MIXTA (ambos microfonos)";
    case FUENTE_AO:    return "AO (microfono analogico)";
    case FUENTE_DO:    return "DO (detector por hardware)";
    default:           return "?";
  }
}

// Aplica el modo de luces elegido (AUTO / ON / OFF).
void actualizarLucesSegunModo() {
```

### Bloque 9 — Comandos de fuente

**BUSCAR:**
```cpp
  else if (text.startsWith("/temperatura")) {
```
**REEMPLAZAR POR:**
```cpp
  else if (text == "/sonido_mixto" || text == "/sonido_ao" || text == "/sonido_do") {
    if (text == "/sonido_mixto")   fuenteGolpe = FUENTE_MIXTA;
    else if (text == "/sonido_ao") fuenteGolpe = FUENTE_AO;
    else                           fuenteGolpe = FUENTE_DO;
    preferencias.putUChar("fuenteGolpe", (uint8_t)fuenteGolpe);
    bot.sendMessage(chat_id, "Fuente de golpes: " + fuenteGolpeTexto() +
                             ". Guardada: sobrevive reinicios.", "");
  }
  else if (text.startsWith("/temperatura")) {
```

### Bloque 10 — Ayuda, /status y /audio

En `armarAyuda()`, **BUSCAR:**
```cpp
  s += "/audio - datos del sonido\n";
```
**REEMPLAZAR POR:**
```cpp
  s += "/audio - datos del sonido\n";
  s += "/sonido_mixto, /sonido_ao, /sonido_do - fuente de golpes\n";
```

En `armarStatus()`, **BUSCAR:**
```cpp
  s += "Sonido: ";
  s += (hayMusica ? "musica detectada" : "silencio");
  s += "\n";
  s += "Cobertor: " + estadoCobertorTexto() + "\n";
```
**REEMPLAZAR POR:**
```cpp
  s += "Sonido: ";
  s += (hayMusica ? "musica detectada" : "silencio");
  s += "\n";
  s += "Fuente golpes: " + fuenteGolpeTexto() + "\n";
  s += "Cobertor: " + estadoCobertorTexto() + "\n";
```

En `armarAudio()`, **BUSCAR:**
```cpp
  s += "Golpe a partir de: " + String(umbralGolpe) + "\n";
```
**REEMPLAZAR POR:**
```cpp
  s += "Golpe a partir de: " + String(umbralGolpe) + "\n";
  s += "Fuente: " + fuenteGolpeTexto() + "\n";
  s += "DO (2do mic): " + String(ultimosFlancosDO) + " flancos";
  if (ultimosFlancosDO > FLANCOS_DO_MAXIMO) s += " (ruido: pin sin conectar?)";
  s += "\n";
```

### Bloque 11 — `/diag` reporta también el DO

En `armarDiagnosticoMicrofono()`, **BUSCAR:**
```cpp
  String s = "DIAGNOSTICO DEL MICROFONO\n";
```
**REEMPLAZAR POR:**
```cpp
  // Estado del canal DO durante la misma medición
  String s = "DIAGNOSTICO DEL MICROFONO\n";
```
**y BUSCAR:**
```cpp
  s += "Promedio (DC): " + String(promedio) + "\n\n";
```
**REEMPLAZAR POR:**
```cpp
  s += "Promedio (DC): " + String(promedio) + "\n";
  s += "DO (2do mic) ahora: ";
  s += (digitalRead(MIC_DO_PIN) == HIGH ? "HIGH" : "LOW");
  s += " | flancos ultima ventana: " + String(ultimosFlancosDO) + "\n\n";
```

## Cambios en la documentación

### Bloque 12 — `README.md`

En la sección "Ajustar la sensibilidad de las luces", **BUSCAR:**
```
> 💡 Para ver los números en vivo: mandá `/audio` con música sonando. Muestra el
> volumen actual, la base aprendida y a partir de qué valor se dispara el golpe.
```
**REEMPLAZAR POR:**
```
> 💡 Para ver los números en vivo: mandá `/audio` con música sonando. Muestra el
> volumen actual, la base aprendida y a partir de qué valor se dispara el golpe.

**Detección bicanal:** además del micrófono analógico (AO), un **segundo módulo** de
sonido conectado por su salida **DO** al GPIO35 actúa como detector de golpes por
hardware: su potenciómetro fija el umbral y el LED de la placa muestra en vivo cuándo
dispara. Con `/sonido_mixto` (default), `/sonido_ao` y `/sonido_do` se elige qué
canal comanda los golpes (queda guardado). La guía de calibración está en
**`PROTOCOLO-TALLER.md`**.
```

En la lista de comandos de Información, **BUSCAR:**
```
- `/audio` — datos del sonido
```
**REEMPLAZAR POR:**
```
- `/audio` — datos del sonido
- `/sonido_mixto` / `/sonido_ao` / `/sonido_do` — elegir la fuente de golpes
```

### Bloque 13 — `CABLEADO-PASO-A-PASO.md`

**BUSCAR:**
```
17. **Cable** → del pin **VCC/+** del sensor → al **riel 3.3V**. *(a 3.3V, no a 5V)*
```
**REEMPLAZAR POR:**
```
17. **Cable** → del pin **VCC/+** del sensor → al pin **VIN (5V)** del ESP32.
    *(este módulo pide 4-6V según su especificación; su salida AO es de nivel bajo
    y no daña al GPIO34 — verificado con /diag: el Máximo debe quedar < 3000)*
```
**Y BUSCAR:**
```
## PASO 8 — Alimentar el ESP32
```
**REEMPLAZAR POR:**
```
## PASO 7-BIS — Segundo sensor de sonido (detector de golpes por hardware)

Es el módulo de sonido de repuesto (KY-037). ⚠️ Este va a **3.3V, NUNCA a 5V**
(su salida DO llega al ESP32 y más de 3.3V lo dañaría).

52a. **Cable** → del pin **VCC/+** del módulo 2 → al **riel 3.3V**.
52b. **Cable** → del pin **GND/G** del módulo 2 → al **riel GND**.
52c. **Cable** → del pin **DO** del módulo 2 → al pin **GPIO35** del ESP32.
     *(el pin AO del módulo 2 queda SIN conectar)*

> El potenciómetro de ESTE módulo sí importa: fija el umbral del detector.
> Cómo calibrarlo está en `PROTOCOLO-TALLER.md`.

## PASO 8 — Alimentar el ESP32
```

### Bloque 14 — `CONEXIONES.md`

En el mapa de pines, **BUSCAR:**
```
| GPIO34 | Luces | Sensor de sonido — salida analógica (AO) |
```
**REEMPLAZAR POR:**
```
| GPIO34 | Luces | Sensor de sonido 1 — salida analógica (AO) |
| GPIO35 | Luces | Sensor de sonido 2 — salida digital (DO, golpes por hardware) |
```
**Y BUSCAR:**
```
Sensor de sonido
   VCC ──── 3.3V   (importante: 3.3V, NO 5V)
   GND ──── GND
   AO  ──── GPIO34  (salida ANALÓGICA, no la digital DO)
```
**REEMPLAZAR POR:**
```
Sensor de sonido 1 (volumen, canal analógico)
   VCC ──── VIN (5V)  (el módulo pide 4-6V; su AO es de nivel bajo y es segura)
   GND ──── GND
   AO  ──── GPIO34    (salida ANALÓGICA; el DO de este módulo queda libre)

Sensor de sonido 2 (golpes por hardware)  ⚠️ a 3.3V, NUNCA a 5V
   VCC ──── 3.3V
   GND ──── GND
   DO  ──── GPIO35    (salida DIGITAL; el AO de este módulo queda libre)
```

### Bloque 15 — `COMPONENTES.md`

**BUSCAR:**
```
| Módulo sensor de sonido | 2 | **S2** | Se usa 1 |
```
**REEMPLAZAR POR:**
```
| Módulo sensor de sonido KY-037 | 2 | **S2** | Se usan LOS DOS: uno por AO (volumen, a 5V) y otro por DO (golpes, a 3.3V) |
```
**Y BUSCAR:**
```
| GPIO34 | S2 | Sensor de sonido |
```
**REEMPLAZAR POR:**
```
| GPIO34 | S2 | Sensor de sonido 1 (AO) |
| GPIO35 | S2 | Sensor de sonido 2 (DO) |
```
**Y BUSCAR:**
```
Quedan libres de sobra: GPIO36, GPIO39 y más.
```
**REEMPLAZAR POR:**
```
Quedan libres: GPIO36, GPIO39 y más.
```

### Bloque 16 — Crear `PiletaInteligente/PROTOCOLO-TALLER.md`

Crear el archivo con EXACTAMENTE este contenido:

```markdown
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
```

## Verificación obligatoria (gate del lote, sincrónico)

1. `grep -c "MIC_DO_PIN" PiletaInteligente/PiletaInteligente.ino` → **≥ 5**
2. `grep -c "fuenteGolpe" PiletaInteligente/PiletaInteligente.ino` → **≥ 8**
3. `grep -c "golpeDO\|ultimosFlancosDO" PiletaInteligente/PiletaInteligente.ino` → **≥ 6**
4. `grep -c "usaAO()\|usaDO()" PiletaInteligente/PiletaInteligente.ino` → **≥ 4**
5. `grep -c "sonido_mixto\|sonido_ao\|sonido_do" PiletaInteligente/PiletaInteligente.ino` → **≥ 3** y en `README.md` → **≥ 1**
6. `grep -c "GPIO35" PiletaInteligente/CONEXIONES.md` → **≥ 2** · `grep -c "PROTOCOLO-TALLER" PiletaInteligente/README.md` → **≥ 1**
7. Existe `PiletaInteligente/PROTOCOLO-TALLER.md` y contiene "plan B" (`grep -ic "plan b"` ≥ 1).
8. Leer con Read `medirSonido()` y `actualizarLucesSonido()` completas: llaves balanceadas, `estadoDO` declarada antes del for, `golpeAO` solo dentro de `actualizarLucesSonido()`.

(`grep -c` sale con código 1 si da 0 — encadenar con `;`.)

## Entrega

- **UN commit**: `feat: deteccion bicanal de golpes (AO adaptativo + DO por hardware) y protocolo de taller`, cuerpo breve con el porqué (redundancia para que la primera prueba funcione; KY-037 a 5V por especificación; DO del 2do modulo a 3.3V por seguridad de niveles), trailer EXACTO `Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>`.
- **NO pushear.**
- Resumen compacto: archivos:líneas, los 8 checks con números, desviaciones, hash.
