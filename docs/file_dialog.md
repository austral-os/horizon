# Documentación de `FileDialog`

El framework Horizon proporciona la clase `FileDialog` para facilitar la selección de archivos y carpetas dentro de las aplicaciones. Esta componente hereda de `WaylandWindow` y ofrece una interfaz estandarizada que incluye una barra lateral de lugares, vista de archivos (rejilla o lista), barra de herramientas con búsqueda y controles de navegación.

## 1. Modos de Operación

Al instanciar un `FileDialog`, se debe especificar el modo de operación mediante el enumerado `FileDialogMode`. Los modos disponibles son:

| Modo | Comportamiento |
| :--- | :--- |
| `FileDialogMode::Open` | Optimizado para abrir archivos existentes. El botón principal muestra "Open". Al hacer doble clic sobre un archivo, se acepta la selección inmediatamente. |
| `FileDialogMode::Save` | Optimizado para guardar archivos. El botón principal muestra "Save" y la etiqueta del campo de texto cambia a "Save as:". |
| `FileDialogMode::SaveAs` | Similar a `Save`. (Actualmente comparte la mayoría de la lógica visual con `Save`). |
| `FileDialogMode::SelectFolder` | Diseñado para seleccionar directorios. |
| `FileDialogMode::New` | Utilizado para la creación de nuevos archivos o proyectos. |

## 2. Uso Básico

Para utilizar un diálogo de archivos, se debe crear una instancia, configurar los callbacks de respuesta y mostrar la ventana.

### Ejemplo: Abrir un archivo

```cpp
#include <horizon/dialogs/FileDialog/FileDialog.hpp>

// ... dentro de algún método de tu aplicación

auto dialog = std::make_unique<horizon::FileDialog>(
    horizon::FileDialogMode::Open, 
    "Abrir Documento"
);

// Configurar qué hacer cuando el usuario selecciona un archivo
dialog->when_accepted.connect([](horizon::FileDialogAcceptedContext &ctx) {
    LOG_INFO("Archivo seleccionado: {}", ctx.selected_path);
    // Lógica para abrir el archivo
});

// Configurar qué hacer si el usuario cancela
dialog->when_cancelled.connect([](horizon::FileDialogCancelledContext &ctx) {
    LOG_INFO("Selección cancelada");
});

// Establecer la ruta inicial (opcional)
dialog->set_current_path("/home/user/Documents");

// El diálogo se muestra automáticamente al ser manejado por el sistema de ventanas
// o puede ser añadido a la aplicación.
```

## 3. API Pública

### Constructor
```cpp
FileDialog(FileDialogMode mode, const std::string &title = "");
```
*   `mode`: El modo de operación (`Open`, `Save`, `SaveAs`, `SelectFolder` o `New`).
*   `title`: El título que se mostrará en la barra de la ventana. Si se deja vacío, se usará un valor por defecto según el modo.

### Métodos
*   `void set_current_path(const std::string &path)`: Cambia el directorio actual que muestra el diálogo.
*   `std::string selected_path() const`: Retorna la ruta completa actualmente ingresada o seleccionada en el diálogo.

### Señales (EventsManager)
*   `when_accepted`: `EventsManager<FileDialogAcceptedContext>`. Se ejecuta cuando el usuario confirma la acción (clic en Open/Save o Enter en el campo de texto). El contexto contiene la propiedad `selected_path`.
*   `when_cancelled`: `EventsManager<FileDialogCancelledContext>`. Se ejecuta cuando el usuario cierra el diálogo o presiona "Cancel".

## 4. Detalles de Implementación y Navegación

*   **Barra Lateral (Sidebar)**: Permite accesos rápidos a carpetas del sistema como **All My Files**, **Aplicaciones**, **Desktop**, **Documents**, **Downloads** e **iCloud Drive**.
*   **Búsqueda**: La barra de herramientas incluye un campo de búsqueda filtrada en tiempo real.
*   **Modos de Vista**: El usuario puede alternar entre vista de **Iconos (Grid)**, **Lista (List)** y **CoverFlow** a través de los controles en la barra de herramientas.
*   **Navegación**: Soporta navegación hacia atrás y hacia adelante, similar a un navegador web.

## 5. Consideraciones para el modo de Guardado

En el modo `Save` o `SaveAs`, el diálogo permite al usuario escribir un nombre de archivo que aún no existe en el directorio actual. La señal `when_accepted` devolverá la ruta completa construida a partir del directorio actual y el nombre ingresado.

---
> [!TIP]
> Dado que `FileDialog` es una `WaylandWindow`, se comporta como una ventana independiente. Asegúrate de gestionar correctamente el ciclo de vida del objeto (por ejemplo, manteniéndolo en un `std::unique_ptr` dentro de tu clase principal) para evitar que se destruya antes de recibir la respuesta.
