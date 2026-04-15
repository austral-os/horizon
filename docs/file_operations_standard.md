# Estándar de Operaciones de Archivos para Ventanas

Horizon proporciona un sistema estandarizado para que las aplicaciones manejen operaciones comunes de archivos (Abrir Archivo, Abrir Carpeta, Cerrar, Guardar y Guardar Como). Este sistema automatiza la inyección de menús en la barra global y la gestión de diálogos de selección.

## Concepto General

En lugar de que cada aplicación implemente su propio menú "Archivo" y gestione manualmente la apertura de diálogos, Horizon permite que un `Window` declare sus **capacidades**. Al hacerlo:
1. El framework inyecta automáticamente un menú "**Archivo**" en la barra global.
2. Horizon gestiona la apertura de `FileDialog` de forma automática.
3. El framework notifica a la aplicación mediante eventos específicos cuando la operación se completa.

## Paso 1: Declarar Capacidades

Para activar el soporte, tu clase de ventana (que debe heredar de `horizon::Window`) debe sobreescribir el método `file_capabilities()` devolviendo una máscara de bits de `FileCapability`.

```cpp
#include <horizon/Window.hpp>

class MyWindow : public horizon::Window {
public:
    MyWindow() : Window("Mi App") {}

    // Declaramos que soportamos Abrir Archivo y Cerrar
    uint32_t file_capabilities() const override {
        return FileOpen | FileClose;
    }
};
```

### Capacidades Disponibles
- `FileNone`: Sin soporte (por defecto).
- `FileOpen`: Habilita "Abrir archivo" (Ctrl+O).
- `FileOpenFolder`: Habilita "Abrir carpeta" (Ctrl+Shift+O).
- `FileClose`: Habilita "Cerrar" (Ctrl+W).
- `FileSave`: Habilita "Guardar" (Ctrl+S).
- `FileSaveAs`: Habilita "Guardar como" (Ctrl+Shift+S).
- `FileAll`: Habilita todas las anteriores.

### Paso 1.1: Proveer la Ruta Actual

Si habilitas `FileSave`, el framework necesita saber si el documento actual ya tiene una ruta en disco para decidir si debe o no mostrar el diálogo de "Guardar como". Para esto debes sobreescribir `current_file_path()`:

```cpp
std::string current_file_path() const override {
    return m_current_document_path; // Devuelve "" si es un archivo nuevo sin guardar
}
```

## Paso 2: Manejar Eventos

Una vez declaradas las capacidades, debes conectarte a los eventos correspondientes para procesar los archivos.

### Abrir Archivo / Carpeta
Cuando el usuario selecciona una opción de "Abrir" en el menú, Horizon abre automáticamente el diálogo de archivos. Si el usuario acepta, se dispara el evento `when_file_opened` (o `when_folder_opened`) con la ruta seleccionada.

```cpp
MyWindow::MyWindow() : Window("Mi App") {
    // Escuchar cuando se abre un archivo
    when_file_opened.connect([this](Window::FileOpenedContext& ctx) {
        LOG_INFO << "Archivo a cargar: " << ctx.path;
        // Lógica de carga aquí...
    });

    // Escuchar cuando se abre una carpeta
    when_folder_opened.connect([this](Window::FileOpenedContext& ctx) {
        LOG_INFO << "Carpeta seleccionada: " << ctx.path;
    });
}
```

### Cerrar
La acción de "Cerrar" no requiere diálogo; simplemente notifica a la ventana para que limpie su estado o cierre la pestaña/documento actual.

```cpp
when_file_close.connect([this](EventContext&) {
    LOG_INFO << "Cerrando archivo actual...";
    // Lógica de limpieza aquí...
});
```

### Guardar / Guardar Como

- **Guardar**: Si `current_file_path()` devuelve una ruta válida, Horizon dispara de inmediato el evento `when_save`. De lo contrario, actúa como "Guardar como".
- **Guardar como**: Siempre abre el diálogo de archivos y, tras la aceptación, dispara `when_save_as`.

```cpp
// Escuchar evento de guardar simple
when_save.connect([this](Window::FileSaveContext& ctx) {
    LOG_INFO << "Guardando en ruta existente: " << ctx.path;
    this->mi_logica_de_guardado(ctx.path);
});

// Escuchar evento de guardar como
when_save_as.connect([this](Window::FileSaveContext& ctx) {
    LOG_INFO << "Guardando en nueva ubicación: " << ctx.path;
    this->mi_logica_de_guardado(ctx.path);
    // IMPORTANTE: Actualiza tu ruta interna para que current_file_path() 
    // devuelva la nueva ruta en el futuro.
    this->m_current_document_path = ctx.path;
});
```

## Flujo de Automatización

El sistema funciona de la siguiente manera:
1. **Detección**: `WaylandWindow` busca un widget `Window` en el árbol que tenga capacidades de archivo.
2. **Inyección**: Se crea el menú global "Archivo" con las opciones permitidas.
3. **Acción**: El usuario selecciona una opción (ej: Ctrl+O).
4. **Diálogo**: Horizon abre un `FileDialog` modal.
5. **Notificación**: Si se selecciona un archivo, Horizon ejecuta el `EventsManager` correspondiente en tu ventana y emite señales locales (`file.opened`, `folder.opened`, `file.close`).

## Ventajas
- **Consistencia**: Todas las aplicaciones Horizon tienen el mismo menú de archivos en el mismo lugar.
- **Simplicidad**: Menos código redundante en cada aplicación (no necesitas instanciar `FileDialog` manualmente).
- **Accesibilidad**: Los atajos de teclado estándar se configuran automáticamente.
