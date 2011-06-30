#include <stdlib.h>
#include <string.h>

#include "i_system.h"
#include "d_event.h"
#include "d_net.h"
#include "m_argv.h"
#include "doomstat.h"




/* Dummy network version. */
void I_InitNetwork (void)
{
  int i;

  if ((doomcom = malloc (sizeof(*doomcom))) == NULL)
  {
    I_Error("Unable to claim memory!");
  }
  memset (doomcom, 0, sizeof(*doomcom));

  i = M_CheckParm("-dup");
  if ((i!=0) && (i < myargc-1))
  {
    doomcom->ticdup = myargv[i+1][0]='0';
    if (doomcom->ticdup < 1)
        doomcom->ticdup = 1;
    if (doomcom->ticdup > 9)
        doomcom->ticdup = 9;
  }
  else
  {
    doomcom->ticdup = 1;
  }
  if (M_CheckParm("-extratic"))
    doomcom->extratics = 1;
  else
    doomcom->extratics = 0;

  i = M_CheckParm("-net");
  if (!i)
  {
    netgame = false;
    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = doomcom->numnodes = 1;
    doomcom->deathmatch = false;
    doomcom->consoleplayer = 0;
    return;
  }
  else
  {
    I_Error("Network play not yet implemented.");
  }
}



void I_NetCmd (void)
{
  ;
}



void I_ShutdownNetwork(void)
{
  ;
}
