# Configuración de la máquina HP Visualize B2000

## Datos del sistema

| Parámetro         | Valor                                          |
|-------------------|------------------------------------------------|
| Hardware          | HP Visualize B2000                             |
| Arquitectura      | PA-RISC 2.0 (chip 9000/785, Big-Endian)        |
| Sistema operativo | HP-UX B.11.00 (hostname: tmp2-80)              |
| IP de acceso      | 192.168.1.37 (telnet, usuario root)            |
| Objetivo          | Compilar y ejecutar Doom It Yourself 4.4.2     |
| Resultado         | **EXITOSO** - Doom corriendo al 85% CPU        |

---

## Estado inicial de la máquina (auditoría)

### Compilador

```
what /usr/bin/cc
  HP92453-01 A.11.01.00 HP C Compiler
  PATCH/11.00:PHCO_95167  Oct  1 1998
```

- **Compilador nativo HP C** versión A.11.01.00 disponible en `/usr/bin/cc`
- **GCC no instalado** (`sh: gcc: not found`)

### X11

| Ruta                        | Contenido                            |
|-----------------------------|--------------------------------------|
| `/usr/include/X11/`         | Headers X11 (Xlib.h, etc.)           |
| `/usr/include/X11R6/X11/`  | Headers X11R6 (alternativo)          |
| `/usr/lib/X11R6/`           | libX11, libXext, libICE, libSM      |
| `/usr/contrib/X11R6/lib/`  | **libXmu** (NO estaba en X11R6/)     |
| `/usr/lib/X11R4/`           | libXmu.sl (versión vieja)            |

**Punto clave:** `libXmu` no estaba en `/usr/lib/X11R6/` sino en `/usr/contrib/X11R6/lib/`. El Makefile original apuntaba a una ruta inexistente.

### Herramientas

| Herramienta | Estado         | Observación                              |
|-------------|----------------|------------------------------------------|
| `make`      | `/usr/bin/make`| HP make (no GNU make; no soporta `--version` ni `-C`) |
| `tar`       | `/usr/bin/tar` | HP tar (no soporta flag `-z` para gzip) |
| `gunzip`    | disponible     | Necesario para descomprimir .tar.gz      |
| `ftp`       | `/usr/bin/ftp` | Usado para transferencia de archivos     |
| `at`        | disponible     | Usado para ejecutar builds sin terminal  |

### Espacio en disco

| Filesystem | Total  | Libre  | Montaje  |
|------------|--------|--------|----------|
| /          | 248MB  | 118MB  | lvol3    |
| /tmp       | 480MB  | 478MB  | lvol6    |
| /diska     | 7.8GB  | 6.5GB  | vg01     |

### Software instalado relevante

| Bundle              | Descripción                                         |
|---------------------|-----------------------------------------------------|
| B3899BA B.11.01.07  | HP C/ANSI C Developer's Bundle para HP-UX 11.00 ✓ |
| FIREFOX 2.0.0.2     | Firefox para HP-UX                                  |
| /root/games/        | Colección de juegos en depot (.depot)               |

---

## Pasos realizados en la máquina

### 1. Transferencia del código fuente

El código fue preparado en la máquina Linux (fuentes convertidos a Unix con `install.sh`)
y transferido al HP-UX con FTP:

```sh
# En Linux:
tar czf /tmp/doom-src.tar.gz src/
ftp -n 192.168.1.37 <<EOF
user root hp2000
binary
put /tmp/doom-src.tar.gz /tmp/doom-src.tar.gz
quit
EOF

# En HP-UX:
cd /tmp && gunzip doom-src.tar.gz && tar xf doom-src.tar
```

**Nota:** HP-UX `tar` no soporta `-z`. Hay que descomprimir primero con `gunzip`.

### 2. Compilación

El build fue enviado al demonio `at` para aislarlo de la sesión telnet y evitar
que SIGHUP interrumpa el compilador al cerrar sesión:

```sh
# Script /tmp/doom_build.sh:
#!/bin/sh
trap "" 1 2 15
cd /tmp/src
make clean_hp 2>/dev/null
make hp8 > /tmp/doom_build.log 2>&1
echo $? > /tmp/doom_build.exit
```

```sh
at -f /tmp/doom_build.sh now
```

**Problema con nohup:** los intentos anteriores con `nohup ... &` fallaban porque
el compilador interno `ccom` recibía SIGHUP del proceso group al cerrar sesión,
incluso con `nohup`. La solución fue usar `at` (corre bajo `atd`, sin terminal).

**Resultado de compilación:**
```
exit code: 0
binario:   /tmp/src/hpdiy8 (663.552 bytes)
```

### 3. Obtención del WAD (archivo de datos del juego)

Doom requiere un archivo WAD con todos los datos del juego. No había ninguno
en la máquina. Se descargó **Freedoom Phase 1** (WAD libre y open-source):

