/*
 * opl2test_nuked.c — OPL2 synthesis test using Nuked-OPL2-Lite (nukeykt),
 * a cycle-accurate emulator verified against real YM3812 hardware.
 *
 * Replaces the hand-rolled OPL2 synth in opl2test_hpux.c, which after
 * extensive debugging still produced dense broadband noise with real
 * Doom GENMIDI patches (multiple contributing bugs were found and fixed,
 * but the synthesis itself remained unreliable). This uses the real
 * chip's register-level model directly: GENMIDI bytes are the OPL2
 * register bytes verbatim (that's what GENMIDI was designed for), so the
 * adapter just writes them through OPL2_WriteReg() and lets Nuked-OPL2
 * do the actual DSP.
 *
 * Build (HP-UX, needs -Ae for the "long long" used by uint64_t):
 *   cc -Ae +O2 -o opl2test_nuked opl2test_nuked.c opl2.c -lm
 *
 * Run:
 *   ./opl2test_nuked doom.wad D_INTRO.mus > out.wav
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "opl2.h"

#define SAMPLE_RATE 11025
#define MUS_TEMPO_HZ 140
#define OPL2_CLOCK 49716

/* ============================================================
 * GENMIDI constants & macros.
 *
 * Real on-disk layout (verified against Chocolate Doom's i_oplmusic.c,
 * genmidi_instr_t/genmidi_voice_t/genmidi_op_t), NOT what the previous
 * version of this file assumed:
 *
 *   8-byte header "#OPL_II#"
 *   175 x 36-byte genmidi_instr_t (128 melodic + 47 percussion), i.e.
 *       flags(u16 LE) + fine_tuning(u8) + fixed_note(u8) + voice[2]
 *       each voice = 16 bytes:
 *         modulator op(6) + feedback(1) + carrier op(6) + unused(1)
 *         + base_note_offset(s16 LE)
 *       each op(6) = tremolo,attack,sustain,waveform,scale,level
 *   175 x 32-byte instrument name strings (unused for playback)
 *
 * Total = 8 + 175*36 + 175*32 = 11908, matching the real GENMIDI lump
 * size. The previous version treated the file as 175 interleaved
 * 68-byte (36+32) records, which only happened to line up for
 * instrument 0 and drifted further out of alignment (eventually
 * reading past the binary section into the name strings) for every
 * instrument after that — this is what made every instrument sound
 * like a plain tone (modulator data effectively randomized/missing)
 * and made some (e.g. instrument 39) produce outright noise.
 *
 * scale/level are two separate bytes that must be OR'd together to
 * form the single OPL 0x40 (KSL/output-level) register write — they
 * are NOT already-combined verbatim register bytes.
 * ============================================================ */

#define GENMIDI_HDR         8
#define GENMIDI_NUM_MELODIC 128
#define GENMIDI_NUM_PERC    47
#define GENMIDI_INSTR       (GENMIDI_NUM_MELODIC + GENMIDI_NUM_PERC)
#define GENMIDI_ISIZ        36   /* flags+fine_tuning+fixed_note+2 voices */
#define GENMIDI_VSIZ        16   /* one genmidi_voice_t */
#define GENMIDI_NAMESIZ     32
#define GENMIDI_TOTAL       (GENMIDI_HDR + GENMIDI_INSTR * GENMIDI_ISIZ \
                              + GENMIDI_INSTR * GENMIDI_NAMESIZ)
#define GENMIDI_FLAG_FIXED  0x0001

#define GM_INSTR_OFS(i)  (GENMIDI_HDR + (i)*GENMIDI_ISIZ)
#define GM_VOICE(i,v)    (GM_INSTR_OFS(i) + 4 + (v)*GENMIDI_VSIZ)

