#!/usr/bin/env bash
set -euo pipefail

# runtime dir (recreated on boot)
mkdir -p /run/user/1001
chmod 0700 /run/user/1001

export XDG_RUNTIME_DIR=/run/user/1001
export WAYLAND_DISPLAY=wayland-1
export WLR_BACKENDS=headless
export WLR_HEADLESS_OUTPUTS=1
export WLR_LIBINPUT_NO_DEVICES=1
export WLR_RENDERER=pixman
export QT_QPA_PLATFORM=wayland
export QT_OPENGL=software
export QT_QUICK_BACKEND=software
export LIBGL_ALWAYS_SOFTWARE=1

# start compositor + vnc
pkill -x sway 2>/dev/null || true
pkill -x wayvnc 2>/dev/null || true
sway -d 2> ~/sway.log &
sleep 1
wayvnc 0.0.0.0 5900 &
