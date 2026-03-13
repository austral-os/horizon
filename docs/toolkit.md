# Horizon Toolkit - UML Diagram

## Overview

Horizon es un toolkit de UI moderno diseñado para aplicaciones Wayland. Proporciona un conjunto completo de widgets y herramientas para crear interfaces gráficas de usuario estilo macOS/Aqua en entornos Linux con Wayland.

## Architecture Overview

```mermaid
graph TB
    subgraph Core
        A[Application] --> W[Window]
        W --> Wdg[Widget]
    end
    
    subgraph Rendering
        Gc[GraphicsContext] --> AObj[AquaObject]
        Gc --> SObj[SolidObject]
        Gc --> AP[AquaPolygon]
    end
    
    subgraph Components
        Wdg --> C[Container Widgets]
        Wdg --> I[Input Widgets]
        Wdg --> V[Visual Widgets]
    end
```

## Class Hierarchy

```mermaid
classDiagram
    class Widget {
        +set_position(int, int)
        +set_size(int, int)
        +set_visible(bool)
        +set_enabled(bool)
        +set_focus(bool)
        +add_child(unique_ptr~Widget~)
        +render(GraphicsContext&, int, int, int, int, bool)
        +draw(GraphicsContext&)
        +when_mouse_press: EventsManager
        +when_click: EventsManager
        +when_mouse_move: EventsManager
    }
    
    class Window {
        +set_root(unique_ptr~Widget~)
        +set_size(int, int)
        +title(): string&
        +maximize()
        +minimize()
        +fullscreen()
        +request_move()
    }
    
    class AquaObject {
        +set_corner_radius(CornerRadius)
        +draw(GraphicsContext&)
    }
    
    class SolidObject {
        +set_corner_radius(CornerRadius)
        +draw(GraphicsContext&)
    }
    
    class AquaPolygon {
        +set_points(vector~Point~)
        +draw(GraphicsContext&)
    }
    
    Widget <|-- Window
    Widget <|-- AquaObject
    Widget <|-- SolidObject
    Widget <|-- AquaPolygon
```

## Widget Classes

### Container Widgets

```mermaid
classDiagram
    class Widget {
        <<abstract>>
    }
    
    class Notebook {
        +add_tab(NotebookPage)
        +set_current_tab(int)
    }
    
    class ScrollArea {
        +set_content(unique_ptr~Widget~)
        +set_scroll_position(int, int)
        +scroll_x(): int
        +scroll_y(): int
        +when_scroll: EventsManager
    }
    
    class VPanel {
        +add_child(unique_ptr~Widget~)
        +set_left_width(int)
    }
    
    class Frame {
        +set_shadow(bool)
    }
    
    class Sidebar {
        +add_group(string)
        +add_item(string, unique_ptr~Widget~)
    }
    
    Widget <|-- Notebook
    Widget <|-- ScrollArea
    Widget <|-- VPanel
    Widget <|-- Frame
    Widget <|-- Sidebar
```

### Input Widgets

```mermaid
classDiagram
    class Widget {
        <<abstract>>
    }
    
    class Button~T~ {
        +set_text(string)
        +set_font_weight(FontWeight)
        +text(): string&
    }
    
    class TextBoxBase {
        +set_text(string)
        +set_placeholder(string)
        +text(): string&
        +when_text_changed: EventsManager
    }
    
    class Checkbox~T~ {
        +set_text(string)
        +set_checked(bool)
        +is_checked(): bool
    }
    
    class RadioButton~T~ {
        +set_text(string)
        +set_selected(bool)
        +is_selected(): bool
        +set_on_select(function)
    }
    
    class Slider {
        +set_value(float)
        +set_tick_count(int)
        +set_show_ticks(bool)
        +set_thumb_shape(ThumbShape)
    }
    
    class Textarea {
        +set_text(string)
        +set_placeholder(string)
    }
    
    class SearchBox {
        +set_placeholder(string)
    }
    
    class ColorPicker {
        +set_color(Color)
        +color(): Color
    }
    
    class ProgressBar {
        +set_progress(float)
    }
    
    class GroupButton {
        +add_item(string)
        +add_item(unique_ptr~Widget~)
    }
    
    Widget <|-- Button
    Widget <|-- TextBoxBase
    Widget <|-- Checkbox
    Widget <|-- RadioButton
    Widget <|-- Slider
    Widget <|-- Textarea
    Widget <|-- SearchBox
    Widget <|-- ColorPicker
    Widget <|-- ProgressBar
    Widget <|-- GroupButton
    
    TextBoxBase <|-- TextBox
```