#define GM_MOD_TREM(i,v)  (genmidi[GM_VOICE(i,v)+0])
#define GM_MOD_ATT(i,v)   (genmidi[GM_VOICE(i,v)+1])
#define GM_MOD_SUS(i,v)   (genmidi[GM_VOICE(i,v)+2])
#define GM_MOD_WAVE(i,v)  (genmidi[GM_VOICE(i,v)+3])
#define GM_MOD_SCALE(i,v) (genmidi[GM_VOICE(i,v)+4])
#define GM_MOD_LEVEL(i,v) (genmidi[GM_VOICE(i,v)+5])
#define GM_FEEDBACK(i,v)  (genmidi[GM_VOICE(i,v)+6])
#define GM_CAR_TREM(i,v)  (genmidi[GM_VOICE(i,v)+7])
#define GM_CAR_ATT(i,v)   (genmidi[GM_VOICE(i,v)+8])
#define GM_CAR_SUS(i,v)   (genmidi[GM_VOICE(i,v)+9])
#define GM_CAR_WAVE(i,v)  (genmidi[GM_VOICE(i,v)+10])
#define GM_CAR_SCALE(i,v) (genmidi[GM_VOICE(i,v)+11])
#define GM_CAR_LEVEL(i,v) (genmidi[GM_VOICE(i,v)+12])
/* +13 = unused, +14/+15 = base_note_offset (s16 LE), not used here */

#define GM_FLAGS(i)      (genmidi[GM_INSTR_OFS(i)+0] | \
                          ((int)genmidi[GM_INSTR_OFS(i)+1]<<8))
#define GM_NOTE(i)       (genmidi[GM_INSTR_OFS(i)+3])

static unsigned char genmidi[GENMIDI_TOTAL];

/* ============================================================
 * OPL2 register offsets per channel (standard AdLib/OPL2 map)
 * ============================================================ */

static const int op_offset[9] = {0x00,0x01,0x02, 0x08,0x09,0x0A, 0x10,0x11,0x12};

static opl2_chip chip;

/* ============================================================
 * MIDI note -> F-Number/Block
 * ============================================================ */

static void note_to_fnum_block(int midi_note, int *out_fnum, int *out_block)
{
    double freq = 440.0 * pow(2.0, (midi_note - 69.0) / 12.0);
    int block;
    for (block = 0; block <= 7; block++) {
        double fnum = freq * pow(2.0, 20 - block) / (double)OPL2_CLOCK;
        if (fnum < 1024.0) {
            *out_fnum = (int)(fnum + 0.5);
            if (*out_fnum > 1023) { *out_fnum = 1023; }
            *out_block = block;
            return;
        }
    }
    *out_fnum = 1023;
    *out_block = 7;
}

/* ============================================================
 * Load a GENMIDI instrument into an OPL2 channel and key it on.
 * GENMIDI bytes are literally OPL2 register bytes (0x20/0x40/0x60/
 * 0x80/0xE0 layout), so this is a direct passthrough plus F-num/
 * block/key-on computed from the MIDI note.
 * ============================================================ */

static void opl_load_instrument(int ch, int instr_idx, int midi_note, int velocity)
{
    int note, fnum, block;
    int mod_off, car_off;
    int mod_level, car_level, car_tl, vel_tl;

    if (instr_idx < 0 || instr_idx >= GENMIDI_INSTR) return;

    note = midi_note;
    if (GM_FLAGS(instr_idx) & GENMIDI_FLAG_FIXED) note = GM_NOTE(instr_idx);
    if (note < 0) note = 0;
    if (note > 127) note = 127;

    mod_off = op_offset[ch];
    car_off = op_offset[ch] + 3;

    /* Modulator. scale/level are two separate GENMIDI bytes that must be
     * OR'd together for the single 0x40 (KSL/output-level) register —
     * see LoadOperatorData() in Chocolate Doom's i_oplmusic.c. */
    mod_level = GM_MOD_SCALE(instr_idx,0) | GM_MOD_LEVEL(instr_idx,0);
    OPL2_WriteReg(&chip, 0x20 + mod_off, GM_MOD_TREM(instr_idx,0));
    OPL2_WriteReg(&chip, 0x40 + mod_off, mod_level);
    OPL2_WriteReg(&chip, 0x60 + mod_off, GM_MOD_ATT(instr_idx,0));
    OPL2_WriteReg(&chip, 0x80 + mod_off, GM_MOD_SUS(instr_idx,0));
    OPL2_WriteReg(&chip, 0xE0 + mod_off, GM_MOD_WAVE(instr_idx,0));

    /* Carrier: TL blended with velocity like real Doom's OPL driver does
     * (higher velocity = less attenuation), applied to the low 6 bits
     * (output level) of the combined scale|level byte. */
    car_tl = GM_CAR_LEVEL(instr_idx,0) & 0x3F;
    vel_tl = (127 - velocity) * 63 / 127;
    car_tl = (car_tl + vel_tl) / 2;
    if (car_tl < 0) car_tl = 0;
    if (car_tl > 63) car_tl = 63;
    car_level = (GM_CAR_SCALE(instr_idx,0) & 0xC0) | car_tl;

    OPL2_WriteReg(&chip, 0x20 + car_off, GM_CAR_TREM(instr_idx,0));
    OPL2_WriteReg(&chip, 0x40 + car_off, car_level);
    OPL2_WriteReg(&chip, 0x60 + car_off, GM_CAR_ATT(instr_idx,0));
    OPL2_WriteReg(&chip, 0x80 + car_off, GM_CAR_SUS(instr_idx,0));
    OPL2_WriteReg(&chip, 0xE0 + car_off, GM_CAR_WAVE(instr_idx,0));

    /* Feedback/connection */
    OPL2_WriteReg(&chip, 0xC0 + ch, GM_FEEDBACK(instr_idx,0));

    /* F-number/block + key-on */
    note_to_fnum_block(note, &fnum, &block);
    OPL2_WriteReg(&chip, 0xA0 + ch, fnum & 0xFF);
    OPL2_WriteReg(&chip, 0xB0 + ch, 0x20 | (block << 2) | ((fnum >> 8) & 0x3));
}

