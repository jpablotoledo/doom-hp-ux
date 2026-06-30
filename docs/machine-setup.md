# HP Visualize B2000 Machine Setup

## System info

| Parameter         | Value                                          |
|-------------------|------------------------------------------------|
| Hardware          | HP Visualize B2000                             |
| Architecture      | PA-RISC 2.0 (chip 9000/785, Big-Endian)        |
| Operating system  | HP-UX B.11.00 (hostname: tmp2-80)             |
| Access            | 192.168.1.37 (telnet, user root)               |
| Goal              | Compile and run Doom It Yourself 4.4.2         |
| Result            | **SUCCESS** - Doom running at 85% CPU          |

---

## Initial machine state (audit)

### Compiler

```
what /usr/bin/cc
  HP92453-01 A.11.01.00 HP C Compiler
  PATCH/11.00:PHCO_95167  Oct  1 1998
```

- **Native HP C compiler** version A.11.01.00 available at `/usr/bin/cc`
- **GCC not installed** (`sh: gcc: not found`)

### X11

| Path                        | Contents                             |
|-----------------------------|--------------------------------------|
| `/usr/include/X11/`         | X11 headers (Xlib.h, etc.)           |
| `/usr/include/X11R6/X11/`  | X11R6 headers (alternative)          |
| `/usr/lib/X11R6/`           | libX11, libXext, libICE, libSM       |
| `/usr/contrib/X11R6/lib/`  | **libXmu** (NOT in X11R6/)           |
| `/usr/lib/X11R4/`           | libXmu.sl (old version)              |

**Key point:** `libXmu` was not in `/usr/lib/X11R6/` but in `/usr/contrib/X11R6/lib/`. The original Makefile pointed to a non-existent path.

### Tools

| Tool    | Status          | Notes                                            |
|---------|-----------------|--------------------------------------------------|
| `make`  | `/usr/bin/make` | HP make (not GNU make; no `--version` or `-C`)   |
| `tar`   | `/usr/bin/tar`  | HP tar (no `-z` flag for gzip)                   |
| `gunzip`| available       | Required to decompress .tar.gz                   |
| `ftp`   | `/usr/bin/ftp`  | Used for file transfers                          |
| `at`    | available       | Used to run builds without a terminal            |

### Disk space

| Filesystem | Total  | Free   | Mount    |
|------------|--------|--------|----------|
| /          | 248MB  | 118MB  | lvol3    |
| /tmp       | 480MB  | 478MB  | lvol6    |
| /diska     | 7.8GB  | 6.5GB  | vg01     |

### Relevant installed software

| Bundle              | Description                                          |
|---------------------|------------------------------------------------------|
| B3899BA B.11.01.07  | HP C/ANSI C Developer's Bundle for HP-UX 11.00 ✓   |
| FIREFOX 2.0.0.2     | Firefox for HP-UX                                    |
| /root/games/        | Game collection in depot format (.depot)             |

---

## Steps performed on the machine

### 1. Source code transfer

The code was prepared on a Linux machine (sources converted to Unix format with `install.sh`) and transferred to HP-UX via FTP:

```sh
# On Linux:
tar czf /tmp/doom-src.tar.gz src/
ftp -n 192.168.1.37 <<EOF
user root hp2000
binary
put /tmp/doom-src.tar.gz /tmp/doom-src.tar.gz
quit
EOF

# On HP-UX:
cd /tmp && gunzip doom-src.tar.gz && tar xf doom-src.tar
```

**Note:** HP-UX `tar` does not support `-z`. Decompress first with `gunzip`.

### 2. Compilation

The build was submitted to the `at` daemon to isolate it from the telnet session and prevent SIGHUP from killing the compiler when the session closes:

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

**Problem with nohup:** Previous attempts with `nohup ... &` failed because the internal `ccom` compiler process received SIGHUP from the process group when the session closed, even with `nohup`. The fix was to use `at` (runs under `atd`, no terminal attached).

**Build result:**
```
exit code: 0
binary:    /tmp/src/hpdiy8 (663,552 bytes)
```

### 3. Obtaining the WAD (game data file)

Doom requires a WAD file with all game data. None was present on the machine. **Freedoom Phase 1** (free, open-source WAD) was downloaded:

```sh
# On Linux:
wget https://github.com/freedoom/freedoom/releases/download/v0.13.0/freedoom-0.13.0.zip
unzip -p freedoom-0.13.0.zip "*/freedoom1.wad" > /tmp/freedoom1.wad

# Transfer to HP-UX (28MB):
ftp -n 192.168.1.37 <<EOF
user root hp2000
binary
put /tmp/freedoom1.wad /tmp/freedoom1.wad
quit
EOF
```

### 4. Running

DIY Doom does not accept full paths with `-iwad`. It looks for the WAD by its standard name (`doom.wad`, `doom2.wad`, etc.) in the directory set by `DOOMWADDIR`. A symlink with the expected name was created:

```sh
# On HP-UX:
ln -s /tmp/freedoom1.wad /tmp/doom.wad
cd /tmp/src
DOOMWADDIR=/tmp DISPLAY=:0.0 ./hpdiy8
```

**Successful startup output:**
```
DOOM Registered Startup v1.11
V_Init: Allocated 4 screens.
M_LoadDefaults: Load system defaults.
Z_Init: Init zone memory allocation daemon.
I_ZoneBase: Starting with 32768k memory.
W_Init: Init WADfiles.
 adding /tmp/doom.wad
 -->  E1M1-E4M9
...
Using MITSHM extension
shared memory id=8197, addr=0xc0d5b000
```

The process runs at **85% CPU** rendering the game.

---

## Final distribution on the machine

The build script generates the self-contained directory `/opt/doom-hpux/`:

```
/opt/doom-hpux/
├── doom-hpux     ~680 KB   ← compiled binary
├── doom.wad    28.795 KB   ← Freedoom Phase 1 (full copy)
├── doom.cfg         0 B    ← config (Doom writes here on exit)
└── doom.sh        144 B    ← launch script
```

**To start Doom from a CDE terminal on the B2000:**
```sh
/opt/doom-hpux/doom.sh
```

The `doom.sh` script sets `DOOMWADDIR` and `DISPLAY` automatically:
```sh
#!/bin/sh
DOOM_DIR=`dirname $0`
cd "$DOOM_DIR"
DOOMWADDIR="$DOOM_DIR" DISPLAY="${DISPLAY:-:0.0}" ./doom-hpux "$@"
```

To **rebuild the full distribution** from scratch (e.g. after a reboot):
```sh
# 1. Copy the script to the machine (from Linux):
ftp -n 192.168.1.37 <<EOF
user root hp2000
binary
put /tmp/doom_build.sh /tmp/doom_build.sh
quit
EOF

# 2. On HP-UX:
chmod +x /tmp/doom_build.sh
at -f /tmp/doom_build.sh now

# 3. Monitor:
tail -f /tmp/doom_build.log
cat /tmp/doom_build.exit   # 0 = success
```

---

## Problems found and solutions

### P1: `tar xzf` does not work on HP-UX
**Cause:** HP-UX tar has no `-z` option for gzip decompression.
**Fix:** `gunzip file.tar.gz && tar xf file.tar`

### P2: `make -C dir` is not compatible with HP make
**Cause:** HP make does not support the `-C` option (GNU make extension).
**Fix:** Change the Makefile to `cd dir && make` (see docs/source-changes.md)

### P3: Compiler `ccom` dies from SIGHUP when closing the telnet session
**Cause:** When ksh session closes, the shell sends SIGHUP to the process group. The `ccom` subprocess of the HP compiler does not ignore SIGHUP even when the parent uses `nohup`.
**Fix:** Use the `at` command to run the build; it runs under `atd` with no terminal and never receives SIGHUP.

### P4: `-iwad /full/path.wad` does not work in DIY Doom
**Cause:** `IdentifyVersionByName()` expects a short name like "doom" or "doom2", not a path. If the name is not in its internal list, it ignores the argument and calls `IdentifyVersion()`, which looks for standard WADs in `DOOMWADDIR`.
**Fix:** Create a symlink with the standard name + `DOOMWADDIR` variable.

### P5: No commercial WAD available
**Fix:** Use Freedoom Phase 1 (freedoom1.wad), a free WAD compatible with the Doom 1 IWAD format. Downloaded from GitHub releases.

### P6: libXmu not in `/usr/lib/X11R6/`
**Cause:** On this system, `libXmu` is in `/usr/contrib/X11R6/lib/`.
**Fix:** Add `-L/usr/contrib/X11R6/lib` to `LDFLAGS` in the Makefile.
