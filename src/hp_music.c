/*
 * hp_music.c
 * In-process OPL2/GENMIDI music synthesizer for Doom on HP-UX.
 *
 * Replaces the earlier musserver-hpux design (a separate process talking
 * to Doom over a pipe). That design worked correctly once the GENMIDI
 * parsing and MUS tempo bugs were fixed (see docs/investigacion-musica.md),
 * but on this machine's single PA-RISC core, running a second process
 * - even one using under 1% CPU on its own - introduced enough
 * inter-process scheduling/IPC latency that audio still audibly cut
 * whenever Doom's own render loop (already ~85% CPU with no music at
 * all) stalled for a frame. A raw capture of Aserver's input confirmed
 * the synth itself wasn't the bottleneck; the pipe hand-off was.
 *
 * This file has no process boundary: hp_music_generate() is a plain
 * function called directly from i_sound.c's audio timer, producing
 * exactly the samples needed for that call with no queue, no pipe, and
 * no dependency on the OS scheduling a second process. The OPL2 FM
 * synth and MUS parser below are the same logic validated on real
 * hardware in musserver_hpux.c/opl2test_nuked.c, adapted from a
 * push-to-a-pipe model to a pull-N-samples-on-demand model.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "hp_music.h"

#define HP_MUSIC_SAMPLE_RATE 11025
#define HP_MUSIC_TEMPO_HZ    140   /* real MUS tick rate; see i_sound.c notes */
#define NUM_OPL_CHAN         9

#define GENMIDI_HDR         8
#define GENMIDI_NUM_MELODIC 128
#define GENMIDI_NUM_PERC    47
#define GENMIDI_INSTR       (GENMIDI_NUM_MELODIC + GENMIDI_NUM_PERC)
#define GENMIDI_ISIZ        36
#define GENMIDI_VSIZ        16
#define GENMIDI_FLAG_FIXED  0x0001

#define GM_INSTR_OFS(i) (GENMIDI_HDR + (i)*GENMIDI_ISIZ)
#define GM_VOICE(i,v)   (GM_INSTR_OFS(i) + 4 + (v)*GENMIDI_VSIZ)
#define GM_MOD_MUL(i,v)  (genmidi[GM_VOICE(i,v)+0])
#define GM_MOD_AD(i,v)   (genmidi[GM_VOICE(i,v)+1])
#define GM_MOD_SR(i,v)   (genmidi[GM_VOICE(i,v)+2])
#define GM_MOD_WAVE(i,v) (genmidi[GM_VOICE(i,v)+3])
#define GM_MOD_LEVEL(i,v)(genmidi[GM_VOICE(i,v)+5])
#define GM_FEEDBACK(i,v) (genmidi[GM_VOICE(i,v)+6])
#define GM_CAR_MUL(i,v)  (genmidi[GM_VOICE(i,v)+7])
#define GM_CAR_AD(i,v)   (genmidi[GM_VOICE(i,v)+8])
#define GM_CAR_SR(i,v)   (genmidi[GM_VOICE(i,v)+9])
#define GM_CAR_WAVE(i,v) (genmidi[GM_VOICE(i,v)+10])
#define GM_CAR_LEVEL(i,v)(genmidi[GM_VOICE(i,v)+12])
#define GM_FLAGS(i)      (genmidi[GM_INSTR_OFS(i)+0] | ((int)genmidi[GM_INSTR_OFS(i)+1]<<8))
#define GM_NOTE(i)       (genmidi[GM_INSTR_OFS(i)+3])

/* ============================================================
 * OPL2 emulator (same design as musserver_hpux.c)
 * ============================================================ */

#define ENV_OFF      0
#define ENV_ATTACK   1
#define ENV_DECAY    2
#define ENV_SUSTAIN  3
#define ENV_RELEASE  4
#define ENV_MAX      511
#define FADE_SAMPLES 32

static const int mult2x[16] = {1,2,4,6,8,10,12,14,16,18,20,20,24,24,30,30};

typedef struct {
    unsigned int phase;
    unsigned int phase_step;
    int env, env_state;
    int env_atk, env_dec, env_sus, env_rel;
    int tl, wave, egt;
    int fb_prev, fb_prev2;
} hp_opl_op_t;