static void opl_key_off(int ch)
{
    /* Re-issue 0xB0 with key-on bit cleared, keeping block/fnum high bits.
     * We don't track prior fnum/block here since MUS note-off doesn't need
     * them — just clear bit 5. Simplest: write block=0 without key-on;
     * the channel's envelope goes to release regardless of fnum value. */
    OPL2_WriteReg(&chip, 0xB0 + ch, 0x00);
}

/* ============================================================
 * MUS-channel <-> OPL-channel allocation (unchanged logic from
 * opl2test_hpux.c: LRU voice stealing across 9 physical channels)
 * ============================================================ */

typedef struct {
    int active;
    int mus_chan;
    int note;
    int timestamp;
} opl_alloc_t;

static opl_alloc_t opl_alloc[9];
static int opl_timestamp = 0;

static int alloc_opl_channel(int mus_chan, int note)
{
    int i, oldest = 0, oldest_ts = 0x7fffffff, free_ch = -1;
    for (i = 0; i < 9; i++) {
        if (opl_alloc[i].active && opl_alloc[i].mus_chan == mus_chan && opl_alloc[i].note == note)
            return i;
    }
    for (i = 0; i < 9; i++) {
        if (!opl_alloc[i].active) { free_ch = i; break; }
        if (opl_alloc[i].timestamp < oldest_ts) { oldest_ts = opl_alloc[i].timestamp; oldest = i; }
    }
    return (free_ch >= 0) ? free_ch : oldest;
}

static int find_opl_channel(int mus_chan, int note)
{
    int i;
    for (i = 0; i < 9; i++) {
        if (opl_alloc[i].active && opl_alloc[i].mus_chan == mus_chan && opl_alloc[i].note == note)
            return i;
    }
    return -1;
}

/* ============================================================
 * MUS parser (same protocol/logic as opl2test_hpux.c)
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
    int event, type, chan, last;
    int note, vel, ctrl, val, delay;
    int opl_ch, instr;

    if (!mus_playing || mus_paused || mus_data == NULL) return 1;

    if (mus_delay > 0) { mus_delay--; return 1; }

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
                opl_key_off(opl_ch);
                opl_alloc[opl_ch].active = 0;
            }
            break;

        case 1: /* Note on */
            note = mus_read_byte();
            if (note & 0x80) {
                vel = mus_read_byte() & 0x7F;
                mus_chan_vol[chan] = vel;
            } else {
                vel = mus_chan_vol[chan];
            }
            note &= 0x7F;
            opl_ch = alloc_opl_channel(chan, note);
            instr = (chan == 15) ? (128 + (note & 0x3F)) : mus_chan_instr[chan];
            if (instr >= GENMIDI_INSTR) instr = GENMIDI_INSTR - 1;
            opl_load_instrument(opl_ch, instr, note, vel);
            opl_alloc[opl_ch].active    = 1;
            opl_alloc[opl_ch].mus_chan  = chan;
            opl_alloc[opl_ch].note      = note;
            opl_alloc[opl_ch].timestamp = opl_timestamp++;
            fprintf(stderr, "  t=%d note_on chan=%d note=%d instr=%d vel=%d opl_ch=%d\n",
                    opl_timestamp, chan, note, instr, vel, opl_ch);
            break;

        case 2: /* Pitch wheel */
            mus_chan_pitch[chan] = mus_read_byte();
            break;

        case 3: /* System event */
            mus_read_byte();
            break;

        case 4: /* Change controller */
            ctrl = mus_read_byte();
            val  = mus_read_byte();
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

