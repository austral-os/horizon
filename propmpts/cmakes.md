Quiero que reorganices completamente la estructura CMake de mi proyecto llamado Horizon para convertirlo en una arquitectura modular y escalable.

Estado actual:

Tengo un CMakeLists.txt principal que compila una librería estática llamada horizon desde src/ e include/.

También compila un ejemplo minimal desde examples/minimal.

Todo está definido en un único CMakeLists.txt.

Se usan dependencias: wayland-client, wayland-cursor, cairo, xkbcommon, librsvg-2.0 y nlohmann_json.

Objetivo:

Reorganizar el proyecto en módulos independientes:

horizon (toolkit, como librería estática)

system/horizon-dialogd (servicio independiente)

examples/minimal (app de ejemplo)

Cada módulo debe tener su propio CMakeLists.txt.

El CMakeLists.txt raíz solo debe:

Definir el proyecto.

Configurar estándar C++17.

Buscar dependencias globales si es necesario.

Usar add_subdirectory() para incluir:

src (o toolkit)

system/horizon-dialogd

examples/minimal

Debe ser posible compilar objetivos individuales:

Solo el toolkit

Solo horizon-dialogd

Solo minimal

Todo el proyecto

No usar file(GLOB_RECURSE). Prefiero listas explícitas de archivos para evitar problemas de regeneración.

horizon debe exponer correctamente sus headers públicos usando target_include_directories con PUBLIC/PRIVATE bien definidos.

horizon-dialogd debe:

Compilar como ejecutable independiente.

Linkear contra horizon.

Vivir en system/horizon-dialogd.

Tener su propio CMakeLists.txt.

examples/minimal debe:

Compilar como ejecutable.

Linkear contra horizon.

Tener su propio CMakeLists.txt.

Debes mostrar:

Estructura final de carpetas recomendada.

CMakeLists.txt raíz.

CMakeLists.txt del toolkit.

CMakeLists.txt de horizon-dialogd.

CMakeLists.txt del ejemplo minimal.

Usa prácticas modernas de CMake (target-based, sin variables globales innecesarias).

Devuélveme todos los archivos CMake completos y bien estructurados.