typedef struct {
    hp_opl_op_t mod, car;
    int fb, con, active, instr, note, mus_chan, timestamp;
    int last_out, fade_remain, fade_from;
} hp_opl_chan_t;

static short sin_table[1024];
static int   tl_attn[64];
static hp_opl_chan_t opl_chan[NUM_OPL_CHAN];
static int opl_master_vol = 15;
static int opl_timestamp = 0;

static unsigned char genmidi[8 + GENMIDI_INSTR * GENMIDI_ISIZ + GENMIDI_INSTR * 32];
static int genmidi_ok = 0;

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

static short opl_wave(unsigned int phase, int wave)
{
    int idx = (int)(phase >> 22) & 1023;
    switch (wave & 3) {
    case 0: return sin_table[idx];
    case 1: return (idx < 512) ? sin_table[idx] : 0;
    case 2: return (short)(sin_table[idx >= 512 ? idx - 512 : idx] < 0
                   ? -sin_table[idx >= 512 ? idx - 512 : idx]
                   :  sin_table[idx >= 512 ? idx - 512 : idx]);
    case 3: return (idx < 256) ? sin_table[idx] : 0;
    default: return 0;
    }
}

static int rate_to_inc(int rate)
{
    static const int tbl[16] = {0,1,1,1,2,3,4,6,8,12,16,24,32,64,128,512};
    if (rate <= 0 || rate > 15) return 0;
    return tbl[rate];
}

/* Real OPL2 envelopes are stepped in the log(dB)-attenuation domain, which
 * is why a "fast" release still tapers the linear PCM output smoothly -
 * each dB step is a *multiplicative* cut, shrinking rapidly as the signal
 * already gets quiet. This synth's envelope is linear (env is a plain
 * 0-511 amplitude scale, not a log/dB one), so the same per-sample rate
 * table entry (rate_to_inc() above can return up to 512, i.e. the whole
 * 0-511 range in a single sample for the fastest release rates, common on
 * percussion instruments such as GENMIDI's hi-hats) instead produces a
 * hard, instant amplitude cliff: a discontinuous derivative straight to
 * silence with no taper at all. That's audible as a sharp "tick"/click on
 * every note-off, confirmed by comparing this synth's release-tail
 * waveform against Nuked-OPL2's for the same instrument (see
 * docs/investigacion-musica.md) - Nuked's tapers smoothly, this synth's
 * cut hard until this fix. This isn't worth porting the full log-domain
 * envelope model for (a much larger rewrite for a narrow benefit), so
 * instead the release rate is capped to a minimum length in samples,
 * turning a possible one-sample cliff into a short (still fast,
 * imperceptible as a fade, but no longer a discontinuity) linear ramp. */
#define ENV_REL_MIN_SAMPLES 48
static int rate_to_inc_release(int rate)
{
    int inc = rate_to_inc(rate);
    int max_inc = ENV_MAX / ENV_REL_MIN_SAMPLES;
    if (inc > max_inc) inc = max_inc;
    return inc;
}

static void op_key_on(hp_opl_op_t *op, unsigned int step, int preserve_phase)
{
    if (!preserve_phase) op->phase = 0;
    op->phase_step = step;
    op->env_state = (op->env_atk > 0) ? ENV_ATTACK : ENV_OFF;
    if (!preserve_phase) op->env = 0;
}

static void op_key_off(hp_opl_op_t *op)
{
    if (op->env_state != ENV_OFF) op->env_state = ENV_RELEASE;
}

static int op_env_step(hp_opl_op_t *op)
{
    switch (op->env_state) {
    case ENV_ATTACK:
        op->env += op->env_atk;
        if (op->env >= ENV_MAX) { op->env = ENV_MAX; op->env_state = ENV_DECAY; }
        break;
    case ENV_DECAY:
        op->env -= op->env_dec;
        if (op->env <= op->env_sus) { op->env = op->env_sus; op->env_state = op->egt ? ENV_SUSTAIN : ENV_RELEASE; }
        break;
    case ENV_SUSTAIN:
        break;
    case ENV_RELEASE:
        op->env -= op->env_rel;
        if (op->env <= 0) { op->env = 0; op->env_state = ENV_OFF; }
        break;
    default:
        op->env = 0;
        break;
    }
    return op->env;
}

