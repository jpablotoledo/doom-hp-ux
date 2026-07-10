/*
 * musserver_hpux.c
 * MUSSERV-protocol OPL2 music server for Doom on HP-UX.
 *
 * Protocol (from Doom's i_sound.c via popen):
 *   Init:    fwrite(GENMIDI lump, 11908 bytes)
 *   N%d\n    set loop flag, followed by fwrite(MUS data)
 *   R        play / resume
 *   P        pause
 *   S        stop
 *   V%d R    set volume (0-15)
 *   Q        quit
 *
 * Audio: 11025 Hz, 16-bit signed, stereo, via HP simpleAudio/Alib.
 *
 * OPL2 synthesis: hand-rolled software FM synth (NOT the Nuked-OPL2-Lite
 * cycle-accurate emulator — that was tried and measured to consume ~84%
 * of this machine's single PA-RISC core, starving Doom's own CPU budget
 * and making the game unplayably slow). This synth is much cheaper: a
 * single sine table with waveform variants, direct GENMIDI-driven FM,
 * and log-scale TL attenuation matching real OPL2 (0.75dB/unit) for
 * both modulator AND carrier — an earlier version of this file used a
 * linear TL scale for the carrier, which made background voices play
 * far louder than they should and produced a dense wall-of-noise effect
 * with many simultaneous channels.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
/* HP-UX: select() and fd_set are in sys/time.h with _HPUX_SOURCE; no sys/select.h */

#include "simpleAudio.h"

/* ============================================================
 * Diagnostic log (must be first — called from functions below)
 * ============================================================ */

static FILE *logfp = NULL;

static void muslog(const char *msg)
{
    if (!logfp) return;
    fprintf(logfp, "%s\n", msg);
    fflush(logfp);
}

static void muslogf(const char *fmt, int a, int b)
{
    char buf[128];
    if (!logfp) return;
    sprintf(buf, fmt, a, b);
    fprintf(logfp, "%s\n", buf);
    fflush(logfp);
}

/* ============================================================
 * Constants
 * ============================================================ */

#define SAMPLE_RATE     11025
#define NUM_OPL_CHAN    9
#define MUS_TEMPO_HZ    140         /* MUS tic rate (real spec value; was
                                       wrongly 70 here, which played every
                                       song at half speed/pitch-shifted-down
                                       feel — confirmed by ear and fixed via
                                       the opl2test_nuked.c standalone test
                                       harness before touching this file) */
#define FRAME_SAMPLES   (SAMPLE_RATE / MUS_TEMPO_HZ)   /* ~78 */

#define GENMIDI_HDR     8
#define GENMIDI_NUM_MELODIC 128
#define GENMIDI_NUM_PERC    47
#define GENMIDI_INSTR   (GENMIDI_NUM_MELODIC + GENMIDI_NUM_PERC)
#define GENMIDI_ISIZ    36          /* real genmidi_instr_t size: was wrongly
                                       68 (36 binary + 32 name, treated as
                                       one interleaved record) — the real
                                       file is 3 separate contiguous blocks:
                                       header, N*36-byte binary structs,
                                       N*32-byte name strings. See
                                       docs/investigacion-musica.md for the
                                       full writeup verified against
                                       Chocolate Doom's i_oplmusic.c. */
#define GENMIDI_VSIZ    16          /* one genmidi_voice_t */
#define GENMIDI_NAMESIZ 32
#define GENMIDI_TOTAL   (GENMIDI_HDR + GENMIDI_INSTR * GENMIDI_ISIZ \
                          + GENMIDI_INSTR * GENMIDI_NAMESIZ)

#define GENMIDI_FLAG_FIXED  0x0001
#define GENMIDI_FLAG_2VOICE 0x0004

/* Real genmidi_voice_t layout (16 bytes): modulator op(6) + feedback(1)
 * + carrier op(6) + unused(1) + base_note_offset(s16 LE, unused here).
 * Each op(6) = tremolo,attack,sustain,waveform,scale,level — scale and
 * level are separate bytes that must be OR'd for the OPL 0x40 register;
 * this simplified synth has no key-scaling, so it just uses `level`
 * directly as the 0-63 total-level and ignores `scale`/KSL. */
#define GM_INSTR_OFS(i) (GENMIDI_HDR + (i)*GENMIDI_ISIZ)
#define GM_VOICE(i,v)   (GM_INSTR_OFS(i) + 4 + (v)*GENMIDI_VSIZ)
#define GM_MOD_MUL(i,v) (genmidi[GM_VOICE(i,v)+0])   /* tremolo/AM/VIB/EGT/KSR/MULT */
#define GM_MOD_AD(i,v)  (genmidi[GM_VOICE(i,v)+1])   /* attack/decay nibbles */
#define GM_MOD_SR(i,v)  (genmidi[GM_VOICE(i,v)+2])   /* sustain/release nibbles */
#define GM_MOD_WAVE(i,v)(genmidi[GM_VOICE(i,v)+3])
#define GM_MOD_LEVEL(i,v)(genmidi[GM_VOICE(i,v)+5])
#define GM_FEEDBACK(i,v)(genmidi[GM_VOICE(i,v)+6])
#define GM_CAR_MUL(i,v) (genmidi[GM_VOICE(i,v)+7])
#define GM_CAR_AD(i,v)  (genmidi[GM_VOICE(i,v)+8])
#define GM_CAR_SR(i,v)  (genmidi[GM_VOICE(i,v)+9])
#define GM_CAR_WAVE(i,v)(genmidi[GM_VOICE(i,v)+10])
#define GM_CAR_LEVEL(i,v)(genmidi[GM_VOICE(i,v)+12])

