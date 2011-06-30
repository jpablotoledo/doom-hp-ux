#!/bin/sh
# Build script para Doom It Yourself en HP-UX B2000
# Compila y genera el directorio /opt/doom-hpux con todo lo necesario.
# Uso: at -f doom_build.sh now
trap "" 1 2 15

SCRIPT_DIR=`cd \`dirname $0\` && pwd`
SRC="$SCRIPT_DIR/src"
DISTDIR=/opt/doom-hpux
LOG=/tmp/doom_build.log

echo "=== Compilando Doom para HP-UX ===" > "$LOG"
echo "Fuentes: $SRC" >> "$LOG"

# Buscar el WAD en el directorio del proyecto o en /tmp
WAD_SOURCE=""
for candidate in \
    "$SCRIPT_DIR/freedoom1.wad" \
    "$SCRIPT_DIR/doom.wad" \
    "/tmp/freedoom1.wad" \
    "/tmp/doom.wad"; do
    if [ -f "$candidate" ]; then
        WAD_SOURCE="$candidate"
        break
    fi
done

if [ -z "$WAD_SOURCE" ]; then
    echo "ERROR: No se encontro ningun archivo WAD." >> "$LOG"
    echo "Coloque freedoom1.wad en el mismo directorio que este script o en /tmp/" >> "$LOG"
    echo "Descargue Freedoom Phase 1 desde: https://freedoom.github.io" >> "$LOG"
    echo 1 > /tmp/doom_build.exit
    exit 1
fi

echo "WAD encontrado: $WAD_SOURCE" >> "$LOG"

# Copiar simpleAudio.c desde la instalacion de HP-UX (no se distribuye con el repo)
if [ ! -f "$SRC/simpleAudio.c" ]; then
    if [ -f /opt/audio/src/simpleAudio/simpleAudio.c ]; then
        cp /opt/audio/src/simpleAudio/simpleAudio.c "$SRC/simpleAudio.c"
        echo "simpleAudio.c copiado desde /opt/audio/src/simpleAudio/" >> "$LOG"
    else
        echo "ERROR: No se encontro /opt/audio/src/simpleAudio/simpleAudio.c" >> "$LOG"
        echo "Instale el paquete de audio de HP-UX (AudioDevKit o similar)" >> "$LOG"
        echo 1 > /tmp/doom_build.exit
        exit 1
    fi
fi

# Compilar
cd "$SRC"
make clean_hp >> "$LOG" 2>&1
make hp8 >> "$LOG" 2>&1
BUILD_RESULT=$?

if [ $BUILD_RESULT -ne 0 ]; then
    echo "ERROR: La compilacion fallo. Revise $LOG" >> "$LOG"
    echo $BUILD_RESULT > /tmp/doom_build.exit
    exit $BUILD_RESULT
fi

echo "Compilacion exitosa. Preparando distribucion..." >> "$LOG"

# Crear directorio de distribucion
rm -rf "$DISTDIR"
mkdir -p "$DISTDIR"

# Binario con nombre definitivo
cp "$SRC/hpdiy8" "$DISTDIR/doom-hpux"
chmod 755 "$DISTDIR/doom-hpux"

# WAD file (copia completa para que el directorio sea autosuficiente)
cp "$WAD_SOURCE" "$DISTDIR/doom.wad"
echo "WAD copiado: $WAD_SOURCE -> $DISTDIR/doom.wad" >> "$LOG"

# Config file vacio (Doom lo populara con valores por defecto al primer cierre)
touch "$DISTDIR/doom.cfg"

# Script de lanzamiento
cat > "$DISTDIR/doom.sh" << 'RUNEOF'
#!/bin/sh
# Lanzador de Doom para HP-UX
DOOM_DIR=`dirname $0`
cd "$DOOM_DIR"
DOOMWADDIR="$DOOM_DIR" DISPLAY="${DISPLAY:-:0.0}" ./doom-hpux "$@"
RUNEOF
chmod 755 "$DISTDIR/doom.sh"

# Resumen
echo "" >> "$LOG"
echo "=== Distribucion lista en: $DISTDIR ===" >> "$LOG"
ls -la "$DISTDIR" >> "$LOG"
echo "" >> "$LOG"
echo "Para jugar, ejecutar desde la terminal CDE:" >> "$LOG"
echo "  $DISTDIR/doom.sh" >> "$LOG"

echo 0 > /tmp/doom_build.exit
