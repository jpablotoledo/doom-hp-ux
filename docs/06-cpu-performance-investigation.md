# Investigation: CPU/compiler optimizations for HP-UX PA-RISC

Architecture report on possible processor-instruction-level and
compilation-flag performance improvements for the HP Visualize B2000
(PA-RISC 2.0, chip 9000/785). Originally written as a pure analysis
document (nothing implemented yet); sections 1-7 below are that initial
analysis, unedited. See the results summary right below for what was
actually tried afterward.

## Results (post-analysis)

Of this document's recommendations, here's what was actually tried on the
real hardware, in a later session:

- **`+O3 +DA2.0 +DS2.0 +Ofastaccess`**: applied and confirmed - real
  performance improvement playing live ("much better now, very few
  stutters"). This is the current state of `HPFLAGS` in `src/Makefile`.
- **PBO (`+I`/`+P`)**: tried up to the point of compiling an instrumented
  binary; abandoned before completing the cycle on cost/benefit grounds
  - the user judged that the effort (instrument, play to generate the
  profile, recompile) wasn't worth the expected gain on top of what
  `+O3` already delivered.
- **`rtprio`: DO NOT USE without extreme care.** Tested at a "moderate"
  real-time priority (80) and **caused a full system hang** (screen,
  mouse, and keyboard all unresponsive; only ICMP ping still worked,
  confirming CPU starvation rather than a network failure or kernel
  panic). Required physically restarting the machine. The assumption
  that 80 was "moderate" on a single-core CPU with an uncapped render
  loop turned out to be wrong. **Not attempted again.** If this path is
  revisited in the future, start with a much lower priority and have
  physical access to the machine (not just telnet) before testing.
- **Hand-written MAX-2 (inline assembly)**: never attempted - left as a
  last resort given the effort involved, since `+O3` alone already
  delivered enough improvement for the session's goal.
- The six audio bugs fixed in the same session (see
  [`docs/05-music-investigation.md`](05-music-investigation.md), section
  10) are independent of this document - they're synthesis bugs, not CPU
  performance ones.

---

*From here on, the original document, unedited:*

## 1. Starting point (confirmed)

- **CPU**: PA-RISC 2.0, model designation `9000/785` (B/C-class family
  of HP Visualize workstations; the B2000 specifically carries a PA-8500
  in that line's most common configuration - **the exact chip revision
  (PA-8500 vs PA-8600) and the number of processors are still pending
  live confirmation**, see section 6).
- **Compiler**: HP C Compiler A.11.01.00 (`/usr/bin/cc`), no GCC
  installed.
- **Current flags** (`src/Makefile`, `HPFLAGS`):
  ```
  +O2 +Onolimit +e -Aa -D__BIG_ENDIAN__ -D_HPUX_SOURCE
  ```
  `+O2` is an intermediate optimization level; `-Aa` is strict ANSI C89
  mode (no extensions); there are no target-architecture flags
  (`+DA`/`+DS`), no interprocedural optimization, and no profile-guided
  optimization.

## 2. Processor SIMD extensions: MAX-2

The PA-8500/PA-8600 family (typical processors in `9000/785` stations)
implements **MAX-2** (Multimedia Acceleration eXtensions, version 2), an
integer SIMD instruction set that operates on the existing 64-bit
registers, treating them as packed 2×32-bit or 4×16-bit values. It lets
a single instruction do what normally takes 2-4 (saturated add/subtract,
shift, rounding "average", register-half permutation).

**Why this is relevant here specifically:**

- **Audio mixing** (`I_SubmitSound()` in `i_sound.c`, the "virtual-analog"
  formula that combines SFX and music sample by sample) is exactly the
  kind of operation MAX-2 accelerates: 16-bit integer sums/multiplies
  with range clamping, repeated thousands of times per second over a
  buffer.
- **Texture column rendering** (`r_draw.c`, Doom's "column drawer" that
  applies the lighting table pixel by pixel) is also a tight loop over
  bytes/words that on architectures with equivalent SIMD (MMX on x86 of
  the same era) benefited a lot from hand vectorization.

**Actual compiler support status on this machine:** HP cc A.11.01.00
dates from **1998** (see `docs/02-machine-setup.md`, patch `PHCO_95167`
from October 1998). Automatic **auto-vectorization** support toward
MAX-2 in HP's compiler arrived later (later compiler versions, well into
the 2000s, and more complete with `aCC`/C++ compilers and specific
vectorization flags). It's **unlikely this particular compiler version
auto-vectorizes** plain C code toward MAX-2 with optimization flags
alone. The two real paths to leverage MAX-2 with this compiler are:

1. **Inline assembly** (`asm()` with HP cc's syntax) for the concrete
   MAX-2 instructions (`HADD,SS`, `HSUB,SS`, `HAVG`, `HSHL`, `HSHR`,
   etc.) at the already-identified hot spots (audio mixing, column
   drawer). This is non-portable, PA-RISC-specific code, but the project
   already has precedent for platform-specific code (all of
   `i_sound.c`/`hp_music.c` under `#ifdef __hpux`).
2. Check whether `/opt/langtools` (HP development tools, visible in the
   `swlist` search PATH from section 6) brings a newer compiler version
   or a separate assembler/optimizer with better support - **pending
   live verification**.

**Risk/effort:** medium-high. Requires writing or adapting PA-RISC
assembly by hand, with no way to test on another machine (this project
is cross-developed/edited on Linux and only ever tested on the real
B2000). The expected benefit is real but limited to the two hot spots
mentioned - not a general engine-wide improvement.

## 3. Unused compiler optimization flags

HP cc has several optimization levels and sub-flags beyond `+O2` that
the project doesn't use today:

| Flag | What it does | Applicable here |
|---|---|---|
| `+O3` | Aggressive module-level optimization (more than `+O2`: better instruction scheduling, more inlining) | Yes, a direct candidate - try first, the lowest-risk change on this whole list |
| `+O4` | Interprocedural optimization (whole-program, requires linking with `+O4` too) | Possible, but changes the build flow (all `.o`s must be compiled and linked with `+O4` consistently) - more invasive |
| `+DA2.0` | Generates code specific to the PA-RISC 2.0 instruction set (instead of the generic/portable code used by default) | Yes - the binary already only runs on this machine, no reason not to pin the target architecture |
| `+DS785` (or the specific CPU model) | Instruction scheduling tuned to the exact chip pipeline | Yes, once the exact CPU model is confirmed (section 6) |
| `+Ofastaccess` | Assumes accesses to global/static data don't need full 32-bit indirection (relevant given this codebase's huge number of globals/statics, e.g. `mixbuffer`, Doom's zone, etc.) | Yes, a candidate - low risk |
| PBO (`+I` instrument, then `+P` recompile with the profile) | Recompiles using real data on which branches/loops run the most, improving code layout and branch prediction | Interesting but higher effort - needs an "instrument, play a bit, recompile" session on the real machine |
| `+Onolimit` | Already in use - removes the optimizable function size limit | - |

**Recommended trial order** (lowest to highest risk/effort):
1. `+O3` alone - a one-word Makefile change, low risk, measure FPS/CPU
   before/after with the same protocol (`vmstat`/`sar` already used in
   `docs/05-music-investigation.md`).
2. Add `+DA2.0 +DS<model>` once the exact CPU model is confirmed.
3. `+Ofastaccess`.
4. PBO, if the above aren't enough - the one requiring the most session
   effort (needs playing with the instrumented binary to generate the
   profile).
5. Interprocedural `+O4` - highest risk of breaking the build (parts of
   the project, like `fastlz`, compile as a separate library with their
   own flags; interprocedural `+O4` across modules built with different
   flags may not be safe).

**Known caution:** there's already a documented precedent in this same
repo (`docs/04-agregar-sonido.md`, problem P3) of raising optimization
from `-O` to `+O2 +Onolimit` changing the sprite renderer's array
overflow behavior, exposing a bounds bug that didn't show up at lower
optimization. Raising to `+O3`/`+O4` can expose similar bugs (latent UB
the previous optimizer didn't exploit) - test carefully, one flag at a
time, and play enough to pass through zones with lots of
enemies/sprites before calling a change good.

## 4. Process priority / real-time scheduling (HP-UX level, not CPU)

This isn't a processor instruction, but it's an HP-UX-specific
performance lever that wasn't thoroughly tested in the previous session
(only standard `nice` was tried, with limited results - see
`docs/05-music-investigation.md`, SIGALRM timer section).

Unlike Linux, HP-UX has a **genuine real-time scheduling class**
separate from `nice`: the `rtprio` command and the `rtsched()`/
`sched_setscheduler()` family of calls with the `RTPRIO`/`RTSCHED`
classes. Unlike `nice` (which only adjusts priority within the normal
time-sharing scheduling class, with the kernel still deciding quotas), a
true real-time priority gives the process **guaranteed scheduling
precedence** over any process in the normal class, including the
hardware-monitoring daemons identified in `docs/05-music-investigation.md`
section 8 (although those turned out not to be the cause of the
stutters, they do compete for CPU in the normal class).

**Concrete candidate to try:** launch `doom-hpux` with `rtprio
<priority> doom-hpux ...` (requires root privileges, already available
on this machine) instead of `nice`, and repeat the same measurement
protocol (`vmstat`/`sar` during a session with `-warp 1 1`) to compare
against the already-documented baseline. This directly attacks the
already-diagnosed real problem (`D_DoomLoop()`'s uncapped `while(1)`
loop, which needs consistent CPU to avoid accumulating jitter) in a way
`nice` cannot guarantee.

**Risk:** low-medium. Misused real-time scheduling can monopolize the
CPU and make the system non-interactive if the process enters an
infinite loop without yielding CPU - but given we already know the
engine does naturally yield CPU (blocking X11 calls, `select()`, etc.),
the practical risk is low. Try a moderate real-time priority first, not
the maximum.

## 5. Other lower-priority avenues, mentioned for completeness

- **64-bit mode (`+DD64`)**: PA-RISC 2.0 supports wide (64-bit) mode,
  but migrating the whole project to 64-bit is a large change
  (pointers, types, ABI) with uncertain benefit for this code (Doom
  isn't limited by address space or register width in its current
  calculations) - not recommended to pursue.
- **`Onolimit` variants / `+Oaggressive`**: newer HP compilers have
  `+Oaggressive`, but it's not confirmed whether A.11.01.00 supports it
  - pending verification (`cc -help` or `man cc` on the machine).
- **Data alignment / `#pragma pack`**: since the engine already
  carefully handles big-endian and packed structs (`i_sound.c`,
  `w_wad.c`), no clear additional gain was identified in this area.

## 6. Pending verifications on the real machine

This session lost connectivity to the B2000 before these points could
be confirmed live - left for the next time there's access:

1. **Exact CPU model**: `model` already confirmed `9000/785/B2000`, but
   the exact chip stepping (PA-8500 vs PA-8600) is still missing -
   relevant for picking the correct `+DS<model>` flag. Can be obtained
   with `/usr/sbin/print_manifest` or by checking
   `/opt/langtools`/`echo | cc -V`.
2. **Number of CPUs**: not confirmed whether the B2000 in this
   configuration is single- or dual-processor (`getconf
   NPROCESSORS_ONLN` or `ioscan -fnC processor`). If there's more than
   one processor, there's a completely different and higher-impact
   avenue: moving the audio timer (`I_HPAudioTick`) or the render
   itself to a separate process/thread pinned to the second CPU - but
   the current engine is single-threaded, so this would be a large
   change.
3. **Flags actually supported by `cc` A.11.01.00**: run `cc -help` or
   check the online manual (`man cc`) on the machine to confirm which
   items from the section 3 list apply to this specific compiler
   version (HP cc's flags changed quite a bit across versions through
   the 90s-2000s).
4. **`rtprio` availability**: confirm the command exists and that this
   install's kernel has the real-time subsystem enabled (`rtprio -l` or
   similar).

## 7. Recommended order of work (when resumed)

1. Confirm the section 6 points (quick, inspection only).
2. Try `+O3` alone - measure with the same already-established
   `vmstat`/`sar` + `-warp 1 1` protocol, comparing against the baseline
   documented in `docs/05-music-investigation.md`.
3. Try `rtprio` on the binary - it's independent of the compiler changes
   and can be tried in parallel/first, since it doesn't require
   recompiling.
4. If there's still room, add `+DA2.0 +DS<model>` and `+Ofastaccess`.
5. Hand-written MAX-2 (section 2) stays as a last resort, given the
   effort involved and that it targets specific hot spots rather than
   overall performance.