#define GM_FLAGS(i)     (genmidi[GM_INSTR_OFS(i)+0] | \
                         (genmidi[GM_INSTR_OFS(i)+1]<<8))
#define GM_FINETUNE(i)  (genmidi[GM_INSTR_OFS(i)+2])
#define GM_NOTE(i)      (genmidi[GM_INSTR_OFS(i)+3])

/* ============================================================
 * OPL2 emulator types
 * ============================================================ */

#define ENV_OFF      0
#define ENV_ATTACK   1
#define ENV_DECAY    2
#define ENV_SUSTAIN  3
#define ENV_RELEASE  4

#define ENV_MAX      511

/* OPL2 multiplier table: index=MULT field, value=2×actual */
static const int mult2x[16] = {1,2,4,6,8,10,12,14,16,18,20,20,24,24,30,30};

typedef struct {
    unsigned int  phase;        /* 32-bit phase, table index = phase >> 22 */
    unsigned int  phase_step;   /* step per sample */
    int           env;          /* current envelope level 0..ENV_MAX */
    int           env_state;    /* ENV_* */
    int           env_atk;      /* attack increment per sample */
    int           env_dec;      /* decay decrement per sample */
    int           env_sus;      /* sustain level */
    int           env_rel;      /* release decrement per sample */
    int           tl;           /* total level 0-63 (0=max,63=silence) */
    int           wave;         /* waveform 0-3 */
    int           egt;          /* envelope generator type (EGT bit) */
    int           fb_prev;      /* feedback history, table-entry units */
} opl_op_t;

typedef struct {
    opl_op_t      mod;          /* modulator */
    opl_op_t      car;          /* carrier */
    int           fb;           /* feedback shift (0=none, 1-7) */
    int           con;          /* 0=FM, 1=additive */
    int           active;       /* channel in use */
    int           instr;        /* current GENMIDI instrument index */
    int           note;         /* MIDI note currently playing */
    int           mus_chan;     /* which MUS channel owns this OPL channel */
    int           timestamp;    /* for LRU voice stealing */
    int           last_out;     /* last rendered sample, for crossfade-in */
    int           fade_remain;  /* samples left in crossfade-in after retrigger/steal */
    int           fade_from;    /* value to fade from */
} opl_chan_t;

/* Crossfade length on retrigger/voice-steal: forcing phase=0 AND env=0
 * simultaneously (the naive key-on) causes an instant amplitude
 * discontinuity — audible as a broadband click/pop, and with many
 * channels retriggering frequently (as real MUS songs do) these clicks
 * stack into what sounds like continuous noise. 32 samples (~2.9ms) is
 * short enough to be inaudible as a separate event but long enough to
 * kill the click. */
#define FADE_SAMPLES 32

/* ============================================================
 * Sine table (1024 entries)
 * ============================================================ */

static short sin_table[1024];

/* OPL2 TL attenuation: tl_attn[i] = 32767 * 2^(-i*0.75/6.0206)
 * 0.75 dB per TL unit, matching OPL2 hardware logarithmic attenuation.
 * Used for BOTH modulator and carrier TL. */
static int tl_attn[64];

static void init_sin_table(void)
{
    int i;
    for (i = 0; i < 1024; i++)
        sin_table[i] = (short)(sin(2.0 * 3.14159265358979 * i / 1024.0) * 32767.0);
    for (i = 0; i < 64; i++) {
        double att = pow(2.0, -(double)i * 0.75 / 6.0206);
        tl_attn[i] = (int)(att * 32767.0 + 0.5);
        if (tl_attn[i] > 32767) tl_attn[i] = 32767;
        if (tl_attn[i] < 0)     tl_attn[i] = 0;
    }
}

/* ============================================================
 * OPL2 global state
 * ============================================================ */

static opl_chan_t opl_chan[NUM_OPL_CHAN];
static int        opl_master_vol = 15;  /* 0-15, from V command */
static int        opl_timestamp  = 0;

/* ============================================================
 * GENMIDI raw data
 * ============================================================ */

static unsigned char genmidi[GENMIDI_TOTAL];
static int           genmidi_ok = 0;

/* ============================================================
 * Waveform lookup
 * ============================================================ */

static short opl_wave(unsigned int phase, int wave)
{
    int idx = (int)(phase >> 22) & 1023;
    switch (wave & 3) {
    case 0: return sin_table[idx];
    case 1: /* half sine: negative half = 0 */
        return (idx < 512) ? sin_table[idx] : 0;
    case 2: /* abs sine */
        return (short)(sin_table[idx >= 512 ? idx - 512 : idx] < 0
                       ? -sin_table[idx >= 512 ? idx - 512 : idx]
                       :  sin_table[idx >= 512 ? idx - 512 : idx]);
    case 3: /* pulse sine: first quarter then zero */
        return (idx < 256) ? sin_table[idx] : 0;
    default: return 0;
    }
}