static unsigned int note_phase_step(int midi_note, int mult_field)
{
    double freq = 440.0 * pow(2.0, (midi_note - 69.0) / 12.0);
    double step;
    freq *= (double)mult2x[mult_field & 15] * 0.5;
    /* Some GENMIDI instruments - several fixed-note percussion voices in
     * particular (e.g. the open hi-hat, instrument 139: fixed note 79 at
     * mult x10 works out to ~7840 Hz) - specify a carrier frequency above
     * this synth's Nyquist limit (HP_MUSIC_SAMPLE_RATE/2 = 5512.5 Hz).
     * Real OPL2 hardware/cycle-accurate emulators (Nuked-OPL2) run their
     * internal DSP well above audible range and band-limit on the way
     * down to output rate, so this never aliases there. This synth
     * generates directly at the final 11025 Hz with no oversampling
     * (too expensive for this machine's CPU budget), so a frequency past
     * Nyquist folds back into an unrelated, dissonant frequency inside
     * the audible band instead - heard as a harsh screech, confirmed by
     * comparing this synth's output for instrument 139 against
     * Nuked-OPL2's for the same instrument/note (see
     * docs/investigacion-musica.md). Clamping the representable
     * frequency below Nyquist trades exact pitch accuracy on a handful
     * of extreme high instruments for not aliasing - an easy trade,
     * since those are already noise-like percussive sounds where the
     * precise pitch is inaudible/unimportant. */
    if (freq > HP_MUSIC_SAMPLE_RATE * 0.45)
        freq = HP_MUSIC_SAMPLE_RATE * 0.45;
    step = freq * 4294967296.0 / (double)HP_MUSIC_SAMPLE_RATE;
    return (unsigned int)(step + 0.5);
}

static void load_instrument(hp_opl_chan_t *ch, int instr_idx, int midi_note, int velocity, int preserve_phase)
{
    int ad, sr, ar, dr, sl, rr, note, tl_car, vel_tl;
    unsigned int step;

    if (!genmidi_ok || instr_idx < 0 || instr_idx >= GENMIDI_INSTR) return;

    ch->instr = instr_idx;
    ch->fb  = (GM_FEEDBACK(instr_idx, 0) >> 1) & 0x7;
    ch->con = (GM_FEEDBACK(instr_idx, 0)) & 0x1;

    note = midi_note;
    if (GM_FLAGS(instr_idx) & GENMIDI_FLAG_FIXED) note = GM_NOTE(instr_idx);
    if (note < 0) note = 0;
    if (note > 127) note = 127;

    ad = GM_MOD_AD(instr_idx, 0); sr = GM_MOD_SR(instr_idx, 0);
    ar = (ad >> 4) & 0xF; dr = ad & 0xF; sl = (sr >> 4) & 0xF; rr = sr & 0xF;
    ch->mod.tl      = GM_MOD_LEVEL(instr_idx, 0) & 0x3F;
    ch->mod.wave    = GM_MOD_WAVE(instr_idx, 0) & 3;
    ch->mod.egt     = (GM_MOD_MUL(instr_idx, 0) >> 5) & 1;
    ch->mod.env_atk = rate_to_inc(ar);
    ch->mod.env_dec = rate_to_inc_release(dr);
    ch->mod.env_sus = (sl == 15) ? 0 : (ENV_MAX - sl * (ENV_MAX / 15));
    ch->mod.env_rel = rate_to_inc_release(rr); if (ch->mod.env_rel < 1) ch->mod.env_rel = 1;
    ch->mod.fb_prev = 0;
    ch->mod.fb_prev2 = 0;
    step = note_phase_step(note, GM_MOD_MUL(instr_idx, 0) & 0xF);
    op_key_on(&ch->mod, step, preserve_phase);

    ad = GM_CAR_AD(instr_idx, 0); sr = GM_CAR_SR(instr_idx, 0);
    ar = (ad >> 4) & 0xF; dr = ad & 0xF; sl = (sr >> 4) & 0xF; rr = sr & 0xF;
    tl_car = GM_CAR_LEVEL(instr_idx, 0) & 0x3F;
    vel_tl = (127 - velocity) * 63 / 127;
    tl_car = (tl_car + vel_tl) / 2;
    if (tl_car < 0) tl_car = 0;
    if (tl_car > 63) tl_car = 63;
    ch->car.tl      = tl_car;
    ch->car.wave    = GM_CAR_WAVE(instr_idx, 0) & 3;
    ch->car.egt     = (GM_CAR_MUL(instr_idx, 0) >> 5) & 1;
    ch->car.env_atk = rate_to_inc(ar);
    ch->car.env_dec = rate_to_inc_release(dr);
    ch->car.env_sus = (sl == 15) ? 0 : (ENV_MAX - sl * (ENV_MAX / 15));
    ch->car.env_rel = rate_to_inc_release(rr); if (ch->car.env_rel < 1) ch->car.env_rel = 1;
    ch->car.fb_prev = 0;
    step = note_phase_step(note, GM_CAR_MUL(instr_idx, 0) & 0xF);
    op_key_on(&ch->car, step, preserve_phase);
}

