/*
 *  p_saven.c
 *
 *  Purpose:	portable savegames;
 *
 *  Author:	Andreas Dehmel (dehmel@forwiss.tu-muenchen.de)
 *
 *  New format savegames, portable and less likely to crash when the levels
 *  don't match. Also usable for all variations of vanilla Doom, Boom, full
 *  new features or not, bells & whistles.
 */

#include "i_system.h"
#include "z_zone.h"
#include "p_local.h"
#include "p_saven.h"
#include "am_map.h"

#include "doomstat.h"
#include "r_state.h"


/* in p_enemy.c */
extern int P_BrainInit(void);


/*#define DEBUG_SAVEGAME*/


#define SAV_WRITE8(v) \
  if (save_p >= save_high_p) P_FlushSavebufferNew(); \
  *save_p++ = (v);

#define SAV_WRITE16(v) \
  if (save_p + 2 > save_high_p) P_FlushSavebufferNew(); \
  *save_p++ = ((v) & 0xff); *save_p++ = ((v) >> 8);

#define SAV_WRITE32(v) \
  if (save_p + 4 > save_high_p) P_FlushSavebufferNew(); \
  *save_p++ = ((v) & 0xff); *save_p++ = (((v) >> 8) & 0xff); \
  *save_p++ = (((v) >> 16) & 0xff); *save_p++ = (((v) >> 24) & 0xff);

#define SAV_READ8(v) \
  (v) = *save_p++;

#define SAV_READ16(v) \
  (v) = (save_p[0] | (save_p[1] << 8)); save_p += 2;

#define SAV_READ32(v) \
  (v) = (save_p[0] | (save_p[1] << 8) | (save_p[2] << 16) | (save_p[3] << 24)); save_p += 4;



int P_FlushSavebufferNew(void)
{
  unsigned long bytes;
  unsigned long written;

  bytes = (save_p - savebuffer);
  if (bytes != 0)
  {
    written = fwrite(savebuffer, 1, bytes, save_file);
    save_p = savebuffer;
    return (written == bytes);
  }
  return 1;
}


static const byte size_ticcmd_norm = 4 + 2*2;
static const byte size_ticcmd_full = 4 + 2*2 + 4;
static const byte size_mobj_norm = (2 * (4+5) + 4 * 23);
static const byte size_mobj_boom = (2 * (4+5) + 4 * 28);
static const byte size_ceiling_norm = (1 * 3 + 4 * 7);
static const byte size_ceiling_boom = (1 * 3 + 2 + 4 * 11);
static const byte size_vldoor_norm = (1 * 2 + 4 * 6);
static const byte size_floor_norm = (1 * 3 + 2 + 4 * 5);
static const byte size_floor_boom = (1 * 3 + 2 + 4 * 6);
static const byte size_plat_norm = (1 * 5 + 4 * 7);
static const byte size_plat_boom = (1 * 5 + 4 * 8);
static const byte size_flash_norm = (1 + 4 * 6);
static const byte size_strobe_norm = (1 + 4 * 6);
static const byte size_glow_norm = (1 + 4 * 4);
static const byte size_flicker_boom = (1 + 4 * 4);
static const byte size_elevator_boom = (1 * 2 + 4 * 5);
static const byte size_scroll_boom = (1 * 2 + 4 * 8);
static const byte size_friction_boom = (1 + 4 * 3);
static const byte size_pusher_boom = (1 * 2 + 4 * 7);




static void P_ArchiveTiccmdNew(const ticcmd_t *t)
{
  byte size;

#ifdef FULL_NEW_FEATURES
  size = size_ticcmd_full;
#else
  size = size_ticcmd_norm;
#endif

  SAV_WRITE8(size);
  SAV_WRITE8(t->forwardmove);
  SAV_WRITE8(t->sidemove);
  SAV_WRITE16(t->angleturn);
  SAV_WRITE16(t->consistancy);
  SAV_WRITE8(t->chatchar);
  SAV_WRITE8(t->buttons);
#ifdef FULL_NEW_FEATURES
  SAV_WRITE8(t->newflags);
  SAV_WRITE8(t->vaccel);
  SAV_WRITE8(t->misc1);
  SAV_WRITE8(t->misc2);
#endif
}

static void P_UnArchiveTiccmdNew(ticcmd_t *t)
{
  byte *sizep = save_p;
  byte size;

  SAV_READ8(size);
  SAV_READ8(t->forwardmove);
  SAV_READ8(t->sidemove);
  SAV_READ16(t->angleturn);
  SAV_READ16(t->consistancy);
  SAV_READ8(t->chatchar);
  SAV_READ8(t->buttons);
#ifdef FULL_NEW_FEATURES
  if (size >= size_ticcmd_full)
  {
    SAV_READ8(t->newflags);
    SAV_READ8(t->vaccel);
    SAV_READ8(t->misc1);
    SAV_READ8(t->misc2);
  }
  else
  {
    t->newflags = 0; t->vaccel = 0; t->misc1 = 0; t->misc2 = 0;
  }
#endif
  save_p = sizep + size + 1;
}

static void P_ArchivePsprNew(const pspdef_t *p)
{
  unsigned long state;

  if (p->state == NULL)
    state = 0;
  else
    state = p->state - states;

  SAV_WRITE16(state);
  SAV_WRITE32(p->tics);
  SAV_WRITE32(p->sx);
  SAV_WRITE32(p->sy);
}

static void P_UnArchivePsprNew(pspdef_t *p)
{
  unsigned long state;

  SAV_READ16(state);
  if (state == 0)
    p->state = NULL;
  else
    p->state = states + state;

  SAV_READ32(p->tics);
  SAV_READ32(p->sx);
  SAV_READ32(p->sy);
}

