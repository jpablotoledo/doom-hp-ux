#!/bin/sh
# Doom launcher for HP-UX - music + sound effects (default)
DOOM_DIR=`dirname $0`
cd "$DOOM_DIR"
DOOMWADDIR="$DOOM_DIR" DISPLAY="${DISPLAY:-:0.0}" ./doom-hpux "$@"