static short render_channel(hp_opl_chan_t *ch)
{
    int env_m, env_c, raw_m, raw_c, out;
    short mod_s, car_s;
    unsigned int car_phase;

    env_m = op_env_step(&ch->mod);
    env_c = op_env_step(&ch->car);

    if (env_c == 0 && ch->car.env_state == ENV_OFF) { ch->last_out = 0; return 0; }

    {
        unsigned int ph = ch->mod.phase;
        if (ch->fb > 0) {
            /* Real OPL2 feeds back the SUM of the modulator's last two raw
             * outputs (see Nuked-OPL2's OPL2_SlotCalcFB: fbmod = (prout +
             * out) >> (9 - fb)), not double the single most recent one.
             * Doubling a single sample amplifies rather than damps any
             * sign flip between consecutive samples, and at high feedback
             * (fb=7, used by several percussion instruments such as the
             * open hi-hat) that turned into runaway alternating-sign
             * oscillation - large sample-to-sample jumps audible as a
             * harsh screech, confirmed by comparing against Nuked-OPL2
             * (cycle-accurate, validated against real YM3812 hardware)
             * rendering the same instrument/note offline: Nuked stayed
             * smooth, this synth did not, until this fix. */
            int fb_idx = (ch->mod.fb_prev + ch->mod.fb_prev2) >> (9 - ch->fb);
            if (fb_idx >= 0) ph += (unsigned int)fb_idx << 22;
            else             ph -= (unsigned int)(-fb_idx) << 22;
        }
        mod_s = opl_wave(ph, ch->mod.wave);
        ch->mod.phase += ch->mod.phase_step;
    }

    raw_m = (int)mod_s * env_m / ENV_MAX;
    raw_m = (int)((long)raw_m * tl_attn[ch->mod.tl] / 32767L);
    ch->mod.fb_prev2 = ch->mod.fb_prev;
    ch->mod.fb_prev = raw_m / 512;

    car_phase = ch->car.phase;
    if (!ch->con) {
        int fm_idx = raw_m / 512;
        if (fm_idx >= 0) car_phase += (unsigned int)fm_idx << 22;
        else             car_phase -= (unsigned int)(-fm_idx) << 22;
    }
    car_s = opl_wave(car_phase, ch->car.wave);
    ch->car.phase += ch->car.phase_step;

    raw_c = (int)car_s * env_c / ENV_MAX;
    raw_c = (int)((long)raw_c * tl_attn[ch->car.tl] / 32767L);

    out = ch->con ? (raw_m + raw_c) / 2 : raw_c;
    out = out * (opl_master_vol + 1) / 16;
    if (out >  32767) out =  32767;
    if (out < -32767) out = -32767;

    if (ch->fade_remain > 0) {
        int t = FADE_SAMPLES - ch->fade_remain;
        out = ch->fade_from + (out - ch->fade_from) * t / FADE_SAMPLES;
        ch->fade_remain--;
    }
    ch->last_out = out;
    return (short)out;
}