### Visual Widgets

```mermaid
classDiagram
    class Widget {
        <<abstract>>
    }
    
    class Label {
        +set_text(string)
        +set_alignment(TextAlignment)
        +set_vertical_alignment(VerticalAlignment)
        +set_font_weight(FontWeight)
        +set_font_slant(FontSlant)
        +set_font_size(int)
        +set_text_color(Color)
    }
    
    class Icon {
        +set_icon_name(string)
        +set_icon_size(int)
        +resolved_path(): string&
    }
    
    class Image {
        +set_path(string)
        +set_mode(ImageMode)
    }
    
    class Menu {
        +add_item(unique_ptr~MenuItem~)
        +add_separator()
        +set_title(string)
        +close_submenus()
    }
    
    class MenuItem {
        +set_text(string)
        +set_shortcut(string)
        +set_submenu(Menu*)
    }
    
    class TableView~T~ {
        +add_column(TableColumn~T~)
        +set_data(vector~T~)
        +get_selected_items(): vector~T~
        +set_selected_index(int)
        +when_row_click: EventsManager
    }
    
    Widget <|-- Label
    Widget <|-- Icon
    Widget <|-- Image
    Widget <|-- Menu
    Widget <|-- MenuItem
    Widget <|-- TableView
```

## Application Architecture

```mermaid
classDiagram
    class Application {
        +app_id(): string&
        +name(): string&
        +set_name(string)
        +set_icon_name(string)
        +set_show_in_dock(bool)
        +set_root(unique_ptr~Window~)
        +register_window(Window*)
        +unregister_window(Window*)
        +active_window(): Window*
        +run()
        +quit()
        +add_timer(int, function~, bool): size_t
        +signal_manager: SignalManager
        +theme_manager: unique_ptr~ThemeManager~
    }
    
    class Window {
        +Window(Application*, string, int, int)
        +set_root(unique_ptr~Widget~)
        +set_size(int, int)
        +title(): string&
        +maximize()
        +minimize()
        +fullscreen()
        +is_active(): bool
        +w_surface(): WaylandSurface*
        +app(): Application*
    }
    
    class WaylandSurface {
        +wl_surface*: wl_surface
        +xdg_surface*: xdg_surface
    }
    
    Application --> Window : creates
    Window --> WaylandSurface : manages
```

## Event System

```mermaid
classDiagram
    class EventsManager~T~ {
        +connect(function~T~): size_t
        +disconnect(size_t)
        +run(T&)
    }
    
    class SignalManager {
        +emit(string, void*)
        +connect(string, function~SignalContext&~)
        +disconnect(string, size_t)
    }
    
    class EventContext {
        +stop_propagation: bool
    }
    
    class MouseButtonEventContext {
        +button: uint32_t
        +x: int
        +y: int
        +modifiers: uint32_t
    }
    
    class MouseMoveEventContext {
        +x: int
        +y: int
        +dx: int
        +dy: int
    }
    
    class KeyEventContext {
        +key: uint32_t
        +modifiers: uint32_t
    }
    
    EventContext <|-- MouseButtonEventContext
    EventContext <|-- MouseMoveEventContext
    EventContext <|-- KeyEventContext
```

## Theme & Styling

```mermaid
classDiagram
    class ThemeManager {
        +get_color(string): Color
        +get_font(string): ThemeFont
        +get_icon_theme(): string
    }
    
    class Color {
        +r: float
        +g: float
        +b: float
        +a: float
    }
    
    class CornerRadius {
        +top_left: int
        +top_right: int
        +bottom_left: int
        +bottom_right: int
    }
    
    class WidgetAccentColor {
        <<enumeration>>
        Default
        Primary
        Secondary
        Success
        Warning
        Error
        Info
    }
    
    class WidgetDrawState {
        <<enumeration>>
        NORMAL
        HOVERED
        PRESSED
        FOCUSED
        DISABLED
    }
```

