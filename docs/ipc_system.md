# Documentación del Sistema IPC en Horizon

El sistema IPC (Inter-Process Communication) en Horizon permite que las diferentes aplicaciones y servicios del sistema operativo se comuniquen entre sí de manera rápida y eficiente usando _Unix Domain Sockets_ (sockets locales).

Esta comunicación se logra principalmente a través de dos clases fundamentales: `IpcServer` e `IpcClient`.

---

## 1. `IpcServer` (El Servidor)

**Archivo:** `include/horizon/IpcServer.hpp`

El `IpcServer` es la clase encargada de "escuchar" peticiones en una dirección específica (un archivo `.sock` en el sistema, por ejemplo `/tmp/horizon_apps.sock`). Piensa en el servidor como una central telefónica que está atenta a recibir llamadas.

### ¿Cómo funciona?

- **Creación (`IpcServer(socket_path, handler)`):** Cuando creas un servidor, le dices en qué ruta debe escuchar (`socket_path`) y qué debe hacer cuando recibe un mensaje. Ese "qué debe hacer" es una función llamada `handler` (manejador) que toma el mensaje recibido (un texto, usualmente en formato JSON), hace algo con él, y devuelve una respuesta de texto.
- **Iniciar y Detener (`start()`, `stop()`):** El servidor no empieza a escuchar hasta que llamas a `start()`. Esto crea hilos secretos (threads) en segundo plano para no congelar la aplicación principal mientras espera mensajes. Para apagarlo, usas `stop()`.
- **Difusión (`broadcast(msg)`):** Un servidor también tiene el "superpoder" de enviar un mensaje a todos los clientes que se hayan suscrito previamente. Por ejemplo, si cambia la lista de aplicaciones abiertas, el servidor puede gritar (hacer un broadcast) a todos los clientes interesados para que actualicen sus interfaces.

---

## 2. `IpcClient` (El Cliente)

**Archivo:** `include/horizon/IpcClient.hpp`

El `IpcClient` es la herramienta que usan otras aplicaciones para "llamar" o conectarse a un servidor existente y enviarle mensajes.

### ¿Cómo funciona?

- **Creación (`IpcClient(socket_path)`):** Al crearlo, le dices a qué dirección (ruta del archivo `.sock`) debe conectarse. Tiene que ser la misma ruta donde un `IpcServer` está escuchando.
- **Enviar y Recibir (`send(message, response)` y `send(message)`):** Puedes enviarle un mensaje de texto al servidor.
    - Si usas la versión con `response`, el cliente se quedará esperando a que el servidor le conteste y pondrá la respuesta en esa variable.
    - Si usas la versión sin `response`, el cliente simplemente dejará el mensaje y seguirá su camino ("fire and forget").
- **Suscribirse (`subscribe(message, callback)`):** Si una aplicación quiere enterarse constantemente de lo que pasa en el servidor (por ejemplo, si se abre una nueva ventana), puede "suscribirse". Envía un mensaje inicial (como `{"type": "subscribe"}`) y a partir de ahí, cada vez que el servidor haga un `broadcast`, el cliente ejecutará la función `callback` con el mensaje recibido.

---

## Ejemplo de Uso en el Ecosistema Horizon

Para entender esto perfectamente, veamos cómo interactúan tres aplicaciones clave del sistema: **app_manager**, **top_panel** y **menu_manager_horizon_d**. Están comunicándose todo el tiempo gracias a este sistema.

### Caso 1: `app_manager` y la gestión de aplicaciones

El **AppManager** es el cerebro que sabe qué aplicaciones están abiertas.

- **Como Servidor:** `app_manager` crea un `IpcServer` en `/tmp/horizon_apps.sock`.
- **Qué hace:** Constantemente recibe mensajes en formato JSON indicando que una aplicación se abrió (`app_started`), se cerró (`app_stopped`) o se minimizó.
- **Broadcast:** Cuando registra un cambio en la lista de aplicaciones, usa el método `broadcast()` para enviar la lista actualizada a todos los clientes suscritos. Así, si tuvieras un dock o una barra de tareas, estos sabrían qué iconos mostrar.

### Caso 2: El menú global (`top_panel` y `menu_manager_horizon_d`)

El panel superior (`top_panel`) y el gestor de menús (`menu_manager_horizon_d`) tienen una danza muy interesante para mostrarte los menús desplegables cuando haces clic arriba.

1. **`top_panel` recibe los menús de las apps:**
    - `top_panel` tiene su propio `IpcServer` escuchando en `/tmp/horizon_global_menu.sock`.
    - Las aplicaciones dibujan sus menús en este panel enviándole mensajes a este socket.
2. **Haces clic en "Archivo" en el `top_panel`:**
    - Para no dibujar los menús desplegables él mismo (ya que el top panel es chiquito y cortaría el menú), le delega el trabajo al **Menu Manager Daemon**.
    - `top_panel` actúa interinamente como **Cliente IPC** (usando la clase `ClientMenu` que usa `IpcClient` por debajo) para decirle al `menu_manager_horizon_d`: _"¡Oye, dibuja este menú desplegable en estas coordenadas X e Y!"_ (mensaje `"create_menu"`).
3. **El Menu Manager responde (`menu_manager_horizon_d`):**
    - Este daemon tiene su propio `IpcServer` en `/tmp/horizon_menu.sock` que estaba esperando esa llamada.
    - Recibe las instrucciones, dibuja el menú gigante sobre la pantalla y se hace visible.
    - **Feedback al Panel:** Para decirle al `top_panel` que el menú ya está visible (para que el botón de "Archivo" se quede marcado/iluminado), el daemon se convierte en **Cliente IPC**. Usa un `IpcClient` para conectarse de vuelta al `/tmp/horizon_global_menu.sock` del `top_panel` y enviarle un mensaje `"menu_daemon_status"` diciendo `visible: true`.
4. **Cierras el menú (presionas Escape):**
    - El daemon detecta el "Escape", esconde el menú.
    - Usa de nuevo su `IpcClient` hacia el `top_panel` enviando `"menu_daemon_status"` con `visible: false`. El botón del panel se desmarca mágicamente.

**En resumen:**

- `app_manager`: Sirve información sobre procesos corriendo en `/tmp/horizon_apps.sock`.
- `top_panel`: Sirve la lista de menús globales en `/tmp/horizon_global_menu.sock`.
- `menu_manager_d`: Sirve el dibujado de menús en `/tmp/horizon_menu.sock`.
- Y además de ser servidores, **todos usan clientes IPC para chusmearse información mutuamente**, creando un sistema rápido y modular donde cada aplicación hace solo una cosa, pero la hace muy bien.
