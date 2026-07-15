> **Historical snapshot (June 2011).** Documents the first working sound
> effects implementation (HP Alib / `simpleAudio`, SFX only - no music
> yet). Accurate for what it covers, but superseded by
> [`docs/05-music-investigation.md`](05-music-investigation.md) for the
> full audio story, including the in-process OPL2/GENMIDI music
> synthesizer and every bug fixed since. See
> [`docs/00-index.md`](00-index.md) for the full chronological list.

# Sound implementation for HP-UX (HP Alib / simpleAudio)

## Context

DIY Doom 4.4.2 compiles for HP-UX with `-DDOOM_NO_SFX` - sound completely disabled. The B2000 has the `Aserver` audio server running and the `simpleAudio` API available in `/opt/audio/`. This document covers all changes required to enable 16-bit stereo sound via HP Alib.

## Audio infrastructure on the B2000

| Component        | Location                                |
|------------------|-----------------------------------------|
| Audio server     | `/opt/audio/bin/Aserver` (always running)|
| Alib library     | `/opt/audio/lib/libAlib.sl`             |
| At library       | `/opt/audio/lib/libAt.sl`               |
| Headers          | `/opt/audio/include/Alib.h`, `Audio.h`  |
| Simplified API   | `/opt/audio/src/simpleAudio/`           |
| Devices          | `/dev/audio`, `/dev/audioBA`            |

`simpleAudio` is a wrapper over Alib that opens a TCP connection to `Aserver` and returns a file descriptor to which PCM is written directly, the same as `/dev/dsp` on Linux. The source is at `/opt/audio/src/simpleAudio/simpleAudio.c` and is not distributed in this repository - it is copied automatically during the build (see `doom_build.sh`).

---

## Changes made

### 1. `src/simpleAudio.h` - API header (new file)

Copied from `/opt/audio/src/simpleAudio/simpleAudio.h` on the B2000. Contains the function declarations and constants needed:

```c
extern int  openAudio();               // connect to Aserver
extern void closeAudio();              // disconnect from Aserver
extern int  openAStream(streamMode, sampleRate, channels,
                        dataFormat, device, startPaused);
extern void closeAStream(int fd);

#define PLAY_STREAM        0
#define USE_STEREO         2
#define USE_LIN16          0    // signed 16-bit PCM
#define USE_DEFAULT_SPEAKER -1
#define START_IMMEDIATELY  0
```

---

### 2. `src/i_sound.c` - audio implementation

#### 2a. Includes for HP-UX

```diff
 #ifdef __hpux
 #include "simpleAudio.h"
+#include <sys/socket.h>
+#include <netinet/in.h>
+#include <netinet/tcp.h>
 #endif
```

`sys/socket.h` and `netinet/in.h` are needed because `audio_fd` is a TCP socket to Aserver and `fcntl` is used on it.

#### 2b. Stream initialization in `I_InitSound()`

Block added after the Solaris block (`#ifdef __sun`):

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

`O_NONBLOCK` is important: if the Aserver buffer is full, `write()` returns `EAGAIN` instead of blocking the game loop.

#### 2c. Stream shutdown in `I_ShutdownSound()`

```c
#elif defined(__hpux)
    closeAStream(audio_fd);
    closeAudio();
```

#### 2d. Throttling in `I_SubmitSound()` - the most important change

HP-UX does not use `SNDINTR` (timer interrupts), so `I_SubmitSound()` is called synchronously from the game loop, 35 times per second. The problem:

- Game loop: calls `I_SubmitSound()` 35 times/sec
- Each call writes `SampleCount` = 512 samples
- Total written: 512 × 35 = **17,920 samples/sec**
- Playback frequency: **11,025 Hz**
- Result: we write 1.6× faster than Aserver plays back → Aserver buffer fills up → growing lag over time

The fix is to measure real elapsed time and skip writes when we are more than 2 buffers ahead of playback:

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

Resulting lag: 2 × 512 / 11025 ≈ **93 ms** (fixed, not growing).

#### 2e. Timer synchronization (for completeness)

The timer-based sync block (used on Solaris with SNDINTR) was extended to include HP-UX, although in practice HP-UX does not define SNDINTR:

```diff
-#ifdef __sun
+#if defined(__sun) || defined(__hpux)
     /* Synchronize via the timer */
```

---

### 3. `src/Makefile` - compilation and linking flags

#### 3a. HPFLAGS: audio and optimization

```diff
-HPFLAGS = COMPFLAGS='-O +e -Aa -D__BIG_ENDIAN__ -D_HPUX_SOURCE -I/usr/include ...'
-          LDFLAGS='... -lX11 -lXext -lICE -lXmu'
+HPFLAGS = COMPFLAGS='+O2 +Onolimit +e -Aa -D__BIG_ENDIAN__ -D_HPUX_SOURCE -I/usr/include -I/opt/audio/include ...'
+          LDFLAGS='... -lX11 -lXext -lICE -lXmu -lAlib -lAt'
```

- `-DDOOM_NO_SFX` removed - enables the audio code
- `-I/opt/audio/include` - simpleAudio and Alib headers
- `-L/opt/audio/lib -lAlib -lAt` - HP audio libraries
- `-O` → `+O2 +Onolimit` - level 2 optimization with no size limit

#### 3b. OBJS: add simpleAudio.o

```diff
 $(OBJPATH)i_sound.o \
+$(OBJPATH)simpleAudio.o
```

#### 3c. Compile rule for simpleAudio.o

```makefile
$(OBJPATH)simpleAudio.o: simpleAudio.c simpleAudio.h
	$(CC) $(CFLAGS) -I/opt/audio/include -c -o $(OBJPATH)simpleAudio.o simpleAudio.c
```

`simpleAudio.c` is not in the repository (belongs to HP). `doom_build.sh` copies it from `/opt/audio/src/simpleAudio/simpleAudio.c` before compiling.

#### 3d. MAXSCREENWIDTH and MAXSCREENHEIGHT

```diff
-OPTIONS = -DFULL_NEW_FEATURES -DMAXSCREENWIDTH=1024 -DMAXSCREENHEIGHT=768
+OPTIONS = -DFULL_NEW_FEATURES -DMAXSCREENWIDTH=1280 -DMAXSCREENHEIGHT=800
```

The sprite renderer arrays (`r_things.c`, `r_plane.c`, `r_state.h`) are statically sized using these constants at compile time. With `-4` (4× scale), the resolution is 1280×800. With the original value of 1024, the renderer overflowed the arrays when drawing enemy sprites → `Memory fault (coredump)`.

---

## Problems encountered during implementation

### P1: `IPPROTO_TCP` undefined
`setsockopt(IPPROTO_TCP, TCP_NODELAY)` requires `<netinet/in.h>` on HP-UX (not just `<netinet/tcp.h>`). It was attempted to reduce TCP latency but discarded - `O_NONBLOCK` is sufficient and safer on non-TCP fds.

### P2: Growing audio lag
`I_SubmitSound()` without throttling writes 1.6× faster than the playback frequency. The Aserver buffer fills up and lag grows over time.
Fix: throttling with `gettimeofday()` (see change 2d).

### P3: Memory fault when seeing enemies with `-4`
`MAXSCREENWIDTH=1024` insufficient for 1280px resolution. The static arrays in the sprite renderer overflowed when indexing by screen column.
Fix: raise limits to 1280×800 (change 3d).

---

## Result

```
Audio:   HP Alib / simpleAudio - PCM 16-bit stereo at 11025 Hz via Aserver
Lag:     ~93 ms (fixed)
Stable:  no coredumps up to 1280×800 resolution (flag -4)
```