void P_ArchivePlayersNew(void)
{
  int i;

  for (i=0; i<MAXPLAYERS; i++)
  {
    if (playeringame[i])
    {
      const player_t *p = players + i;
      int j;

      SAV_WRITE8(p->playerstate);	/* enum */
      P_ArchiveTiccmdNew(&(p->cmd));
      SAV_WRITE32(p->viewz);
      SAV_WRITE32(p->viewheight);
      SAV_WRITE32(p->deltaviewheight);
      SAV_WRITE32(p->bob);
      SAV_WRITE32(p->health);
      SAV_WRITE32(p->armorpoints);
      SAV_WRITE32(p->armortype);

      SAV_WRITE16(NUMPOWERS);
      for (j=0; j<NUMPOWERS; j++)
      {
	SAV_WRITE32(p->powers[j]);
      }
      SAV_WRITE16(NUMCARDS);
      for (j=0; j<NUMCARDS; j++)
      {
	SAV_WRITE8(p->cards[j]);
      }
      SAV_WRITE8(p->backpack);
      for (j=0; j<MAXPLAYERS; j++)
      {
	SAV_WRITE32(p->frags[j]);
      }
      SAV_WRITE8(p->readyweapon);	/* enum */
      SAV_WRITE8(p->pendingweapon);	/* enum */
      for (j=0; j<NUMWEAPONS; j++)
      {
	SAV_WRITE8(p->weaponowned[j]);
      }
      for (j=0; j<NUMAMMO; j++)
      {
	SAV_WRITE32(p->ammo[j]);
	SAV_WRITE32(p->maxammo[j]);
      }
      SAV_WRITE32(p->attackdown);
      SAV_WRITE32(p->usedown);
      SAV_WRITE32(p->cheats);
      SAV_WRITE32(p->refire);
      SAV_WRITE32(p->killcount);
      SAV_WRITE32(p->itemcount);
      SAV_WRITE32(p->secretcount);
      /* message ignored */
      SAV_WRITE32(p->damagecount);
      SAV_WRITE32(p->bonuscount);
      /* attacker ignored */
      SAV_WRITE32(p->extralight);
      SAV_WRITE32(p->fixedcolormap);
      SAV_WRITE32(p->colormap);
      for (j=0; j<NUMPSPRITES; j++)
      {
	P_ArchivePsprNew(p->psprites + j);
      }
      SAV_WRITE8(p->didsecret);
    }
  }
#ifdef DEBUG_SAVEGAME
  fprintf(logfile, "player offset %d\n", save_p - savebuffer); fflush(logfile);
#endif
}


void P_UnArchivePlayersNew(void)
{
  int i;

  for (i=0; i<MAXPLAYERS; i++)
  {
    if (playeringame[i])
    {
      player_t *p = players + i;
      short number;
      byte *ptr;
      int j, high;

      /* just to make sure */
      memset(p, 0, sizeof(player_t));
      SAV_READ8(p->playerstate);
      P_UnArchiveTiccmdNew(&(p->cmd));
      SAV_READ32(p->viewz);
      SAV_READ32(p->viewheight);
      SAV_READ32(p->deltaviewheight);
      SAV_READ32(p->bob);
      SAV_READ32(p->health);
      SAV_READ32(p->armorpoints);
      SAV_READ32(p->armortype);

      SAV_READ16(number);
      ptr = save_p;
      high = (number > NUMPOWERS) ? NUMPOWERS : number;
      for (j=0; j<high; j++)
      {
	SAV_READ32(p->powers[j]);
      }
      for (; j<NUMPOWERS; j++)
      {
	p->powers[j] = 0;
      }
      save_p = ptr + number * 4;
      SAV_READ16(number);
      ptr = save_p;
      high = (number > NUMCARDS) ? NUMCARDS : number;
      for (j=0; j<high; j++)
      {
	SAV_READ8(p->cards[j]);
      }
      for (; j<NUMCARDS; j++)
      {
        p->cards[j] = 0;
      }
      save_p = ptr + number;
      SAV_READ8(p->backpack);
      for (j=0; j<MAXPLAYERS; j++)
      {
	SAV_READ32(p->frags[j]);
      }
      SAV_READ8(p->readyweapon);
      SAV_READ8(p->pendingweapon);
      for (j=0; j<NUMWEAPONS; j++)
      {
	SAV_READ8(p->weaponowned[j]);
      }
      for (j=0; j<NUMAMMO; j++)
      {
	SAV_READ32(p->ammo[j]);
	SAV_READ32(p->maxammo[j]);
      }
      SAV_READ32(p->attackdown);
      SAV_READ32(p->usedown);
      SAV_READ32(p->cheats);
      SAV_READ32(p->refire);
      SAV_READ32(p->killcount);
      SAV_READ32(p->itemcount);
      SAV_READ32(p->secretcount);
      SAV_READ32(p->damagecount);
      SAV_READ32(p->bonuscount);
      SAV_READ32(p->extralight);
      SAV_READ32(p->fixedcolormap);
      SAV_READ32(p->colormap);
      for (j=0; j<NUMPSPRITES; j++)
      {
	P_UnArchivePsprNew(p->psprites + j);
      }
      SAV_READ8(p->didsecret);
      p->mo = NULL;
      p->message = NULL;
      p->attacker = NULL;
    }
  }
#ifdef DEBUG_SAVEGAME
  fprintf(logfile, "player offset %d\n", save_p - savebuffer); fflush(logfile);
#endif
}


void P_ArchiveWorldNew(void)
{
  const sector_t *s;
  const line_t *l;
  unsigned long off, savesides;
  int i;

  SAV_WRITE32(numsectors);
  for (i=0, s=sectors; i<numsectors; i++, s++)
  {
    SAV_WRITE16(s->floorheight >> FRACBITS);
    SAV_WRITE16(s->ceilingheight >> FRACBITS);
    SAV_WRITE16(s->floorpic);
    SAV_WRITE16(s->ceilingpic);
    SAV_WRITE16(s->lightlevel);
    SAV_WRITE16(s->special);
    SAV_WRITE16(s->tag);
    off = (s->soundtarget == NULL) ? 0 : (long)(s->soundtarget->thinker.prev);
    SAV_WRITE16(off);
  }

  savesides = 0;
  for (i=0, l=lines; i<numlines; i++, l++)
  {
    int j;

    for (j=0; j<2; j++)
    {
      if (l->sidenum[j] != -1)
	savesides++;
    }
  }
  SAV_WRITE32(numlines);
  SAV_WRITE32(savesides);
  for (i=0, l=lines; i<numlines; i++, l++)
  {
    int j;

    SAV_WRITE16(l->flags);
    SAV_WRITE16(l->special);
    SAV_WRITE16(l->tag);
    for (j=0; j<2; j++)
    {
      if (l->sidenum[j] != -1)
      {
	const side_t *d = sides + l->sidenum[j];

	SAV_WRITE16(d->textureoffset >> FRACBITS);
	SAV_WRITE16(d->rowoffset >> FRACBITS);
	SAV_WRITE16(d->toptexture);
	SAV_WRITE16(d->bottomtexture);
	SAV_WRITE16(d->midtexture);
      }
    }
  }
#ifdef DEBUG_SAVEGAME
  fprintf(logfile, "world offset %d\n", save_p - savebuffer); fflush(logfile);
#endif
}


