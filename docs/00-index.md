# Documentation index - how the project grew

Every doc in this repo, in the order it was actually written, read as a
timeline. Each entry links both language versions (this project keeps a
Spanish and an English copy of every doc). Numeric prefixes on files
inside `docs/` reflect this same order; `README.md`/`LEAME.md` stay
unprefixed at the repo root (GitHub renders `README.md` specially).

| # | Date | English | Spanish | What it covers |
|---|---|---|---|---|
| 1 | 2011-06-30 | [`README.md`](../README.md) | [`LEAME.md`](../LEAME.md) | Project overview: hardware, build steps, current state. Kept up to date - this is the only pair still actively maintained rather than left as a historical snapshot. |
| 2 | 2011-06-30 | [`02-machine-setup.md`](02-machine-setup.md) | [`02-configuracion-maquina.md`](02-configuracion-maquina.md) | *Historical.* First machine audit and first successful build - Doom running on the B2000 with no audio at all. |
| 3 | 2011-06-30 | [`03-source-changes.md`](03-source-changes.md) | [`03-cambios-codigo.md`](03-cambios-codigo.md) | *Historical.* The source changes needed for that first build: X11 paths, HP make compatibility, config file location. |
| 4 | 2011-06-30 | [`04-add-sound.md`](04-add-sound.md) | [`04-agregar-sonido.md`](04-agregar-sonido.md) | *Historical.* First working sound effects (HP Alib / `simpleAudio`), SFX only, no music yet. |
| 5 | 2026-07-10 | [`05-music-investigation.md`](05-music-investigation.md) | [`05-investigacion-musica.md`](05-investigacion-musica.md) | The full music story: architecture research, the abandoned external-process synth, the in-process OPL2/GENMIDI rewrite that fixed the stutter, and the six synthesis bugs found and fixed in a later "screech" investigation. The project's largest and most detailed doc. |
| 6 | 2026-07-12 | [`06-cpu-performance-investigation.md`](06-cpu-performance-investigation.md) | [`06-investigacion-rendimiento-cpu.md`](06-investigacion-rendimiento-cpu.md) | Compiler/CPU optimization research (PA-RISC flags, MAX-2, PBO, `rtprio`) plus a results section on what was actually tried, including a hard warning about `rtprio` hanging the machine. |
| 7 | 2026-07-14 | [`07-commit-audit.md`](07-commit-audit.md) | [`07-auditoria-commits.md`](07-auditoria-commits.md) | Retrospective, not a development-phase doc: a commit-by-commit audit of the whole audio/performance history, judging what's still necessary vs. what got superseded or never shipped. |

## Reading it as a growth story

1. **2011, one day**: the project starts completely silent
   (`-DDOOM_NO_SFX`), gets sound effects working via HP's Alib
   audio library, and gets a real WAD + a couple of build-script/render
   fixes - docs 1-4 (plus `03191a4e`'s WAD switch and `i_video.c`
   change, folded into doc 1's ongoing updates rather than a doc of its
   own).
2. **A 15-year gap in the commit history** (2011 → 2026) - the project
   sits untouched.
3. **2026-07-10**: an attempt to add music fails first (external
   process, audible stutter), gets root-caused (inter-process
   scheduling latency on a single core), and gets rebuilt as an
   in-process synthesizer that actually works - doc 5.
4. **2026-07-12**: with the engine's stutter mostly solved by compiler
   flags, a different, previously-masked problem surfaces (an audio
   "screech") and gets tracked down to six separate synthesis bugs -
   also folded into doc 5 (section 10), and doc 6 covers the CPU side
   of that same work.
5. **2026-07-14**: a retrospective pass audits the whole history to
   answer "was all of this actually necessary?" - doc 7.

## Notes on the historical docs

Docs 2-4 describe the project's state in June 2011, before audio, music,
or any of the compiler/performance work existed. They're kept exactly as
originally written rather than rewritten, so the project's early state
stays documented accurately - each carries a note at the top pointing to
where the current information actually lives. Doc 1 (`README.md`/
`LEAME.md`) is the exception: it gets updated to stay current, since
it's the project's front door.
