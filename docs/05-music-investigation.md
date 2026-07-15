# Investigation: implementing music for HP-UX

Architecture report on the current state of music in the port and what's
needed to implement it, both at the game level and the operating system
level (HP-UX / HP Visualize B2000). This is an analysis document - it
does not describe a completed implementation.

## 1. Current state of the code

Music is **not implemented**. This is a structural problem, not
HP-UX-specific: the low-level layer was never finished for any
`Makefile` target, not even Linux.

- **`src/s_sound.c`**: complete and correct game-level layer.
  `S_ChangeMusic()` locates the `d_<name>` lump via `W_GetNumForName`,
  caches it with `W_CacheLumpNum(..., PU_MUSIC)`, and calls
  `I_RegisterSong()` / `I_PlaySong()`. `S_Start()` computes the music
  per level/episode on every map change. All of this always runs, with
  no compile-time flags.
- **`src/i_sound.c`, "MUSIC API" section** (line ~1372): every function
  (`I_InitMusic`, `I_PlaySong`, `I_RegisterSong`, `I_PauseSong`,
  `I_StopSong`, etc.) has its real body wrapped in `#ifdef MUSSERV`.
  **`MUSSERV` is never defined** in `src/Makefile` (not in the HP-UX
  target, not in any other). Outside that `#ifdef` the functions are
  no-ops or fake state (`I_PlaySong` only does `musicdies = gametic +
  TICRATE*30`; `I_UnRegisterSong` is completely empty).
- The original `MUSSERV` design pointed to an **external process**
  launched with `popen("some-binary", "w")` and a one-character
  protocol (`R` play/resume, `P` pause, `S` stop, `Q` quit, `N%d` + MUS
  data to register a song, `V%i` volume). That external binary never
  existed in the repo, and it was never documented what it was.
- Unlike SFX, **there is no `#elif defined(__hpux)` block for music at
  all** in `i_sound.c`.

### Assets available in the WAD

Verified directly against `shareware/doom1.wad`: it contains **13
`D_*` lumps** in MUS format (E1M1–E1M9, INTER, INTRO, VICTOR, INTROA)
plus the **`GENMIDI`** lump (11,908 bytes, DMX's standard FM instrument
bank). All the data needed is already in the WAD distributed with the
project - nothing needs to be downloaded or converted externally. MUS is
an id Software/DMX proprietary format (similar to MIDI but compressed);
the MUS header parser (`struct musheader_s`) exists in the code but is
dead inside `#ifdef MUSSERV`.

## 2. Verification on the real machine (HP Visualize B2000, over telnet)