void P_UnArchiveWorldNew(void)
{
  unsigned char *ptr;
  sector_t *s;
  line_t *l;
  unsigned long off, savesides;
  int i, number, high;

  SAV_READ32(number);
  ptr = save_p;
  high = (number > numsectors) ? numsectors : number;
  for (i=0, s=sectors; i<high; i++, s++)
  {
    SAV_READ16(off);
    s->floorheight = off << FRACBITS;
    SAV_READ16(off);
    s->ceilingheight = off << FRACBITS;
    SAV_READ16(s->floorpic);
    SAV_READ16(s->ceilingpic);
    SAV_READ16(s->lightlevel);
    SAV_READ16(s->special);
    SAV_READ16(s->tag);
    s->BOOMCEILGDATA = 0;
    SAV_READ16(off);
    s->soundtarget = (mobj_t*)(off);
#ifdef DIYBOOM
    s->floordata = 0;
    s->lightingdata = 0;
#endif
  }
  for (; i<numsectors; i++, s++)
  {
    memset(s, 0, sizeof(sector_t));
  }
  save_p = ptr + (number * 2 * 8);
  SAV_READ32(number);
  SAV_READ32(savesides);
  ptr = save_p;
  high = (number > numlines) ? numlines : number;
  for (i=0, l=lines; i<high; i++, l++)
  {
    int j;

    SAV_READ16(l->flags);
    SAV_READ16(l->special);
    SAV_READ16(l->tag);
    for (j=0; j<2; j++)
    {
      if (l->sidenum[j] != -1)
      {
	side_t *d = sides + l->sidenum[j];

	SAV_READ16(off);
	d->textureoffset = off << FRACBITS;
	SAV_READ16(off);
	d->rowoffset = off << FRACBITS;
	SAV_READ16(d->toptexture);
	SAV_READ16(d->bottomtexture);
	SAV_READ16(d->midtexture);
      }
    }
  }
  for (; i<numlines; i++, l++)
  {
    memset(l, 0, sizeof(line_t));
    l->sidenum[0] = -1; l->sidenum[1] = -1;
  }
  save_p = ptr + (number * 2 * 3 + savesides * 2 * 5);
#ifdef DEBUG_SAVEGAME
  fprintf(logfile, "world offset %d\n", save_p - savebuffer); fflush(logfile);
#endif
}


static void P_ArchiveMapThingNew(const mapthing_t *m)
{
  SAV_WRITE16(m->x);
  SAV_WRITE16(m->y);
  SAV_WRITE16(m->angle);
  SAV_WRITE16(m->type);
  SAV_WRITE16(m->options);
}

static void P_UnArchiveMapThingNew(mapthing_t *m)
{
  SAV_READ16(m->x);
  SAV_READ16(m->y);
  SAV_READ16(m->angle);
  SAV_READ16(m->type);
  SAV_READ16(m->options);
}

void P_ArchiveThinkersNew(void)
{
  thinker_t *t;
  byte size;

#ifdef DIYBOOM
  size = size_mobj_boom;
#else
  size = size_mobj_norm;
#endif

  SAV_WRITE8(size);
  for (t=thinkercap.next; t != &thinkercap; t = t->next)
  {
    if (t->function.acp1 == (actionf_p1)P_MobjThinker)
    {
      const mobj_t *o = (const mobj_t*)t;
      long off;

      SAV_WRITE8(tc_mobj);
      SAV_WRITE32(o->x);
      SAV_WRITE32(o->y);
      SAV_WRITE32(o->z);
      SAV_WRITE32(o->angle);
      SAV_WRITE16(o->sprite);
      SAV_WRITE16(o->frame);
      SAV_WRITE32(o->floorz);
      SAV_WRITE32(o->ceilingz);
      SAV_WRITE32(o->radius);
      SAV_WRITE32(o->height);
      SAV_WRITE32(o->momx);
      SAV_WRITE32(o->momy);
      SAV_WRITE32(o->momz);
      SAV_WRITE32(o->validcount);
      SAV_WRITE16(o->type);
      SAV_WRITE32(o->tics);
      off = (o->state == NULL) ? 0 : o->state - states;
      SAV_WRITE16(off);
      SAV_WRITE32(o->flags);
      SAV_WRITE32(o->health);
      SAV_WRITE32(o->movedir);
      SAV_WRITE32(o->movecount);
      off = 0;
      if (o->target != NULL)
      {
	if (o->target->thinker.function.acp1 == (actionf_p1)P_MobjThinker)
	  off = (long)(o->target->thinker.prev);
      }
      SAV_WRITE32(off);
      off = 0;
      if (o->tracer != NULL)
      {
        if (o->tracer->thinker.function.acp1 == (actionf_p1)P_MobjThinker)
	  off = (long)(o->tracer->thinker.prev);
      }
      SAV_WRITE32(off);
      SAV_WRITE32(o->reactiontime);
      SAV_WRITE32(o->threshold);
      off = (o->player == NULL) ? 0 : (o->player - players) + 1;
      SAV_WRITE32(off);
      SAV_WRITE32(o->lastlook);
      P_ArchiveMapThingNew(&(o->spawnpoint));
#ifdef DIYBOOM
      off = 0;
      if (o->lastenemy != NULL)
      {
        if (o->lastenemy->thinker.function.acp1 == (actionf_p1)P_MobjThinker)
	  off = (long)(o->lastenemy->thinker.prev);
      }
      SAV_WRITE32(off);
      off = 0;
      if (o->above_thing != NULL)
      {
        if (o->above_thing->thinker.function.acp1 == (actionf_p1)P_MobjThinker)
	  off = (long)(o->above_thing->thinker.prev);
      }
      SAV_WRITE32(off);
      off = 0;
      if (o->below_thing != NULL)
      {
        if (o->below_thing->thinker.function.acp1 == (actionf_p1)P_MobjThinker)
	  off = (long)(o->below_thing->thinker.prev);
      }
      SAV_WRITE32(off);
      SAV_WRITE32(o->friction);
      SAV_WRITE32(o->movefactor);
#endif
    }
  }
  SAV_WRITE8(tc_end);
#ifdef DEBUG_SAVEGAME
  fprintf(logfile, "thinker offset %d\n", save_p - savebuffer); fflush(logfile);
#endif
}



typedef struct mobj_table_s {
  long number;
  mobj_t **mobjs;
} mobj_table_t;


