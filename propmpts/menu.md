Estoy desarrollando mi propio toolkit gráfico llamado Horizon (basado en Wayland y Cairo, sin GTK ni Qt).
Necesito crear una nueva aplicación dentro de la carpeta examples/ llamada menu_manager.

Requisitos funcionales:

La aplicación debe:

Crear una superficie Wayland que ocupe toda la pantalla.

No tener decoración de ventana.

No comportarse como una ventana tradicional redimensionable.

Inicializarse como una capa superior (tipo overlay).

Debe usar el protocolo wlr-layer-shell si está disponible, configurándose como:

layer = overlay

Anchored a los bordes necesarios para ocupar toda la pantalla.

Con tamaño igual al output activo.

Con exclusive_zone = -1 si corresponde.

Debe poder mostrar dinámicamente un widget de Horizon (por ejemplo un panel o popup) en una posición arbitraria de la pantalla.

El widget debe renderizarse por encima de cualquier otra aplicación Wayland.

Debe permitir cambiar su posición programáticamente.

Debe poder mostrarse y ocultarse.

Debe integrarse correctamente al sistema de build actual basado en CMake:

Agregar el subdirectorio examples/menu_manager

Crear su propio CMakeLists.txt

Enlazar contra las librerías internas de Horizon

Enlazar contra wayland-client y layer-shell si es necesario.

La estructura de carpetas debe quedar así:

examples/
 └── menu_manager/
      ├── main.cpp
      └── CMakeLists.txt

El código debe:

Inicializar Wayland

Detectar outputs

Crear superficie layer-shell fullscreen

Renderizar usando Cairo

Integrarse con el loop de eventos existente de Horizon

Generar:

El main.cpp

El CMakeLists.txt

Las modificaciones necesarias en el CMakeLists.txt raíz

Comentarios técnicos explicando cada decisión relevante

El código debe ser claro, minimalista y coherente con un toolkit propio (no usar librerías externas de UI).