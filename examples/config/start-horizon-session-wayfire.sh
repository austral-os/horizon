#!/bin/bash
export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP=HZN-WAYFIRE

wayfire &
export WAYFIRE_PID=$!
/home/horacio/Desarrollo/austral-os/horizon/build/apps/horizon_session/horizon_session --compositor wayfire