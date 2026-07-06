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

Full details in [docs/machine-setup.md](docs/machine-setup.md) and [docs/add-sound.md](docs/add-sound.md).

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
├── doom-hpux   ← binary (~680 KB)
├── doom.wad    ← Doom 1 shareware (~4 MB)
├── doom.cfg    ← config (created on game exit)
└── doom.sh     ← launch script
```

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

- [docs/machine-setup.md](docs/machine-setup.md) - Machine state, build steps, problems found and solutions
- [docs/source-changes.md](docs/source-changes.md) - Source code changes with diffs and technical rationale
- [docs/add-sound.md](docs/add-sound.md) - Sound implementation via HP Alib: changes, problems and solutions
