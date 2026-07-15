> **Instantánea histórica (junio 2011).** Cubre los cambios necesarios
> para el primer build funcional (rutas X11, compatibilidad con HP make,
> ubicación del config file). Escrito antes de que existiera audio,
> música o el trabajo de rendimiento/compilador - no refleja los flags
> actuales de `src/Makefile` ni el resto del árbol de fuentes. Ver
> [`docs/00-indice.md`](00-indice.md) para el listado cronológico
> completo de documentos, incluyendo los que superaron este contenido.

# Cambios al código fuente de Doom It Yourself para HP-UX

## Código base

**Proyecto:** Doom It Yourself (DIY) v4.4.2  
**Fuente original:** https://zarquon.hier-im-netz.de/Programs/DIYSource.zip  
**Base:** LinuxDoom 1.10 (id Software, licencia GPL)  
**Archivo original:** `DIYSource.zip` (759.197 bytes, fecha mayo 2003)

---

## Preparación del árbol de fuentes

El zip de DIY distribuye los fuentes en formato Acorn RISC OS (sin extensiones).
El script `install.sh` los convierte al formato Unix estándar:

```sh
cd src/
bash install.sh
```

Esto realiza:
- Renombra `c/filename` → `filename.c`
- Renombra `h/filename` → `filename.h`
- Copia `linux-c/i_net.c`, `linux-c/i_video.c` y `linux-c/Makefile` al raíz
- Mueve los archivos Acorn-específicos a `Acorn/`

---

## Target de compilación HP-UX

El Makefile incluido ya tiene soporte HP-UX nativo:

| Target  | Descripción                          | Binario    |
|---------|--------------------------------------|------------|
| `hp8`   | 8-bit color - **el usado** ✓        | `hpdiy8`   |
| `hp16`  | 16-bit color                         | `hpdiy16`  |
| `hp32`  | 32-bit color                         | `hpdiy32`  |
| `hp32r` | 32-bit con resampling                | `hpdiy32r` |

---

## Modificaciones realizadas al código fuente

### Cambio 1: Rutas X11 en HPFLAGS

**Archivo:** `src/Makefile` (línea 30-32)  
**Problema:** Las rutas X11 apuntaban a `/usr/local/DIR/X11/R6.1/` que no existe en HP-UX 11.  
**Causa raíz:** El Makefile original fue escrito para una instalación X11 no estándar.
En HP-UX 11.00, los headers están en `/usr/include/X11/` y las libs en `/usr/lib/X11R6/`.
Además, `libXmu` no está en `/usr/lib/X11R6/` sino en `/usr/contrib/X11R6/lib/`.

**Cambio:**
```diff
- HPFLAGS = COMPFLAGS='-O +e -Aa -D__BIG_ENDIAN__ -D_HPUX_SOURCE -DDOOM_NO_SFX -I/usr/local/DIR/X11/R6.1/include $(DBGFLAG) $$(SPECIALFLAGS)' \
-     LDFLAGS='-L/usr/local/DIR/X11/R6.1/lib -lX11 -lXext -lICE -lXmu' \
-     LIBS='$(BASELIBS) -lm'
+ HPFLAGS = COMPFLAGS='-O +e -Aa -D__BIG_ENDIAN__ -D_HPUX_SOURCE -DDOOM_NO_SFX -I/usr/include $(DBGFLAG) $$(SPECIALFLAGS)' \
+     LDFLAGS='-L/usr/lib/X11R6 -L/usr/contrib/X11R6/lib -lX11 -lXext -lICE -lXmu' \
+     LIBS='$(BASELIBS) -lm'
```

---

### Cambio 2: `make -C` incompatible con HP make

**Archivo:** `src/Makefile` (línea 156-157)  
**Problema:** La regla `libfastlz` usaba `make -C dir` que es una extensión de GNU make.
HP-UX solo tiene HP make, que no soporta la opción `-C`.  
**Causa raíz:** El Makefile fue diseñado asumiendo GNU make.