void P_UnArchiveThinkerRef(mobj_table_t *table, int type, mobj_t **ref, const char *name)
{
  if (*ref != NULL)
  {
    long off;

    off = (long)(*ref);
    if ((off >= 0) && (off <= table->number))
    {
      (*ref) = (table->mobjs)[off];
    }
    else
    {
      (*ref) = NULL;
      fprintf(logfile, "P_UnArchiveThinkerRef: bad %s %ld (otype %d)\n", name, off, type);
      fflush(logfile);
    }
  }
}


void P_UnArchiveThinkersNew(void)
{
  long numThinkers, readThinker;
  thinker_t *t;
  byte tclass;
  byte *sp;
  byte size;
  mobj_t **mobj_p;

  t = thinkercap.next;
  while (t != &thinkercap)
  {
    thinker_t *next = t->next;

    if (t->function.acp1 == (actionf_p1)P_MobjThinker)
      P_RemoveMobj((mobj_t*)t);
    else
      Z_Free(t);

    t = next;
  }

  P_InitThinkers();

  SAV_READ8(size);
  numThinkers = 1; sp = save_p;
  while (1)
  {
    SAV_READ8(tclass);
    if (tclass != tc_mobj)
      break;
    save_p += size;
    numThinkers++;
  }
  if (tclass != tc_end)
  {
    I_Error("P_UnArchiveThinkersNew: unknown tclass %d in savegame\n", tclass);
  }

  save_p = sp;

  mobj_p = (mobj_t**)Z_MallocNoAbort((numThinkers+1) * sizeof(mobj_t*), PU_STATIC, NULL);
  if (mobj_p != NULL)
    mobj_p[0] = NULL;

  readThinker = 1;
  while (1)
  {
    mobj_t *o;
    long off;

    SAV_READ8(tclass);
    if (tclass != tc_mobj)
      break;
    sp = save_p;

    o = (mobj_t*)Z_Malloc(sizeof(mobj_t), PU_LEVEL, NULL);
    memset(o, 0, sizeof(mobj_t));
    if ((mobj_p != NULL) && (readThinker <= numThinkers))
      mobj_p[readThinker++] = o;

    memset(o, 0, sizeof(mobj_t));

    SAV_READ32(o->x);
    SAV_READ32(o->y);
    SAV_READ32(o->z);
    SAV_READ32(o->angle);
    SAV_READ16(o->sprite);
    SAV_READ16(o->frame);
    SAV_READ32(o->floorz);
    SAV_READ32(o->ceilingz);
    SAV_READ32(o->radius);
    SAV_READ32(o->height);
    SAV_READ32(o->momx);
    SAV_READ32(o->momy);
    SAV_READ32(o->momz);
    SAV_READ32(o->validcount);
    SAV_READ16(o->type);
    SAV_READ32(o->tics);
    SAV_READ16(off);
    o->state = (off == 0) ? NULL : states + off;
    SAV_READ32(o->flags);
    SAV_READ32(o->health);
    SAV_READ32(o->movedir);
    SAV_READ32(o->movecount);
    SAV_READ32(off);
    if (mobj_p != NULL)
      o->target = (mobj_t*)off;
    SAV_READ32(off);
    if (mobj_p != NULL)
      o->tracer = (mobj_t*)off;
    SAV_READ32(o->reactiontime);
    SAV_READ32(o->threshold);
    SAV_READ32(off);
    o->player = (off == 0) ? NULL : players + (off-1);
    SAV_READ32(o->lastlook);
    P_UnArchiveMapThingNew(&(o->spawnpoint));
#ifdef DIYBOOM
    if (size >= size_mobj_boom)
    {
      SAV_READ32(off);
      if (mobj_p != NULL)
	o->lastenemy = (mobj_t*)off;
      SAV_READ32(off);
      if (mobj_p != NULL)
	o->above_thing = (mobj_t*)off;
      SAV_READ32(off);
      if (mobj_p != NULL)
	o->below_thing = (mobj_t*)off;
      SAV_READ32(o->friction);
      SAV_READ32(o->movefactor);
    }
#endif
    if (o->player != NULL)
    {
      o->player->mo = o;
    }
    P_SetThingPosition(o);
    if ((o->type >= 0) && (o->type < NUMMOBJTYPES))
    {
      o->info = mobjinfo + o->type;
    }
    else
    {
      fprintf(logfile, "P_UnArchiveThinkersNew: bad otype %d\n", o->type);
      fflush(logfile);
      o->info = NULL;
    }
    o->thinker.function.acp1 = (actionf_p1)P_MobjThinker;
    P_AddThinker(&(o->thinker));

    save_p = sp + size;
  }

  if (mobj_p != NULL)
  {
    mobj_table_t table;
    sector_t *s;
    int i;

    table.number = numThinkers;
    table.mobjs = mobj_p;

    for (t=thinkercap.next; t != &thinkercap; t = t->next)
    {
      mobj_t *o = (mobj_t*)t;

      P_UnArchiveThinkerRef(&table, o->type, &(o->target), "target");
      P_UnArchiveThinkerRef(&table, o->type, &(o->tracer), "tracer");
#ifdef DIYBOOM
      P_UnArchiveThinkerRef(&table, o->type, &(o->lastenemy), "lastenemy");
      P_UnArchiveThinkerRef(&table, o->type, &(o->above_thing), "above_thing");
      P_UnArchiveThinkerRef(&table, o->type, &(o->below_thing), "below_thing");
#endif
    }
    for (i=0, s=sectors; i<numsectors; i++, s++)
    {
      P_UnArchiveThinkerRef(&table, i, &(s->soundtarget), "soundtarget");
    }

    Z_Free(mobj_p);
    mobj_p = NULL;
  }
  else
  {
    sector_t *s;
    int i;

    fprintf(logfile, "P_UnArchiveThinkersNew: unable to claim thing table.\n");
    for (i=0, s=sectors; i<numsectors; i++, s++)
    {
      s->soundtarget = NULL;
    }
  }

  if (P_BrainInit() != 0)
  {
    fprintf(logfile, "P_UnArchiveThinkersNew: woke up brain.\n");
  }

#ifdef DEBUG_SAVEGAME
  fprintf(logfile, "thinker offset %d\n", save_p - savebuffer); fflush(logfile);
#endif
}


