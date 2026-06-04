# Informe de Responsabilidades de la Clase `Application`

Este documento detalla las responsabilidades y funciones clave de la clase `Application` en el framework Horizon. La clase `Application` actúa como el orquestador central para cualquier aplicación dentro del ecosistema Horizon, gestionando desde la interacción con el servidor Wayland hasta el ciclo de vida de los widgets y la comunicación entre procesos.

## 1. Orquestación del Ciclo de Vida y Sistema Base
La responsabilidad primaria de `Application` es gestionar el ciclo de vida completo de la aplicación:
*   **Inicialización**: Configura el entorno base, incluyendo el registro del logger, el manejo de señales del sistema (como ignorar `SIGPIPE`) y la detección del compositor actual (Meteor, Labwc, etc.).
*   **Bucle de Eventos (Main Loop)**: Implementa el bucle principal utilizando `poll()`, lo que permite una espera eficiente de eventos de múltiples fuentes (Wayland, IPC, timers, `eventfd`).
*   **Finalización (Graceful Shutdown)**: Gestiona el cierre ordenado de la aplicación, notificando a los interesados y liberando recursos de hardware y Wayland.
*   **Wakeup Mechanism**: Provee un mecanismo basado en `eventfd` para despertar el bucle de eventos de forma segura desde otros hilos.

## 2. Gestión de Superficies Wayland
`Application` encapsula la complejidad de interactuar con el protocolo Wayland:
*   **Propiedad de la Superficie**: Posee y gestiona una instancia de `WaylandSurface`.
*   **Configuración de Ventanas**: Establece roles de ventanas (XDG Toplevel por defecto) y gestiona metadatos como el ID de aplicación, el título y los iconos.
*   **Propiedades Visuales**: Controla el tamaño de la ventana, la transparencia, el desenfoque (blur) y el estado del cursor del mouse.

## 3. Motor de Renderizado e Interfaz Gráfica
Actúa como el puente entre los widgets y el hardware gráfico:
*   **Jerarquía de Widgets**: Mantiene el widget raíz (`m_root`) y facilita la propagación descendente de estados y el renderizado.
*   **Gestión de Repintado (Invalidación)**: Implementa un sistema de invalidación selectiva para repintar solo los widgets "sucios" o la ventana completa según sea necesario.
*   **Integración Cairo y GL**:
    *   Provee un `GraphicsContext` basado en Cairo para el dibujo 2D de alta calidad.
    *   Gestiona recursos de OpenGL (Shaders, VBOs) para renderizar el buffer de Cairo y soporta dibujos 3D sincronizados mediante una cola de llamadas (`queue_gl_draw`).
*   **Limitador de Frames**: Sincroniza el renderizado con la tasa de refresco (capado a ~60fps) para evitar parpadeos y optimizar el uso de CPU/GPU.

## 4. Gestión de Entrada y Propagación de Eventos
Captura y enruta las interacciones del usuario hacia los componentes correctos:
*   **Eventos de Puntero (Mouse)**:
    *   Realiza *hit-testing* en el árbol de widgets para determinar el destino de clics y movimientos.
    *   Gestiona los estados de `hover`, `pressed` y `focused`.
    *   Implementa la lógica de redimensionado de ventanas detectando los bordes.
*   **Eventos de Teclado**:
    *   Enruta pulsaciones al widget enfocado.
    *   Implementa una lógica propia de **repetición de teclas** basada en software para asegurar consistencia.
    *   Maneja atajos globales (ej: F11 para pantalla completa, Ctrl+Q para salir).
*   **Gestión de Modificadores**: Rastrea el estado global de teclas como Shift, Ctrl y Alt.

## 5. Sistema de Menús Globales y de Aplicación
`Application` es fundamental para la integración con el sistema de menús de Horizon:
*   **Menús Globales**: Mantiene y expone la estructura de menús que se mostrará en la barra superior del sistema.
*   **Sincronización IPC**: Se comunica con el demonio de menús para actualizar la barra global cuando la aplicación gana o pierde el foco.
*   **Menú de Aplicación**: Gestiona un menú predeterminado (Salir, Preferencias, etc.) que se integra automáticamente.

## 6. Comunicación entre Procesos (IPC) y Señales
Facilita la interacción entre la aplicación y el resto del sistema:
*   **Suscripción a Mensajes**: Escucha mensajes IPC (vía sockets) para reaccionar a peticiones externas (ej: peticiones de maximizar desde un dock).
*   **Señales Internas**: Utiliza un `SignalManager` para desacoplar la lógica interna mediante un patrón de observador.
*   **Notificación de Estado**: Informa constantemente al gestor de sesiones (`horizon_session`) sobre cambios en su estado (si está minimizada, activa, o si se ha cerrado).
*   **Señalización Remota**: Permite enviar comandos a otros procesos del sistema.

## 7. Servicios de Utilidad y Gestión de Recursos
*   **Gestión de Temas**: Se integra con `ThemeManager` para responder dinámicamente a cambios de apariencia en todo el sistema.
*   **Caché de Imágenes**: Gestiona cachés para SVGs y superficies de Cairo para evitar el re-procesamiento costoso de recursos gráficos comunes.
*   **Administración de Timers**: Ofrece un API para programar tareas diferidas o periódicas integradas en el bucle principal.
*   **Monitoreo de "Foreign Toplevels"**: Permite a la aplicación (especialmente útil para docks o gestores de tareas) monitorear otras ventanas abiertas en la sesión Wayland.
*   **Task Posting**: Permite a hilos secundarios enviar funciones para ser ejecutadas de forma segura en el hilo principal de la aplicación.

---

## Especialización: `LayerApplication`
Cuando la aplicación es un componente del sistema (Panel, Dock, Escritorio), `LayerApplication` añade responsabilidades específicas:
*   **Protocolo Layer-Shell**: Uso de protocolos especializados para posicionamiento en capas específicas (Overlay, Background, etc.).
*   **Zonas de Exclusión**: Reserva de espacio en la pantalla para que otras ventanas no lo cubran.
*   **Anclaje y Margen**: Control preciso sobre qué bordes de la pantalla se "pega" la superficie.
*   **Interactividad dinámica**: Capacidad de alternar entre ser una interfaz interactiva o una superficie transparente que deja pasar los clics.