- **Alib/Aserver is a pure PCM channel**: `/opt/audio/include/Alib.h` and
  `Audio.h` were checked directly on the machine - zero MIDI-related
  symbols. `/opt/audio/bin/` only contains `Aserver`, `asecure`,
  `attributes`, `convert`, `send_sound`. No MIDI synthesizer, internal
  or external, is installed (`swlist`, filesystem `find`: negative.
  **This rules out any MIDI-gateway option** - the only viable path is
  generating PCM in software and playing it through the same channel SFX
  already use (`openAStream`/`write(audio_fd, ...)`).
- **Available resources**: `/` with ~103 MB free, `/opt` ~3.2 GB free,
  `/tmp` ~424 MB free; idle CPU (load average 0.02) outside of gameplay.
  Enough headroom for either pre-rendered PCM (a few MB per track) or an
  additional real-time synthesis process.
- The system runs **HP-UX B.11.00, PA-RISC 2.0 (9000/785, big-endian)**,
  compiler **HP cc A.11.01.00 in ANSI `-Aa` mode** (no GNU extensions,
  no C99).

## 3. Key finding: there was already a complete first attempt, abandoned

There's a branch `adding-music` (commit `974a77b "music with noise"`)
that implements **an end-to-end real-time OPL2 synthesis architecture**:

- `src/musserver_hpux.c` (1093 lines): a separate process with its own
  OPL2 emulator (9 FM channels, ADSR envelope, 4 waveforms, feedback
  modulation), that parses the real `GENMIDI` lump and real MUS data.
- Modified `src/i_sound.c`: uses `fork()` + two pipes (instead of
  `popen()`) so the music process writes PCM to a pipe that Doom reads
  with a **ring buffer** and mixes in real time with the SFX buffer
  before writing to the same Aserver `audio_fd` - solving the "single
  stream vs. several simultaneous streams" problem in favor of a single
  mixed stream.
- Already fixed a real HP-compiler portability issue:
  `__attribute__((packed))` (rejected by `cc -Aa`) was replaced with
  byte-by-byte reading of the MUS header.
- The `musserver-hpux` binary is still installed in `/opt/doom-hpux/` on
  the real machine, and a recent session log (`doom.log`) shows the
  mechanism working (pipes, throttling, mixing, with consistent stats:
  `musmix: calls=700 throttled=278 read_ok=422 ... hp_queued=1217`) -
  but the commit message itself states the audible result was
  **noise**, not clean music. There are 16 debug WAV files in
  `test_audio/` (names like `..._ENDIANNESS_FIXED`,
  `..._fixed_retrigger`, `..._crossfade`, `poly_1chan`→`poly_9chan_all`)
  documenting a long debugging session on polyphony, note retriggering,
  and endianness that never got closed before restarting on the current
  branch (`adding-music-try2`), which contains none of that code.

## 4. Validation analysis: does the synthesizer already work?

Doom's music isn't generic MIDI: it's **MUS**, an id Software/DMX
proprietary format very similar to MIDI but compressed. The `GENMIDI`
lump isn't a MIDI sound bank either - it's literally **OPL2 chip
register bytes** (the FM synthesizer chip on 90s AdLib/SoundBlaster
cards). That's why "building a MIDI player" to validate before touching
the game means, in practice, building an OPL2 emulator that consumes
those registers.

That validation player **was already built twice** inside commit
`974a77b`, and the resulting WAVs stayed in the git history
(`test_audio/`). They were extracted and run through spectral analysis
(spectral flatness: 0 = tonal/music signal, 1 = white noise) to
objectively determine, without needing to listen, at which stage the
signal breaks:

| File | Stage | Spectral flatness | RMS | Clipping | Reading |
|---|---|---|---|---|---|
| `1_piano_bass_notes.wav` | Home-grown OPL2 emulator, isolated simple notes | 0.016 (very tonal) | 11771 | 0% | Clean |
| `9_d_intro_all_fixes.wav` | Home-grown OPL2 emulator, full real song | 0.846 (almost pure noise) | 16748 | 0.08% | Noise |
| `10_..._NUKED_real_emulator.wav` | Switched to Nuked-OPL2 (cycle-accurate emulator, verified against the real YM3812 chip) | 0.795 (noise) | 18954 | 0.16% | Noise (still buggy) |
| `11_..._ENDIANNESS_FIXED.wav` | Nuked-OPL2 + endianness fix, isolated (not going through Doom) | 0.050 (tonal) | 3397 | 0% | Clean |
| `14_..._after_ringbuffer_fix.wav` | Real capture from the full pipeline (Doom→pipe→ring buffer→mix→Aserver) | 0.172 (partially tonal, dominated by 0-120 Hz) | 4550 | 0% | Degraded, not pure noise but suspicious |

**Reading:**

1. The home-grown OPL2 emulator (`src/opl2test_hpux.c`) played isolated
   simple notes correctly, but failed on real polyphonic songs (confirms
   the code's own comment: *"dense broadband noise"*).
2. Replacing it with **Nuked-OPL2-Lite** (`src/opl2.c`/`opl2.h`,
   cycle-accurate emulator verified against real hardware, LGPL 2.1
   license) fed directly with `GENMIDI`'s register bytes -
   architecturally correct, since GENMIDI bytes *are* OPL2 registers -
   and fixing an endianness bug, gave a result that's **clean and tonal
   in isolation** (file 11). The synthesis engine is, in practice,
   solved.
3. The real capture from the full pipeline (file 14: child process →
   pipe → ring buffer → mix with SFX → `write()` to Aserver) instead
   shows a degraded signal, dominated by very low frequencies (0-120
   Hz) - a **different** bug, in the integration stage (reading the
   pipe, sample format/ordering in the ring buffer, or the
   "virtual-analog" mixing formula in `I_SubmitSound()`), not in the
   synthesis itself.

**Conclusion:** there's no need to build a test player from scratch -
one already exists and already demonstrated the synthesis works (file
11). The real, bounded problem is in the last stretch of the pipeline
(pipe → ring buffer → mix → Aserver), the newest and most fragile part
of `i_sound.c`. The cheapest next validation step is isolating that
stage (capturing what comes out of the pipe *before* mixing) instead of
rebuilding the synthesizer.

## 5. Options considered

| Option | Description | Risk | Reusable work |
|---|---|---|---|
| **A. Resume `adding-music`** | Debug the integration stage (pipe/ring buffer/mixing/Aserver) starting from the fact that OPL2 synthesis (Nuked-OPL2 + GENMIDI) is already validated in isolation | Low-medium - the problem is already narrowed to a specific stage | ~2500 lines + hours of debugging already documented in `test_audio/`, including the working synthesis |
| **B. Offline pre-render to PCM** | Convert the 13 MUS tracks to PCM/WAV outside of HP-UX (reusing Nuked-OPL2 itself, already proven correct) and play them back via simple streaming, same as SFX | Low - no real-time synthesis, no CPU risk or emulation bugs | The already-validated Nuked-OPL2 emulator (file 11) would be reused the same way, only how the result is played back changes |
| **C. Gateway to an external MIDI synthesizer** | Discarded - no MIDI backend exists on the machine (verified) | - | - |

## 6. Live debugging session (2026-07-10): two real bugs found and fixed

It was decided to validate synthesis directly on the real hardware
before touching the game: `src/opl2.c`, `opl2.h`, `opl2_stdint_hpux.h`,
and `opl2test_nuked.c` were brought over from commit `974a77b` to the
current branch (`adding-music-try2`), and a repeatable workflow against
the B2000 over telnet/FTP was established (no SSH/SCP access
available):

1. Edit the `.c` locally.
2. Upload via FTP to `/tmp/opltest/` on the B2000.
3. Compile and run with `cc -Ae +O2 ...` launched via `at -f script now`
   (avoids `ccom` dying from `SIGHUP` when the telnet session closes -
   the same problem P3 already documented in `docs/02-machine-setup.md`).
4. Download the resulting `.wav` via FTP to `test_audio/loop/` in this
   repo.
5. Listen and/or run spectral analysis, iterate.

With this cycle, listening directly to the WAVs (not just the automatic
spectral analysis, which doesn't catch pitch shifts), two real bugs
were found that the previous attempt (`adding-music`) had not resolved:

### Bug 1: incorrect MUS tempo (70 Hz instead of 140 Hz)

Rendering `D_E1M1` ("At Doom's Gate") in isolation, the result didn't
sound like noise but didn't resemble the real music either: it sounded
very slow and low-pitched, like a record played at fewer RPM.
`opl2test_nuked.c` used `MUS_TEMPO_HZ 70`, but Doom's MUS format defines
its events at a fixed rate of **140 Hz** (not 70). Every music event
ended up playing for twice as long as it should. Fixed by changing the
constant to 140.

### Bug 2: misinterpreted `GENMIDI` structure

With the tempo fixed, a new "instrument sweep" was tried (`--instruments`
added to `opl2test_nuked.c`, plays each of the 175 `GENMIDI` instruments
in sequence on a fixed note) to isolate the timbre problem from the
full-song problem. Result: every instrument sounded the same, like
simple "beeps" with no recognizable timbre, and one of them (instrument
39, Synth Bass 2) sounded like noise/static.

It was verified against Chocolate Doom's real source code
(`src/i_oplmusic.c`, `LoadOperatorData`/`LoadInstrumentTable`) that the
real structure of each instrument in the `GENMIDI` lump is:

```
8-byte header "#OPL_II#"
175 × 36-byte genmidi_instr_t (128 melodic + 47 percussion):
    flags(u16 LE) + fine_tuning(u8) + fixed_note(u8) + 2 voices of 16 bytes
    each voice = modulator operator(6) + feedback(1) + carrier operator(6)
             + unused(1) + base_note_offset(s16 LE)
    each operator(6) = tremolo, attack, sustain, waveform, scale, level
175 × 32-byte instrument names (unused for playback)
```

Total: 8 + 175×36 + 175×32 = 11,908 bytes - matches exactly the real
lump size verified in the WAD.

The previous code instead assumed each instrument occupied 68
consecutive bytes (36+32 merged) and that each operator had only 5
already-combined fields. This "accidentally" read instrument 0 almost
correctly (the starting offset lines up), but drifted further
out of alignment for later instruments, eventually reading bytes
straight out of the name section (ASCII text) as if they were OPL2
registers - exactly what produced the "beep with no timbre" (the
modulator operator never got loaded with coherent data) and the
occasional "TV static" for specific instruments where the
misaligned read landed on extreme feedback/waveform values. In
addition, the `scale` and `level` fields are two **separate** bytes
that must be combined with OR for register 0x40 (KSL + output level),
not a single already-combined byte as the code assumed.

Fixed by rewriting the access macros and `opl_load_instrument()` in
`src/opl2test_nuked.c` with the real offsets. Result: instruments sound
recognizably distinct (piano, organ, guitar, bass) and full `D_E1M1`,
with both fixes, is recognizable as real Doom music ("sounds like Doom"
- confirmed by listening to the WAV).

The WAVs from every iteration of this session were kept in
`test_audio/loop/` (files `01_...` through
`06_e1m1_tempo_and_genmidi_fixed.wav`) as reference.

## 7. Extending isolated validation: percussion and more songs

With both fixes from section 6 applied, the following were also tested:

- Percussion sweep (GENMIDI indices 128-174, the 47 drum/cymbal/etc.
  sounds mapped by fixed note on MIDI channel 15): confirmed by ear
  ("sounds very good").
- `D_E1M8` (the shareware's longest/densest track, a good polyphony
  stress test): confirmed by ear, tonal and clipping-free.

With this, isolated synthesis was fully validated: tempo, GENMIDI
structure, melodic instruments, percussion, and full songs.

## 8. Integration with Doom: from "works but cuts out" to solved

### 8.1 First attempt: porting the fixes to `musserver_hpux.c` (separate-process architecture)

`musserver_hpux.c`, `i_sound.c`, and the Makefile target from commit
`974a77b` were brought over (the original `adding-music` architecture:
Doom does `fork()` + pipes to a separate `musserver-hpux` process that
synthesizes and sends PCM back). The same two fixes from section 6
(140Hz tempo, 36-byte GENMIDI structure) were applied to
`musserver_hpux.c`'s home-grown OPL2 synthesizer (note: that home-grown
synthesizer, not Nuked-OPL2 - a comment in the code itself explains that
Nuked-OPL2 inside the live server ended up consuming ~84% CPU,
unworkable alongside the ~85% Doom's own render already uses).

Compiled and tested in a separate test directory (`/tmp/doombuild-test`
on the B2000, without touching the production `/opt/doom-hpux`) via the
same telnet/FTP/`at` cycle. Result heard live on the machine: **it's
already real music**, but the game "tends to hang" and the music cuts
out.

### 8.2 Diagnosing the stutter: buffer, not synth CPU

Live CPU profiling (`ps -eo pid,pcpu,comm` sampled every second) over
20s of gameplay showed: `musserver-hpux` settles at **under 1% CPU**;
`doom-hpux` alone climbs to ~18-19% accumulated over the same span -
consistent with the game's render already being heavy before adding
music. The `musserver` log showed a growing and high frame-drop rate
(`dropped` climbing with no ceiling). Conclusion: Doom wasn't draining
the music pipe with the regularity needed because its own render loop,
already overloaded, wasn't calling `I_SubmitSound()` (which drains the
pipe) at the assumed frequency.

The music ring buffer (`MUSIC_RING_SIZE` in `i_sound.c`) was enlarged
from ~185ms → ~3s → ~12s. Verified with real captures of what's sent to
Aserver (`/tmp/aserver_capture.raw`, dumped by `i_sound.c` itself during
the first ~15s, converted to WAV locally respecting PA-RISC's
big-endianness): cuts dropped from 24.2% → 4.4% of the audio, but
enlarging the buffer further stopped helping (a ceiling was hit).
Isolating with `-warp 1 1` (starts directly in E1M1, no song changes,
where most of the drops concentrated) the cut level was already
approaching the **score's own natural silence** (7.0% measured on the
clean reference validated in section 6, vs. 6.1-8.8% measured live
depending on the run) - meaning the pipeline was already practically at
the limit of what a larger buffer could fix.

### 8.3 Real timer (SIGALRM/setitimer) to decouple audio from render

A real OS timer was activated, analogous to the generic `SNDINTR`
mechanism that already existed in the code for other platforms (never
enabled for HP-UX) but adapted so it wouldn't step on the music work
already done: `I_HPStartAudioTimer()`/`I_HPStopAudioTimer()` in
`i_sound.c` install a periodic `SIGALRM` that calls `I_UpdateSound()` +
`I_SubmitSound()` directly, without depending on the main game loop
invoking them in time. `d_main.c` blocks `SIGALRM` with `sigprocmask`
around its own synchronous calls so they never overlap with the timer.
This further improved the pipe numbers (`dropped` fell to 3-4,
practically the minimum possible) but the user was still hearing cuts
live.

### 8.4 Final refactor: in-process synthesizer, no fork/pipe (`hp_music.c`)

Faced with the reasonable doubt "can this machine really not play music
while running the game?", the underlying architecture was reconsidered:
`musserver-hpux` ran as a **separate process** - even though its own
computation is cheap (<1% CPU), the *cost of inter-process
communication* (fork, pipes, waiting for the OS scheduler to give a
second process time) on a single-core machine can outweigh the
synthesis itself.

`src/hp_music.c` (+ `hp_music.h`) was created: the same OPL2 synthesis
engine and MUS parser from `musserver_hpux.c` (with the tempo/GENMIDI
fixes already included), but restructured from a "push to a pipe" model
to a "generate N samples on demand" model (`HPMusic_Generate(buf, n)`),
called as a plain function directly from `I_SubmitSound()` - no
`fork()`, no `pipe()`, no `popen()`, no second process for the OS to
schedule. `i_sound.c` was rewritten so `I_InitMusic`/`I_PlaySong`/
`I_RegisterSong`/etc. call `HPMusic_*` directly instead of sending text
commands over a pipe. The `musserver_hp` target and the `-DMUSSERV=...`
flag were removed from the `Makefile` (no longer used on HP-UX).

Implementation note: since `I_SubmitSound()` can now run inside a real
signal handler (`SIGALRM`), the diagnostic dump to
`/tmp/aserver_capture.raw` was rewritten using raw `open()`/`write()`/
`close()` instead of `fopen()`/`fwrite()` - stdio functions aren't safe
to use inside a signal handler (`fprintf` no longer appears anywhere in
the audio hot path, explicitly verified).

Compiled and tested the same way as previous steps: no errors. A real
Aserver capture with this architecture gave **6.7% silence - practically
matching the score's intrinsic 7.0%**. At the signal level, the audio
pipeline no longer has meaningful room for improvement.

### 8.5 Conclusion: the stutter predates the music work

The user reported that, even with this architecture, they were still
hearing cuts live. The decisive test was run: play the same level
**with no music** (`-nomusic`) and compare. Result: **the game stutters
exactly the same with no music**, and the stutter persists even at a
lower screen resolution. This confirms the perceived stutter/cut **has
nothing to do with this session's music work** - it's a pre-existing
performance characteristic of the Doom engine on this specific hardware
(already known from `docs/02-machine-setup.md` that rendering alone,
with no audio at all, runs at ~85% CPU on this machine). Any future work
on that stutter is a general engine/hardware performance problem,
outside the scope of this investigation.

## 9. Final state

**Music working correctly, final architecture: in-process OPL2/GENMIDI
synthesizer (`hp_music.c`), no external process, fed by a real timer
independent of rendering.** Validated by ear on the real hardware:
correct tempo, recognizable instruments, percussion, full songs. The
lingering "stutter" feeling is a general Doom performance problem on
this machine, not a music problem - confirmed by playing with no music
and seeing the same stutter.

Key files of the final implementation:
- `src/hp_music.c` / `src/hp_music.h` - in-process OPL2 synthesizer/MUS
  parser.
- `src/i_sound.c` - Music API (`I_InitMusic` etc.) calling `HPMusic_*`;
  `SIGALRM`/`setitimer` timer (`I_HPStartAudioTimer`/
  `I_HPStopAudioTimer`).
- `src/d_main.c` - blocks `SIGALRM` around the main loop's synchronous
  calls to `I_UpdateSound()`/`I_SubmitSound()`.
- `src/Makefile` - `hp_music.o` added to the build; `musserver_hp`
  target and `-DMUSSERV` flag removed (no longer apply).

Pending (out of scope for this investigation): investigate the engine's
general performance on this hardware, independent of audio.

## 10. Investigating the residual "screech" (CPU optimization session)

After the CPU optimization work (`+O3 +DA2.0 +DS2.0 +Ofastaccess`, see
`docs/06-cpu-performance-investigation.md`), with the engine's stutters
already greatly reduced, the user reported a different problem that had
until then been masked by the stutters: an intermittent **screech** in
E1M1's music ("like when a speaker is loose or making bad contact"),
initially confused with the engine stutters (section 8.5) but identified
as a real synthesis problem once the game stopped stuttering.

### 10.1 Methodology: isolating synthesis from real time

To answer "does the noise come from running the game or from the
instrument implementation?" an **offline renderer** was built (not part
of the Doom build, it only lives as a session diagnostic tool): loads
the `GENMIDI` lump and a `MUS` song directly from `shareware/doom1.wad`,
and calls `HPMusic_Init`/`HPMusic_LoadSong`/`HPMusic_Generate` - the
same code that runs in the game - to write a `.wav`, with no Doom
engine, no `SIGALRM` timer, no real-time constraints. Any noise that
shows up there is a bug in the synthesis itself, not in how it
integrates with the game.