static void P_ArchiveCeilingNew(const ceiling_t *c)
{
  boolean hasfunc;
  long off;

  SAV_WRITE8(tc_ceiling);
  hasfunc = (c->thinker.function.acp1 != NULL);
  SAV_WRITE8(hasfunc);
  SAV_WRITE8(c->type);
  off = c->sector - sectors;
  SAV_WRITE32(off);
  SAV_WRITE32(c->bottomheight);
  SAV_WRITE32(c->topheight);
  SAV_WRITE32(c->speed);
  SAV_WRITE8(c->crush);
  SAV_WRITE32(c->direction);
  SAV_WRITE32(c->tag);
  SAV_WRITE32(c->olddirection);
#ifdef DIYBOOM
  SAV_WRITE32(c->oldspeed);
  SAV_WRITE32(c->newspecial);
  SAV_WRITE32(c->oldspecial);
  SAV_WRITE16(c->texture);
  SAV_WRITE32(c->activeidx);
#endif
}

static thinker_t *P_UnArchiveCeilingNew(byte size)
{
  ceiling_t *c;
  byte *ptr = save_p;
  boolean hasfunc;
  long off;

  c = (ceiling_t*)Z_Malloc(sizeof(ceiling_t), PU_LEVEL, NULL);
  memset(c, 0, sizeof(ceiling_t));
  SAV_READ8(hasfunc);
  if (hasfunc)
    c->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;
  SAV_READ8(c->type);
  SAV_READ32(off);
  c->sector = sectors + off;
  SAV_READ32(c->bottomheight);
  SAV_READ32(c->topheight);
  SAV_READ32(c->speed);
  SAV_READ8(c->crush);
  SAV_READ32(c->direction);
  SAV_READ32(c->tag);
  SAV_READ32(c->olddirection);
#ifdef DIYBOOM
  if (size == size_ceiling_boom)
  {
    SAV_READ32(c->oldspeed);
    SAV_READ32(c->newspecial);
    SAV_READ32(c->oldspecial);
    SAV_READ16(c->texture);
    SAV_READ32(c->activeidx);
  }
#endif

  c->sector->BOOMCEILGDATA = c;

  P_AddThinker(&(c->thinker));
  P_AddActiveCeiling(c);

  save_p = ptr + size;

  return &(c->thinker);
}

static void P_ArchiveDoorNew(const vldoor_t *d)
{
  boolean hasfunc;
  long off;

  SAV_WRITE8(tc_door);
  hasfunc = (d->thinker.function.acp1 != NULL);
  SAV_WRITE8(hasfunc);
  SAV_WRITE8(d->type);
  off = d->sector - sectors;
  SAV_WRITE32(off);
  SAV_WRITE32(d->topheight);
  SAV_WRITE32(d->speed);
  SAV_WRITE32(d->direction);
  SAV_WRITE32(d->topwait);
  SAV_WRITE32(d->topcountdown);
}

static thinker_t *P_UnArchiveDoorNew(byte size)
{
  vldoor_t *d;
  byte *ptr = save_p;
  boolean hasfunc;
  long off;

  d = (vldoor_t*)Z_Malloc(sizeof(vldoor_t), PU_LEVEL, NULL);
  memset(d, 0, sizeof(vldoor_t));
  SAV_READ8(hasfunc);
  if (hasfunc)
    d->thinker.function.acp1 = (actionf_p1)T_VerticalDoor;
  SAV_READ8(d->type);
  SAV_READ32(off);
  d->sector = sectors + off;
  SAV_READ32(d->topheight);
  SAV_READ32(d->speed);
  SAV_READ32(d->direction);
  SAV_READ32(d->topwait);
  SAV_READ32(d->topcountdown);

  d->sector->BOOMCEILGDATA = d;
  P_AddThinker(&(d->thinker));

  save_p = ptr + size;

  return &(d->thinker);
}

static void P_ArchiveFloorNew(const floormove_t *f)
{
  boolean hasfunc;
  long off;

  SAV_WRITE8(tc_floor);
  hasfunc = (f->thinker.function.acp1 != NULL);
  SAV_WRITE8(hasfunc);
  SAV_WRITE8(f->type);
  SAV_WRITE8(f->crush);
  off = f->sector - sectors;
  SAV_WRITE32(off);
  SAV_WRITE32(f->direction);
  SAV_WRITE32(f->newspecial);
  SAV_WRITE16(f->texture);
  SAV_WRITE32(f->floordestheight);
  SAV_WRITE32(f->speed);
#ifdef DIYBOOM
  SAV_WRITE32(f->oldspecial);
#endif
}

static thinker_t *P_UnArchiveFloorNew(byte size)
{
  floormove_t *f;
  byte *ptr = save_p;
  boolean hasfunc;
  long off;

  f = (floormove_t*)Z_Malloc(sizeof(floormove_t), PU_LEVEL, NULL);
  memset(f, 0, sizeof(floormove_t));
  SAV_READ8(hasfunc);
  if (hasfunc)
    f->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
  SAV_READ8(f->type);
  SAV_READ8(f->crush);
  SAV_READ32(off);
  f->sector = sectors + off;
  SAV_READ32(f->direction);
  SAV_READ32(f->newspecial);
  SAV_READ16(f->texture);
  SAV_READ32(f->floordestheight);
  SAV_READ32(f->speed);
#ifdef DIYBOOM
  if (size >= size_floor_boom)
  {
    SAV_READ32(f->oldspecial);
  }
#endif

  f->sector->BOOMFLOORDATA = f;
  P_AddThinker(&(f->thinker));

  save_p = ptr + size;

  return &(f->thinker);
}

static void P_ArchivePlatformNew(const plat_t *p)
{
  boolean hasfunc;
  long off;

  SAV_WRITE8(tc_plat);
  hasfunc = (p->thinker.function.acp1 != NULL);
  SAV_WRITE8(hasfunc);
  off = p->sector - sectors;
  SAV_WRITE32(off);
  SAV_WRITE32(p->speed);
  SAV_WRITE32(p->low);
  SAV_WRITE32(p->high);
  SAV_WRITE32(p->wait);
  SAV_WRITE32(p->count);
  SAV_WRITE32(p->tag);
  SAV_WRITE8(p->status);
  SAV_WRITE8(p->oldstatus);
  SAV_WRITE8(p->crush);
  SAV_WRITE8(p->type);
#ifdef DIYBOOM
  SAV_WRITE32(p->activeidx);
#endif
}

