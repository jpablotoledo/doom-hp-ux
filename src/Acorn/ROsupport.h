/*
 *  ROSupport.h
 *
 *  Purpose:	RISC OS specific support code
 *
 *  Author:	Andreas Dehmel (dehmel@forwiss.tu-muenchen.de)
 *
 *  These are the prototypes for most of the RISC OS specific code.
 */

#ifndef __RO_SUPPORT__
#define __RO_SUPPORT__


#include <kernel.h>
#include "doomtype.h"
#include "r_defs.h"
#include "m_fixed.h"


#ifdef __cplusplus
extern "C" {
#endif





/* Define this if you want buffered keyboard input */
/*#define BUFFERED_KEYBOARD*/



/* For access-simulation */
#define R_OK	1
#define W_OK	2
#define X_OK	4





int access(const char *file, int mode);
/* no const char, adds /wad extension if base name not found! */
int accessWAD(char *wadfile);

/* Special compare functions */
int strcasecmp (const char *str1, const char *str2);
int strncasecmp (const char *str1, const char *str2, size_t size);





/* Holds information about the screen mode */
typedef struct {
  unsigned char *baseadr;
  int size;
  int resx;
  int resy;
} screen_desc;


/* For storing information about the mouse state */
typedef struct {
  int x, y;
  int buttons;
  unsigned int time;
} mouse_desc;


struct draw_context_s;
struct game_supp_frame_buffer_s;



/*
 *  RISC OS system calls (assembler interface)
 */

/* General stuff */
unsigned int ReadMonotonicTime(void);
unsigned char *LowerCaseTable(void);
int ScanKeyboard(int from);
void FlushABuffer(int number);
int InsertInBuffer(int buffer, int code);
int ReadFileInfo(const char *name, unsigned int info[4]);
void SetFiletype(const char *name, int filetype);
int ChangeEscapeEffect(int eorval, int andval);
const int *ReadExceptionRegisters(void);
int WimpReadSlotSize(void);
int RunningIn32bitMode(void);
#ifndef __UNIXLIB_TYPES_H
void ReadProcessorRegisters(_kernel_unwindblock *ub);
#endif


/* Frame buffer and video stuff */
void ReadScreenInfo(screen_desc *sd);
int  PrepareWithoutFrames(struct game_supp_frame_buffer_s *fbd);
void WaitForVSync(void);
void HomeCursor(void);
void ReadMouseState(mouse_desc *md);
void PutMouseAt(int x, int y);



/* Fast native assembler stuff */
/* The functions InstallPalette, TranslateColourmaps16 and TranslateColourmaps32 are */
/* used to make sure nobody can link the wrong plotter class with the C-object files. */
#if (LD_PIXEL_DEPTH == 3)
/* Hardware palette manipulation is only necessary at 8bpp */
void InstallPalette(const byte *palette);
void ReadPalette(byte *buffer, int buffersize);
byte ReturnClosestColour(byte red, byte green, byte blue);
#else
/* Hardware gamma correction */
void InstallGamma(const char *gtable);
void I_TranslateBaseMap(const byte *palette, lighttable_t *tmap, const byte *cmap);
# if (LD_PIXEL_DEPTH == 4)
void TranslateColourmaps16(struct draw_context_s *ctx, const byte *palette, const byte *cmaps);
/* Eye-watering hack to render 2 columns in parallel in 16bpp modes */
void R_DrawDoubleColumn(struct draw_context_s *ctx);
# elif (LD_PIXEL_DEPTH == 5)
void TranslateColourmaps32(struct draw_context_s *ctx, const byte *palette, const byte *cmaps);
# endif
#endif
void Rarm_ResampleColumn(struct draw_context_s *ctx, const byte *col1, const byte *col2, int of);
void Rarm_DrawViewBorder(pixel_t *dest, const pixel_t *src, int top, int side, int screenwidth, int viewheight);
void Rarm_DrawMaskedColumn(struct draw_context_s *ctx, const column_t *column);
void Rarm_DrawMaskedColumnLow(struct draw_context_s *ctx, const column_t *column);
void Rarm_DrawMaskedColumnTranslucent(struct draw_context_s *ctx, const column_t *column);
void Rarm_DrawMaskedColumnLowTranslucent(struct draw_context_s *ctx, const column_t *column);
void Varm_CopyRect(const pixel_t *src, pixel_t *dest, int width, int height, int srcadd, int destadd);
void Varm_DrawPatch(struct draw_context_s *ctx, const patch_t *patch, pixel_t *desttop, int coladd);


/* Sound */
extern int InstallDoomSound(int channelBase, int sampleLength, int samplePeriod);
extern void RemoveDoomSound(void);
extern int InstallDoomSound16(unsigned int freq);

#ifdef __cplusplus
}
#endif

#endif