static short opl_mix_sample(void)
{
    static long dc_x1 = 0, dc_y1 = 0;
    long yn;
    int i, sum = 0, n = 0;
    for (i = 0; i < NUM_OPL_CHAN; i++) {
        if (opl_chan[i].active) {
            sum += render_channel(&opl_chan[i]);
            n++;
            if (opl_chan[i].car.env_state == ENV_OFF && opl_chan[i].mod.env_state == ENV_OFF)
                opl_chan[i].active = 0;
        }
    }
    /* Mix by a FIXED divisor (total polyphony), not by how many channels
     * happen to be active on this particular sample. Dividing by the
     * live active-channel count n meant the perceived volume of every
     * already-sounding note jumped the instant any other channel turned
     * on or off - e.g. a new note starting quiet (early in its own
     * attack ramp, contributing almost nothing yet) still immediately
     * diluted every other note's share of the mix by upping the
     * denominator, an audible sudden volume "duck" with no accompanying
     * new sound to justify it. That's a different defect from the
     * per-sample synth glitches fixed above - it's an artifact of the
     * mixing/normalization strategy, not of any one instrument - and
     * matches what a user found by visually inspecting the waveform in
     * Audacity: a sharp envelope-level drop with no corresponding
     * synthesis discontinuity underneath it. A fixed divisor means a
     * channel's own volume is unaffected by unrelated channels turning
     * on/off; the tradeoff is headroom (all NUM_OPL_CHAN channels at
     * simultaneous full volume could in principle need it), which is
     * fine since the final clamp below still protects against overflow
     * and brief mild clipping under dense chords is far less audible
     * than periodic per-note volume pumping. Divisor 5 (rather than
     * NUM_OPL_CHAN=9) was chosen empirically: it's rare for all 9
     * channels to be simultaneously near full volume in practice, and
     * dividing by 9 unconditionally left the mix needlessly quiet
     * (measured ~3.6x lower RMS on docs/investigacion-musica.md's E1M1
     * test render than the old dynamic-n approach) for no additional
     * benefit. Divisor 4 measured just 1 >5000-magnitude sample jump
     * over a 60s render (vs. 0 at divisor 5, 93 at divisor 3, 816 - back
     * to being audibly bad - at divisor 2), while being noticeably
     * louder (RMS ~3956 vs. 3163 at divisor 5) - picked over divisor 5
     * for the better loudness/stability trade found by direct
     * measurement (see docs/investigacion-musica.md). */
    sum /= 4;
    if (sum >  32767) sum =  32767;
    if (sum < -32767) sum = -32767;

    /* DC blocker. Several GENMIDI carrier waveforms (half sine, full
     * rectified sine, quarter sine - opl_wave() cases 1-3) are, by OPL2
     * design, not centered on zero: their average level is intentionally
     * above zero (that "buzz"/edge is part of the intended timbre, e.g.
     * overdriven/distortion guitar patches). Real OPL2 sound hardware has
     * an output coupling capacitor that removes that DC bias before it
     * reaches the speaker; this software synth writes samples straight
     * through with no such filtering, so the bias is audible directly -
     * both as a visibly asymmetric waveform and, more importantly, as an
     * audible click on every note-on/note-off: the DC level itself steps
     * up or down abruptly whenever such a note starts or stops, and a
     * sudden DC step is a broadband transient just like any other click.
     * This compounds the envelope-release click already addressed above
     * (rate_to_inc_release()). Standard one-pole DC blocker (Julius O.
     * Smith's classic form: y[n] = x[n] - x[n-1] + R*y[n-1]), applied
     * once to the final mixed sample rather than per-channel since it's
     * the summed/averaged signal that actually reaches the output. R is
     * close to 1 so the cutoff (~2 Hz at this 11025 Hz rate) sits far
     * below any audible musical content - only true DC/near-DC is
     * removed. */
    yn = (long)sum - dc_x1 + (dc_y1 * 32723L) / 32768L;
    dc_x1 = sum;
    dc_y1 = yn;
    sum = (int)yn;
    if (sum >  32767) sum =  32767;
    if (sum < -32767) sum = -32767;

    return (short)sum;
}

