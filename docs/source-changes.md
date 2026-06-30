# Source code changes to Doom It Yourself for HP-UX

## Code base

**Project:** Doom It Yourself (DIY) v4.4.2
**Original source:** https://zarquon.hier-im-netz.de/Programs/DIYSource.zip
**Base:** LinuxDoom 1.10 (id Software, GPL license)
**Original archive:** `DIYSource.zip` (759,197 bytes, dated May 2003)

---

## Preparing the source tree

The DIY zip distributes sources in Acorn RISC OS format (no extensions). The `install.sh` script converts them to standard Unix format:

```sh
cd src/
bash install.sh
```

This performs:
- Renames `c/filename` → `filename.c`
- Renames `h/filename` → `filename.h`
- Copies `linux-c/i_net.c`, `linux-c/i_video.c` and `linux-c/Makefile` to the root
- Moves Acorn-specific files to `Acorn/`

---

## HP-UX build target

The included Makefile already has native HP-UX support:

| Target  | Description                          | Binary     |
|---------|--------------------------------------|------------|
| `hp8`   | 8-bit color - **the one used** ✓    | `hpdiy8`   |
| `hp16`  | 16-bit color                         | `hpdiy16`  |
| `hp32`  | 32-bit color                         | `hpdiy32`  |
| `hp32r` | 32-bit with resampling               | `hpdiy32r` |

---

## Changes made to the source code

### Change 1: X11 paths in HPFLAGS

**File:** `src/Makefile` (line 30-32)
**Problem:** The X11 paths pointed to `/usr/local/DIR/X11/R6.1/` which does not exist on HP-UX 11.
**Root cause:** The Makefile was written for a non-standard X11 installation. On HP-UX 11.00, headers are in `/usr/include/X11/` and libs in `/usr/lib/X11R6/`. Also, `libXmu` is not in `/usr/lib/X11R6/` but in `/usr/contrib/X11R6/lib/`.

**Change:**
```diff
- HPFLAGS = COMPFLAGS='-O +e -Aa -D__BIG_ENDIAN__ -D_HPUX_SOURCE -DDOOM_NO_SFX -I/usr/local/DIR/X11/R6.1/include ...'
-     LDFLAGS='-L/usr/local/DIR/X11/R6.1/lib -lX11 -lXext -lICE -lXmu' \
+ HPFLAGS = COMPFLAGS='+O2 +Onolimit +e -Aa -D__BIG_ENDIAN__ -D_HPUX_SOURCE -I/usr/include -I/opt/audio/include ...'
+     LDFLAGS='-L/usr/lib/X11R6 -L/usr/contrib/X11R6/lib -L/opt/audio/lib -lX11 -lXext -lICE -lXmu -lAlib -lAt' \
```

---

### Change 2: `make -C` incompatible with HP make

**File:** `src/Makefile` (line 156-157)
**Problem:** The `libfastlz` rule used `make -C dir`, which is a GNU make extension. HP-UX only has HP make, which does not support the `-C` option.
**Root cause:** The Makefile was designed assuming GNU make.

**Change:**
```diff
 libfastlz:
-     make -C $(FASTLZDIR) CFLAGS="$(COMPFLAGS)"
+     cd $(FASTLZDIR) && make CC=$(CC) CFLAGS="$(COMPFLAGS)" libfastlz.a
```

**Additional notes:**
- `CC=$(CC)` added so fastlz uses the HP compiler (`cc`) instead of `gcc`
- Target `libfastlz.a` specified to avoid compiling the unnecessary test binary

---

## HP-UX compilation flags applied

```
cc -DNORMALUNIX -DDIYINLINE
   +O2 +Onolimit +e -Aa
   -D__BIG_ENDIAN__
   -D_HPUX_SOURCE
   -DFULL_NEW_FEATURES
   -DMAXSCREENWIDTH=1280
   -DMAXSCREENHEIGHT=800
   -I/usr/include
   -I/opt/audio/include
```

| Flag                | Purpose                                                     |
|---------------------|-------------------------------------------------------------|
| `+O2 +Onolimit +e -Aa` | Optimization level 2, no size limit, ANSI mode - native HP compiler flags |
| `-D__BIG_ENDIAN__`  | PA-RISC is big-endian - enables byte swapping in WAD reader |
| `-D_HPUX_SOURCE`    | Enables POSIX/BSD extensions in HP-UX headers              |
| `-DNORMALUNIX`      | Enables Unix paths (separator `/`, `CURRENTDIR="."`, etc.) |
| `-DDIYINLINE`       | Enables DIY inline functions                               |

---

### Change 3: Config file in the distribution directory

**File:** `src/d_main.c` (function `IdentifyVersion`, `#ifdef NORMALUNIX` block)
**Problem:** The config file was always saved to `$HOME/.doomrc`, with no way to keep it alongside the binary and WAD.
**Fix:** If `DOOMWADDIR` is set, use `$DOOMWADDIR/doom.cfg` as the config. Otherwise, the original behavior (`$HOME/.doomrc`) is preserved.

**Change:**
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

## Final result

```
Directory: /opt/doom-hpux/
├── doom-hpux     ~680 KB  ← compiled binary (with sound)
├── doom.wad       28 MB   ← Freedoom Phase 1 v0.13.0
├── doom.cfg        0 B    ← config (Doom fills it on exit)
└── doom.sh       ~144 B   ← launch script

Platform: HP-UX B.11.00 / PA-RISC 9000/785
Command:  /opt/doom-hpux/doom.sh
Audio:    HP Alib / simpleAudio - 16-bit linear stereo via Aserver
```

---

## Technical notes on PA-RISC / HP-UX compatibility

- **Big-endian:** WAD files store integers in little-endian. The `-D__BIG_ENDIAN__` flag enables byte-swap macros in `w_wad.c` and `m_swap.h`.
- **`_HPUX_SOURCE`:** Required to expose POSIX extensions like `ITIMER_REAL` and `sigaction` in HP-UX headers.
- **Shared memory:** MIT-SHM (`XShmPutImage`) works on HP-UX X11R6 and is detected at runtime. Provides faster video without X protocol copies.
- **No GCC:** The HP C compiler (`cc`) is ANSI C (flag `-Aa`). The `+e` flag allows the `register` keyword in ANSI mode.