With that tool, several test `.wav`s were generated (`test_audio/loop/`
folder, files `14` through `18`) that the user listened to directly and
used to pinpoint problems by ear and, in one key case, by inspecting the
waveform in Audacity.

`opl2test_nuked.c` + `opl2.c` (Nuked-OPL2-Lite, the cycle-accurate
emulator validated against real YM3812 hardware, see section 5) was also
used as **ground truth**: rendering the same instrument/note with the
home-grown synthesizer and with Nuked-OPL2 and comparing made it
possible to tell "this really happens on real hardware" apart from
"this is a bug in our synthesis."

### 10.2 Bug: incorrect MUS percussion mapping

MUS channel 15 (percussion) mapped the event's "key" to a GENMIDI
instrument with `128 + (note & 0x3F)`, clamping to the last instrument
(index 174) if out of range. The real formula (verified against
Chocolate Doom's `i_oplmusic.c`, function `KeyOnEvent`) is:

```c
if (key < 35 || key > 81) return;      /* out of range: ignore the note */
instrument = &percussion_instrs[key - 35];
```

That is: valid range `[35, 81]` (47 percussion sounds), index `key -
35`, and **ignore** (don't play) any note outside that range instead of
clamping to an arbitrary instrument. The bug made completely different
percussion instruments play than what the song asked for. Fixed in
`hp_music.c`, function `mus_process_tic()`.

### 10.3 Bug: incorrect OPL2 feedback formula

The modulator's feedback (register `fb`, used by several percussion
instruments with `fb=7`, the maximum) was computed as double the single
previous raw sample: `fb_idx = (fb_prev * 2) >> (9 - fb)`. The real chip
(verified against Nuked-OPL2's `OPL2_SlotCalcFB()`) uses the **sum** of
the last two samples: `fbmod = (prout + out) >> (9 - fb)`. Doubling a
single sample amplifies rather than damps any sign change between
consecutive samples, which at high feedback can run away into
uncontrolled oscillation. Fixed by storing the last two samples
(`fb_prev`, `fb_prev2`) and summing them. Technically more correct,
though in practice it didn't turn out to be the dominant cause of the
screech for the specific case investigated (see 10.4).

### 10.4 The real bug behind the "static hi-hat": Nyquist aliasing

Isolating percussion instrument 139 (open hi-hat, GENMIDI fixed note =
79), sample-to-sample jumps of up to 18516 (57% of the full range) were
measured when rendered with the home-grown synthesizer - something
Nuked-OPL2, rendering the same instrument/note, showed none of at all
(maximum 3579). The cause: that instrument's fixed note, with its
multiplier (`mult`), gives a carrier frequency of **~7840 Hz** - well
above the Nyquist limit at this synthesizer's sample rate (11025 Hz →
Nyquist = 5512 Hz). A frequency above Nyquist "folds back" (fold-back
aliasing) into a completely different, inharmonic frequency inside the
audible range.

Real hardware / Nuked-OPL2 don't suffer this because they synthesize
internally at a much higher rate (~49716 Hz) and resample with a proper
low-pass filter on the way down to output rate. Implementing
oversampling with filtering here would have been CPU-expensive (exactly
what this machine doesn't have to spare, see
`docs/06-cpu-performance-investigation.md`), so a cheaper limit was
applied instead: capping the maximum representable frequency just below
Nyquist in `note_phase_step()` (`freq > SAMPLE_RATE*0.45` gets clamped
to that value). The cost is pitch accuracy on a handful of extreme
percussion instruments (which already sound like noise/metallic timbre
by design, where the exact pitch is inaudible/irrelevant); the benefit
is eliminating the aliasing entirely. With the fix, instrument 139's
sharp jumps in isolation went from 70 to 0.

### 10.5 Bug: hard envelope cutoff (linear release/decay)

Inspecting the waveform in Audacity, the user identified hard vertical
drops at the end of every note - not an instrument problem but how the
amplitude ends. This synthesizer's envelope is **linear** (a 0-511 scale
subtracted at a constant per-sample rate), and for fast decay/release
rates (range 12-15 of the `rate_to_inc` table, up to 512 per sample) it
can go from near-maximum amplitude to zero in **a single sample** - an
instant cutoff, audible as a "tick". The real OPL2 chip works its
envelope in the logarithmic (dB) domain, where the same "fast release"
gives a multiplicative decay that naturally tapers as it approaches
zero, without the linear model's hard edge.