static thinker_t *P_UnArchivePlatformNew(byte size)
{
  plat_t *p;
  byte *ptr = save_p;
  boolean hasfunc;
  long off;

  p = (plat_t*)Z_Malloc(sizeof(plat_t), PU_LEVEL, NULL);
  memset(p, 0, sizeof(plat_t));
  SAV_READ8(hasfunc);
  if (hasfunc)
    p->thinker.function.acp1 = (actionf_p1)T_PlatRaise;
  SAV_READ32(off);
  p->sector = sectors + off;
  SAV_READ32(p->speed);
  SAV_READ32(p->low);
  SAV_READ32(p->high);
  SAV_READ32(p->wait);
  SAV_READ32(p->count);
  SAV_READ32(p->tag);
  SAV_READ8(p->status);
  SAV_READ8(p->oldstatus);
  SAV_READ8(p->crush);
  SAV_READ8(p->type);
#ifdef DIYBOOM
  if (size >= size_plat_boom)
  {
    SAV_READ32(p->activeidx);
  }
#endif

  p->sector->BOOMFLOORDATA = p;
  P_AddThinker(&(p->thinker));
  P_AddActivePlat(p);

  save_p = ptr + size;

  return &(p->thinker);
}

static void P_ArchiveFlashNew(const lightflash_t *f)
{
  boolean hasfunc;
  long off;

  SAV_WRITE8(tc_flash);
  hasfunc = (f->thinker.function.acp1 != NULL);
  SAV_WRITE8(hasfunc);
  off = f->sector - sectors;
  SAV_WRITE32(off);
  SAV_WRITE32(f->count);
  SAV_WRITE32(f->maxlight);
  SAV_WRITE32(f->minlight);
  SAV_WRITE32(f->maxtime);
  SAV_WRITE32(f->mintime);
}

static thinker_t *P_UnArchiveFlashNew(byte size)
{
  lightflash_t *f;
  byte *ptr = save_p;
  boolean hasfunc;
  long off;

  f = (lightflash_t*)Z_Malloc(sizeof(lightflash_t), PU_LEVEL, NULL);
  memset(f, 0, sizeof(lightflash_t));
  SAV_READ8(hasfunc);
  if (hasfunc)
    f->thinker.function.acp1 = (actionf_p1)T_LightFlash;
  SAV_READ32(off);
  f->sector = sectors + off;
  SAV_READ32(f->count);
  SAV_READ32(f->maxlight);
  SAV_READ32(f->minlight);
  SAV_READ32(f->maxtime);
  SAV_READ32(f->mintime);

  P_AddThinker(&(f->thinker));

  save_p = ptr + size;

  return &(f->thinker);
}

static void P_ArchiveStrobeNew(const strobe_t *s)
{
  boolean hasfunc;
  long off;

  SAV_WRITE8(tc_strobe);
  hasfunc = (s->thinker.function.acp1 != NULL);
  SAV_WRITE8(hasfunc);
  off = s->sector - sectors;
  SAV_WRITE32(off);
  SAV_WRITE32(s->count);
  SAV_WRITE32(s->minlight);
  SAV_WRITE32(s->maxlight);
  SAV_WRITE32(s->darktime);
  SAV_WRITE32(s->brighttime);
}

static thinker_t *P_UnArchiveStrobeNew(byte size)
{
  strobe_t *s;
  byte *ptr = save_p;
  boolean hasfunc;
  long off;

  s = (strobe_t*)Z_Malloc(sizeof(strobe_t), PU_LEVEL, NULL);
  memset(s, 0, sizeof(strobe_t));
  SAV_READ8(hasfunc);
  if (hasfunc)
    s->thinker.function.acp1 = (actionf_p1)T_StrobeFlash;
  SAV_READ32(off);
  s->sector = sectors + off;
  SAV_READ32(s->count);
  SAV_READ32(s->minlight);
  SAV_READ32(s->maxlight);
  SAV_READ32(s->darktime);
  SAV_READ32(s->brighttime);

  P_AddThinker(&(s->thinker));

  save_p = ptr + size;

  return &(s->thinker);
}

static void P_ArchiveGlowNew(const glow_t *g)
{
  boolean hasfunc;
  long off;

  SAV_WRITE8(tc_glow);
  hasfunc = (g->thinker.function.acp1 != NULL);
  SAV_WRITE8(hasfunc);
  off = g->sector - sectors;
  SAV_WRITE32(off);
  SAV_WRITE32(g->minlight);
  SAV_WRITE32(g->maxlight);
  SAV_WRITE32(g->direction);
}

static thinker_t *P_UnArchiveGlowNew(byte size)
{
  glow_t *g;
  byte *ptr = save_p;
  boolean hasfunc;
  long off;

  g = (glow_t*)Z_Malloc(sizeof(glow_t), PU_LEVEL, NULL);
  memset(g, 0, sizeof(glow_t));
  SAV_READ8(hasfunc);
  if (hasfunc)
    g->thinker.function.acp1 = (actionf_p1)T_Glow;
  SAV_READ32(off);
  g->sector = sectors + off;
  SAV_READ32(g->minlight);
  SAV_READ32(g->maxlight);
  SAV_READ32(g->direction);

  P_AddThinker(&(g->thinker));

  save_p = ptr + size;

  return &(g->thinker);
}

#ifdef DIYBOOM
static void P_ArchiveFlickerNew(const fireflicker_t *f)
{
  boolean hasfunc;
  long off;

  SAV_WRITE8(tc_flicker);
  hasfunc = (f->thinker.function.acp1 != NULL);
  SAV_WRITE8(hasfunc);
  off = f->sector - sectors;
  SAV_WRITE32(off);
  SAV_WRITE32(f->count);
  SAV_WRITE32(f->maxlight);
  SAV_WRITE32(f->minlight);
}

static void P_ArchiveElevatorNew(const elevator_t *e)
{
  boolean hasfunc;
  long off;

  SAV_WRITE8(tc_elevator);
  hasfunc = (e->thinker.function.acp1 != NULL);
  SAV_WRITE8(hasfunc);
  SAV_WRITE8(e->type);
  off = e->sector - sectors;
  SAV_WRITE32(off);
  SAV_WRITE32(e->direction);
  SAV_WRITE32(e->floordestheight);
  SAV_WRITE32(e->ceilingdestheight);
  SAV_WRITE32(e->speed);
}

static void P_ArchiveScrollNew(const scroll_t *s)
{
  boolean hasfunc;

  SAV_WRITE8(tc_scroll);
  hasfunc = (s->thinker.function.acp1 != NULL);
  SAV_WRITE8(hasfunc);
  SAV_WRITE32(s->dx);
  SAV_WRITE32(s->dy);
  SAV_WRITE32(s->affectee);
  SAV_WRITE32(s->control);
  SAV_WRITE32(s->last_height);
  SAV_WRITE32(s->vdx);
  SAV_WRITE32(s->vdy);
  SAV_WRITE32(s->accel);
  SAV_WRITE8(s->type);
}

