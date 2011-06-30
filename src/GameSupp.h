/*
 *  GameSupp.h
 *
 *  Purpose:	Prototypes for GameSupport module
 *
 *  Author:	Andreas Dehmel (dehmel@forwiss.tu-muenchen.de)
 *
 *  The GameSupport module provides safe (against hard crashes) frame buffer and
 *  sound fill code. It is written as an application-independent module providing
 *  an interface to low-level components typically needed in games and similar
 *  programs.
 */

#ifndef _GAME_SUPP_H_
#define _GAME_SUPP_H_

#include <kernel.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct game_supp_frame_buffer_s {
  unsigned int width;
  unsigned int height;
  unsigned int depth;
  unsigned int pitch;
  unsigned int colours;
  unsigned int buffers;
  unsigned int eigx;
  unsigned int eigy;
  unsigned int show;
  unsigned int plot;
  void *screens[1];	/* actual array size determined by buffers member */
} game_supp_frame_buffer_t;


typedef struct game_supp_dynamic_area_s {
  int number;
  unsigned int size;
  void *memory;
  unsigned int flags;
  char name[32];
} game_supp_dynamic_area_t;


extern _kernel_oserror *GameSupp_FrameBufferInfo(unsigned int numBuff, unsigned int width, unsigned int height, unsigned int depth, game_supp_frame_buffer_t **fbd);

extern _kernel_oserror *GameSupp_ClaimFrameBuffer(unsigned int numBuff, unsigned int width, unsigned int height, unsigned int depth, game_supp_frame_buffer_t **fbd);

extern void GameSupp_ReleaseFrameBuffer(void);

extern game_supp_frame_buffer_t *GameSupp_GetFrameBuffer(void);

extern int GameSupp_GetNextFrame(void);

extern void GameSupp_MarkFrameNumber(unsigned int num);

extern void GameSupp_DisplayFrameNumber(unsigned int num);

extern int GameSupp_SetFlags(int eor, int and);

extern void GameSupp_SetLogFile(const char *filename);


#define GameSupp_AbortRepair	1
#define GameSupp_AbortCallback	2

extern _kernel_oserror *GameSupp_InstallAbortGuard(int flags, void (*callback)(void *), void *context);

extern void GameSupp_RemoveAbortGuard(void);

extern _kernel_oserror *GameSupp_InstallExitHandler(void (*callback)(void *), void *context);

extern void GameSupp_RemoveExitHandler(void);

extern _kernel_oserror *GameSupp_ClaimSound(unsigned int channels, unsigned int sample_length, unsigned int sample_period, void (*callback)(void), const char *chmap);

extern void GameSupp_ReleaseSound(void);

extern _kernel_oserror *GameSupp_ClaimSound16(unsigned int freq, void (*callback)(void));

extern _kernel_oserror *GameSupp_ClaimDynamicArea(unsigned int size, unsigned int leavemem, unsigned int flags, const char *name);

extern const game_supp_dynamic_area_t *GameSupp_GetDynamicArea(void);

extern void GameSupp_ReleaseDynamicArea(void);

extern _kernel_oserror *GameSupp_ClaimKeyPress(void);

extern void GameSupp_ReleaseKeyPress(void);

extern void GameSupp_FlushKeyPress(void);

extern int GameSupp_GetKeyPress(void);

extern void GameSupp_FillMemory16(void *dest, short value, unsigned int size);

extern void GameSupp_FillMemory32(void *dest, long value, unsigned int size);

extern void *GameSupp_ScreenBaseAddress(int *size);

#ifdef __cplusplus
}
#endif

#endif
