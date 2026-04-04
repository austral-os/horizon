#!/bin/bash
export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP=HZN-LABWC

labwc &
export LABWC_PID=$!
/home/horacio/Desarrollo/austral-os/horizon/build/horizon_session --compositor labwc