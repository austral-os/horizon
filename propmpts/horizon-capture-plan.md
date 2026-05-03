# Plan de Desarrollo: Horizon Capture

Este documento detalla el plan para la creación de una herramienta nativa de captura de pantalla e imagen/audio/video para el ecosistema Horizon, compatible con compositores basados en `wlroots`.

## 1. Identidad del Proyecto

- **Nombre:** Capture
- **Objetivo:** Proporcionar una solución integrada y de alto rendimiento para la captura de medios en Austral OS.
- **Estética:** Alineada con el lenguaje de diseño de Horizon (glassmorphism, micro-animaciones, minimalismo).

## 2. Requerimientos Técnicos

### Protocolos Wayland (vía wlroots)

- `wlr-screencopy-unstable-v1`: Captura de fotogramas de la salida del compositor.
- `xdg-output-unstable-v1`: Identificación y mapeo de monitores.
- `wlr-layer-shell-unstable-v1`: Interfaz de usuario para selección de área (overlay).

### Dependencias Principales

Para el desarrollo, se requieren las siguientes librerías de sistema (Debian/Ubuntu):

```bash
# Dependencias de desarrollo
sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev \
                 libpipewire-0.3-dev libspa-0.2-dev \
                 libpng-dev libjpeg-dev wayland-protocols

# Dependencias de ejecución (producción)
sudo apt install libavcodec61 libavformat61 libavutil59 libswscale8 \
                 libpipewire-0.3-0t64 libspa-0.2-modules \
                 libpng16-16t64 libjpeg62-turbo
```

- **Framework Horizon:** Para la interfaz de usuario y el ciclo de vida de la aplicación.
- **FFmpeg (libavcodec, libavformat, libswscale):** Codificación y multiplexación de video/audio.
- **PipeWire (libpipewire-0.3):** Captura de audio del sistema y fuentes de entrada.
- **libpng / libjpeg:** Guardado de capturas de pantalla estáticas.

## 3. Arquitectura del Sistema

El proyecto se dividirá en dos componentes principales para maximizar la reutilización y modularidad:

### A. Librería Core (`libs/horizon-capture`)

Una librería compartida que encapsula toda la lógica de bajo nivel y procesamiento de medios:

1. **CaptureEngine:**
    - Manejo del cliente Wayland.
    - Gestión de buffers (SHM/DMA-BUF).
    - Sincronización de VSync para capturar a la tasa de refresco correcta.

2. **AudioEngine:**
    - Conexión al servidor PipeWire.
    - Captura de flujos de audio en formato PCM.

3. **MediaRecorder:**
    - Pipeline de FFmpeg.
    - Mapeo de fotogramas a frames de video.
    - Sincronización de audio/video mediante timestamps.

### B. Aplicación Front-end (`apps/capture`)

Un binario ligero que utiliza la librería core y el framework Horizon para la interacción con el usuario:

- **SelectionUI:** Overlay de pantalla completa para selección de región con estética glassmorphism.
- **Control Bar:** Interfaz flotante para gestionar el estado de la grabación (pausa, stop, micro).
- **CLI Wrapper:** Soporte para ejecución mediante línea de comandos (ej. para atajos de teclado).

## 4. Fases de Implementación

### Fase 1: Captura de Imagen (MVP)

- Implementar `wlr-screencopy` para volcar el buffer de pantalla a un archivo PNG.
- Comando de terminal inicial: `horizon-capture --screenshot`.

### Fase 2: Interfaz de Selección

- Crear el widget `SelectionOverlay` usando Horizon.
- Permitir dibujo de rectángulo con el mouse para definir la región de interés.

### Fase 3: Grabación de Video Básica

- Implementar el motor de FFmpeg para codificar en H.264.
- Soporte inicial para grabación de área seleccionada sin audio.

### Fase 4: Integración de Audio y PipeWire

- Capturar audio del sistema durante la grabación.
- Muxing de audio y video en contenedor MP4/MKV.

### Fase 5: Refinamiento y UX

- Añadir indicador visual en el panel superior mientras se graba.
- Diálogo de post-captura para previsualización y acciones rápidas.

## 5. Consideraciones de Rendimiento

- Uso de **VA-API** para codificación por hardware si está disponible.
- Minimizar las copias de memoria entre el compositor y el codificador (Zero-copy path).
- Buffer circular para evitar bloqueos en el hilo principal durante la escritura a disco.
