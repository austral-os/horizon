#!/bin/bash
export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP=HZN-WAYFIRE

# Forzar a GTK, Qt y Electron a usar los portales XDG para los diálogos
export GTK_USE_PORTAL=1
export QT_QPA_PLATFORMTHEME=xdgdesktopportal

wayfire &
export WAYFIRE_PID=$!

# Esperar a que el socket del compositor esté disponible y exportar WAYLAND_DISPLAY
for i in {1..50}; do
    for socket in "$XDG_RUNTIME_DIR"/wayland-[0-9]; do
        if [ -S "$socket" ]; then
            export WAYLAND_DISPLAY=$(basename "$socket")
            break 2
        fi
    done
    sleep 0.1
done

# Esperar a que el socket de XWayland esté disponible y exportar DISPLAY
for i in {1..50}; do
    for socket in /tmp/.X11-unix/X[0-9]*; do
        if [ -S "$socket" ] && [ -O "$socket" ]; then
            export DISPLAY=":${socket#/tmp/.X11-unix/X}"
            break 2
        fi
    done
    sleep 0.1
done

/home/horacio/Desarrollo/austral-os/horizon/build/apps/horizon_session/horizon_session --compositor wayfire