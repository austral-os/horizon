# Documentación Técnica: Sistema de Eventos en Horizon

Este documento describe el funcionamiento del `EventsManager` y la estructura del `EventContext` dentro del framework Horizon, detallando cómo se propagan los eventos y qué parámetros se envían en cada caso.

## 1. Arquitectura General

El sistema de eventos se basa en el patrón **Observer/Publisher-Subscriber**.

- **`EventsManager`**: Actúa como el despachador de eventos al que se pueden conectar múltiples callbacks.
- **`EventContext`**: Estructura de datos que encapsula toda la información relevante de un evento (sender, tipo, posición, teclas, etc.).

## 2. Estructura del EventContext

Cada vez que se invoca el método `.run(EventContext &ev)`, se pasan los siguientes campos principales:

| Campo              | Tipo          | Descripción                                                                |
| :----------------- | :------------ | :------------------------------------------------------------------------- |
| `sender`           | `void*`       | Puntero al objeto que disparó el evento.                                   |
| `type`             | `EventType`   | Enum que define la categoría del evento (MousePress, KeyPress, etc.).      |
| `button`           | `uint32_t`    | Código del botón del mouse o tecla (según el contexto).                    |
| `eventX / eventY`  | `double`      | Coordenadas del cursor en el momento del evento.                           |
| `key / keysym`     | `uint32_t`    | Identificadores de tecla para eventos de teclado.                          |
| `modifiers`        | `uint32_t`    | Estado de teclas especiales (Shift, Ctrl, Alt).                            |
| `text`             | `std::string` | Texto asociado (útil en entradas de texto).                                |
| `data`             | `void*`       | Puntero a datos personalizados específicos del evento.                     |
| `stop_propagation` | `bool`        | Si se establece en `true`, detiene la burbuja del evento hacia los padres. |

---

## 3. Disparadores de Eventos (Calls a .run)

A continuación se listan los lugares clave donde se originan los eventos y los parámetros que configuran en el `EventContext`:

### Sistema (Clase `Application`)

- **Activación/Desactivación**:
    - `when_activated / when_deactivated.run(ev)`
    - Parámetros: `sender = this`, `type = AppActivated / AppDeactivated`.
- **Entrada de Teclado**:
    - `when_key_press / when_key_release.run(new_ev)`
    - Parámetros: `type`, `button`, `key`, `keysym`, `modifiers`, `text`.
- **Interacción de Mouse**:
    - `when_mouse_press / release / move / enter / leave / drag / hover`.
    - Parámetros: `sender = widget_objetivo`, `eventX`, `eventY`, `button`, `modifiers`, `serial`.

### Capa de Widgets (Clase `Widget`)

- **Foco**:
    - `when_focus / when_blur.run(ev)`
    - Parámetros: `sender = this`.

### Administrador de Temas (`ThemeManager`)

- **Cambio de Tema**:
    - `when_change.run(ev)`
    - Parámetros: `type = EventType::ThemeChanged`.

---

## 4. Eventos de Componentes Específicos

Muchos widgets extienden el sistema para notificar cambios de estado internos utilizando el campo `data`:

| Clase                | EventManager          | Datos en `EventContext.data`                          |
| :------------------- | :-------------------- | :---------------------------------------------------- |
| `Slider`             | `when_value_changed`  | Puntero al valor float (`&m_value`).                  |
| `ColorPicker`        | `when_color_changed`  | Puntero a la estructura `Color` (`&m_color`).         |
| `TextBox / Textarea` | `when_text_changed`   | Reutiliza el contexto del evento de teclado original. |
| `ScrollArea`         | `when_scroll`         | `sender = this`, `data = nullptr`.                    |
| `GradientBar`        | `when_value_changed`  | Puntero al valor float (`&m_value`).                  |
| `SearchBox`          | `when_text_changed`   | `sender = this` (disparado al limpiar el texto).      |
| `ColorArea2D`        | `when_values_changed` | `data = this` (puntero a la instancia).               |

## 5. Propagación de Eventos

En Horizon, los eventos de mouse se propagan desde el widget que recibe el impacto (`hit_test`) hacia sus ancestros en el árbol visual, a menos que un manejador establezca `ev.stop_propagation = true`. Esto permite que contenedores como `Titlebar` capturen eventos de arrastre incluso si el click ocurrió en su fondo.
