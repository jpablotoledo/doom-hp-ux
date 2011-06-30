/*
 *  p_saven.h
 *
 *  Purpose:	portable savegames;
 *
 *  Author:	Andreas Dehmel (dehmel@forwiss.tu-muenchen.de)
 *
 *  New format savegames, portable and less likely to crash when the levels
 *  don't match. Also usable for all variations of vanilla Doom, Boom, full
 *  new features or not, bells & whistles.
 */

#ifndef __P_SAVEG2__
#define __P_SAVEG2__

#ifdef __GNUG__
#pragma interface
#endif

#include "p_saveg.h"


int  P_FlushSavebufferNew(void);

void P_ArchivePlayersNew(void);
void P_UnArchivePlayersNew(void);
void P_ArchiveWorldNew(void);
void P_UnArchiveWorldNew(void);
void P_ArchiveThinkersNew(void);
void P_UnArchiveThinkersNew(void);
void P_ArchiveSpecialsNew(void);
void P_UnArchiveSpecialsNew(void);
void P_ArchiveMapNew(void);
void P_UnArchiveMapNew(void);

#endif