Instead of rewriting the whole envelope model into the logarithmic
domain (a much more invasive change for a narrow benefit), the maximum
decay/release fall rate was capped so no segment can complete in fewer
than `ENV_REL_MIN_SAMPLES` = 48 samples (~4.3 ms at 11025 Hz) - still
fast/imperceptible as a fade, but no longer a single-sample
discontinuity. New function `rate_to_inc_release()` used for both
`env_dec` and `env_rel`, on both the modulator and the carrier.

### 10.6 Bug: DC bias (waveform asymmetry)

The user noticed, looking at the waveform in Audacity, that the mixed
signal sat mostly in the positive half of the Y axis instead of
oscillating symmetrically. Several OPL2 waveforms (`opl_wave()` cases
1-3: half sine, full rectified sine, quarter sine) are, **by chip
design**, asymmetric - their average isn't zero (this is intentional,
part of the timbre for instruments with distortion/edge, like E1M1's
distorted guitar). On real hardware this isn't audible as bias because
the audio output has a coupling capacitor (analog high-pass filter)
that removes any DC component before it reaches the speaker. This
software synthesizer writes raw samples directly, with no such
filtering - and an unfiltered DC bias isn't just a visual problem: every
time a note with an asymmetric waveform starts or stops, the DC level
jumps abruptly, and a DC jump is itself a broadband transient (a
"click").