static int alloc_opl_channel(int mus_chan, int note)
{
    int i, oldest = 0, oldest_ts = 0x7fffffff, free_ch = -1;
    for (i = 0; i < NUM_OPL_CHAN; i++)
        if (opl_chan[i].active && opl_chan[i].mus_chan == mus_chan && opl_chan[i].note == note)
            return i;
    for (i = 0; i < NUM_OPL_CHAN; i++) {
        if (!opl_chan[i].active) { free_ch = i; break; }
        if (opl_chan[i].timestamp < oldest_ts) { oldest_ts = opl_chan[i].timestamp; oldest = i; }
    }
    return (free_ch >= 0) ? free_ch : oldest;
}

static int find_opl_channel(int mus_chan, int note)
{
    int i;
    for (i = 0; i < NUM_OPL_CHAN; i++)
        if (opl_chan[i].active && opl_chan[i].mus_chan == mus_chan && opl_chan[i].note == note)
            return i;
    return -1;
}

/* ============================================================
 * MUS parser
 * ============================================================ */

static unsigned char *mus_data = NULL;
static int mus_len = 0, mus_pos = 0, mus_score = 0, mus_loop = 1;
static int mus_delay = 0, mus_playing = 0, mus_paused = 0;
static int mus_chan_instr[16], mus_chan_vol[16], mus_chan_pitch[16];

static int mus_read_byte(void)
{
    if (mus_data == NULL || mus_pos >= mus_len) return -1;
    return (unsigned char)mus_data[mus_pos++];
}

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

