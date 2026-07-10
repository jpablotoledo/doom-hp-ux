/*
 * hp_music.h
 * In-process OPL2/GENMIDI music synthesizer for Doom on HP-UX.
 * See hp_music.c for design notes.
 */
#ifndef __HP_MUSIC_H__
#define __HP_MUSIC_H__

void HPMusic_Init(const unsigned char *genmidi_data, int genmidi_len);
int  HPMusic_GenMidiOk(void);
void HPMusic_LoadSong(const unsigned char *data, int len, int loop);
void HPMusic_Play(int loop);
void HPMusic_Pause(void);
void HPMusic_Resume(void);
void HPMusic_Stop(void);
void HPMusic_SetVolume(int vol_0_15);
int  HPMusic_IsPlaying(void);
void HPMusic_Generate(short *buf, int n);

#endif
