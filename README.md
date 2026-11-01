# KVoiceMemos — Notas de Voz con IA Local para KDE Plasma 6.8 + Wayland

**KVoiceMemos** porta la experiencia de **macOS Voice Memos + Apple Intelligence** a **CachyOS Linux / KDE Plasma 6.8 (Wayland)** con estética **Liquid Glass**, captura de audio nativa mediante **PipeWire** (respetando la cadena de filtros de EasyEffects), transcripción local con **Whisper.cpp** y resúmenes ejecutivos automáticos con **Ollama (`qwen2.5:7b`)**.

## Características principales

- **Captura de audio vía PipeWire (`pw-record`)**: graba a 16 kHz / 16-bit mono desde la fuente por defecto de PipeWire sin fijar dispositivos ALSA (`hw:*`), manteniendo intacto el procesado de **EasyEffects**.
- **Reproducción instantánea (`pw-play`)**: escucha tus notas directamente desde la interfaz.
- **Transcripción local automática con Whisper.cpp**: utiliza `~/.local/bin/whisper-cli` con el modelo `~/.local/share/kiri/models/ggml-base.bin` en español (`-l es -nt`) y limpia marcas de tiempo o `[BLANK_AUDIO]`.
- **Resumen IA con Ollama (`qwen2.5:7b`)**: genera automáticamente una sugerencia de título y lista de puntos clave con viñetas a través de `http://127.0.0.1:11434/api/chat`.
- **Integración directa con Kiri Local RAG**: exporta con un clic cualquier nota de voz (título, fecha, resumen y transcripción completa) como documento Markdown en `~/.local/share/kiri/knowledge/`.
- **Búsqueda en tiempo real**: filtra notas por título, contenido transcrito o resumen IA.
- **Interfaz Liquid Glass**: diseño de doble panel basado en Kirigami + `LiquidGlassCard`.

## Requisitos

- **KDE Plasma 6.8+** y **Qt 6.7+** (`Core`, `Gui`, `Widgets`, `Network`, `Qml`, `Quick`, `QuickControls2`, `Test`)
- **KDE Frameworks 6** (`CoreAddons`, `I18n`, `Kirigami`, `ECM`)
- **PipeWire** (`pw-record`, `pw-play`)
- **Whisper.cpp**: binario en `~/.local/bin/whisper-cli` y modelo en `~/.local/share/kiri/models/ggml-base.bin`
- **Ollama**: servicio local en `http://127.0.0.1:11434` con el modelo `qwen2.5:7b`

## Compilación, pruebas e instalación

```bash
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=$HOME/.local -DBUILD_TESTING=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
cmake --install build
kbuildsycoca6 >/dev/null 2>&1
```

## Uso

Ejecuta la aplicación desde el lanzador de aplicaciones de KDE (**Notas de Voz (KVoiceMemos)**) o desde la terminal:

```bash
~/.local/bin/kvoicememos
```

Las grabaciones WAV y su base de datos `memos.json` se almacenan en `~/.local/share/kvoicememos/`.

## Limitaciones conocidas

- Si `whisper-cli` o el modelo `ggml-base.bin` no están presentes en las rutas esperadas, la grabación WAV se guarda igualmente pero se muestra un aviso en la barra de estado.
- Si el servidor local de Ollama no está activo, la transcripción se conserva y el resumen puede solicitarse más tarde pulsando **Resumir**.