static void P_ArchiveFrictionNew(const friction_t *f)
{
  boolean hasfunc;

  SAV_WRITE8(tc_friction);
  hasfunc = (f->thinker.function.acp1 != NULL);
  SAV_WRITE8(hasfunc);
  SAV_WRITE32(f->friction);
  SAV_WRITE32(f->movefactor);
  SAV_WRITE32(f->affectee);
}

static void P_ArchivePusherNew(const pusher_t *p)
{
  boolean hasfunc;

  SAV_WRITE8(tc_pusher);
  hasfunc = (p->thinker.function.acp1 != NULL);
  SAV_WRITE8(hasfunc);
  SAV_WRITE8(p->type);
  SAV_WRITE32(p->x_mag);
  SAV_WRITE32(p->y_mag);
  SAV_WRITE32(p->magnitude);
  SAV_WRITE32(p->radius);
  SAV_WRITE32(p->x);
  SAV_WRITE32(p->y);
  SAV_WRITE32(p->affectee);
}
#endif

static thinker_t *P_UnArchiveFlickerNew(byte size)
{
#ifdef DIYBOOM
  fireflicker_t *f;
  byte *ptr = save_p;
  boolean hasfunc;
  long off;

  f = (fireflicker_t*)Z_Malloc(sizeof(fireflicker_t), PU_LEVEL, NULL);
  memset(f, 0, sizeof(fireflicker_t));
  SAV_READ8(hasfunc);
  if (hasfunc)
    f->thinker.function.acp1 = (actionf_p1)T_FireFlicker;
  SAV_READ32(off);
  f->sector = sectors + off;
  SAV_READ32(f->count);
  SAV_READ32(f->maxlight);
  SAV_READ32(f->minlight);

  P_AddThinker(&(f->thinker));

  save_p = ptr + size;

  return &(f->thinker);
#else
  save_p += size;
  return NULL;
#endif
}

static thinker_t *P_UnArchiveElevatorNew(byte size)
{
#ifdef DIYBOOM
  elevator_t *e;
  byte *ptr = save_p;
  boolean hasfunc;
  long off;

  e = (elevator_t*)Z_Malloc(sizeof(elevator_t), PU_LEVEL, NULL);
  memset(e, 0, sizeof(elevator_t));
  SAV_READ8(hasfunc);
  if (hasfunc)
    e->thinker.function.acp1 = (actionf_p1)T_MoveElevator;
  SAV_READ8(e->type);
  SAV_READ32(off);
  e->sector = sectors + off;
  SAV_READ32(e->direction);
  SAV_READ32(e->floordestheight);
  SAV_READ32(e->ceilingdestheight);
  SAV_READ32(e->speed);

  e->sector->floordata = e;
  e->sector->ceilingdata = e;
  P_AddThinker(&(e->thinker));

  save_p = ptr + size;

  return &(e->thinker);
#else
  save_p += size;
  return NULL;
#endif
}

static thinker_t *P_UnArchiveScrollNew(byte size)
{
#ifdef DIYBOOM
  scroll_t *s;
  byte *ptr = save_p;
  boolean hasfunc;

  s = (scroll_t*)Z_Malloc(sizeof(scroll_t), PU_LEVEL, NULL);
  memset(s, 0, sizeof(scroll_t));
  SAV_READ8(hasfunc);
  if (hasfunc)
    s->thinker.function.acp1 = (actionf_p1)T_Scroll;
  SAV_READ32(s->dx);
  SAV_READ32(s->dy);
  SAV_READ32(s->affectee);
  SAV_READ32(s->control);
  SAV_READ32(s->last_height);
  SAV_READ32(s->vdx);
  SAV_READ32(s->vdy);
  SAV_READ32(s->accel);
  SAV_READ8(s->type);

  P_AddThinker(&(s->thinker));

  save_p = ptr + size;

  return &(s->thinker);
#else
  save_p += size;
  return NULL;
#endif
}

static thinker_t *P_UnArchiveFrictionNew(byte size)
{
#ifdef DIYBOOM
  friction_t *f;
  byte *ptr = save_p;
  boolean hasfunc;

  f = (friction_t*)Z_Malloc(sizeof(friction_t), PU_LEVEL, NULL);
  memset(f, 0, sizeof(friction_t));
  SAV_READ8(hasfunc);
  if (hasfunc)
    f->thinker.function.acp1 = (actionf_p1)T_Friction;
  SAV_READ32(f->friction);
  SAV_READ32(f->movefactor);
  SAV_READ32(f->affectee);

  P_AddThinker(&(f->thinker));

  save_p = ptr + size;

  return &(f->thinker);
#else
  save_p += size;
  return NULL;
#endif
}

static thinker_t *P_UnArchivePusherNew(byte size)
{
#ifdef DIYBOOM
  pusher_t *p;
  byte *ptr = save_p;
  boolean hasfunc;

  p = (pusher_t*)Z_Malloc(sizeof(pusher_t), PU_LEVEL, NULL);
  memset(p, 0, sizeof(pusher_t));
  SAV_READ8(hasfunc);
  if (hasfunc)
    p->thinker.function.acp1 = (actionf_p1)T_Pusher;
  SAV_READ8(p->type);
  SAV_READ32(p->x_mag);
  SAV_READ32(p->y_mag);
  SAV_READ32(p->magnitude);
  SAV_READ32(p->radius);
  SAV_READ32(p->x);
  SAV_READ32(p->y);
  SAV_READ32(p->affectee);

  p->source = P_GetPushThing(p->affectee);
  P_AddThinker(&(p->thinker));

  save_p = ptr + size;

  return &(p->thinker);
#else
  save_p += size;
  return NULL;
#endif
}