static int mus_process_tic(void)
{
    int event, type, chan, last, note, vel, ctrl, val, delay, opl_ch, instr;

    if (!mus_playing || mus_paused || mus_data == NULL) return 1;
    if (mus_delay > 0) { mus_delay--; return 1; }

    for (;;) {
        event = mus_read_byte();
        if (event < 0) goto done;
        last = (event >> 7) & 1;
        type = (event >> 4) & 7;
        chan = event & 0xF;

        switch (type) {
        case 0:
            note = mus_read_byte() & 0x7F;
            opl_ch = find_opl_channel(chan, note);
            if (opl_ch >= 0) { op_key_off(&opl_chan[opl_ch].mod); op_key_off(&opl_chan[opl_ch].car); }
            break;
        case 1:
            note = mus_read_byte();
            if (note & 0x80) { vel = mus_read_byte() & 0x7F; mus_chan_vol[chan] = vel; }
            else vel = mus_chan_vol[chan];
            note &= 0x7F;
            /* Percussion channel: key must fall in the General MIDI drum
             * range [35, 81] (47 instruments); anything outside that is
             * not a valid percussion sound and must be ignored, not
             * clamped to some other instrument. Chocolate Doom's
             * i_oplmusic.c KeyOnEvent() does the same: instrument index is
             * key-35, out-of-range keys are dropped. Clamping instead
             * (as this code used to do via "128 + (note & 0x3F)") could
             * pick an unrelated, much harsher-sounding percussion
             * instrument for any note outside that window. */
            if (chan == 15 && (note < 35 || note > 81)) break;
            {
                int same_note, was_active;
                opl_ch = alloc_opl_channel(chan, note);
                was_active = opl_chan[opl_ch].active;
                same_note = was_active && opl_chan[opl_ch].mus_chan == chan && opl_chan[opl_ch].note == note;
                instr = (chan == 15) ? (128 + (note - 35)) : mus_chan_instr[chan];
                if (instr >= GENMIDI_INSTR) instr = GENMIDI_INSTR - 1;
                if (was_active) {
                    opl_chan[opl_ch].fade_from = opl_chan[opl_ch].last_out;
                    opl_chan[opl_ch].fade_remain = FADE_SAMPLES;
                }
                load_instrument(&opl_chan[opl_ch], instr, note, vel, same_note);
                opl_chan[opl_ch].active = 1;
                opl_chan[opl_ch].mus_chan = chan;
                opl_chan[opl_ch].note = note;
                opl_chan[opl_ch].timestamp = opl_timestamp++;
            }
            break;
        case 2:
            mus_chan_pitch[chan] = mus_read_byte();
            break;
        case 3:
            mus_read_byte();
            break;
        case 4:
            ctrl = mus_read_byte(); val = mus_read_byte();
            if (ctrl == 0) mus_chan_instr[chan] = val & 0x7F;
            else if (ctrl == 3) mus_chan_vol[chan] = val & 0x7F;
            break;
        case 6:
        case 7:
            if (mus_loop) { mus_pos = mus_score; mus_delay = 0; return 1; }
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
 * Public API
 * ============================================================ */

void HPMusic_Init(const unsigned char *genmidi_data, int genmidi_len)
{
    init_sin_table();
    memset(opl_chan, 0, sizeof(opl_chan));
    genmidi_ok = 0;
    if (genmidi_data && genmidi_len >= 8 && memcmp(genmidi_data, "#OPL_II#", 8) == 0
        && genmidi_len <= (int)sizeof(genmidi))
    {
        memcpy(genmidi, genmidi_data, genmidi_len);
        genmidi_ok = 1;
    }
}

int HPMusic_GenMidiOk(void)
{
    return genmidi_ok;
}

void HPMusic_LoadSong(const unsigned char *data, int len, int loop)
{
    int i;
    if (len < 16 || data[0]!='M' || data[1]!='U' || data[2]!='S' || data[3]!=0x1A) return;

    if (mus_data) { free(mus_data); mus_data = NULL; }
    mus_data = (unsigned char*)malloc(len);
    if (!mus_data) return;
    memcpy(mus_data, data, len);
    mus_len = len;
    mus_loop = loop;
    mus_score = (data[6] & 0xFF) | ((data[7] & 0xFF) << 8);
    mus_pos = mus_score;
    mus_delay = 0;
    for (i = 0; i < 16; i++) { mus_chan_instr[i] = 0; mus_chan_vol[i] = 100; mus_chan_pitch[i] = 0; }
    memset(opl_chan, 0, sizeof(opl_chan));
}

void HPMusic_Play(int loop)
{
    mus_loop = loop;
    mus_playing = 1;
    mus_paused = 0;
}

void HPMusic_Pause(void)  { mus_paused = 1; }
void HPMusic_Resume(void) { mus_paused = 0; }

void HPMusic_Stop(void)
{
    int i;
    mus_playing = 0;
    mus_paused = 0;
    for (i = 0; i < NUM_OPL_CHAN; i++) opl_chan[i].active = 0;
}

void HPMusic_SetVolume(int vol_0_15)
{
    if (vol_0_15 < 0) vol_0_15 = 0;
    if (vol_0_15 > 15) vol_0_15 = 15;
    opl_master_vol = vol_0_15;
}

int HPMusic_IsPlaying(void)
{
    return mus_playing;
}

/* Generates `n` stereo-interleaved samples (2*n shorts) directly into buf.
 * Advances the MUS tic clock at the real HP_MUSIC_TEMPO_HZ rate relative
 * to HP_MUSIC_SAMPLE_RATE output samples, so tempo is always correct
 * regardless of how often/irregularly this function is called - same
 * self-pacing principle as I_SubmitSound()'s existing wall-clock
 * throttle, just applied to score advancement instead of Aserver writes. */
void HPMusic_Generate(short *buf, int n)
{
    static long tic_accum = 0;
    int i;

    for (i = 0; i < n; i++) {
        short s;
        if (mus_playing && !mus_paused) {
            tic_accum += HP_MUSIC_TEMPO_HZ;
            while (tic_accum >= HP_MUSIC_SAMPLE_RATE) {
                tic_accum -= HP_MUSIC_SAMPLE_RATE;
                mus_process_tic();
            }
        }
        s = opl_mix_sample();
        buf[i * 2]     = s;
        buf[i * 2 + 1] = s;
    }
}
