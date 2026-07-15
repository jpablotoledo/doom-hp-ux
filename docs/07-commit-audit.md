# Commit audit: what changed and why (da4f9db6..HEAD)

Commit-by-commit report on the audio/performance work in the Doom-to-HP-UX
port, branch `adding-music-try2`. Details what each file was touched for,
and whether that change is still necessary in the binary that runs today.
Commits that only touched documentation are omitted, per explicit
instruction.

**Range**: `da4f9db61256acd2d9d918ee3ea500e820ed8275..HEAD`
**Commits in range**: 11 - **documentation-only (omitted)**: 3 -
**analyzed**: 8

## Summary

- **5 commits necessary / part of today's binary.**
- **1 commit fully replaced** by the next one (abandoned music
  architecture).
- **1 piece of correct code that never actually executes** in the
  distributed binary.
- **2 merges with no diff of their own** (Git bookkeeping, not engineering
  decisions).

| Commit | Message | Verdict |
|---|---|---|
| `c47ad96` | implementacion usando simpleAudio sobre Alib | Foundational |
| `a096d1a` | Cambios en i_sound mejora el rendimiento | Necessary |
| `afceff1` | Merge branch 'adding-sound' | Empty merge |
| `3191a4e` | change FreeDooM to DooM shareware, update docs and improvements | Mixed (partly necessary / partly dead code) |
| `49cd9d2` | Merge pull request #1 from jpablotoledo/improvements | Empty merge |
| `45c3bee` | many test, im have music, but not the best | Fully replaced |
| `7cb4f13` | music and doc | Foundational |
| `ea4b68d` | Habemus music | Necessary |

Omitted as documentation-only: `56a8ec5` (actualizacion source-changes),
`91c4c23` (translate documents), `423e720` (update doc).

---

## 1. `c47ad96` - sound implementation over Alib

*2011-06-30 · Verdict: **foundational***

**`src/Makefile`**
- `HPFLAGS` goes from `-O +e -Aa … -DDOOM_NO_SFX` to `+O2 +Onolimit +e
  -Aa …` - **removes `-DDOOM_NO_SFX`**, the macro that literally
  disabled compilation of the entire sound subsystem. This is the
  change that turns audio on for the first time in the port.
- Adds `-I/opt/audio/include` and `-L/opt/audio/lib -lAlib -lAt` to link
  against the HP-UX audio library (Alib).
- Adds `simpleAudio.o` to `OBJS` and its build rule.

**`src/i_sound.c`**
- Includes `simpleAudio.h` under `#ifdef __hpux`.
- `I_InitSound()`: adds the HP-UX branch that opens audio via
  `openAudio()`/`openAStream()` (Alib, 16-bit linear stereo).
- `I_ShutdownSound()`: adds the matching teardown (`closeAStream`/
  `closeAudio`).
- Extends the timer-sync branch that previously only covered `__sun` so
  it also covers `__hpux`.

**`src/simpleAudio.h`** (new)
- Declarations for the Alib wrapper provided by the HP-UX audio SDK
  (not original code - these are the system library's signatures, which
  `doom_build.sh` copies in at build time).

