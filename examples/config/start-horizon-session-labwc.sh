#!/bin/bash
export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP=HZN-LABWC

labwc &
export LABWC_PID=$!
/home/horacio/Desarrollo/austral-os/horizon/build/apps/horizon_session/horizon_session --compositor labwc