/* ============================================================
 * Envelope helpers
 * ============================================================ */

static int rate_to_inc(int rate, int max_val)
{
    int tbl[16] = {0,1,1,1,2,3,4,6,8,12,16,24,32,64,128,512};
    if (rate <= 0 || rate > 15) return 0;
    return tbl[rate] ? tbl[rate] : 0;
}

/* preserve_phase: keep current phase/env instead of snapping to 0/max.
 * Used together with the render-time crossfade (FADE_SAMPLES) below —
 * this alone isn't enough to fully avoid clicks on a note change (a
 * different note needs phase=0 for correct pitch), so the real
 * click-avoidance mechanism is the output crossfade in render_channel().
 * preserve_phase is kept for the same-note-retrigger case where
 * continuing the waveform's phase is also musically correct, not just
 * click avoidance. */
/* AR=0 in GENMIDI: confirmed by listening test that giving these voices
 * ANY audible attack (instant OR a fast ~46ms fade-in) reintroduces the
 * chirp/noise problem. Reverted to real-OPL2-hardware behavior: an
 * operator with AR=0 never attacks and stays silent. This does mute a
 * few D_INTRO/E1M1 voices whose GENMIDI data looks degenerate anyway
 * (all-zero ADSR, garbage flags field) — better silent than noisy. */
static void op_key_on(opl_op_t *op, unsigned int step, int preserve_phase)
{
    if (!preserve_phase)
        op->phase = 0;
    op->phase_step = step;
    if (op->env_atk > 0) {
        op->env_state = ENV_ATTACK;
        if (!preserve_phase) op->env = 0;
    } else {
        op->env_state = ENV_OFF;
        if (!preserve_phase) op->env = 0;
    }
}

static void op_key_off(opl_op_t *op)
{
    if (op->env_state != ENV_OFF)
        op->env_state = ENV_RELEASE;
}

/* Advance envelope by one sample, return current level */
static int op_env_step(opl_op_t *op)
{
    switch (op->env_state) {
    case ENV_ATTACK:
        op->env += op->env_atk;
        if (op->env >= ENV_MAX) {
            op->env       = ENV_MAX;
            op->env_state = ENV_DECAY;
        }
        break;
    case ENV_DECAY:
        op->env -= op->env_dec;
        if (op->env <= op->env_sus) {
            op->env = op->env_sus;
            op->env_state = op->egt ? ENV_SUSTAIN : ENV_RELEASE;
        }
        break;
    case ENV_SUSTAIN:
        /* hold */
        break;
    case ENV_RELEASE:
        op->env -= op->env_rel;
        if (op->env <= 0) {
            op->env       = 0;
            op->env_state = ENV_OFF;
        }
        break;
    case ENV_OFF:
    default:
        op->env = 0;
        break;
    }
    return op->env;
}

/* ============================================================
 * Channel frequency setup
 * ============================================================ */

static unsigned int note_phase_step(int midi_note, int mult_field)
{
    double freq;
    double step;
    int m = mult2x[mult_field & 15];

    freq = 440.0 * pow(2.0, (midi_note - 69.0) / 12.0);
    step = freq * (double)m * 0.5 * 4294967296.0 / (double)SAMPLE_RATE;
    return (unsigned int)(step + 0.5);
}

/* ============================================================
 * Instrument loading
 * ============================================================ */

