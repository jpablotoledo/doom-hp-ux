# Doom for HP-UX - HP Visualize B2000

Port of **Doom It Yourself (DIY) v4.4.2** compiled and running on an HP Visualize B2000 with HP-UX 11.00 PA-RISC.

## Hardware and system

| | |
|---|---|
| Machine | HP Visualize B2000 |
| Architecture | PA-RISC 2.0 (9000/785, Big-Endian) |
| Operating system | HP-UX B.11.00 |
| Compiler | HP C Compiler A.11.01.00 |
| Video | X11 with MIT-SHM extension |
| Audio | HP Alib / simpleAudio - 16-bit linear stereo via `Aserver` |
| Music | In-process OPL2/GENMIDI synthesizer (`hp_music.c`), fed by a real `SIGALRM` timer |
| Compiler flags | `+O3 +Onolimit +DA2.0 +DS2.0 +Ofastaccess` (confirmed real improvement on hardware) |

## Source base

[Doom It Yourself (DIY) v4.4.2](https://zarquon.hier-im-netz.de/Programs/DIYSource.zip) - multiplatform port of LinuxDoom 1.10 with native HP-UX support included.

## Source code changes

**`src/Makefile` - X11 paths**
The original paths pointed to `/usr/local/DIR/X11/R6.1/` (non-existent). Fixed to `/usr/include` and `/usr/lib/X11R6`, with an additional `-L/usr/contrib/X11R6/lib` for `libXmu`.

**`src/Makefile` - HP make compatibility**
`make -C $(FASTLZDIR)` is not supported by HP make. Replaced with `cd $(FASTLZDIR) && make CC=$(CC) ...`.

**`src/d_main.c` - Config file next to the binary**
If `DOOMWADDIR` is set, the config file is written as `doom.cfg` inside that directory instead of `$HOME/.doomrc`.

**`src/i_sound.c` + `src/simpleAudio.h` - Sound via HP Alib**
DIY Doom compiles for HP-UX with `-DDOOM_NO_SFX` (no sound). Audio support was implemented using `simpleAudio`, HP's wrapper over Alib. `openAStream()` returns a socket fd connected to `Aserver` where PCM 16-bit stereo is written directly, the same as `/dev/dsp` on Linux. `simpleAudio.c` is not in the repo (belongs to HP) and is copied from `/opt/audio/src/simpleAudio/` during the build.

The HP-UX synchronous path (`I_SubmitSound`) is called 35 times/sec while the playback frequency is 11025 Hz, causing growing lag without throttling. Time-based throttling via `gettimeofday()` was implemented to skip writes when the buffer exceeds 2 frames ahead (~93 ms fixed lag).

**`src/Makefile` - Optimization and screen limits**
Optimization raised from `-O` to `+O2 +Onolimit`. `MAXSCREENWIDTH` raised from 1024 to 1280 and `MAXSCREENHEIGHT` from 768 to 800 to support `-4` mode (1280×800) without overflowing the sprite renderer arrays.

**`src/hp_music.c` - In-process music synthesizer**
Music was originally unimplemented (`MUSSERV` was never defined for any platform). An in-process OPL2/GENMIDI FM synthesizer + MUS parser was built, fed by a real `SIGALRM`/`setitimer` timer independent of the render loop - no external process, no pipe. Along the way, several real synthesis bugs were found and fixed: wrong MUS percussion instrument mapping, an incorrect OPL2 feedback formula, aliasing above Nyquist on high-frequency percussion instruments, a hard one-sample envelope cutoff on release/decay, an unfiltered DC bias, and a dynamic-divisor mixing bug that caused audible volume "pumping" on every note.

**`src/Makefile` - Compiler optimization flags**
`+O2` raised to `+O3 +Onolimit +DA2.0 +DS2.0 +Ofastaccess` (target-architecture codegen, instruction scheduling for this CPU model, and faster global/static data access) - confirmed as a real, noticeable improvement playing on the actual hardware.

Full details in [docs/02-machine-setup.md](docs/02-machine-setup.md), [docs/04-add-sound.md](docs/04-add-sound.md), [docs/05-music-investigation.md](docs/05-music-investigation.md), and [docs/06-cpu-performance-investigation.md](docs/06-cpu-performance-investigation.md).

## WAD

**Doom 1 shareware** (`doom1.wad`) is used. Included in the distribution directory as `doom.wad`.

## Build and distribute

### WAD file

The **Doom 1 shareware** WAD (`shareware/doom1.wad`) is included in the repository -
the original id Software shareware release, freely distributable. No extra download needed.

### Compiling on HP-UX

The [`doom_build.sh`](doom_build.sh) script compiles the binary and generates `/opt/doom-hpux/` with everything needed. It automatically searches for the WAD in the project directory or in `/tmp/`.

**Transfer to HP-UX** (from Linux/other Unix):

```sh
tar czf /tmp/doom-hpux-project.tar.gz src/ doom_build.sh shareware/
ftp -n <hpux-ip> <<EOF
user root <password>
binary
put /tmp/doom-hpux-project.tar.gz /tmp/doom-hpux-project.tar.gz
quit
EOF
```

**On HP-UX**, extract and run the build:

```sh
# HP-UX tar does not support -z, decompress in two steps
cd /tmp && gunzip doom-hpux-project.tar.gz && tar xf doom-hpux-project.tar

sh /tmp/doom-hpux/doom_build.sh
```

The script prints compiler output in real time. If you need to run it detached
(e.g. over telnet), use `at` and monitor the log:

```sh
at -f /tmp/doom-hpux/doom_build.sh now
tail -f /tmp/doom_build.log
```

## Result

```
/opt/doom-hpux/
├── doom-hpux                    ← binary (~750 KB)
├── doom.wad                     ← Doom 1 shareware (~4 MB)
├── doom.cfg                     ← config (created on game exit)
├── doom.sh                      ← launch script: music + sound effects (default)
├── doom-hpux-nomusic.sh         ← launch script: sound effects only
├── doom-hpux-nomusic-nofx.sh    ← launch script: no audio (best performance)
└── doom-hpux-nosound.sh         ← launch script: no audio (best performance)
```

`-nosound` skips all audio mixing work entirely at runtime (not just
muting the output), for the best possible performance when audio isn't
needed.

## Running

From a CDE terminal on the B2000:

```sh
/opt/doom-hpux/doom.sh
```

Scale options (software scaling, CPU cost increases with factor):

| Flag | Resolution | Notes |
|------|-----------|-------|
| *(none)* | 320×200 | native, minimal CPU |
| `-2` | 640×400 | recommended |
| `-3` | 960×600 | |
| `-4` | 1280×800 | fills screen, highest CPU load |

## Documentation

Listed in the order they were written, so they also read as a timeline of
how the project grew. See [docs/00-index.md](docs/00-index.md) for the
same list with dates and one-line summaries.

1. This file / [LEAME.md](LEAME.md) - project overview
2. [docs/02-machine-setup.md](docs/02-machine-setup.md) *(historical)* - Machine state, build steps, problems found and solutions
3. [docs/03-source-changes.md](docs/03-source-changes.md) *(historical)* - Source code changes with diffs and technical rationale
4. [docs/04-add-sound.md](docs/04-add-sound.md) *(historical)* - Sound implementation via HP Alib: changes, problems and solutions
5. [docs/05-music-investigation.md](docs/05-music-investigation.md) - Full music investigation: architecture, bugs found and fixed, final state
6. [docs/06-cpu-performance-investigation.md](docs/06-cpu-performance-investigation.md) - Compiler/CPU optimization research and results
7. [docs/07-commit-audit.md](docs/07-commit-audit.md) - Commit-by-commit audit of what was necessary vs. superseded

*(historical)* docs describe the project's 2011 starting state and are
kept unedited for historical accuracy; they no longer reflect the
current source tree. See the note at the top of each.