void P_ArchiveSpecialsNew(void)
{
  byte size_ceiling, size_vldoor, size_floor, size_plat, size_flash, size_strobe, size_glow;
  thinker_t *t;

  /* write number of specials, followed by the size of each */
  size_vldoor = size_vldoor_norm;
  size_flash = size_flash_norm;
  size_strobe = size_strobe_norm;
  size_glow = size_glow_norm;

#ifdef DIYBOOM
  SAV_WRITE8(tc_endspecials);
  size_ceiling = size_ceiling_boom;
  size_floor = size_floor_boom;
  size_plat = size_plat_boom;
#else
  SAV_WRITE8(tc_flicker);	/* first of the Boom types */
  size_ceiling = size_ceiling_norm;
  size_floor = size_floor_norm;
  size_plat = size_plat_norm;
#endif

  SAV_WRITE8(size_ceiling);
  SAV_WRITE8(size_vldoor);
  SAV_WRITE8(size_floor);
  SAV_WRITE8(size_plat);
  SAV_WRITE8(size_flash);
  SAV_WRITE8(size_strobe);
  SAV_WRITE8(size_glow);

#ifdef DIYBOOM
  SAV_WRITE8(size_flicker_boom);
  SAV_WRITE8(size_elevator_boom);
  SAV_WRITE8(size_scroll_boom);
  SAV_WRITE8(size_friction_boom);
  SAV_WRITE8(size_pusher_boom);
#endif

  for (t=thinkercap.next; t != &thinkercap; t = t->next)
  {
    if (t->function.acv == (actionf_v)NULL)
    {
      int i;

#ifdef DIYBOOM
      for (i=0; i<maxplatforms; i++)
	if (activeplats[i] == (plat_t*)t)
	  break;

      if (i < maxplatforms)
	P_ArchivePlatformNew((const plat_t*)t);
#endif

      for (i=0; i<maxceilings; i++)
	if (activeceilings[i] == (ceiling_t*)t)
	  break;

      if (i < maxceilings)
	P_ArchiveCeilingNew((const ceiling_t*)t);
    }

    else if (t->function.acp1 == (actionf_p1)T_MoveCeiling)
      P_ArchiveCeilingNew((const ceiling_t*)t);

    else if (t->function.acp1 == (actionf_p1)T_VerticalDoor)
      P_ArchiveDoorNew((const vldoor_t*)t);

    else if (t->function.acp1 == (actionf_p1)T_MoveFloor)
      P_ArchiveFloorNew((const floormove_t*)t);

    else if (t->function.acp1 == (actionf_p1)T_PlatRaise)
      P_ArchivePlatformNew((const plat_t*)t);

    else if (t->function.acp1 == (actionf_p1)T_LightFlash)
      P_ArchiveFlashNew((const lightflash_t*)t);

    else if (t->function.acp1 == (actionf_p1)T_StrobeFlash)
      P_ArchiveStrobeNew((const strobe_t*)t);

    else if (t->function.acp1 == (actionf_p1)T_Glow)
      P_ArchiveGlowNew((const glow_t*)t);

#ifdef DIYBOOM
    else if (t->function.acp1 == (actionf_p1)T_FireFlicker)
      P_ArchiveFlickerNew((const fireflicker_t*)t);

    else if (t->function.acp1 == (actionf_p1)T_MoveElevator)
      P_ArchiveElevatorNew((const elevator_t*)t);

    else if (t->function.acp1 == (actionf_p1)T_Scroll)
      P_ArchiveScrollNew((const scroll_t*)t);

    else if (t->function.acp1 == (actionf_p1)T_Friction)
      P_ArchiveFrictionNew((const friction_t*)t);

    else if (t->function.acp1 == (actionf_p1)T_Pusher)
      P_ArchivePusherNew((const pusher_t*)t);
#endif
  }
  SAV_WRITE8(tc_endspecials);
#ifdef DEBUG_SAVEGAME
  fprintf(logfile, "specials offset %d\n", save_p - savebuffer); fflush(logfile);
#endif
}


void P_UnArchiveSpecialsNew(void)
{
  byte size_ceiling, size_vldoor, size_floor, size_plat, size_flash, size_strobe, size_glow;
  byte size_flicker, size_elevator, size_scroll, size_friction, size_pusher;
  byte num_specials;

  SAV_READ8(num_specials);
  SAV_READ8(size_ceiling);
  SAV_READ8(size_vldoor);
  SAV_READ8(size_floor);
  SAV_READ8(size_plat);
  SAV_READ8(size_flash);
  SAV_READ8(size_strobe);
  SAV_READ8(size_glow);
  if (num_specials > tc_glow+1)
  {
    SAV_READ8(size_flicker);
    SAV_READ8(size_elevator);
    SAV_READ8(size_scroll);
    SAV_READ8(size_friction);
    SAV_READ8(size_pusher);
  }
  else
  {
    size_flicker = size_flicker_boom;
    size_elevator = size_elevator_boom;
    size_scroll = size_scroll_boom;
    size_friction = size_friction_boom;
    size_pusher = size_pusher_boom;
  }

  while (1)
  {
    byte tclass;

    SAV_READ8(tclass);

    switch(tclass)
    {
      case tc_endspecials:
#ifdef DEBUG_SAVEGAME
        fprintf(logfile, "specials offset %d\n", save_p - savebuffer); fflush(logfile);
#endif
	return;

      case tc_ceiling:
	P_UnArchiveCeilingNew(size_ceiling);
	break;
      case tc_door:
	P_UnArchiveDoorNew(size_vldoor);
	break;
      case tc_floor:
	P_UnArchiveFloorNew(size_floor);
	break;
      case tc_plat:
	P_UnArchivePlatformNew(size_plat);
	break;
      case tc_flash:
	P_UnArchiveFlashNew(size_flash);
	break;
      case tc_strobe:
	P_UnArchiveStrobeNew(size_strobe);
	break;
      case tc_glow:
	P_UnArchiveGlowNew(size_glow);
	break;

      /* Boom types, unarchivers will ignore these types on vanilla Doom */
      case tc_flicker:
	P_UnArchiveFlickerNew(size_flicker);
	break;
      case tc_elevator:
	P_UnArchiveElevatorNew(size_elevator);
	break;
      case tc_scroll:
	P_UnArchiveScrollNew(size_scroll);
	break;
      case tc_friction:
	P_UnArchiveFrictionNew(size_friction);
	break;
      case tc_pusher:
	P_UnArchivePusherNew(size_pusher);
	break;

      default:
	I_Error("P_UnArchiveSpecialsNew: Unknown tclass %d in savegame", tclass);
    }
  }
}


void P_ArchiveMapNew(void)
{
  SAV_WRITE8(2);
  SAV_WRITE8(automapactive);
  SAV_WRITE8(viewactive);
}


void P_UnArchiveMapNew(void)
{
  byte size;
  byte *ptr;

  SAV_READ8(size);
  ptr = save_p;
  SAV_READ8(automapactive);
  SAV_READ8(viewactive);
  save_p = ptr + size;

  if (automapactive)
    AM_Start();
}