**Cambio:**
```diff
 libfastlz:
-     make -C $(FASTLZDIR) CFLAGS="$(COMPFLAGS)"
+     cd $(FASTLZDIR) && make CC=$(CC) CFLAGS="$(COMPFLAGS)" libfastlz.a
```

**Notas adicionales:**
- Se agrega `CC=$(CC)` para que fastlz use el compilador HP (`cc`) en vez de `gcc`
- Se especifica target `libfastlz.a` para no compilar el test innecesario

---

## Flags de compilación HP-UX aplicados

```
cc -DNORMALUNIX -DDIYINLINE
   -O +e -Aa
   -D__BIG_ENDIAN__
   -D_HPUX_SOURCE
   -DDOOM_NO_SFX
   -DFULL_NEW_FEATURES
   -DMAXSCREENWIDTH=1024
   -DMAXSCREENHEIGHT=768
   -I/usr/include
```

| Flag              | Propósito                                                  |
|-------------------|------------------------------------------------------------|
| `-O +e -Aa`       | Optimización y modo ANSI - flags del compilador nativo HP  |
| `-D__BIG_ENDIAN__`| PA-RISC es big-endian - activa swap de bytes en WAD reader |
| `-D_HPUX_SOURCE`  | Activa extensiones POSIX/BSD en headers HP-UX              |
| `-DDOOM_NO_SFX`   | Deshabilita sonido (no soportado en HP-UX en esta versión) |
| `-DNORMALUNIX`    | Activa paths Unix (separador `/`, `CURRENTDIR="."`, etc.)  |
| `-DDIYINLINE`     | Activa funciones inline del DIY                            |

---

### Cambio 3: Config file en el directorio de distribución

**Archivo:** `src/d_main.c` (función `IdentifyVersion`, bloque `#ifdef NORMALUNIX`)  
**Problema:** El archivo de configuración siempre se guardaba en `$HOME/.doomrc`,
sin posibilidad de tenerlo junto al ejecutable y el WAD.  
**Solución:** Si `DOOMWADDIR` está definido, usar `$DOOMWADDIR/doom.cfg` como config.
Si no, el comportamiento original (`$HOME/.doomrc`) se mantiene sin cambios.

**Cambio:**
```diff
 #ifdef NORMALUNIX
     home = getenv("HOME");
     if (!home)
       I_Error("Please set $HOME to your home directory");
-    sprintf(basedefault, "%s/.doomrc", home);
+    {
+        char *waddir = getenv("DOOMWADDIR");
+        if (waddir)
+            sprintf(basedefault, "%s/doom.cfg", waddir);
+        else
+            sprintf(basedefault, "%s/.doomrc", home);
+    }
 #define USEWADEXT	".wad"
 #endif
```

---

### Cambio 4: Soporte de sonido para HP-UX via HP Alib / simpleAudio

**Archivos modificados:** `src/i_sound.c`, `src/Makefile`  
**Archivo agregado:** `src/simpleAudio.h`  
**Archivo copiado en build:** `src/simpleAudio.c` (desde `/opt/audio/src/simpleAudio/` en HP-UX)

**Contexto:**  
El port HP-UX original compilaba con `-DDOOM_NO_SFX` (sonido deshabilitado).
HP-UX 11.00 tiene el servidor de audio `Aserver` (parte de HP Alib) corriendo
en el B2000, y provee la API `simpleAudio` que devuelve un socket fd al que se
escriben samples PCM directamente - igual que `/dev/dsp` en Linux.

**Infraestructura de audio en el B2000:**

| Componente        | Ubicación                                |
|-------------------|------------------------------------------|
| Servidor de audio | `/opt/audio/bin/Aserver` (PID ~1719)     |
| Librería Alib     | `/opt/audio/lib/libAlib.sl`              |
| Librería At       | `/opt/audio/lib/libAt.sl`                |
| Headers           | `/opt/audio/include/Alib.h`, `Audio.h`  |
| API simplificada  | `/opt/audio/src/simpleAudio/`            |
| Dispositivos      | `/dev/audio`, `/dev/audioBA`, etc.       |