A classic one-pole DC-blocking filter was added
(`y[n] = x[n] - x[n-1] + R*y[n-1]`, R≈0.9986, cutoff ≈2 Hz) applied to
the final already-mixed sample in `opl_mix_sample()`. E1M1's first
minute's average DC level dropped from 5738 to 72 (practically zero) on
a ±32767 scale.

### 10.7 The real bug behind the residual screech: dynamic-divisor mixing

With the previous fixes applied, the screech persisted. Another waveform
inspection by the user showed an **abrupt drop mid-envelope**, with no
channel-retrigger event nearby (ruled out by instrumenting and logging
every channel-steal/reuse event: there were none in the song's first
minute). The real cause: `opl_mix_sample()` summed the samples from
every active OPL2 channel and divided by **the number of channels active
at that instant** (`sum /= n`). Every time a new note activated a
previously-idle channel - even if that note was just starting its own
attack and contributing almost no volume yet - `n` jumped up, instantly
diluting the volume of **every other** note already playing. Same thing
in reverse when a channel went silent. The result is a volume "pump"
(ducking) every time a voice enters or leaves - a defect of the mixing
strategy, not of any specific instrument, and consistent with the
hundreds of note events per minute typical of any song.

Fix: divide by a **fixed divisor**, not by the count of active channels,
so a channel's own volume doesn't depend on other channels turning on or
off. The divisor value was tuned empirically by measuring sharp jumps
(>5000 difference between consecutive samples) and RMS over E1M1's first
minute rendered offline:

| Divisor | Sharp jumps (60s) | Peak | RMS |
|---|---|---|---|
| 2 | 816 (bad, similar to the original bug) | 32114 | 7716 |
| 3 | 93 | 27932 | 5275 |
| **4 (chosen)** | **1** | **27018** | **3956** |
| 5 | 0 | 21627 | 3163 |
| 9 (=NUM_OPL_CHAN, clipping never possible) | 0 | ~12045 | ~1758 (too quiet) |

Compensating volume with a 2x gain applied at the final mix with SFX
(`i_sound.c`) was tried first, but at the user's suggestion a single
volume parameter in a single place was preferred: adjusting
`hp_music.c`'s divisor directly (from 5 to 4) instead of keeping two
volume controls in two different files. Divisor 4 gives just one
residual sharp jump in 60 seconds (practically inaudible) with
noticeably more volume than 5.

### 10.8 Final state (screech)

With the six fixes from section 10 combined (percussion, feedback,
Nyquist anti-aliasing, envelope without a hard cutoff, DC-block, fixed
mixing divisor), the user confirmed on the real hardware: "feels much
better" / the annoying noise is no longer perceptible. Music volume
ended up balanced against sound effects after adjusting the divisor from
5 to 4.

Files modified in this section: `src/hp_music.c` only
(`mus_process_tic()`, `note_phase_step()`, `load_instrument()`,
`render_channel()`, `opl_mix_sample()`, new function
`rate_to_inc_release()`). `src/i_sound.c` required no final changes (an
additional gain was tried there and reverted, see 10.7).