static void load_instrument(opl_chan_t *ch, int instr_idx, int midi_note, int velocity,
                             int preserve_phase)
{
    int ad, sr, tl_car;
    int ar, dr, sl, rr;
    int note;
    unsigned int step;

    if (!genmidi_ok) return;
    if (instr_idx < 0 || instr_idx >= GENMIDI_INSTR) return;

    ch->instr = instr_idx;

    ch->fb  = (GM_FEEDBACK(instr_idx, 0) >> 1) & 0x7;  /* bits 3:1 */
    ch->con = (GM_FEEDBACK(instr_idx, 0)) & 0x1;        /* bit 0 */

    note = midi_note;
    if (GM_FLAGS(instr_idx) & GENMIDI_FLAG_FIXED)
        note = GM_NOTE(instr_idx);
    if (note < 0) note = 0;
    if (note > 127) note = 127;

    /* --- Modulator --- */
    ad = GM_MOD_AD(instr_idx, 0);
    sr = GM_MOD_SR(instr_idx, 0);
    ar = (ad >> 4) & 0xF;
    dr = ad & 0xF;
    sl = (sr >> 4) & 0xF;
    rr = sr & 0xF;

    ch->mod.tl        = (GM_MOD_LEVEL(instr_idx, 0)) & 0x3F;
    ch->mod.wave      = GM_MOD_WAVE(instr_idx, 0) & 3;
    ch->mod.egt       = (GM_MOD_MUL(instr_idx, 0) >> 5) & 1;
    ch->mod.env_atk   = rate_to_inc(ar, ENV_MAX);
    ch->mod.env_dec   = rate_to_inc(dr, ENV_MAX);
    ch->mod.env_sus   = (sl == 15) ? 0 : (ENV_MAX - sl * (ENV_MAX / 15));
    ch->mod.env_rel   = rate_to_inc(rr, ENV_MAX);
    if (ch->mod.env_rel < 1) ch->mod.env_rel = 1;
    ch->mod.fb_prev   = 0;

    step = note_phase_step(note, GM_MOD_MUL(instr_idx, 0) & 0xF);
    op_key_on(&ch->mod, step, preserve_phase);

    /* --- Carrier --- */
    ad = GM_CAR_AD(instr_idx, 0);
    sr = GM_CAR_SR(instr_idx, 0);
    ar = (ad >> 4) & 0xF;
    dr = ad & 0xF;
    sl = (sr >> 4) & 0xF;
    rr = sr & 0xF;

    /* Carrier TL: blend instrument base level with MIDI velocity */
    tl_car = (GM_CAR_LEVEL(instr_idx, 0)) & 0x3F;
    {
        /* Higher velocity = lower TL (less attenuation) */
        int vel_tl = (127 - velocity) * 63 / 127;
        tl_car = (tl_car + vel_tl) / 2;
        if (tl_car < 0) tl_car = 0;
        if (tl_car > 63) tl_car = 63;
    }

    ch->car.tl        = tl_car;
    ch->car.wave      = GM_CAR_WAVE(instr_idx, 0) & 3;
    ch->car.egt       = (GM_CAR_MUL(instr_idx, 0) >> 5) & 1;
    ch->car.env_atk   = rate_to_inc(ar, ENV_MAX);
    ch->car.env_dec   = rate_to_inc(dr, ENV_MAX);
    ch->car.env_sus   = (sl == 15) ? 0 : (ENV_MAX - sl * (ENV_MAX / 15));
    ch->car.env_rel   = rate_to_inc(rr, ENV_MAX);
    if (ch->car.env_rel < 1) ch->car.env_rel = 1;
    ch->car.fb_prev   = 0;

    step = note_phase_step(note, GM_CAR_MUL(instr_idx, 0) & 0xF);
    op_key_on(&ch->car, step, preserve_phase);
}

/* ============================================================
 * OPL2 channel rendering (one sample)
 * ============================================================ */

/* raw_m/512: modulator output converted to sine-table-entry units before
 * the <<22 shift into the 32-bit phase accumulator. Matches the scale
 * derived from Nuked-OPL2-Lite's real phase generator (pg_phase_out is
 * 16-bit with 64 units/entry; our sine table is 8x theirs, and our
 * phase has 2^22 units/entry vs their 64 — combined scale factor 512).
 * Both modulator AND carrier TL use the same log attenuation table
 * (0.75dB/unit) — using a linear scale for the carrier was a bug that
 * made mid/high-TL voices far louder than real hardware, causing many
 * simultaneous channels to compete near full volume instead of most
 * sitting low in the mix. */
static short render_channel(opl_chan_t *ch)
{
    int env_m, env_c, raw_m, raw_c, out;
    short mod_s, car_s;
    unsigned int car_phase;

    env_m = op_env_step(&ch->mod);
    env_c = op_env_step(&ch->car);

    if (env_c == 0 && ch->car.env_state == ENV_OFF) {
        ch->last_out = 0;
        return 0;
    }

    /* Modulator sample */
    {
        unsigned int ph = ch->mod.phase;
        if (ch->fb > 0 && ch->mod.fb_prev != 0) {
            /* Nuked-OPL2: fbmod = (prout+out) >> (9-fb).
             * fb_prev is in table-entry units → ×2 >> (9-fb) → entries → <<22 */
            int fb_idx = (ch->mod.fb_prev * 2) >> (9 - ch->fb);
            if (fb_idx >= 0) ph += (unsigned int)fb_idx << 22;
            else             ph -= (unsigned int)(-fb_idx) << 22;
        }
        mod_s = opl_wave(ph, ch->mod.wave);
        ch->mod.phase += ch->mod.phase_step;
    }

    raw_m = (int)mod_s * env_m / ENV_MAX;
    raw_m = (int)((long)raw_m * tl_attn[ch->mod.tl] / 32767L);
    ch->mod.fb_prev = raw_m / 512;

    /* Carrier phase: in FM mode, shift by modulator output. */
    car_phase = ch->car.phase;
    if (!ch->con) {
        int fm_idx = raw_m / 512;
        if (fm_idx >= 0) car_phase += (unsigned int)fm_idx << 22;
        else             car_phase -= (unsigned int)(-fm_idx) << 22;
    }

    car_s = opl_wave(car_phase, ch->car.wave);
    ch->car.phase += ch->car.phase_step;

    /* Apply carrier envelope and TL (log scale, same table as modulator) */
    raw_c = (int)car_s * env_c / ENV_MAX;
    raw_c = (int)((long)raw_c * tl_attn[ch->car.tl] / 32767L);

    if (ch->con) {
        /* Additive: sum modulator + carrier */
        out = (raw_m + raw_c) / 2;
    } else {
        out = raw_c;
    }

    /* Apply master volume */
    out = out * (opl_master_vol + 1) / 16;

    if (out >  32767) out =  32767;
    if (out < -32767) out = -32767;

    /* Crossfade-in after a retrigger/voice-steal to avoid a broadband
     * click (see FADE_SAMPLES comment above). */
    if (ch->fade_remain > 0) {
        int t = FADE_SAMPLES - ch->fade_remain;
        out = ch->fade_from + (out - ch->fade_from) * t / FADE_SAMPLES;
        ch->fade_remain--;
    }
    ch->last_out = out;
    return (short)out;
}

