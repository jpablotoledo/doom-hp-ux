#!/bin/sh
# Build script for Doom It Yourself on HP-UX B2000
# Compiles and creates /opt/doom-hpux with everything needed to run.
# Usage: sh doom_build.sh
#        at -f doom_build.sh now   (to run detached from terminal)
trap "" 1 2 15

SCRIPT_DIR=`cd \`dirname $0\` && pwd`
SRC="$SCRIPT_DIR/src"
DISTDIR=/opt/doom-hpux
LOG=/tmp/doom_build.log

> "$LOG"

log() {
    echo "$1"
    echo "$1" >> "$LOG"
}

log "=== Building Doom for HP-UX ==="
log "Sources: $SRC"

# Search for the WAD in the project directory or /tmp
WAD_SOURCE=""
for candidate in \
    "$SCRIPT_DIR/shareware/doom1.wad" \
    "$SCRIPT_DIR/doom1.wad" \
    "$SCRIPT_DIR/doom.wad" \
    "/tmp/doom1.wad" \
    "/tmp/doom.wad"; do
    if [ -f "$candidate" ]; then
        WAD_SOURCE="$candidate"
        break
    fi
done

if [ -z "$WAD_SOURCE" ]; then
    log "ERROR: No WAD file found."
    log "Place doom1.wad in the same directory as this script or in /tmp/"
    log "Obtain the Doom 1 shareware WAD (doom1.wad) and install it on this machine."
    echo 1 > /tmp/doom_build.exit
    exit 1
fi

log "WAD found: $WAD_SOURCE"

# Copy simpleAudio.c from the HP-UX installation (not distributed with the repo)
if [ ! -f "$SRC/simpleAudio.c" ]; then
    if [ -f /opt/audio/src/simpleAudio/simpleAudio.c ]; then
        cp /opt/audio/src/simpleAudio/simpleAudio.c "$SRC/simpleAudio.c"
        log "simpleAudio.c copied from /opt/audio/src/simpleAudio/"
    else
        log "ERROR: /opt/audio/src/simpleAudio/simpleAudio.c not found."
        log "Install the HP-UX audio package (AudioDevKit or similar)."
        echo 1 > /tmp/doom_build.exit
        exit 1
    fi
fi

# Compile
log ""
log "=== Cleaning previous objects ==="
cd "$SRC"
make clean_hp 2>&1 | tee -a "$LOG"

log ""
log "=== Compiling ==="
make hp8 2>&1 | tee -a "$LOG"

if [ ! -f "$SRC/hpdiy8" ]; then
    log "ERROR: Compilation failed. Check the log above."
    echo 1 > /tmp/doom_build.exit
    exit 1
fi

log ""
log "=== Compilation successful. Preparing distribution... ==="

# Create distribution directory
rm -rf "$DISTDIR"
mkdir -p "$DISTDIR"

# Binary
cp "$SRC/hpdiy8" "$DISTDIR/doom-hpux"
chmod 755 "$DISTDIR/doom-hpux"
log "Binary copied: $DISTDIR/doom-hpux"

# WAD file (full copy so the directory is self-contained)
cp "$WAD_SOURCE" "$DISTDIR/doom.wad"
log "WAD copied:    $WAD_SOURCE -> $DISTDIR/doom.wad"

# Empty config file (Doom will populate it with defaults on first exit)
touch "$DISTDIR/doom.cfg"

# Launch scripts. doom.sh is the default: music + sound effects.
# The others pass -nomusic/-nosound straight through to the binary; on
# HP-UX -nosound also skips the SIGALRM audio timer and all per-frame
# mixing work entirely (not just muting output), for better performance
# on this machine when audio isn't needed. See docs/05-investigacion-musica.md.
cat > "$DISTDIR/doom.sh" << 'RUNEOF'
#!/bin/sh
# Doom launcher for HP-UX - music + sound effects (default)
DOOM_DIR=`dirname $0`
cd "$DOOM_DIR"
DOOMWADDIR="$DOOM_DIR" DISPLAY="${DISPLAY:-:0.0}" ./doom-hpux "$@"
RUNEOF
chmod 755 "$DISTDIR/doom.sh"

cat > "$DISTDIR/doom-hpux-nomusic.sh" << 'RUNEOF'
#!/bin/sh
# Doom launcher for HP-UX - sound effects only, no music
DOOM_DIR=`dirname $0`
cd "$DOOM_DIR"
DOOMWADDIR="$DOOM_DIR" DISPLAY="${DISPLAY:-:0.0}" ./doom-hpux -nomusic "$@"
RUNEOF
chmod 755 "$DISTDIR/doom-hpux-nomusic.sh"

cat > "$DISTDIR/doom-hpux-nomusic-nofx.sh" << 'RUNEOF'
#!/bin/sh
# Doom launcher for HP-UX - no music, no sound effects (best performance)
DOOM_DIR=`dirname $0`
cd "$DOOM_DIR"
DOOMWADDIR="$DOOM_DIR" DISPLAY="${DISPLAY:-:0.0}" ./doom-hpux -nomusic -nosound "$@"
RUNEOF
chmod 755 "$DISTDIR/doom-hpux-nomusic-nofx.sh"

cat > "$DISTDIR/doom-hpux-nosound.sh" << 'RUNEOF'
#!/bin/sh
# Doom launcher for HP-UX - no audio at all (best performance)
DOOM_DIR=`dirname $0`
cd "$DOOM_DIR"
DOOMWADDIR="$DOOM_DIR" DISPLAY="${DISPLAY:-:0.0}" ./doom-hpux -nosound "$@"
RUNEOF
chmod 755 "$DISTDIR/doom-hpux-nosound.sh"

log ""
log "=== Distribution ready at: $DISTDIR ==="
ls -la "$DISTDIR" 2>&1 | tee -a "$LOG"
log ""
log "To play, run from the CDE terminal:"
log "  $DISTDIR/doom.sh                    (music + sound effects, default)"
log "  $DISTDIR/doom-hpux-nomusic.sh        (sound effects only)"
log "  $DISTDIR/doom-hpux-nomusic-nofx.sh   (no audio, best performance)"
log "  $DISTDIR/doom-hpux-nosound.sh        (no audio, best performance)"

echo 0 > /tmp/doom_build.exit