```sh
# En Linux:
wget https://github.com/freedoom/freedoom/releases/download/v0.13.0/freedoom-0.13.0.zip
unzip -p freedoom-0.13.0.zip "*/freedoom1.wad" > /tmp/freedoom1.wad

# Transferencia al HP-UX (28MB):
ftp -n 192.168.1.37 <<EOF
user root hp2000
binary
put /tmp/freedoom1.wad /tmp/freedoom1.wad
quit
EOF
```

### 4. Ejecución

DIY Doom no acepta rutas completas en `-iwad`. Busca el WAD por su nombre estándar
(`doom.wad`, `doom2.wad`, etc.) en el directorio indicado por `DOOMWADDIR`.
Se creó un symlink con el nombre esperado:

```sh
# En HP-UX:
ln -s /tmp/freedoom1.wad /tmp/doom.wad
cd /tmp/src
DOOMWADDIR=/tmp DISPLAY=:0.0 ./hpdiy8
```

**Salida de inicio exitosa:**
```
DOOM Registered Startup v1.11
V_Init: Allocated 4 screens.
M_LoadDefaults: Load system defaults.
Z_Init: Init zone memory allocation daemon.
I_ZoneBase: Starting with 32768k memory.
W_Init: Init WADfiles.
 adding /tmp/doom.wad
 -->  E1M1-E4M9
M_Init: Init miscellaneous info.
R_Init: Init DOOM refresh daemon - ...
P_Init: Init Playloop state.
I_Init: Setting up machine state.
D_CheckNetGame: Checking network game status.
S_Init: Setting up sound.
HU_Init: Setting up heads up display.
ST_Init: Init status bar.
Using MITSHM extension
shared memory id=8197, addr=0xc0d5b000
```

El proceso corre al **85% de CPU** renderizando el juego.

---

## Distribución final en la máquina

El build script genera el directorio autosuficiente `/opt/doom-hpux/`:

```
/opt/doom-hpux/
├── doom-hpux     663.552 bytes  ← binario
├── doom.wad   28.795.076 bytes  ← Freedoom Phase 1 (copia completa)
├── doom.cfg            0 bytes  ← config (Doom escribe aquí al salir)
└── doom.sh           144 bytes  ← script de lanzamiento
```

**Para iniciar Doom desde la terminal CDE del B2000:**
```sh
/opt/doom-hpux/doom.sh
```

El script `doom.sh` se encarga de configurar `DOOMWADDIR` y `DISPLAY` automáticamente:
```sh
#!/bin/sh
DOOM_DIR=`dirname $0`
cd "$DOOM_DIR"
DOOMWADDIR="$DOOM_DIR" DISPLAY="${DISPLAY:-:0.0}" ./doom-hpux "$@"
```

Para **regenerar la distribución completa** desde cero (por ejemplo después de un reboot):
```sh
# 1. Copiar el script a la máquina (desde Linux):
ftp -n 192.168.1.37 <<EOF
user root hp2000
binary
put /tmp/doom_build.sh /tmp/doom_build.sh
quit
EOF

# 2. En HP-UX:
chmod +x /tmp/doom_build.sh
at -f /tmp/doom_build.sh now

# 3. Monitorear:
tail -f /tmp/doom_build.log
cat /tmp/doom_build.exit   # 0 = éxito
```

---

## Problemas encontrados y soluciones

### P1: `tar xzf` no funciona en HP-UX
**Causa:** HP-UX tar no tiene la opción `-z` para descompresión gzip.  
**Solución:** `gunzip archivo.tar.gz && tar xf archivo.tar`

### P2: `make -C dir` no es compatible con HP make
**Causa:** HP make no soporta la opción `-C` (extensión de GNU make).  
**Solución:** Cambiar en el Makefile a `cd dir && make` (ver docs/source-changes.md)

### P3: Compilador `ccom` muere por SIGHUP al cerrar sesión telnet
**Causa:** Al cerrar la sesión ksh, el shell envía SIGHUP al process group.
El subproceso `ccom` del compilador HP no ignora SIGHUP aunque el padre use `nohup`.  
**Solución:** Usar el comando `at` para ejecutar el build, que corre bajo `atd`
sin terminal y nunca recibe SIGHUP.

### P4: `-iwad /ruta/completa.wad` no funciona en DIY Doom
**Causa:** `IdentifyVersionByName()` espera un nombre corto como "doom" o "doom2",
no una ruta. Si el nombre no está en su lista interna, ignora el argumento y
llama a `IdentifyVersion()`, que busca WADs estándar en `DOOMWADDIR`.  
**Solución:** Crear symlink con nombre estándar + variable `DOOMWADDIR`.

### P5: No hay WAD comercial disponible
**Solución:** Usar Freedoom Phase 1 (freedoom1.wad), WAD libre compatible con
el formato IWAD de Doom 1. Descargado de GitHub releases.

### P6: libXmu no en `/usr/lib/X11R6/`
**Causa:** En este sistema, `libXmu` está en `/usr/contrib/X11R6/lib/`.  
**Solución:** Agregar `-L/usr/contrib/X11R6/lib` a `LDFLAGS` en el Makefile.
