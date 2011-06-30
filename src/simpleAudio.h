/*
 * Simplified audio stream playing package
 * Source: /opt/audio/src/simpleAudio/ (HP-UX B.11.00)
 */

extern int openAudio();
extern void closeAudio();

#define PLAY_STREAM   0
#define RECORD_STREAM 1

#define USE_MONO   1
#define USE_STEREO 2

#define USE_MULAW  1
#define USE_LIN16  0
#define USE_ALAW  -1

#define USE_INTERNAL_SPEAKER   1
#define USE_EXTERNAL_SPEAKER   0
#define USE_DEFAULT_SPEAKER   -1

#define USE_MIKE_IN           1
#define USE_LINE_IN           0
#define USE_DEFAULT_IN       -1

#define START_PAUSED       1
#define START_IMMEDIATELY  0

#define AUDIO_HAS_LIN16  1
#define AUDIO_HAS_MULAW  2
#define AUDIO_HAS_ALAW   4
#define AUDIO_HAS_LIN8   8
#define AUDIO_HAS_LIN8O  0x10

#ifdef __STDC__
extern int openAStream(int streamMode, int sampleRate, int channels,
                       int dataFormat, int device, int startPaused);
extern void closeAStream(int fd);
extern int  getAudioCapabilities(int *numChannels, int **sampleRates,
                                 int *numRates, int *dataFormats);
extern void pauseAStream(int fd);
extern void resumeAStream(int fd);
extern void stopAStream(int fd);
extern void setAStreamVolume(int fd, int volume, int useDB);
#else
extern int openAStream();
extern void closeAStream();
extern void pauseAStream();
extern void resumeAStream();
extern void stopAStream();
extern void setAStreamVolume();
#endif
