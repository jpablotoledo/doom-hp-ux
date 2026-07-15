> **Instantánea histórica (junio 2011).** Documenta la primera
> implementación funcional de efectos de sonido (HP Alib / `simpleAudio`,
> solo SFX - todavía sin música). Preciso para lo que cubre, pero superado
> por [`docs/05-investigacion-musica.md`](05-investigacion-musica.md) para
> la historia completa de audio, incluyendo el sintetizador de música
> OPL2/GENMIDI en proceso y cada bug corregido desde entonces. Ver
> [`docs/00-indice.md`](00-indice.md) para el listado cronológico
> completo.

# Implementación de sonido para HP-UX (HP Alib / simpleAudio)

## Contexto

DIY Doom 4.4.2 compila para HP-UX con `-DDOOM_NO_SFX` - sonido completamente
deshabilitado. El B2000 tiene el servidor de audio `Aserver` corriendo y la API
`simpleAudio` disponible en `/opt/audio/`. Esta guía documenta todos los cambios
necesarios para habilitar sonido 16-bit stereo via HP Alib.

## Infraestructura de audio en el B2000

| Componente       | Ubicación                               |
|------------------|-----------------------------------------|
| Servidor de audio| `/opt/audio/bin/Aserver` (corre siempre)|
| Librería Alib    | `/opt/audio/lib/libAlib.sl`             |
| Librería At      | `/opt/audio/lib/libAt.sl`               |
| Headers          | `/opt/audio/include/Alib.h`, `Audio.h`  |
| API simplificada | `/opt/audio/src/simpleAudio/`           |
| Dispositivos     | `/dev/audio`, `/dev/audioBA`            |

`simpleAudio` es un wrapper sobre Alib que abre una conexión TCP al `Aserver`
y devuelve un file descriptor al que se escribe PCM directamente, igual que
`/dev/dsp` en Linux. El fuente está en `/opt/audio/src/simpleAudio/simpleAudio.c`
y no se distribuye en este repositorio - se copia automáticamente durante el
build (ver `doom_build.sh`).

---

## Cambios realizados

### 1. `src/simpleAudio.h` - header de la API (archivo nuevo)

Copiado desde `/opt/audio/src/simpleAudio/simpleAudio.h` en el B2000.
Contiene las declaraciones de las funciones y las constantes necesarias:

```c
extern int  openAudio();               // conecta al Aserver
extern void closeAudio();              // desconecta del Aserver
extern int  openAStream(streamMode, sampleRate, channels,
                        dataFormat, device, startPaused);
extern void closeAStream(int fd);

#define PLAY_STREAM        0
#define USE_STEREO         2
#define USE_LIN16          0    // PCM 16-bit con signo
#define USE_DEFAULT_SPEAKER -1
#define START_IMMEDIATELY  0
```

---

### 2. `src/i_sound.c` - implementación de audio

#### 2a. Includes para HP-UX

```diff
 #ifdef __hpux
 #include "simpleAudio.h"
+#include <sys/socket.h>
+#include <netinet/in.h>
+#include <netinet/tcp.h>
 #endif
```

`sys/socket.h` y `netinet/in.h` son necesarios porque `audio_fd` es un socket
TCP hacia Aserver y se usa `fcntl` sobre él.

#### 2b. Inicialización del stream en `I_InitSound()`

Bloque agregado después del bloque Solaris (`#ifdef __sun`):

```c
#elif defined(__hpux)
    if (openAudio() != 0)
    {
      fprintf(logfile, "Could not connect to HP audio server\n");
      return;
    }
    audio_fd = openAStream(PLAY_STREAM, UseFrequency, USE_STEREO,
                           USE_LIN16, USE_DEFAULT_SPEAKER, START_IMMEDIATELY);
    if (audio_fd < 0)
    {
      fprintf(logfile, "Could not open HP-UX audio stream\n");
      closeAudio();
      return;
    }
    fcntl(audio_fd, F_SETFL, fcntl(audio_fd, F_GETFL) | O_NONBLOCK);
    fprintf(logfile, "using HP Alib 16bit linear stereo; ");
```

El `O_NONBLOCK` es importante: si el buffer de Aserver está lleno, el `write()`
devuelve `EAGAIN` en lugar de bloquear el game loop.

#### 2c. Cierre del stream en `I_ShutdownSound()`

```c
#elif defined(__hpux)
    closeAStream(audio_fd);
    closeAudio();
```

#### 2d. Throttling en `I_SubmitSound()` - el cambio más importante

HP-UX no usa `SNDINTR` (interrupciones de timer), así que `I_SubmitSound()` se
llama sincrónicamente desde el game loop, 35 veces por segundo. El problema:

- Game loop: llama `I_SubmitSound()` 35 veces/seg
- Cada llamada escribe `SampleCount` = 512 samples
- Total escrito: 512 × 35 = **17.920 samples/seg**
- Frecuencia de reproducción: **11.025 Hz**
- Resultado: escribimos 1,6× más rápido de lo que Aserver reproduce → el buffer
  de Aserver se llena → lag creciente con el tiempo

La solución es medir el tiempo real transcurrido y saltar escrituras cuando
estamos más de 2 buffers por delante de la reproducción:

```c
#ifdef __hpux
    {
      static struct timeval prev = {0, 0};
      static int hp_queued = 0;
      struct timeval now;
      long usec_diff;
      int consumed;

      gettimeofday(&now, NULL);
      if (prev.tv_sec == 0) { prev = now; hp_queued = 0; }

      usec_diff = (now.tv_sec - prev.tv_sec) * 1000000L +
                  (now.tv_usec - prev.tv_usec);
      consumed = (int)((long)usec_diff * UseFrequency / 1000000L);
      prev = now;

      hp_queued -= consumed;
      if (hp_queued < 0) hp_queued = 0;

      if (hp_queued < 2 * SampleCount)
      {
        write(audio_fd, mixbuffer, MixBufferSize);
        hp_queued += SampleCount;
      }
    }
#else
    write(audio_fd, mixbuffer, MixBufferSize);
#endif
```

Lag resultante: 2 × 512 / 11025 ≈ **93 ms** (fijo, no creciente).

#### 2e. Sincronización de timer (para completitud)

El bloque de sincronización por timer (usado en Solaris con SNDINTR) se amplió
para incluir HP-UX, aunque en la práctica HP-UX no define SNDINTR:

```diff
-#ifdef __sun
+#if defined(__sun) || defined(__hpux)
     /* Synchronize via the timer */
```

---

### 3. `src/Makefile` - flags de compilación y linkeo

#### 3a. HPFLAGS: audio y optimización

```diff
-HPFLAGS = COMPFLAGS='-O +e -Aa -D__BIG_ENDIAN__ -D_HPUX_SOURCE -I/usr/include $(DBGFLAG)'
-          LDFLAGS='-L/usr/lib/X11R6 -L/usr/contrib/X11R6/lib -lX11 -lXext -lICE -lXmu'
+HPFLAGS = COMPFLAGS='+O2 +Onolimit +e -Aa -D__BIG_ENDIAN__ -D_HPUX_SOURCE -I/usr/include -I/opt/audio/include $(DBGFLAG)'
+          LDFLAGS='-L/usr/lib/X11R6 -L/usr/contrib/X11R6/lib -L/opt/audio/lib -lX11 -lXext -lICE -lXmu -lAlib -lAt'
```

- `-DDOOM_NO_SFX` eliminado - habilita el código de audio
- `-I/opt/audio/include` - headers de simpleAudio y Alib
- `-L/opt/audio/lib -lAlib -lAt` - librerías de HP audio
- `-O` → `+O2 +Onolimit` - optimización nivel 2 sin límite de tamaño

#### 3b. OBJS: agregar simpleAudio.o

```diff
 $(OBJPATH)i_sound.o \
+$(OBJPATH)simpleAudio.o
```

#### 3c. Regla de compilación para simpleAudio.o

```makefile
$(OBJPATH)simpleAudio.o: simpleAudio.c simpleAudio.h
	$(CC) $(CFLAGS) -I/opt/audio/include -c -o $(OBJPATH)simpleAudio.o simpleAudio.c
```

`simpleAudio.c` no está en el repositorio (pertenece a HP). `doom_build.sh` lo
copia desde `/opt/audio/src/simpleAudio/simpleAudio.c` antes de compilar.

#### 3d. MAXSCREENWIDTH y MAXSCREENHEIGHT

```diff
-OPTIONS = -DFULL_NEW_FEATURES -DMAXSCREENWIDTH=1024 -DMAXSCREENHEIGHT=768
+OPTIONS = -DFULL_NEW_FEATURES -DMAXSCREENWIDTH=1280 -DMAXSCREENHEIGHT=800
```

Los arrays del renderer de sprites (`r_things.c`, `r_plane.c`, `r_state.h`) se
dimensionan con estas constantes en tiempo de compilación. Con `-4` (escala 4×),
la resolución es 1280×800. Con el valor original de 1024, el renderer
desbordaba los arrays al dibujar sprites de enemigos → `Memory fault (coredump)`.

---

## Problemas encontrados durante la implementación

### P1: `IPPROTO_TCP` undefined
`setsockopt(IPPROTO_TCP, TCP_NODELAY)` requiere `<netinet/in.h>` en HP-UX
(no solo `<netinet/tcp.h>`). Se intentó usar para reducir latencia TCP pero
se descartó - `O_NONBLOCK` es suficiente y más seguro sobre fds no-TCP.

### P2: Lag de audio creciente
`I_SubmitSound()` sin throttling escribe 1,6× más rápido que la frecuencia de
reproducción. El buffer de Aserver se llena y el lag crece con el tiempo.
Solución: throttling con `gettimeofday()` (ver cambio 2d).

### P3: Memory fault al ver enemigos con `-4`
`MAXSCREENWIDTH=1024` insuficiente para resolución 1280px. Los arrays estáticos
del renderer de sprites se desbordaban al indexar por columna de pantalla.
Solución: subir límites a 1280×800 (cambio 3d).

---

## Resultado

```
Audio:   HP Alib / simpleAudio - PCM 16-bit stereo a 11025 Hz via Aserver
Lag:     ~93 ms (fijo)
Estable: sin coredumps hasta resolución 1280×800 (flag -4)
```
