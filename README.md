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

Detalles completos en [docs/source-changes.md](docs/source-changes.md) y [docs/add-sound.md](docs/add-sound.md).

## WAD

Se usa **Freedoom Phase 1** (open-source, reemplaza Doom 1). Incluido en el directorio de distribución como `doom.wad`.

## Compilar y distribuir

### Requisito previo: WAD file

El juego necesita un archivo WAD con los datos. No está incluido en el repositorio.
Descargue **Freedoom Phase 1** (open-source, gratuito):

```sh
wget https://github.com/freedoom/freedoom/releases/download/v0.13.0/freedoom-0.13.0.zip
unzip -p freedoom-0.13.0.zip "*/freedoom1.wad" > freedoom1.wad
```

Coloque `freedoom1.wad` en la raíz del proyecto (junto a `doom_build.sh`).

### Compilar en HP-UX

El script [`doom_build.sh`](doom_build.sh) compila el binario y genera `/opt/doom-hpux/` con todo lo necesario. Busca el WAD automáticamente en el directorio del proyecto o en `/tmp/`.

**Transferir al HP-UX** (desde Linux/otro Unix):

```sh
tar czf /tmp/doom-hpux-project.tar.gz src/ doom_build.sh freedoom1.wad
ftp -n <ip-del-hpux> <<EOF
user root <password>
binary
put /tmp/doom-hpux-project.tar.gz /tmp/doom-hpux-project.tar.gz
quit
EOF
```

**En el HP-UX**, descomprimir y lanzar la compilación con `at`
(`at` es necesario para sobrevivir el cierre de la sesión telnet/ssh):

```sh
# HP-UX tar no soporta -z, descomprimir en dos pasos
cd /tmp && gunzip doom-hpux-project.tar.gz && tar xf doom-hpux-project.tar

chmod +x /tmp/doom_build.sh
at -f /tmp/doom_build.sh now
```

Monitorear el progreso:

```sh
tail -f /tmp/doom_build.log
cat /tmp/doom_build.exit   # 0 = éxito
```

## Resultado

```
/opt/doom-hpux/
├── doom-hpux   ← binario (663 KB)
├── doom.wad    ← Freedoom Phase 1 (28 MB)
├── doom.cfg    ← configuración (se crea al salir del juego)
└── doom.sh     ← script de lanzamiento
```

## Ejecutar

Desde la terminal de CDE en el B2000:

```sh
/opt/doom-hpux/doom.sh
```

## Documentación

- [docs/machine-setup.md](docs/machine-setup.md) - Estado de la máquina, pasos de compilación, problemas encontrados y soluciones
- [docs/source-changes.md](docs/source-changes.md) - Cambios al código fuente con diffs y justificación técnica
- [docs/add-sound.md](docs/add-sound.md) - Implementación de sonido via HP Alib: cambios, problemas y soluciones