/* Mix all active OPL channels to one sample */
static short opl_mix_sample(void)
{
    int i, sum = 0, n = 0;
    for (i = 0; i < NUM_OPL_CHAN; i++) {
        if (opl_chan[i].active) {
            sum += render_channel(&opl_chan[i]);
            n++;
            /* Deactivate if fully silent */
            if (opl_chan[i].car.env_state == ENV_OFF &&
                opl_chan[i].mod.env_state == ENV_OFF) {
                opl_chan[i].active = 0;
            }
        }
    }
    if (n == 0) return 0;
    sum /= n;
    if (sum >  32767) sum =  32767;
    if (sum < -32767) sum = -32767;
    return (short)sum;
}

/* ============================================================
 * OPL2 channel allocation
 * ============================================================ */

/* Find OPL channel for MUS channel + note.
 * If already playing same (mus_chan, note), return it.
 * Otherwise allocate a free or LRU channel. */
static int alloc_opl_channel(int mus_chan, int note)
{
    int i, oldest = 0, oldest_ts = 0x7fffffff;
    int free_ch = -1;

    for (i = 0; i < NUM_OPL_CHAN; i++) {
        if (opl_chan[i].active &&
            opl_chan[i].mus_chan == mus_chan &&
            opl_chan[i].note == note)
            return i;
    }
    for (i = 0; i < NUM_OPL_CHAN; i++) {
        if (!opl_chan[i].active) { free_ch = i; break; }
        if (opl_chan[i].timestamp < oldest_ts) {
            oldest_ts = opl_chan[i].timestamp;
            oldest = i;
        }
    }
    return (free_ch >= 0) ? free_ch : oldest;
}

/* Find active channel by MUS channel + note (for note-off) */
static int find_opl_channel(int mus_chan, int note)
{
    int i;
    for (i = 0; i < NUM_OPL_CHAN; i++) {
        if (opl_chan[i].active &&
            opl_chan[i].mus_chan == mus_chan &&
            opl_chan[i].note == note)
            return i;
    }
    return -1;
}

/* ============================================================
 * MUS state
 * ============================================================ */

static unsigned char *mus_data  = NULL; /* current song data */
static int            mus_len   = 0;
static int            mus_pos   = 0;    /* current byte position in song */
static int            mus_score = 0;    /* offset of score data */
static int            mus_loop  = 1;    /* 0=play once, 1=loop */
static int            mus_delay = 0;    /* tics until next event */
static int            mus_playing = 0;
static int            mus_paused  = 0;

static int mus_chan_instr[16];   /* current instrument per MUS channel */
static int mus_chan_vol[16];     /* current volume per MUS channel */
static int mus_chan_pitch[16];   /* pitch wheel (unused for now) */

/* Read one byte from current MUS position */
static int mus_read_byte(void)
{
    if (mus_data == NULL || mus_pos >= mus_len) return -1;
    return (unsigned char)mus_data[mus_pos++];
}

/* Read variable-length time delay */
static int mus_read_delay(void)
{
    int delay = 0, b;
    do {
        b = mus_read_byte();
        if (b < 0) return 0;
        delay = (delay << 7) | (b & 0x7F);
    } while (b & 0x80);
    return delay;
}

/* Process all MUS events for one tic. Returns 1 if still going, 0 if end. */
static int mus_process_tic(void)
{
    int event, type, chan, last;
    int note, vel, ctrl, val, delay;
    int opl_ch, instr;

    if (!mus_playing || mus_paused || mus_data == NULL) return 1;

    if (mus_delay > 0) {
        mus_delay--;
        return 1;
    }

    /* Process events until we hit a "last in tic" flag */
    for (;;) {
        event = mus_read_byte();
        if (event < 0) goto done;

        last = (event >> 7) & 1;
        type = (event >> 4) & 7;
        chan = event & 0xF;

        switch (type) {
        case 0: /* Note off */
            note = mus_read_byte() & 0x7F;
            opl_ch = find_opl_channel(chan, note);
            if (opl_ch >= 0) {
                op_key_off(&opl_chan[opl_ch].mod);
                op_key_off(&opl_chan[opl_ch].car);
            }
            break;

        case 1: /* Note on */
            note = mus_read_byte();
            if (note & 0x80) {
                /* bit7=1: velocity byte follows (original Doom musserv protocol) */
                vel = mus_read_byte() & 0x7F;
                mus_chan_vol[chan] = vel;
            } else {
                /* bit7=0: no velocity, reuse previous */
                vel = mus_chan_vol[chan];
            }
            note &= 0x7F;
            {
                int same_note, was_active;
                opl_ch = alloc_opl_channel(chan, note);
                was_active = opl_chan[opl_ch].active;
                /* Retriggering the same (chan,note) already active on this OPL
                 * channel: preserve phase too (musically correct — same pitch).
                 * A genuinely new note still resets phase for correct pitch,
                 * but EITHER way gets a crossfade-in below to avoid a click. */
                same_note = was_active &&
                            opl_chan[opl_ch].mus_chan == chan &&
                            opl_chan[opl_ch].note == note;
                instr = (chan == 15) ? (128 + (note & 0x3F)) : mus_chan_instr[chan];
                if (instr >= GENMIDI_INSTR) instr = GENMIDI_INSTR - 1;
                if (was_active) {
                    opl_chan[opl_ch].fade_from   = opl_chan[opl_ch].last_out;
                    opl_chan[opl_ch].fade_remain = FADE_SAMPLES;
                }
                load_instrument(&opl_chan[opl_ch], instr, note, vel, same_note);
                opl_chan[opl_ch].active   = 1;
                opl_chan[opl_ch].mus_chan  = chan;
                opl_chan[opl_ch].note     = note;
                opl_chan[opl_ch].timestamp = opl_timestamp++;
                if (opl_timestamp <= 10)
                    muslogf("musserver: note_on note=%d instr=%d", note, instr);
            }
            break;

        case 2: /* Pitch wheel */
            mus_chan_pitch[chan] = mus_read_byte();
            break;

        case 3: /* System event */
            mus_read_byte(); /* controller number, ignore */
            break;

        case 4: /* Change controller */
            ctrl = mus_read_byte();
            val  = mus_read_byte();
            if (ctrl == 0) {
                /* Program/instrument change */
                mus_chan_instr[chan] = val & 0x7F;
            } else if (ctrl == 3) {
                /* Volume */
                mus_chan_vol[chan] = val & 0x7F;
            }
            break;

        case 6: /* Score end */
        case 7:
            if (mus_loop) {
                mus_pos = mus_score;
                mus_delay = 0;
                return 1;
            }
            goto done;

        default:
            break;
        }

        if (last) {
            delay = mus_read_delay();
            mus_delay = delay;
            return 1;
        }
    }

done:
    mus_playing = 0;
    return 0;
}