static void mus_load(unsigned char *data, int len, int loop)
{
    int i;
    if (mus_data) { free(mus_data); mus_data = NULL; }
    mus_data = (unsigned char*)malloc(len);
    if (!mus_data) return;
    memcpy(mus_data, data, len);
    mus_len = len;
    mus_loop = loop;
    mus_score = (data[6] & 0xFF) | ((data[7] & 0xFF) << 8);
    mus_pos = mus_score;
    mus_delay = 0;
    for (i = 0; i < 16; i++) {
        mus_chan_instr[i] = 0;
        mus_chan_vol[i] = 100;
        mus_chan_pitch[i] = 0;
    }
}

/* ============================================================
 * WAD / GENMIDI loading (same as opl2test_hpux.c)
 * ============================================================ */

static int read_le32(const unsigned char *p)
{
    return p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
}

static int load_genmidi_from_wad(const char *wadpath)
{
    FILE *f;
    unsigned char hdr[12];
    int numlumps, dirofs, i;
    unsigned char entry[16];

    f = fopen(wadpath, "rb");
    if (!f) { fprintf(stderr, "Cannot open WAD: %s\n", wadpath); return 0; }
    if (fread(hdr, 1, 12, f) != 12) { fclose(f); return 0; }
    if (memcmp(hdr, "IWAD", 4) != 0 && memcmp(hdr, "PWAD", 4) != 0) {
        fprintf(stderr, "Not a WAD file\n"); fclose(f); return 0;
    }
    numlumps = read_le32(hdr+4);
    dirofs   = read_le32(hdr+8);

    fseek(f, dirofs, SEEK_SET);
    for (i = 0; i < numlumps; i++) {
        int lump_ofs, lump_sz;
        char name[9];
        if (fread(entry, 1, 16, f) != 16) break;
        lump_ofs = read_le32(entry+0);
        lump_sz  = read_le32(entry+4);
        memcpy(name, entry+8, 8);
        name[8] = '\0';
        if (strncmp(name, "GENMIDI", 7) == 0) {
            if (lump_sz < GENMIDI_TOTAL) { fclose(f); return 0; }
            fseek(f, lump_ofs, SEEK_SET);
            if (fread(genmidi, 1, GENMIDI_TOTAL, f) != GENMIDI_TOTAL) { fclose(f); return 0; }
            if (memcmp(genmidi, "#OPL_II#", 8) != 0) { fclose(f); return 0; }
            fprintf(stderr, "GENMIDI loaded from %s\n", wadpath);
            fclose(f);
            return 1;
        }
    }
    fprintf(stderr, "GENMIDI lump not found in WAD\n");
    fclose(f);
    return 0;
}

/* ============================================================
 * WAV writer
 * ============================================================ */

static void write_le16(unsigned char *p, int v) { p[0]=v&0xFF; p[1]=(v>>8)&0xFF; }
static void write_le32(unsigned char *p, int v) { p[0]=v&0xFF; p[1]=(v>>8)&0xFF; p[2]=(v>>16)&0xFF; p[3]=(v>>24)&0xFF; }

static void write_wav_header(int num_samples)
{
    unsigned char h[44];
    int data_bytes = num_samples * 2 * 2;
    memset(h, 0, 44);
    memcpy(h, "RIFF", 4);
    write_le32(h+4, 36 + data_bytes);
    memcpy(h+8, "WAVE", 4);
    memcpy(h+12, "fmt ", 4);
    write_le32(h+16, 16);
    write_le16(h+20, 1);
    write_le16(h+22, 2);
    write_le32(h+24, SAMPLE_RATE);
    write_le32(h+28, SAMPLE_RATE * 2 * 2);
    write_le16(h+32, 4);
    write_le16(h+34, 16);
    memcpy(h+36, "data", 4);
    write_le32(h+40, data_bytes);
    fwrite(h, 1, 44, stdout);
}

