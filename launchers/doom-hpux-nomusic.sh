#!/bin/sh
# Doom launcher for HP-UX - sound effects only, no music
DOOM_DIR=`dirname $0`
cd "$DOOM_DIR"
DOOMWADDIR="$DOOM_DIR" DISPLAY="${DISPLAY:-:0.0}" ./doom-hpux -nomusic "$@"