/* ============================================================
 * MUS song loading
 * ============================================================ */

static void mus_load(unsigned char *data, int len, int loop)
{
    int i;
    /* Validate header */
    if (len < 14) return;
    if (data[0]!='M' || data[1]!='U' || data[2]!='S' || data[3]!=0x1A) return;

    if (mus_data) { free(mus_data); mus_data = NULL; }
    mus_data = (unsigned char*)malloc(len);
    if (!mus_data) return;
    memcpy(mus_data, data, len);
    mus_len   = len;
    mus_loop  = loop;

    /* MUS header: sig(4) + scorelen(2) + scorestart(2) + ... */
    mus_score = (data[6] & 0xFF) | ((data[7] & 0xFF) << 8);
    mus_pos   = mus_score;
    mus_delay = 0;

    for (i = 0; i < 16; i++) {
        mus_chan_instr[i] = 0;
        mus_chan_vol[i]   = 100;
        mus_chan_pitch[i] = 0;
    }
}

/* ============================================================
 * Audio output
 * ============================================================ */

static int audio_fd = -1;
static int audio_inherited = 0; /* 1 = fd came from Doom, don't close Aserver */

/* explicit_fd >= 0: use that fd (inherited from Doom via fork/exec stdout pipe).
 * explicit_fd < 0: open own Aserver connection (standalone mode). */