/* ============================================================
 * Song playback: growable output buffer, MUS-tic-driven loop
 * ============================================================ */

/* Raw little-endian bytes, not native shorts — see opl2test_hpux.c's
 * out_push() for why: HP-UX PA-RISC is big-endian, standard WAV PCM
 * data is always little-endian. out_len stays in "logical short" units. */
static unsigned char *out_buf = NULL;
static int out_cap = 0, out_len = 0;

static void out_push(short s)
{
    if (out_len >= out_cap) {
        out_cap = out_cap ? out_cap * 2 : 65536;
        out_buf = (unsigned char*)realloc(out_buf, out_cap * 2);
    }
    out_buf[out_len*2]     = (unsigned char)(s & 0xFF);
    out_buf[out_len*2 + 1] = (unsigned char)((s >> 8) & 0xFF);
    out_len++;
}

static int play_mus_file(const char *path, double max_seconds)
{
    FILE *f;
    unsigned char *buf;
    long fsize;
    int max_samples = (int)(max_seconds * SAMPLE_RATE) * 2;
    int frame_samples = SAMPLE_RATE / MUS_TEMPO_HZ;

    f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Cannot open MUS file: %s\n", path); return 0; }
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (unsigned char*)malloc(fsize);
    if (fread(buf, 1, fsize, f) != (size_t)fsize) { fclose(f); free(buf); return 0; }
    fclose(f);

    if (buf[0]!='M' || buf[1]!='U' || buf[2]!='S' || buf[3]!=0x1A) {
        fprintf(stderr, "Not a MUS file\n"); free(buf); return 0;
    }

    fprintf(stderr, "Loaded MUS %s: %ld bytes\n", path, fsize);
    mus_load(buf, (int)fsize, 0);
    free(buf);

    mus_playing = 1;
    mus_paused = 0;

    {
    int frame_count = 0;
    while (mus_playing && !mus_paused && out_len < max_samples) {
        int i;
        mus_process_tic();
        frame_count++;
        if (frame_count % 20 == 0) {
            fprintf(stderr, "frame %d out_len=%d\n", frame_count, out_len);
            fflush(stderr);
        }
        for (i = 0; i < frame_samples; i++) {
            int16_t s;
            OPL2_GenerateResampled(&chip, &s);
            out_push((short)s);  /* L */
            out_push((short)s);  /* R */
        }
    }
    }
    fprintf(stderr, "Playback ended: %.2fs\n", (double)out_len / 2.0 / SAMPLE_RATE);
    fflush(stderr);
    return 1;
}

/* ============================================================
 * Instrument sweep: play each GENMIDI instrument in turn on a
 * fixed note, so timbres can be checked in isolation without the
 * MUS parser/channel-allocation/tempo logic in the way.
 * ============================================================ */

