#!/bin/sh
# Doom launcher for HP-UX - no audio at all (best performance)
DOOM_DIR=`dirname $0`
cd "$DOOM_DIR"
DOOMWADDIR="$DOOM_DIR" DISPLAY="${DISPLAY:-:0.0}" ./doom-hpux -nosound "$@"
