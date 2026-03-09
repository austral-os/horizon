# Informe de Mecanismos de Manejo de Ventanas en Horizon

Este documento detalla los mecanismos actuales implementados en el proyecto Horizon para las operaciones de maximizar, minimizar, mover y redimensionar aplicaciones.

## 1. Movimiento de Ventanas (Move)

El movimiento de las ventanas está integrado principalmente en el widget `Titlebar` y se comunica al compositor a través del protocolo Wayland.

*   **Activación**: Se dispara cuando el usuario hace clic y arrastra en la barra de título (`Titlebar.cpp`).
*   **Proceso**:
    1.  El widget `Titlebar` detecta un evento de arrastre (`when_mouse_drag`).
    2.  Llama a `application()->request_move()`.
    3.  `Application::request_move()` delega en `WaylandSurface::request_move(serial)`.
    4.  Finalmente, se ejecuta la función de Wayland `xdg_toplevel_move`, que permite al compositor tomar el control del movimiento de la ventana de forma nativa.
*   **Requisito**: Es necesario el `serial` del evento de presión del ratón para validar la petición ante el compositor.

## 2. Redimensionamiento (Resize)

El redimensionamiento es manejado de forma interactiva por la clase `Application`, que detecta la proximidad de los bordes.

*   **Detección**: En `Application::handle_move`, se comprueba si el puntero se encuentra a menos de 8 píxeles (`m_resize_proximity`) de cualquier borde de la ventana.
*   **Interfaz Visual**: El cursor cambia automáticamente a las variantes de redimensionamiento (NS, EW, NESW, NWSE) según el borde detectado, utilizando `WaylandSurface::set_cursor`.
*   **Activación**: Si el usuario presiona el botón del ratón mientras el puntero está en un borde detectado:
    1.  `Application::handle_press` identifica el `m_resize_edge`.
    2.  Llama a `m_surface->request_resize(serial, edge)`.
    3.  `WaylandSurface` utiliza `xdg_toplevel_resize` para entregar el control del redimensionado al compositor.
*   **Restricción**: Esta operación está deshabilitada si la ventana ya está maximizada.

## 3. Maximizar y Minimizar (Maximize/Minimize)

Estas operaciones pueden ser disparadas tanto localmente por la interfaz de la aplicación como remotamente vía IPC.

### Maximizar / Restaurar
*   **Local**: Botones en la `Titlebar` llaman a `application()->maximize()` o `application()->restore()`.
*   **Wayland**: Se utilizan `xdg_toplevel_set_maximized` y `xdg_toplevel_unset_maximized`.
*   **Lógica**: La aplicación realiza un seguimiento del estado (`is_maximized()`) para decidir si restaurar o maximizar.

### Minimizar
*   **Local**: Botón en la `Titlebar` llama a `application()->minimize()`.
*   **Wayland**: Se utiliza `xdg_toplevel_set_minimized`.
*   **Notificación**: La aplicación informa al `horizon_session` del cambio de estado mediante un mensaje IPC de tipo `window_state_changed`.

## 4. Control Remoto e IPC (Inter-Process Communication)

Horizon utiliza un sistema de mensajería basado en JSON sobre Sockets de Dominio Unix (`/tmp/horizon_session.sock`) para permitir que el compositor u otras aplicaciones controlen las ventanas.

*   **Suscripción**: Cada aplicación se suscribe al bus de mensajes en `Application::run()`.
*   **Señales soportadas**: El sistema escucha mensajes de tipo `app_signal` con las señales:
    *   `maximize`
    *   `minimize`
    *   `restore` (requiere un token de activación de Wayland para recuperar el foco).
    *   `close`
    *   `fullscreen` / `unfullscreen`
*   **Restauración Compleja**: Debido a las restricciones de seguridad de Wayland, la restauración desde un estado minimizado a menudo requiere el uso de "Activation Tokens" (`xdg_activation_v1`) para permitir que la aplicación recupere el foco correctamente.

## Resumen de Clases Involucradas

| Clase | Responsabilidad |
| :--- | :--- |
| `Titlebar` | Punto de entrada del usuario para mover, cerrar, maximizar y minimizar. |
| `Application` | Lógica de alto nivel, detección de bordes para resize y manejo de mensajes IPC. |
| `WaylandSurface` | Implementación de bajo nivel de las peticiones al compositor via Protocolo Wayland (XDG Shell). |
| `IpcClient`/`IpcServer` | Infraestructura para la comunicación remota entre `horizon_session` y las aplicaciones. |
