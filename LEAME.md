# Doom para HP-UX - HP Visualize B2000

Port de **Doom It Yourself (DIY) v4.4.2** compilado y ejecutándose en una HP Visualize B2000 con HP-UX 11.00 PA-RISC.

## Hardware y sistema

| | |
|---|---|
| Máquina | HP Visualize B2000 |
| Arquitectura | PA-RISC 2.0 (9000/785, Big-Endian) |
| Sistema operativo | HP-UX B.11.00 |
| Compilador | HP C Compiler A.11.01.00 |
| Video | X11 con extensión MIT-SHM |
| Audio | HP Alib / simpleAudio - 16-bit linear stereo via `Aserver` |
| Música | Sintetizador OPL2/GENMIDI en proceso (`hp_music.c`), alimentado por un timer real `SIGALRM` |
| Flags de compilador | `+O3 +Onolimit +DA2.0 +DS2.0 +Ofastaccess` (mejora real confirmada en el hardware) |

## Fuente base

[Doom It Yourself (DIY) v4.4.2](https://zarquon.hier-im-netz.de/Programs/DIYSource.zip) - port multiplataforma de LinuxDoom 1.10 con soporte HP-UX nativo incluido.

## Cambios al código fuente

**`src/Makefile` - Rutas X11**
Las rutas originales apuntaban a `/usr/local/DIR/X11/R6.1/` (inexistente). Corregido a `/usr/include` y `/usr/lib/X11R6`, con `-L/usr/contrib/X11R6/lib` adicional para `libXmu`.

**`src/Makefile` - Compatibilidad HP make**
`make -C $(FASTLZDIR)` no es soportado por HP make. Reemplazado por `cd $(FASTLZDIR) && make CC=$(CC) ...`.

**`src/d_main.c` - Config file junto al ejecutable**
Si `DOOMWADDIR` está definido, el archivo de configuración se guarda como `doom.cfg` dentro de ese mismo directorio, en lugar de `$HOME/.doomrc`.

**`src/i_sound.c` + `src/simpleAudio.h` - Sonido via HP Alib**
DIY Doom compila para HP-UX con `-DDOOM_NO_SFX` (sin sonido). Se implementó soporte
de audio usando `simpleAudio`, wrapper de HP sobre Alib. `openAStream()` devuelve un
socket fd conectado al `Aserver` al que se escribe PCM 16-bit stereo, igual que
`/dev/dsp` en Linux. `simpleAudio.c` no está en el repo (pertenece a HP) y se copia
desde `/opt/audio/src/simpleAudio/` durante el build.

El path síncrono de HP-UX (`I_SubmitSound`) llama al audio 35 veces/seg mientras la
frecuencia de reproducción es 11025 Hz, lo que provoca lag creciente sin throttling.
Se implementó control de tiempo con `gettimeofday()` para saltar escrituras cuando el
buffer supera 2 frames de adelanto (~93 ms de lag fijo).

**`src/Makefile` - Optimización y límites de pantalla**
Optimización subida de `-O` a `+O2 +Onolimit`. `MAXSCREENWIDTH` subido de 1024 a 1280
y `MAXSCREENHEIGHT` de 768 a 800 para soportar el modo `-4` (1280×800) sin desbordamiento
de los arrays del renderer de sprites.

**`src/hp_music.c` - Sintetizador de música en proceso**
La música originalmente no estaba implementada (`MUSSERV` nunca se definía para
ninguna plataforma). Se construyó un sintetizador FM OPL2/GENMIDI + parser MUS
en el mismo proceso, alimentado por un timer real `SIGALRM`/`setitimer`
independiente del bucle de render - sin proceso externo, sin pipe. En el camino
se encontraron y corrigieron varios bugs reales de síntesis: mapeo incorrecto de
instrumentos de percusión MUS, fórmula de feedback OPL2 equivocada, aliasing por
encima de Nyquist en instrumentos de percusión de alta frecuencia, corte abrupto
de envolvente en una sola muestra al hacer release/decay, sesgo DC sin filtrar, y
un bug de mezcla con divisor dinámico que causaba "bombeo" de volumen audible en
cada nota.

**`src/Makefile` - Flags de optimización del compilador**
`+O2` subido a `+O3 +Onolimit +DA2.0 +DS2.0 +Ofastaccess` (código específico para
la arquitectura destino, scheduling de instrucciones afinado a este modelo de
CPU, y acceso más rápido a datos globales/estáticos) - confirmado como mejora
real y notoria jugando en el hardware real.

Detalles completos en [docs/02-configuracion-maquina.md](docs/02-configuracion-maquina.md), [docs/04-agregar-sonido.md](docs/04-agregar-sonido.md), [docs/05-investigacion-musica.md](docs/05-investigacion-musica.md) y [docs/06-investigacion-rendimiento-cpu.md](docs/06-investigacion-rendimiento-cpu.md).

## WAD

Se usa el **shareware de Doom 1** (`doom1.wad`). Incluido en el directorio de distribución como `doom.wad`.

## Compilar y distribuir

### WAD file

El WAD del **shareware de Doom 1** (`shareware/doom1.wad`) está incluido en el repositorio -
la versión shareware original de id Software, de distribución libre. No hace falta descargarlo.

### Compilar en HP-UX

El script [`doom_build.sh`](doom_build.sh) compila el binario y genera `/opt/doom-hpux/` con todo lo necesario. Busca el WAD automáticamente en el directorio del proyecto o en `/tmp/`.

**Transferir al HP-UX** (desde Linux/otro Unix):

```sh
tar czf /tmp/doom-hpux-project.tar.gz src/ doom_build.sh shareware/
ftp -n <ip-del-hpux> <<EOF
user root <password>
binary
put /tmp/doom-hpux-project.tar.gz /tmp/doom-hpux-project.tar.gz
quit
EOF
```

**En el HP-UX**, descomprimir y ejecutar el build:

```sh
# HP-UX tar no soporta -z, descomprimir en dos pasos
cd /tmp && gunzip doom-hpux-project.tar.gz && tar xf doom-hpux-project.tar

sh /tmp/doom-hpux/doom_build.sh
```

El script muestra la salida del compilador en tiempo real. Si se necesita ejecutar
desconectado (p.ej. sobre telnet), usar `at` y monitorear el log:

```sh
at -f /tmp/doom-hpux/doom_build.sh now
tail -f /tmp/doom_build.log
```

## Resultado

```
/opt/doom-hpux/
├── doom-hpux                    ← binario (~750 KB)
├── doom.wad                     ← shareware Doom 1 (~4 MB)
├── doom.cfg                     ← configuración (se crea al salir del juego)
├── doom.sh                      ← lanzador: música + efectos de sonido (por defecto)
├── doom-hpux-nomusic.sh         ← lanzador: solo efectos de sonido
├── doom-hpux-nomusic-nofx.sh    ← lanzador: sin audio (mejor rendimiento)
└── doom-hpux-nosound.sh         ← lanzador: sin audio (mejor rendimiento)
```

`-nosound` se salta todo el trabajo de mezcla de audio en tiempo de
ejecución (no solo silencia la salida), para el mejor rendimiento posible
cuando no se necesita audio.

## Ejecutar

Desde la terminal de CDE en el B2000:

```sh
/opt/doom-hpux/doom.sh
```

## Documentación

Listados en el orden en que se escribieron, así también se leen como una
línea de tiempo de cómo creció el proyecto. Ver
[docs/00-indice.md](docs/00-indice.md) para el mismo listado con fechas y
un resumen de una línea.

1. Este archivo / [README.md](README.md) - resumen del proyecto
2. [docs/02-configuracion-maquina.md](docs/02-configuracion-maquina.md) *(histórico)* - Estado de la máquina, pasos de compilación, problemas encontrados y soluciones
3. [docs/03-cambios-codigo.md](docs/03-cambios-codigo.md) *(histórico)* - Cambios al código fuente con diffs y justificación técnica
4. [docs/04-agregar-sonido.md](docs/04-agregar-sonido.md) *(histórico)* - Implementación de sonido via HP Alib: cambios, problemas y soluciones
5. [docs/05-investigacion-musica.md](docs/05-investigacion-musica.md) - Investigación completa de música: arquitectura, bugs encontrados y corregidos, estado final
6. [docs/06-investigacion-rendimiento-cpu.md](docs/06-investigacion-rendimiento-cpu.md) - Investigación y resultados de optimización de CPU/compilador
7. [docs/07-auditoria-commits.md](docs/07-auditoria-commits.md) - Auditoría commit por commit de qué fue necesario y qué quedó superado

Los docs *(histórico)* describen el estado inicial del proyecto en 2011 y
se mantienen sin editar por precisión histórica; ya no reflejan el árbol
de fuentes actual. Ver la nota al inicio de cada uno.
