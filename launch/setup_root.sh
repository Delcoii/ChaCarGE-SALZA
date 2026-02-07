#!/usr/bin/env bash
set -euo pipefail

# stop weston
systemctl stop weston || true
systemctl stop weston.socket || true

# seat group (idempotent)
grep '^seat:' /etc/group || echo "seat:x:1002:" >> /etc/group
usermod -aG seat topst

# ipc group (idempotent)
groupadd ipc 2>/dev/null || true
usermod -aG ipc topst

# device permissions (must be done every boot)
if [ -e /dev/tcc_ipc_micom ]; then
  chgrp ipc /dev/tcc_ipc_micom
  chmod 660 /dev/tcc_ipc_micom
fi

# seatd daemon
pkill -x seatd 2>/dev/null || true
/usr/bin/seatd -g seat &
