# KVoiceMemos — Notas de Voz con IA Local para KDE Plasma 6.8 + Wayland

**KVoiceMemos** porta la experiencia de **macOS Voice Memos + Apple Intelligence** a **CachyOS Linux / KDE Plasma 6.8 (Wayland)** con estética **Liquid Glass**, captura de audio nativa mediante **PipeWire** (respetando la cadena de filtros de EasyEffects), transcripción local con **Whisper.cpp** y resúmenes ejecutivos automáticos con **Ollama (`qwen2.5:7b`)**.

## Características principales

- **Captura de audio vía PipeWire (`pw-record`)**: graba a 16 kHz / 16-bit mono desde la fuente por defecto de PipeWire sin fijar dispositivos ALSA (`hw:*`), manteniendo intacto el procesado de **EasyEffects**.
- **Reproducción instantánea (`pw-play`)**: escucha tus notas directamente desde la interfaz.
- **Transcripción local automática con Whisper.cpp**: utiliza `whisper-cli` (desde `/usr/bin/whisper-cli`) con el modelo `/usr/share/kity/models/ggml-base.bin` (o en `XDG_DATA_HOME/kity/models/`) en español (`-l es -nt`) y limpia marcas de tiempo o `[BLANK_AUDIO]`.
- **Resumen IA con Ollama (`qwen2.5:7b`)**: genera automáticamente una sugerencia de título y lista de puntos clave con viñetas a través de `http://127.0.0.1:11434/api/chat`.
- **Integración directa con Kity Local RAG**: exporta con un clic cualquier nota de voz (título, fecha, resumen y transcripción completa) como documento Markdown en `XDG_DATA_HOME/kity/knowledge/`.
- **Búsqueda en tiempo real**: filtra notas por título, contenido transcrito o resumen IA.
- **Interfaz Liquid Glass Golden Gate**: diseño de 3 columnas basado en `CachyOS.UI` con barra lateral de carpetas, lista de grabaciones, visor de forma de onda PCM con recorte no destructivo, ajustes de reproducción (`ffplay`/`pw-play`) y transcripción sincronizada.

## Requisitos

- **KDE Plasma 6.8+** y **Qt 6.7+** (`Core`, `Gui`, `Widgets`, `Network`, `Qml`, `Quick`, `QuickControls2`, `Test`)
- **KDE Frameworks 6** (`CoreAddons`, `I18n`, `Kirigami`, `ECM`)
- **PipeWire** (`pw-record`, `pw-play`) y opcionalmente **FFmpeg** (`ffplay`) para reproducción avanzada (velocidad, omitir silencios y mejora vocal)
- **Whisper.cpp**: binario `whisper-cli` en `/usr/bin` y modelo `ggml-base.bin`
- **Ollama**: servicio local en `http://127.0.0.1:11434` con el modelo `qwen2.5:7b`

## Compilación, pruebas e instalación

```bash
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
sudo -n cmake --install build --prefix /usr
kbuildsycoca6 >/dev/null 2>&1
```

## Uso

Ejecuta la aplicación desde el lanzador de aplicaciones de KDE (**Notas de Voz (KVoiceMemos)**) o desde la terminal:

```bash
/usr/bin/kvoicememos
```

Las grabaciones WAV y su base de datos `memos.json` se almacenan en `$XDG_DATA_HOME/kvoicememos/`.

## Limitaciones conocidas

- Si `whisper-cli` o el modelo `ggml-base.bin` no están presentes en las rutas esperadas, la grabación WAV se guarda igualmente pero se muestra un aviso en la barra de estado.
- Si el servidor local de Ollama no está activo, la transcripción se conserva y el resumen puede solicitarse más tarde pulsando **Resumir**.