## Component Diagram - Example Usage

```mermaid
graph TD
    subgraph "Minimal App Example"
        App[Application] --> Wnd[Window]
        Wnd --> NB[Notebook]
        
        NB --> Tab1[Tab: Buttons]
        NB --> Tab2[Tab: Icons]
        NB --> Tab3[Tab: TextBox]
        NB --> Tab4[Tab: Label]
        NB --> Tab5[Tab: Check/Radio]
        NB --> Tab6[Tab: Scroll]
        NB --> Tab7[Tab: Table]
        NB --> Tab8[Tab: Color]
        NB --> Tab9[Tab: VPanel]
        NB --> Tab10[Tab: Textarea]
        NB --> Tab11[Tab: Menus]
        
        Tab1 --> Btn1[Button~AquaObject~]
        Tab1 --> Btn2[Button~SolidObject~]
        Tab1 --> Btn3[Button with Accent Colors]
        
        Tab2 --> Icon1[Icon]
        Tab2 --> Icon2[Icon]
        
        Tab3 --> TB[TextBox]
        Tab3 --> SB[SearchBox]
        Tab3 --> PB[ProgressBar]
        Tab3 --> SL[Slider]
        Tab3 --> GB[GroupButton]
        
        Tab4 --> Lbl1[Label]
        Tab4 --> Lbl2[Label Bold/Centered]
        Tab4 --> Lbl3[Label Italic/Right]
        
        Tab5 --> Cb[Checkbox]
        Tab5 --> Rb[RadioButton]
        
        Tab6 --> SA[ScrollArea]
        
        Tab7 --> TV[TableView~Person~]
        
        Tab8 --> CP[ColorPicker]
        
        Tab9 --> VP[VPanel]
        VP --> Side[Sidebar]
        VP --> FT[TableView~FileEntry~]
        
        Tab10 --> TA[Textarea]
        
        Tab11 --> Menu[Menu]
    end
```

## Widget Factory Pattern

```mermaid
classDiagram
    class Button~T~ {
        <<template>>
    }
    
    class AquaObject
    class SolidObject
    
    Button~AquaObject~ --|> AquaObject
    Button~SolidObject~ --|> SolidObject
    
    class Checkbox~T~ {
        <<template>>
    }
    
    Checkbox~AquaObject~ --|> AquaObject
    Checkbox~SolidObject~ --|> SolidObject
    
    class RadioButton~T~ {
        <<template>>
    }
    
    RadioButton~AquaObject~ --|> AquaObject
    RadioButton~SolidObject~ --|> SolidObject
```

## Usage Example

```cpp
// Creating an application
Application app("horizon.minimal", 800, 600);
app.set_name("Minimal Demo");
app.set_icon_name("system-help");

// Creating a window
auto wnd = std::make_unique<Window>(&app, "Horizon Application");
wnd->set_size(800, 600);

// Adding widgets
auto notebook = std::make_unique<Notebook>();
auto container = std::make_unique<Widget>();
container->set_margin(10);
container->set_spacing(10);

auto btn = std::make_unique<Button<AquaObject>>();
btn->set_text("Aceptar");
btn->set_accent_color(WidgetAccentColor::Primary);

container->add_child(std::move(btn));
notebook->add_tab(NotebookPage("Buttons", "", std::move(container)));

wnd->add_child(std::move(notebook));
app.set_root(std::move(wnd));

app.run();
```

## Event Handling

Todos los widgets en Horizon proporcionan un sistema de eventos flexible mediante propiedades del tipo `when_nombre_del_evento`. Cada evento puede tener múltiples callbacks conectados mediante la función `connect()`.

### Eventos disponibles en Widget