static const char *gm_name(int i)
{
    static const char *melodic[128] = {
        "Acoustic Grand Piano","Bright Acoustic Piano","Electric Grand Piano","Honky-tonk Piano",
        "Electric Piano 1","Electric Piano 2","Harpsichord","Clavinet",
        "Celesta","Glockenspiel","Music Box","Vibraphone",
        "Marimba","Xylophone","Tubular Bells","Dulcimer",
        "Drawbar Organ","Percussive Organ","Rock Organ","Church Organ",
        "Reed Organ","Accordion","Harmonica","Tango Accordion",
        "Acoustic Guitar (nylon)","Acoustic Guitar (steel)","Electric Guitar (jazz)","Electric Guitar (clean)",
        "Electric Guitar (muted)","Overdriven Guitar","Distortion Guitar","Guitar Harmonics",
        "Acoustic Bass","Electric Bass (finger)","Electric Bass (pick)","Fretless Bass",
        "Slap Bass 1","Slap Bass 2","Synth Bass 1","Synth Bass 2",
        "Violin","Viola","Cello","Contrabass",
        "Tremolo Strings","Pizzicato Strings","Orchestral Harp","Timpani",
        "String Ensemble 1","String Ensemble 2","Synth Strings 1","Synth Strings 2",
        "Choir Aahs","Voice Oohs","Synth Voice","Orchestra Hit",
        "Trumpet","Trombone","Tuba","Muted Trumpet",
        "French Horn","Brass Section","Synth Brass 1","Synth Brass 2",
        "Soprano Sax","Alto Sax","Tenor Sax","Baritone Sax",
        "Oboe","English Horn","Bassoon","Clarinet",
        "Piccolo","Flute","Recorder","Pan Flute",
        "Blown Bottle","Shakuhachi","Whistle","Ocarina",
        "Lead 1 (square)","Lead 2 (sawtooth)","Lead 3 (calliope)","Lead 4 (chiff)",
        "Lead 5 (charang)","Lead 6 (voice)","Lead 7 (fifths)","Lead 8 (bass+lead)",
        "Pad 1 (new age)","Pad 2 (warm)","Pad 3 (polysynth)","Pad 4 (choir)",
        "Pad 5 (bowed)","Pad 6 (metallic)","Pad 7 (halo)","Pad 8 (sweep)",
        "FX 1 (rain)","FX 2 (soundtrack)","FX 3 (crystal)","FX 4 (atmosphere)",
        "FX 5 (brightness)","FX 6 (goblins)","FX 7 (echoes)","FX 8 (sci-fi)",
        "Sitar","Banjo","Shamisen","Koto",
        "Kalimba","Bagpipe","Fiddle","Shanai",
        "Tinkle Bell","Agogo","Steel Drums","Woodblock",
        "Taiko Drum","Melodic Tom","Synth Drum","Reverse Cymbal",
        "Guitar Fret Noise","Breath Noise","Seashore","Bird Tweet",
        "Telephone Ring","Helicopter","Applause","Gunshot"
    };
    if (i >= 0 && i < 128) return melodic[i];
    return "Percussion (fixed-note)";
}

static void play_instrument_sweep(int start, int count)
{
    int i, s;
    double note_secs = 0.6, rel_secs = 0.25;
    int note_samples = (int)(note_secs * SAMPLE_RATE);
    int rel_samples = (int)(rel_secs * SAMPLE_RATE);

    if (start < 0) start = 0;
    if (start + count > GENMIDI_INSTR) count = GENMIDI_INSTR - start;

    for (i = start; i < start + count; i++) {
        fprintf(stderr, "instrument %3d: %s\n", i, gm_name(i));
        opl_load_instrument(0, i, 60 /* middle C */, 127);
        for (s = 0; s < note_samples; s++) {
            int16_t v;
            OPL2_GenerateResampled(&chip, &v);
            out_push((short)v);
            out_push((short)v);
        }
        opl_key_off(0);
        for (s = 0; s < rel_samples; s++) {
            int16_t v;
            OPL2_GenerateResampled(&chip, &v);
            out_push((short)v);
            out_push((short)v);
        }
    }
}

/* ============================================================
 * main
 * ============================================================ */

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <doom.wad> <song.mus>\n", argv[0]);
        fprintf(stderr, "       %s <doom.wad> --instruments [start] [count]\n", argv[0]);
        return 1;
    }

    if (!load_genmidi_from_wad(argv[1]))
        return 1;

    OPL2_Reset(&chip, SAMPLE_RATE);
    /* Enable waveform select (register 0x01, bit 5) — without this the
     * chip silently ignores non-sine OPL2_WriteReg(0xE0+op, ...) writes
     * and every operator falls back to plain sine, regardless of what
     * GENMIDI's wave field says. Real AdLib/OPL2 drivers (including
     * Doom's) always set this once at startup. */
    OPL2_WriteReg(&chip, 0x01, 0x20);
    memset(opl_alloc, 0, sizeof(opl_alloc));

    if (strcmp(argv[2], "--instruments") == 0) {
        int start = (argc > 3) ? atoi(argv[3]) : 0;
        int count = (argc > 4) ? atoi(argv[4]) : GENMIDI_INSTR;
        play_instrument_sweep(start, count);
    } else {
        if (!play_mus_file(argv[2], 30.0))
            return 1;
    }

    write_wav_header(out_len / 2);
    fwrite(out_buf, 1, out_len * 2, stdout);
    fflush(stdout);
    return 0;
}