**`doom_build.sh`**
- `SCRIPT_DIR=\`dirname $0\`` → `SCRIPT_DIR=\`cd \`dirname $0\` && pwd\``:
  resolves the absolute path instead of a relative one (needed so `at -f
  script now` doesn't lose the real working directory).
- Adds the step that copies `simpleAudio.c` from the HP-UX install, with
  an explicit error if it's missing.

**Verdict**: necessary in full. Without this commit there is no sound -
it's the foundation everything else runs on.

---

## 2. `a096d1a` - write throttle and non-blocking I/O

*2011-06-30 · Verdict: **necessary***

**`src/Makefile`**
- `MAXSCREENWIDTH`/`MAXSCREENHEIGHT` from 1024×768 to 1280×800 - a
  screen-resolution change unrelated to the commit message (audio).

**`src/i_sound.c`**
- Adds `<sys/socket.h>`, `<netinet/in.h>`, `<netinet/tcp.h>` - none of
  the three are used in this commit's diff or anywhere in the rest of
  the reviewed history; they remain dead includes from this point
  forward.
- Implements the **wall-clock write throttle** in `I_SubmitSound()`:
  measures real elapsed time and only writes to Aserver if fewer than 2
  buffers are queued - fixes `I_SubmitSound` being called 35×/sec while
  the real playback rate was 11025 Hz, overflowing the Aserver buffer.
  This mechanism is still alive today, unchanged from its original
  form.
- Marks `audio_fd` non-blocking (`O_NONBLOCK`).

**Verdict**: necessary - the throttle is why audio doesn't progressively
degrade over time. Only weak spot: three `#include`s that were never
used, no functional cost but no purpose either.

---

## 3. `afceff1` - merge with no content of its own

*2011-06-30 · Verdict: **empty merge***

Merges the two previous commits (`c47ad96` + `a096d1a`) back into the
main branch. `git show afceff1 -- src/` produces no diff of its own - a
clean merge, no conflict resolution, no additional changes.

**Verdict**: history bookkeeping, not an independent code change.

---

## 4. `3191a4e` - the real WAD, and an optimization that never runs

*2026-07-06 · Verdict: **mixed** (build script necessary / i_video.c dead code)*

**`doom_build.sh`**
- Changes the WAD search from `freedoom1.wad` to `doom1.wad` (the real
  shareware WAD the project distributes) - a direct operational fix.
- Rewrites logging with a `log()` function that prints to the console
  and the file at once; switches from silent redirection to `tee` (build
  progress is visible live, not only at the end).
- Adds an explicit check that `hpdiy8` exists as the success signal,
  instead of trusting `make`'s exit code alone.

**`shareware/doom1.wad`**
- Adds the real binary WAD (4,196,020 bytes) - needed for the game to
  have content to load.

**`src/i_video.c`**
- Optimizes `Expand4()` (pixel doubling for the wider screen modes):
  removes a redundant temporary variable and replaces 4 repeated
  per-row manual writes with a single computation + `memcpy()` to
  replicate the remaining 3 output rows.
- **This code never runs in the distributed binary.** `Expand4()` is
  gated behind `#if (LD_PIXEL_DEPTH == 4)`, and that macro is only
  defined by the Makefile's `hp16` target
  (`SPECIALFLAGS='-DLD_PIXEL_DEPTH=4'`). `doom_build.sh` compiles
  exclusively with `make hp8`, which never defines `LD_PIXEL_DEPTH` -
  the preprocessor treats it as 0, and this entire branch is compiled
  out of the final binary.

**Verdict**: the `doom_build.sh` changes and the real WAD are necessary
and still in effect. The `i_video.c` optimization is correct in
isolation but **dead code** for how the project is actually built and
deployed.

---

## 5. `49cd9d2` - second merge with no content of its own

*2026-07-06 · Verdict: **empty merge***

`git show 49cd9d2 -- src/i_video.c doom_build.sh` produces no diff -
identical case to `afceff1`, brings `3191a4e` in from a PR branch with
no additional changes.

**Verdict**: history bookkeeping only.

---

## 6. `45c3bee` - the music architecture that got abandoned

*2026-07-10 · Verdict: **fully replaced one commit later***

**`src/Makefile`**
- Adds `-DMUSSERV="/opt/doom-hpux/musserver-hpux"`, the `HPMUSFLAGS`
  variable, and the `musserver_hp` target, which compiles
  `musserver_hpux.c` as a **separate executable**.

**`src/i_sound.c`** (+269 lines)
- `fork()` + two-pipe architecture: Doom launches `musserver-hpux` as a
  child process and talks to it over a command pipe, receiving PCM back
  over another.
- A 4096-*short* ring buffer to decouple reading the pipe from the
  Aserver write throttle.
- Diagnostic instrumentation: an SFX trigger counter logged to
  `logfile`, `musmix` statistics, and a temporary raw dump to
  `/tmp/aserver_capture.raw` - all explicitly diagnostic, not meant to
  stay.
- Rewrites MUS header parsing from a cast to a packed struct to explicit
  byte access - a real portability fix (packed-struct behavior in HP cc
  is compiler-specific). **This piece does survive**, reused verbatim in
  the next commit.

**`src/musserver_hpux.c`, `src/opl2.c`, `src/opl2.h`, `src/opl2test_nuked.c`**
- `musserver_hpux.c` (1117 lines): the OPL2 synth/MUS parser that runs
  as this commit's external process. Not part of the Doom binary - a
  separate executable.
- `opl2.c`/`opl2.h` (third-party Nuked-OPL2-Lite) + `opl2test_nuked.c`:
  a standalone reference/diagnostic tool, never linked into
  `doomengine`.

**Why it was abandoned**: the separate process - even though its own
computation is cheap - introduced enough scheduling/IPC latency on this
single-core machine to keep causing audible cuts under load, no matter
how much the ring buffer size was tuned. The commit message itself
already flags it as non-final ("not the best"), and one commit later
(`7cb4f13`) it's entirely replaced by an in-process synthesizer.

**What survived unchanged**: the byte-based MUS header parsing fix
(carried over verbatim). `opl2.c`/`opl2test_nuked.c` kept earning their
keep as a reference tool - it's exactly what this session used to
validate every one of the "screech" bugs against a cycle-accurate
emulator. `musserver_hpux.c` was left as dead weight in the tree: its
build target was removed in the next commit and the file is neither
compiled nor referenced from anywhere today.

**Verdict**: not a single line of the fork/pipe/ring-buffer architecture
in `i_sound.c` survives in `HEAD` - it was exploratory code necessary to
*learn* that path didn't work, but it contributes nothing to the current
binary besides the MUS parsing fix and the reference tools.

---

## 7. `7cb4f13` - the in-process synthesizer: today's foundation

*2026-07-10 · Verdict: **foundational***

**`src/Makefile`**
- Removes the `musserver_hp` target, the `-DMUSSERV` flag, and
  `HPMUSFLAGS` (undoes `45c3bee`'s build wiring). Adds `hp_music.o` to
  `OBJS`.

**`src/d_main.c`**
- Replaces the direct `I_UpdateSound()` + `I_SubmitSound()` calls in
  `D_DoomLoop()` with a single call to `I_HPAudioTick()` - a shared
  entry point also used by the `SIGALRM` timer.

**`src/i_sound.c`** (+461/-220 lines)
- Fully removes the fork/pipe/ring-buffer block from `45c3bee`.
- Rewrites the music API (`I_InitMusic`, `I_PlaySong`, `I_RegisterSong`,
  etc.) to call `HPMusic_*` directly.
- Adds `I_HPStartAudioTimer`/`I_HPStopAudioTimer`: a real
  `SIGALRM`/`setitimer` timer that feeds audio at a fixed rate,
  independent of how long the current render frame takes - with a
  `sig_atomic_t` reentrancy guard (no locks, safe inside a signal
  handler).

**`src/hp_music.c` / `src/hp_music.h`** (new, 509 lines)
- In-process OPL2/GENMIDI synthesizer + MUS parser - no `fork()`, no
  pipe, no second process for the OS to schedule around. This is the
  architecture still running today.

**Verdict**: the commit that actually fixed the stuttering music
problem - replaces the failed architecture with one that removes the
root cause (inter-process latency) instead of continuing to chase its
symptoms.

---

## 8. `ea4b68d` - CPU performance and the six "screech" fixes

*2026-07-12 · Verdict: **necessary***

**`src/Makefile`**
- `HPFLAGS`: `+O2` → `+O3 +DA2.0 +DS2.0 +Ofastaccess` - compiler
  optimization flags, confirmed as a real improvement by playing on the
  real hardware ("much better now, very few stutters").

**`doom_build.sh` + `launchers/*.sh`**
- Adds 3 additional launcher scripts (`-nomusic`, `-nomusic -nosound`,
  `-nosound`) alongside the default `doom.sh`.

**`src/i_sound.c`**
- `I_HPAudioTick()` now returns immediately if `SoundDisabled` - with
  `-nosound`, all audio mixing work is skipped, not just the output
  being muted.

**`src/i_system.c`**
- `kb_used` (the size of Doom's internal `Z_Malloc` zone): 32MB →
  128MB. Investigated as a possible cause of a periodic stutter ("hard
  drive is working"); that hypothesis was later ruled out in the same
  session (the stutter persists even without music and independent of
  this change). Kept because it's a safe, low-risk improvement, not
  because it fixed what it was originally suspected of causing.

**`src/hp_music.c`** (+155 lines) - this session's six fixes
- **MUS percussion mapping**: `128 + (note & 0x3F)` with a clamp to the
  last instrument → `128 + (note - 35)` with valid range [35,81],
  verified against Chocolate Doom.
- **OPL2 feedback formula**: doubling the last sample → summing the
  last two, verified against Nuked-OPL2.
- **Anti-aliasing frequency clamp** in `note_phase_step()`: prevents
  fixed-note percussion instruments from exceeding Nyquist (11025/2 Hz)
  and folding back - the real confirmed cause of the screech in the
  hi-hat (instrument 139).
- **`rate_to_inc_release()`**: caps the maximum decay/release fall rate
  to a minimum of 48 samples (~4.3ms), avoiding a linear envelope
  cutting off hard in a single sample.
- **One-pole DC-blocking filter** in `opl_mix_sample()`: removes the
  intentional DC bias of several OPL2 waveforms (half sine, rectified
  sine, quarter sine), which otherwise produced an audible level jump
  on every new note.
- **Fixed mixing divisor (÷4)** instead of dividing by the number of
  currently active channels - the real dominant cause of the residual
  screech: every new note instantly diluted the volume of the ones
  already playing.

**`.claude/settings.json`**
- Claude Code tooling configuration - not game code.

**Verdict**: necessary in full for the current state (performance + the
six audio bugs), with the one caveat that `kb_used` is kept out of
caution, not because it was the cause of anything it was originally
attributed to.

---

## Synthesis

1. **Nothing that runs today is surplus.** The chain c47ad96 → a096d1a
   → 3191a4e (build script) → 7cb4f13 → ea4b68d is a linear progression
   where each link depends on the previous one and remains in effect in
   `HEAD`.
2. **One entire commit (`45c3bee`) was written, tested, and discarded
   within the same work cycle.** That doesn't make it "unnecessary" in
   the sense of carelessness - it's how it was empirically discovered
   that the separate-process architecture wasn't viable on this
   hardware, and two of its byproducts (the MUS parsing fix, the
   `opl2.c`/`opl2test_nuked.c` tools) still add value today. But
   `musserver_hpux.c` (1117 lines) is dead weight in the tree: it isn't
   compiled, isn't referenced, and could be removed with no effect.
3. **The `i_video.c` optimization in `3191a4e` never executes** in the
   binary that `doom_build.sh` actually builds and deploys (the `hp8`
   target, with `LD_PIXEL_DEPTH` undefined). It's the only piece of
   "productive" code in the whole range that can genuinely be called
   unnecessary for the final result, beyond being correct on its own
   terms.
4. **Two merges (`afceff1`, `49cd9d2`) add or remove no code** - they're
   normal Git history artifacts, not engineering decisions to evaluate.

Taken together: the volume of changes is better explained by *iterative
learning on real hardware with no way to simulate locally* (every audio
architecture was validated by playing on the physical machine) than by
superfluous changes. The one real candidate for "shouldn't have been
touched" is `musserver_hpux.c`, which can be deleted from the repo with
confidence if reducing surface area matters.