| Evento | Descripción | Contexto |
|--------|-------------|----------|
| `when_mouse_press` | Se dispara al presionar un botón del mouse | MouseButtonEventContext |
| `when_mouse_release` | Se dispara al soltar un botón del mouse | MouseButtonEventContext |
| `when_click` | Se dispara al hacer click (presionar y soltar) | MouseButtonEventContext |
| `when_dbl_click` | Se dispara al hacer doble click | MouseButtonEventContext |
| `when_mouse_move` | Se dispara al mover el mouse sobre el widget | MouseMoveEventContext |
| `when_mouse_enter` | Se dispara cuando el mouse entra al widget | EventContext |
| `when_mouse_leave` | Se dispara cuando el mouse sale del widget | EventContext |
| `when_mouse_drag` | Se dispara al arrastrar el mouse | MouseMoveEventContext |
| `when_mouse_hover` | Se dispara cuando el mouse está sobre el widget | MouseMoveEventContext |
| `when_mouse_wheel` | Se dispara al usar la rueda del mouse | MouseWheelEventContext |
| `when_key_press` | Se dispara al presionar una tecla | KeyEventContext |
| `when_key_release` | Se dispara al soltar una tecla | KeyEventContext |
| `when_focus` | Se dispara cuando el widget recibe foco | EventContext |
| `when_blur` | Se dispara cuando el widget pierde foco | EventContext |

### Ejemplos de manejo de eventos

```cpp
// Click en un botón
btn->when_click.connect([](horizon::MouseButtonEventContext &ev) {
    LOG_INFO << "¡Botón presionado!";
    ev.stop_propagation = true; // Detener propagación del evento
});

// Presión del mouse (diferente a click)
btn->when_mouse_press.connect([&app](horizon::MouseButtonEventContext &ev) {
    LOG_INFO << "Mouse presionado en botón";
    // Emitir señal global
    app.signal_manager.emit("on_exit", &app);
});

// Movimiento del mouse
widget->when_mouse_move.connect([](horizon::MouseMoveEventContext &ev) {
    LOG_INFO << "Mouse en posición: " << ev.x << ", " << ev.y;
});

// Teclas
textbox->when_key_press.connect([](horizon::KeyEventContext &ev) {
    if (ev.key == 65307) { // ESC
        LOG_INFO << "Tecla ESC presionada";
    }
});

// Cambio de texto en TextBox
textbox->when_text_changed.connect([](horizon::KeyEventContext &ev) {
    LOG_INFO << "El texto ha cambiado";
});

// Scroll en ScrollArea
scroll_area->when_scroll.connect([](horizon::EventContext &ev) {
    LOG_INFO << "Scrolling detectado";
});

// Doble click en TableView
table_view->when_row_dbl_click.connect([](auto &ctx) {
    LOG_INFO << "Fila seleccionada: " << ctx.row_index;
    LOG_INFO << "Datos: " << ctx.row_data.name;
});

// Selección en Sidebar
sidebar->when_item_selected.connect([](horizon::EventContext &ev) {
    LOG_INFO << "Item de sidebar seleccionado";
});
```

### Señales globales con SignalManager

Para comunicación entre widgets o con la aplicación:

```cpp
// Conectar a una señal global
app.signal_manager.connect("on_exit", [](horizon::SignalContext &ctx) {
    LOG_INFO << "Signal 'on_exit' recibida. Cerrando aplicación.";
    exit(0);
});

// Emitir una señal desde cualquier widget
app.signal_manager.emit("on_exit", &app);
```

### Contextos de eventos

Cada tipo de evento proporciona un contexto con información específica:

- **EventContext**: Contexto base con `stop_propagation`
- **MouseButtonEventContext**: `button`, `x`, `y`, `modifiers`
- **MouseMoveEventContext**: `x`, `y`, `dx`, `dy`
- **KeyEventContext**: `key`, `modifiers`
- **SignalContext**: Datos asociados a la señal

## Key Features

1. **Wayland Native**: Built specifically for Wayland compositors
2. **Template-based Styling**: Widgets use templates (AquaObject, SolidObject) for different visual styles
3. **Event System**: Flexible multi-callback event system with propagation control
4. **Theme Support**: Centralized theme management through ThemeManager
5. **Container Widgets**: Complex layouts with Notebook, VPanel, ScrollArea
6. **Table Views**: Generic template-based TableView for data display
7. **Signal System**: Inter-widget communication through SignalManager

## Dependencies

- Wayland protocols (xdg-shell, layer-shell)
- OpenGL ES 2.0 / EGL for rendering
- Cairo for some graphics operations
- Standard C++17 features