**Cambio en `src/i_sound.c`:**

1. Include de `simpleAudio.h` para HP-UX:
```diff
+#ifdef __hpux
+#include "simpleAudio.h"
+#endif
```

2. Inicialización del stream de audio (en `I_InitSound`):
```diff
 #ifdef __sun
     /* Solaris: open /dev/audio con libaudio ... */
+#elif defined(__hpux)
+    if (openAudio() != 0) { ... return; }
+    audio_fd = openAStream(PLAY_STREAM, UseFrequency, USE_STEREO,
+                           USE_LIN16, USE_DEFAULT_SPEAKER, START_IMMEDIATELY);
+    if (audio_fd < 0) { closeAudio(); return; }
 #else
     /* Linux: open /dev/dsp con ioctls */
```

3. Sincronización de escritura - HP-UX usa el mismo método timer que Solaris:
```diff
-#ifdef __sun
+#if defined(__sun) || defined(__hpux)
     /* Synchronize via the timer */
```

4. Cierre del stream (en `I_ShutdownSound`):
```diff
 #ifdef __riscos__
     RemoveDoomSound();
+#elif defined(__hpux)
+    closeAStream(audio_fd);
+    closeAudio();
 #else
     close(audio_fd);
```

**Cambio en `src/Makefile` HPFLAGS:**
```diff
-COMPFLAGS='... -DDOOM_NO_SFX -I/usr/include ...'
-LDFLAGS='... -lX11 -lXext -lICE -lXmu'
+COMPFLAGS='... -I/usr/include -I/opt/audio/include ...'
+LDFLAGS='... -lX11 -lXext -lICE -lXmu -lAlib -lAt'
```

`simpleAudio.o` se agrega a la lista de objetos. `simpleAudio.c` no se
distribuye en el repositorio (pertenece a HP); `doom_build.sh` lo copia
automáticamente desde `/opt/audio/src/simpleAudio/` durante el build.

---

## Resultado final

```
Directorio: /opt/doom-hpux/
├── doom-hpux     ~680 KB  ← binario compilado con sonido
├── doom.wad        4 MB   ← shareware Doom 1
├── doom.cfg        0 B    ← config (Doom lo rellena al cerrar)
└── doom.sh       ~144 B   ← script de lanzamiento

Plataforma: HP-UX B.11.00 / PA-RISC 9000/785
Comando:    /opt/doom-hpux/doom.sh
Audio:      HP Alib / simpleAudio - 16-bit linear stereo via Aserver
```

---

## Notas técnicas sobre compatibilidad PA-RISC / HP-UX

### Endianness
PA-RISC es big-endian. El macro `__BIG_ENDIAN__` activa las macros de swap en
`m_swap.h` que corrigen el byte order al leer estructuras del WAD (que están en
little-endian, formato x86 original de Doom).

### Alineación de memoria
El flag `-DDIYNOSHORT` (que en otras plataformas Unix evita accesos `short` no
alineados) no está en `HPFLAGS`. En este sistema no causó problemas, pero si en
el futuro aparecen Bus Errors, ese sería el primer flag a agregar.

### Compilador nativo vs GCC
El sistema usa `cc` (HP C Compiler A.11.01.00). Sus flags difieren de GCC:
- HP: `-O +e -Aa`  vs  GCC: `-O2 -ansi`
- No soporta `-fwritable-strings`, `-Wall`, `-pedantic`

### Sin sonido
`-DDOOM_NO_SFX` desactiva toda la capa de audio. `i_sound.c` compila como
stubs vacíos. Esta es la única limitación conocida del port HP-UX original.

### MIT-SHM (shared memory)
El binario usa la extensión X11 MIT-SHM para actualizar la pantalla eficientemente
sin copias extra. Funciona correctamente en la pantalla local del B2000 (`DISPLAY=:0.0`).