static void audio_open(int explicit_fd)
{
    int flags;
    if (explicit_fd >= 0) {
        audio_fd = explicit_fd;
        audio_inherited = 1;
        flags = fcntl(audio_fd, F_GETFL, 0);
        if (flags >= 0)
            fcntl(audio_fd, F_SETFL, flags | O_NONBLOCK);
        return;
    }
    if (openAudio() != 0) return;
    audio_fd = openAStream(PLAY_STREAM, SAMPLE_RATE, USE_STEREO,
                           USE_LIN16, USE_DEFAULT_SPEAKER, START_IMMEDIATELY);
    if (audio_fd < 0) {
        closeAudio();
        audio_fd = -1;
        return;
    }
    /* Non-blocking writes: if Aserver socket buffer is full, skip the frame
     * rather than blocking the command loop and deadlocking with Doom's pipe. */
    flags = fcntl(audio_fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(audio_fd, F_SETFL, flags | O_NONBLOCK);
}

static void audio_close(void)
{
    if (audio_fd >= 0) {
        if (!audio_inherited) {
            closeAStream(audio_fd);
            closeAudio();
        }
        audio_fd = -1;
    }
}

/* Audio delivery: a small ring buffer of pending frames, decoupled from
 * song-tempo advancement.
 *
 * History: the pipe to Doom fills up and write() fails with EAGAIN on
 * roughly a third of all frames under normal gameplay (SFX bursts,
 * throttled reads on Doom's side, etc). An earlier version silently
 * dropped the whole frame on EAGAIN — mus_process_tic() had already
 * advanced the song position, so the dropped audio was gone forever,
 * causing audible clicks/noise. A later attempt fixed that by only
 * calling mus_process_tic() once a frame was fully flushed to the pipe —
 * but that ties the song's TEMPO to the pipe's congestion: with ~30% of
 * iterations blocked, the whole song drags in real time (measured:
 * needed ~1.85x playback speed to sound right again), even though the
 * pitch of each individual note stays mathematically correct (confirmed
 * via FFT against expected GENMIDI frequencies) — a *tempo* bug wearing
 * a *pitch* bug's clothes.
 *
 * Fix: generate a new frame from the current channel state EVERY loop
 * iteration (so mus_process_tic() always advances at the real ~70Hz
 * clock), and push it onto a ring queue. A separate flush step drains
 * as much of the queue as the non-blocking pipe currently accepts. Only
 * if the queue fills completely (sustained congestion beyond ~450ms,
 * well past normal jitter) do we drop the oldest queued frame to make
 * room — a single ~14ms gap, not a dragged-out tempo. */
#define QUEUE_FRAMES 32   /* ~457ms of buffered audio at 70Hz */

static short queue_buf[QUEUE_FRAMES][FRAME_SAMPLES * 2];
static int   queue_head = 0;    /* next slot to fill */
static int   queue_tail = 0;    /* next slot to send */
static int   queue_count = 0;   /* frames currently queued */
static int   queue_send_off = 0;/* bytes already sent from queue_buf[queue_tail] */

static int frame_ok = 0, frame_fail = 0, frame_dropped = 0;
static int frames_since_log = 0;

/* Always generates a new frame from the current channel state and
 * enqueues it. Must be called once per loop iteration whenever the song
 * is playing, right after mus_process_tic(), so tempo tracks real time
 * regardless of pipe state. */
static void generate_frame(void)
{
    int i;
    short s;
    short *buf;

    if (queue_count >= QUEUE_FRAMES) {
        /* Queue full: drop the oldest frame instead of stalling the song. */
        queue_tail = (queue_tail + 1) % QUEUE_FRAMES;
        queue_count--;
        queue_send_off = 0;
        frame_dropped++;
    }

    buf = queue_buf[queue_head];
    for (i = 0; i < FRAME_SAMPLES; i++) {
        s = opl_mix_sample();
        buf[i * 2]     = s;
        buf[i * 2 + 1] = s;
    }
    queue_head = (queue_head + 1) % QUEUE_FRAMES;
    queue_count++;
}

/* Sends as many queued frames as the non-blocking pipe currently accepts.
 * Called every loop iteration unconditionally (even when paused/stopped)
 * so already-queued audio keeps draining. */
static void flush_audio_queue(void)
{
    if (audio_fd < 0) return;

    while (queue_count > 0) {
        unsigned char *base = (unsigned char *)queue_buf[queue_tail];
        int frame_bytes = (int)sizeof(queue_buf[0]);
        int remain = frame_bytes - queue_send_off;
        int wret = write(audio_fd, base + queue_send_off, remain);
        if (wret <= 0) {
            frame_fail++;
            break;  /* pipe congested, retry next iteration */
        }
        queue_send_off += wret;
        if (queue_send_off < frame_bytes) {
            frame_fail++;  /* partial write, finish next iteration */
            break;
        }
        queue_send_off = 0;
        queue_tail = (queue_tail + 1) % QUEUE_FRAMES;
        queue_count--;
        frame_ok++;
    }

    frames_since_log++;
    if (frames_since_log >= 350) {  /* ~5s at 70Hz */
        muslogf("musserver: stats ok=%d fail=%d", frame_ok, frame_fail);
        muslogf("musserver: dropped=%d queued=%d", frame_dropped, queue_count);
        frames_since_log = 0;
    }
}

/* ============================================================
 * GENMIDI loading
 * ============================================================ */

static int read_genmidi(void)
{
    int n, total = 0;
    unsigned char *p = genmidi;
    while (total < (int)sizeof(genmidi)) {
        n = read(0, p + total, sizeof(genmidi) - total);
        if (n <= 0) break;
        total += n;
    }
    if (total != (int)sizeof(genmidi)) return 0;
    if (memcmp(genmidi, "#OPL_II#", 8) != 0) return 0;
    return 1;
}

/* ============================================================
 * Protocol: read MUS data after N command
 * ============================================================ */

static void read_mus_data(int loop)
{
    unsigned char header[16];
    unsigned char *buf;
    int i, c, len;
    unsigned short scorelen, scorestart;

    /* Use fgetc so reads stay in the same stdio buffer as getchar() command parsing.
     * Mixing read(0,..) with getchar() loses bytes already buffered by stdio. */
    for (i = 0; i < 16; i++) {
        c = fgetc(stdin);
        if (c == EOF) return;
        header[i] = (unsigned char)c;
    }
    if (header[0]!='M' || header[1]!='U' || header[2]!='S' || header[3]!=0x1A)
        return;

    scorelen   = (unsigned short)(header[4] | (header[5] << 8));
    scorestart = (unsigned short)(header[6] | (header[7] << 8));

    len = (int)scorestart + (int)scorelen;
    if (len < 16 || len > 65536) return;

    buf = (unsigned char*)malloc(len);
    if (!buf) return;
    memcpy(buf, header, 16);

    for (i = 16; i < len; i++) {
        c = fgetc(stdin);
        if (c == EOF) { free(buf); return; }
        buf[i] = (unsigned char)c;
    }

    mus_load(buf, len, loop);
    free(buf);
}

/* ============================================================
 * SIGPIPE / SIGCHLD
 * ============================================================ */

static volatile int quit_flag = 0;
static void sig_quit(int sig) { quit_flag = 1; }

/* ============================================================
 * Main loop
 * ============================================================ */

int main(int argc, char *argv[])
{
    struct timeval tv;
    fd_set rfds;
    int cmd, loop;
    char cmdbuf[32];
    int n;
    int explicit_audio_fd = (argc >= 2) ? atoi(argv[1]) : -1;

    /* Setup signals */
    signal(SIGPIPE, sig_quit);
    signal(SIGTERM, sig_quit);
    signal(SIGINT,  sig_quit);

    /* Unbuffered stdin: getchar() reads one byte at a time directly from fd 0,
     * so select() on fd 0 accurately reflects what's available.
     * Without this, getchar() buffers a large chunk and select() thinks fd is
     * empty while commands are stuck in the stdio buffer. */
    setbuf(stdin, NULL);

    /* Close all file descriptors inherited from Doom (popen inherits everything).
     * This prevents Doom's audio socket and X11 connections from leaking into
     * musserver's fd table, which can confuse Aserver into thinking Doom's stream
     * belongs to musserver and interfere with SFX playback.
     * NOTE: logfp must be opened AFTER this loop or it would be closed here. */
    {
        int i, fd_max = (int)sysconf(_SC_OPEN_MAX);
        if (fd_max < 0 || fd_max > 256) fd_max = 256;
        for (i = 3; i < fd_max; i++)
            close(i);
    }

    /* Open log after the fd close loop — logfp gets fd 3 (first clean fd).
     * If opened before the loop it would be closed immediately. */
    logfp = fopen("/tmp/musserver.log", "w");
    muslog("musserver: started");

    /* Init OPL state */
    init_sin_table();
    memset(opl_chan, 0, sizeof(opl_chan));

    /* Read GENMIDI from stdin */
    genmidi_ok = read_genmidi();
    muslogf("musserver: genmidi_ok=%d", genmidi_ok, 0);

    /* Open audio: use fd from Doom if provided (avoids opening a second Aserver
     * connection which would cause Aserver stream exclusivity and silence SFX). */
    audio_open(explicit_audio_fd);
    muslogf("musserver: audio_fd=%d inherited=%d", audio_fd, audio_inherited);

    /* Main event loop */
    while (!quit_flag) {
        /* Wait up to one MUS tic for stdin data */
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        tv.tv_sec  = 0;
        tv.tv_usec = 1000000 / MUS_TEMPO_HZ;  /* ~14286 us */

        n = select(1, &rfds, NULL, NULL, &tv);

        if (n < 0) {
            /* select error */
            break;
        }

        if (n > 0 && FD_ISSET(0, &rfds)) {
            /* Command byte available */
            cmd = getchar();
            if (cmd == EOF) break;

            switch (cmd) {
            case 'N':
                /* N%d\n loop_flag, followed by MUS data */
                loop = 1;
                {
                    int i = 0;
                    int c;
                    while (i < (int)sizeof(cmdbuf)-1 && (c=getchar()) != '\n' && c != EOF)
                        cmdbuf[i++] = (char)c;
                    cmdbuf[i] = '\0';
                    loop = atoi(cmdbuf);
                }
                read_mus_data(loop);
                muslogf("musserver: cmd N loop=%d mus_len=%d", loop, mus_len);
                break;

            case 'R':
                /* Play / resume */
                if (mus_data) {
                    mus_playing = 1;
                    mus_paused  = 0;
                }
                muslogf("musserver: cmd R playing=%d", mus_playing, 0);
                break;

            case 'P':
                /* Pause */
                mus_paused = 1;
                muslog("musserver: cmd P");
                break;

            case 'S':
                /* Stop */
                mus_playing = 0;
                mus_paused  = 0;
                muslog("musserver: cmd S");
                {
                    int i;
                    for (i = 0; i < NUM_OPL_CHAN; i++) {
                        op_key_off(&opl_chan[i].mod);
                        op_key_off(&opl_chan[i].car);
                    }
                }
                /* Drop any queued audio so a subsequent song doesn't play
                 * a leftover tail of this one (up to ~457ms could still
                 * be queued). */
                queue_head = queue_tail = queue_count = queue_send_off = 0;
                break;

            case 'V':
                /* V%d R  (volume) */
                {
                    int i = 0, c;
                    while (i < (int)sizeof(cmdbuf)-1 && (c=getchar()) != ' ' && c != EOF)
                        cmdbuf[i++] = (char)c;
                    cmdbuf[i] = '\0';
                    opl_master_vol = atoi(cmdbuf);
                    if (opl_master_vol < 0)  opl_master_vol = 0;
                    if (opl_master_vol > 15) opl_master_vol = 15;
                    /* consume trailing 'R' */
                    if ((c = getchar()) == 'R') { /* ok */ }
                }
                break;

            case 'Q':
                /* Quit */
                quit_flag = 1;
                break;

            default:
                break;
            }
        }

        /* Advance the song and generate its next frame unconditionally
         * (only while playing) — tempo must track real time, not pipe
         * congestion. flush_audio_queue() runs every iteration regardless,
         * so already-queued audio keeps draining even while paused/stopped
         * instead of sitting stuck. */
        if (mus_playing && !mus_paused) {
            mus_process_tic();
            generate_frame();
        }
        flush_audio_queue();
    }

    /* Cleanup */
    if (mus_data) free(mus_data);
    audio_close();
    return 0;
}
