Quiero que diseñes e implementes un servicio del sistema llamado horizon-dialogd dentro del proyecto Horizon.

Contexto general del proyecto:

Horizon es un toolkit gráfico propio escrito en C++.

Funciona sobre Wayland.

No utiliza GTK ni Qt.

Tiene arquitectura modular.

Posee una carpeta system/ destinada a servicios del sistema.

El toolkit ya tiene una clase Application que gestiona el loop de eventos y la integración con Wayland.

Objetivo del servicio:

horizon-dialogd es el primer servicio oficial del ecosistema Horizon.

Debe:

Ser un proceso independiente.

Ser una Application de Horizon (es decir, usar el toolkit para crear ventanas).

Actuar como daemon persistente.

No mostrar ninguna ventana al iniciar.

Crear diálogos bajo demanda cuando otras aplicaciones lo soliciten mediante IPC.

Ubicación obligatoria en el repositorio:

system/horizon-dialogd/

Estructura esperada:

system/
horizon-dialogd/
CMakeLists.txt
include/
src/

Debe integrarse correctamente al CMake principal del proyecto, pero mantenerse modular e independiente del core del toolkit.

ARQUITECTURA GENERAL

El proceso debe contener:

Una instancia de Horizon::Application.

Un servidor IPC basado en Unix Domain Socket.

Un DialogService interno que:

Reciba solicitudes JSON.

Cree instancias de diálogos usando Horizon.

Devuelva resultados asíncronamente.

Gestione múltiples solicitudes simultáneas.

No debe existir ningún código monolítico. Separar claramente:

IPC Layer

Request Router

Dialog Manager

Dialog Base Class

Implementaciones concretas (FileDialog, MessageBox)

Integración Wayland (a través del toolkit Horizon)

REQUISITOS FUNCIONALES

Debe soportar múltiples aplicaciones solicitando diálogos al mismo tiempo.

Cada solicitud debe:

Generar una instancia independiente de diálogo.

Tener su propio request_id.

Asociarse como transient al parent_surface_id correcto.

Ser aislada del resto.

No debe existir ningún estado global compartido incorrectamente.

El servicio debe manejar:

Múltiples conexiones IPC simultáneas.

Múltiples diálogos activos al mismo tiempo.

Limpieza automática si un cliente IPC se desconecta.

Cierre automático de diálogos huérfanos.

PROTOCOLO IPC

Comunicación mediante Unix Domain Socket.

Mensajes JSON versionados y extensibles.

Ejemplo de request:

{
"version": 1,
"request_id": "uuid",
"type": "open_file",
"parent_surface_id": "...",
"options": {
"title": "Abrir archivo",
"filters": [".txt", ".png"],
"allow_multiple": false
}
}

Ejemplo de respuesta:

{
"request_id": "uuid",
"status": "accepted",
"result": {
"paths": ["/home/user/file.txt"]
}
}

o

{
"request_id": "uuid",
"status": "cancelled"
}

Requisitos:

Validación estricta de JSON.

Manejo robusto de errores.

Aislamiento por conexión.

No permitir que un cliente interfiera con diálogos de otro.

Manejar reconexiones si corresponde.

INTEGRACIÓN CON WAYLAND

Usar xdg-shell.

Crear xdg_toplevel.

Asociar cada diálogo como transient del parent_surface_id recibido.

Soportar modalidad configurable.

No bloquear el loop principal.

Usar exclusivamente el toolkit Horizon para crear y gestionar ventanas.

DIÁLOGOS SOPORTADOS EN ESTA PRIMERA VERSIÓN

File Open Dialog

Message Box (info, warning, error)

Diseñar el sistema para permitir agregar fácilmente:

Save File Dialog

Color Picker

Portal de permisos

INTEGRACIÓN DEL LADO CLIENTE (MUY IMPORTANTE)

Además del daemon, se debe diseñar e implementar el cliente dentro del toolkit Horizon.

Crear una clase:

DialogServiceClient

Requisitos del cliente:

Vivir dentro del toolkit (no en el daemon).

Encapsular completamente:

Conexión al socket

Serialización y deserialización JSON

Gestión de request_id

Manejo asíncrono de respuestas

Integrarse con el event loop existente del toolkit.

Soportar múltiples requests simultáneos.

Manejar reconexión automática si el servicio cae.

INTEGRACIÓN CON Application:

La clase Application debe contener una instancia privada de DialogServiceClient.

Ejemplo conceptual:

class Application {
private:
std::unique*ptr<DialogServiceClient> dialog_service*;
};

La Application no debe:

Manejar sockets directamente.

Parsear JSON.

Conocer detalles del protocolo.

Debe exponer una API limpia como:

application.dialogs().open_file(...);

El código de Application debe mantenerse limpio y desacoplado.

REQUISITOS DE DISEÑO

C++ moderno.

Arquitectura modular.

Separación estricta de responsabilidades.

Modelo completamente asíncrono.

Preparado para escalar a múltiples servicios futuros dentro de system/.

ENTREGABLES ESPERADOS

Diagrama de arquitectura completo.

Estructura de carpetas detallada.

CMakeLists.txt funcional.

Clases principales bien separadas.

Código base del daemon funcional.

Implementación del cliente DialogServiceClient.

Ejemplo mínimo de aplicación Horizon que use:

application.dialogs().open_file(...);

Explicación del flujo completo:

App → DialogServiceClient → IPC → horizon-dialogd → creación de diálogo → respuesta → callback en app.

No quiero prototipo improvisado.
No quiero acoplamiento fuerte.
No quiero código monolítico.
Quiero arquitectura lista para crecer.
