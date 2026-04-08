# Decoupled & State-Atomic Clipboard Architecture (Final Plan)

This plan finalizes the Horizon clipboard system by enforcing the **Single Writer Rule**: the `ClipboardBackend` is the exclusive owner and mutator of the clipboard state. This prevents architectural desyncs and ensures that the system always respects the underlying protocol's constraints.

## User Review Required

> [!IMPORTANT]
> - **Single Writer Rule**: ONLY the `ClipboardBackend` (e.g., `WaylandClipboardBackend`) is allowed to mutate the `SelectionState` and the `Generation ID`. All other components (`WaylandWindow`, `Widget`) only request actions or observe the state.
> - **Protocol Supremacy**: By centralizing state in the backend, we ensure that transitions only happen when the protocol (Wayland) confirms them, preventing re-entrancy bugs.
> - **Opaque Identity**: `WaylandWindow` will hold a read-only view of the current selection identity to coordinate UI updates, but it cannot modify it directly.

## Proposed Changes

---

### [Component] Horizon Core - State & Integrity

#### [MODIFY] [ClipboardState.hpp](file:///home/horacio/Desarrollo/austral-os/horizon/include/horizon/ClipboardState.hpp)
- **SelectionState Enum**: (IDLE, LOCAL_OWNER, REMOTE_OFFER, TRANSFERRING).
- **SelectionIdentity**: generation_id + provider_ptr.

---

### [Component] Horizon Core - Clipboard Backend (The Single Writer)

#### [MODIFY] [ClipboardBackend.hpp](file:///home/horacio/Desarrollo/austral-os/horizon/include/horizon/ClipboardBackend.hpp)
- **State Ownership**: The backend stores the `SelectionState` and `Generation ID`.
- **Primary Interface**:
  - `virtual void set_provider(ClipboardProvider* provider, const std::vector<std::string>& mime_types) = 0;`
  - `virtual void request_data(const std::string& mime, DataSink& sink) = 0;`
  - `virtual SelectionState get_state() const = 0;`
  - `virtual uint64_t get_current_generation() const = 0;`
- **Signals**:
  - `when_state_changed`: Emitted when the backend successfully transitions between states.
  - `when_selection_cancelled`: Emitted when the system loses the selection.

---

### [Component] Horizon Core - Window Management (WaylandWindow)

#### [MODIFY] [WaylandWindow.cpp](file:///home/horacio/Desarrollo/austral-os/horizon/src/core/WaylandWindow.cpp)
- **Role: Coordinator**: 
  - `WaylandWindow` no longer manages the clipboard state. 
  - It listens to `m_clipboard_backend->when_state_changed` to update Global Menu and refresh UI labels.
  - It proxies `Ctrl+C/V/X` to the backend.
- **Lifetime Safety**: 
  - On `unregister_widget(Widget*)`, if the widget is the current provider stored in the backend, `WaylandWindow` tells the backend: `invalidate_provider()`.

---

### [Component] Horizon Core - Implementation (Wayland)

#### [MODIFY] [WaylandClipboardBackend.cpp](file:///home/horacio/Desarrollo/austral-os/horizon/src/core/WaylandClipboardBackend.cpp)
- **State Transitions**:
  - Transition to `LOCAL_OWNER` when `wl_data_device.set_selection` is called.
  - Transition to `REMOTE_OFFER` when `data_device_handle_selection` is received from the compositor.
  - Increment `generation_id` internally on every transition.

---

## Open Questions

> [!NOTE]
> 1. **Persistence (Reminder)**: Persistence remains an "Optional Future" extension to be handled via `wlr-data-control` if needed.

## Verification Plan

### Integrity Tests
- **State Violation Test**: Attempt to manually change `m_focused` data from a background thread. Verify that since it doesn't go through the `ClipboardBackend`'s logic, the system remains in a safe state.
- **Protocol Re-entrancy Test**: Rapidly emit Wayland `selection` and `cancelled` events. Verify that the `ClipboardBackend` serializes these transitions correctly.

### Manual Verification
- Verify that `Ctrl+C` and Context Menu "Copiar" both trigger the same state transition in the backend.
- Ensure that if the backend transitions to `IDLE` (selection lost), all UI indicators (Global Menu items) update immediately via signal.
