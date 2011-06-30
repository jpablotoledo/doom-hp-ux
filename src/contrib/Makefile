################################################################
#
#  for RISC OS using gcc and SharedCLibrary
#

CC	= gcc
LINK	= gcc
AS	= gcc
# Network stuff is Acorn's socketlib (on ftp.acorn.co.uk)
#NETINC  = -ISocketlib:Include
# Link order is important!
#NETLINK = Socketlib:Libs/inetlib.o Socketlib:Libs/unixlib.o Socketlib:Libs/socklib.o
# Flags shared between gcc and mpp
ASCCFLAGS = -DDIYNOSHORT -DDIYARMASS
# MPP baseflags: add APCS26/32 here
MPPBASEFLAGS = 
# isolated MPP flags: DIYDEBUGPLOT, DIYNOUNROLL
MPPFLAGS = $(MPPBASFLAGS) #-DDIYDEBUGPLOT
# MPP flags for building the fixpoint code: M_LONGMULT (StrongARM/ARM7DM only!), M_FPA
FIXPOINTFLAGS	= $(MPPBASEFLAGS) #-DM_LONGMULT
# MPP flags for building the voices module
VOICEFLAGS	= $(MPPBASEFLAGS)
# additional sound flags
SOUNDFLAGS	=
# base flags, without optimization
BASEFLAGS	= -Wall -pedantic -ansi -mlibscl -mpoke-function-name -DDIYINLINE $(ASCCFLAGS) $(SPECIALFLAGS)
CFLAGS	= $(BASEFLAGS) -O3
# The patch to the fastlz library
FASTLZDIR = fastlz
# Assembler stuff
ASFLAGS = -c
LDFLAGS	= -mlibscl #$(NETLINK)
LIBS	= -L$(FASTLZDIR) -lfastlz
LINKER = gcc

# Fix varargs in some SCL versions
#VARARGSFIX = -DRISCOS_FIX_VARARGS
# Safe optimization level for some files
SAFEOPT	= -O3
VERYSAFEOPT = -O3

# OBJPATH =
# LDFLAGS	= -res


OBJPATH		= $(OBJBASE)/
OBJWRITE	= $(OBJBASE)/

# Object-directories
ODIR8		= o8
ODIR16		= o16
ODIR32		= o32
ODIR32R		= o32r
ODIR16T		= o16t
ODIR16RT	= o16x
ODIR32T		= o32t
ODIR32RT	= o32x
ODIR8B		= b8
ODIR16B		= b16
ODIR16RB	= b16r
ODIR32B		= b32
ODIR32RB	= b32r

# Executables
RUNIMAGE8	= !Doom8
RUNIMAGE16	= !Doom16
RUNIMAGE32	= !Doom32
RUNIMAGE32R	= !Doom32R
RUNIMAGE16T	= !Doom16
RUNIMAGE16RT	= !Doom16R
RUNIMAGE32T	= !Doom32
RUNIMAGE32RT	= !Doom32R
RUNIMAGE8B	= !Boom8
RUNIMAGE16B	= !Boom16
RUNIMAGE16RB	= !Boom16R
RUNIMAGE32B	= !Boom32
RUNIMAGE32RB	= !Boom32R

# to satisfy old make versions
RUNIMAGE	= !NoImage


# Horrible hack, but this is the only way that works with the aged make....
DOOMFLAGS8	= OBJBASE=$(ODIR8) PLOTTERS=plot8 RUNIMAGE=$(RUNIMAGE8) SPECIALFLAGS=-DLD_PIXEL_DEPTH=3
DOOMFLAGS16	= OBJBASE=$(ODIR16) PLOTTERS=plot16 RUNIMAGE=$(RUNIMAGE16) SPECIALFLAGS=-DLD_PIXEL_DEPTH=4
DOOMFLAGS32	= OBJBASE=$(ODIR32) PLOTTERS=plot32 RUNIMAGE=$(RUNIMAGE32) SPECIALFLAGS=-DLD_PIXEL_DEPTH=5
DOOMFLAGS32R	= OBJBASE=$(ODIR32R) PLOTTERS=plot32r RUNIMAGE=$(RUNIMAGE32R) SPECIALFLAGS="-DLD_PIXEL_DEPTH=5 -DDIYRESAMPLE -DDIYRESAMPLETHINGS"
DOOMFLAGS16T	= OBJBASE=$(ODIR16T) PLOTTERS=plot16 RUNIMAGE=$(RUNIMAGE16T) SPECIALFLAGS="-DLD_PIXEL_DEPTH=4 -DDIYTRANSPARENCY"
DOOMFLAGS16RT	= OBJBASE=$(ODIR16RT) PLOTTERS=plot16r RUNIMAGE=$(RUNIMAGE16RT) SPECIALFLAGS="-DLD_PIXEL_DEPTH=4 -DDIYRESAMPLE -DDIYRESAMPLETHINGS -DDIYTRANSPARENCY"
DOOMFLAGS32T	= OBJBASE=$(ODIR32T) PLOTTERS=plot32 RUNIMAGE=$(RUNIMAGE32T) SPECIALFLAGS="-DLD_PIXEL_DEPTH=5 -DDIYTRANSPARENCY"
DOOMFLAGS32RT	= OBJBASE=$(ODIR32RT) PLOTTERS=plot32r RUNIMAGE=$(RUNIMAGE32RT) SPECIALFLAGS="-DLD_PIXEL_DEPTH=5 -DDIYRESAMPLE -DDIYRESAMPLETHINGS -DDIYTRANSPARENCY"
DOOMFLAGS8B	= OBJBASE=$(ODIR8B) PLOTTERS=plot8b RUNIMAGE=$(RUNIMAGE8B) SPECIALFLAGS="-DLD_PIXEL_DEPTH=3 -DDIYBOOM"
DOOMFLAGS16B	= OBJBASE=$(ODIR16B) PLOTTERS=plot16b RUNIMAGE=$(RUNIMAGE16B) SPECIALFLAGS="-DLD_PIXEL_DEPTH=4 -DDIYTRANSPARENCY -DDIYBOOM"
DOOMFLAGS16RB	= OBJBASE=$(ODIR16RB) PLOTTERS=plot16rb RUNIMAGE=$(RUNIMAGE16RB) SPECIALFLAGS="-DLD_PIXEL_DEPTH=4 -DDIYRESAMPLE -DDIYRESAMPLETHINGS -DDIYTRANSPARENCY -DDIYBOOM"
DOOMFLAGS32B	= OBJBASE=$(ODIR32B) PLOTTERS=plot32b RUNIMAGE=$(RUNIMAGE32B) SPECIALFLAGS="-DLD_PIXEL_DEPTH=5 -DDIYTRANSPARENCY -DDIYBOOM"
DOOMFLAGS32RB	= OBJBASE=$(ODIR32RB) PLOTTERS=plot32rb RUNIMAGE=$(RUNIMAGE32RB) SPECIALFLAGS="-DLD_PIXEL_DEPTH=5 -DDIYTRANSPARENCY -DDIYRESAMPLE -DDIYRESAMPLETHINGS -DDIYBOOM"




# not too sophisticated dependency
OBJS=	$(OBJPATH)doomdef.o \
	$(OBJPATH)doomstat.o \
	$(OBJPATH)dstrings.o \
	$(OBJPATH)i_system.o \
	$(OBJPATH)i_sound.o \
	$(OBJPATH)i_main.o \
	$(OBJPATH)i_video.o \
	$(OBJPATH)i_net0.o \
	$(OBJPATH)tables.o \
	$(OBJPATH)f_finale.o \
	$(OBJPATH)f_wipe.o \
	$(OBJPATH)d_main.o \
	$(OBJPATH)d_net.o \
	$(OBJPATH)d_items.o \
	$(OBJPATH)g_game.o \
	$(OBJPATH)m_menu.o \
	$(OBJPATH)m_misc.o \
	$(OBJPATH)m_argv.o \
	$(OBJPATH)m_bbox.o \
	$(OBJPATH)m_fixed.o \
	$(OBJPATH)m_cheat.o \
	$(OBJPATH)m_random.o \
	$(OBJPATH)am_map.o \
	$(OBJPATH)p_ceilng.o \
	$(OBJPATH)p_doors.o \
	$(OBJPATH)p_enemy.o \
	$(OBJPATH)p_floor.o \
	$(OBJPATH)p_inter.o \
	$(OBJPATH)p_lights.o \
	$(OBJPATH)p_map.o \
	$(OBJPATH)p_maputl.o \
	$(OBJPATH)p_plats.o \
	$(OBJPATH)p_pspr.o \
	$(OBJPATH)p_setup.o \
	$(OBJPATH)p_sight.o \
	$(OBJPATH)p_spec.o \
	$(OBJPATH)p_switch.o \
	$(OBJPATH)p_mobj.o \
	$(OBJPATH)p_telept.o \
	$(OBJPATH)p_tick.o \
	$(OBJPATH)p_saveg.o \
	$(OBJPATH)p_saven.o \
	$(OBJPATH)p_user.o \
	$(OBJPATH)p_genlin.o \
	$(OBJPATH)r_bsp.o \
	$(OBJPATH)r_data.o \
	$(OBJPATH)r_draw.o \
	$(OBJPATH)r_main.o \
	$(OBJPATH)r_plane.o \
	$(OBJPATH)r_segs.o \
	$(OBJPATH)r_sky.o \
	$(OBJPATH)r_things.o \
	$(OBJPATH)w_wad.o \
	$(OBJPATH)wi_stuff.o \
	$(OBJPATH)v_video.o \
	$(OBJPATH)st_lib.o \
	$(OBJPATH)st_stuff.o \
	$(OBJPATH)hu_stuff.o \
	$(OBJPATH)hu_lib.o \
	$(OBJPATH)s_sound.o \
	$(OBJPATH)z_zone.o \
	$(OBJPATH)info.o \
	$(OBJPATH)sounds.o \
	$(OBJPATH)dehacked.o \
	$(OBJPATH)ROsupport.o \
	ROasm.o \
	fixpoint.o \
	GameSupp.o \
	voices.o



all:	doom8


clean:
	wipe $(ODIR8) $(ODIR16) $(ODIR32) $(ODIR32R) ~V~CFR


doom8:
	-cdir $(ODIR8)
	make $(DOOMFLAGS8) $(RUNIMAGE8)


doom16:
	-cdir $(ODIR16)
	make $(DOOMFLAGS16) $(RUNIMAGE16)


doom32:
	-cdir $(ODIR32)
	make $(DOOMFLAGS32) $(RUNIMAGE32)


doom32r:
	-cdir $(ODIR32R)
	make $(DOOMFLAGS32R) $(RUNIMAGE32R)


doom16t:
	-cdir $(ODIR16T)
	make $(DOOMFLAGS16T) $(RUNIMAGE16T)


doom16rt:
	-cdir $(ODIR16RT)
	make $(DOOMFLAGS16RT) $(RUNIMAGE16RT)


doom32t:
	-cdir $(ODIR32T)
	make $(DOOMFLAGS32T) $(RUNIMAGE32T)


doom32rt:
	-cdir $(ODIR32RT)
	make $(DOOMFLAGS32RT) $(RUNIMAGE32RT)

# Boom targets

boom8:
	-cdir $(ODIR8B)
	make $(DOOMFLAGS8B) $(RUNIMAGE8B)

boom16:
	-cdir $(ODIR16B)
	make $(DOOMFLAGS16B) $(RUNIMAGE16B)

boom16r:
	-cdir $(ODIR16RB)
	make $(DOOMFLAGS16RB) $(RUNIMAGE16RB)

boom32:
	-cdir $(ODIR32B)
	make $(DOOMFLAGS32B) $(RUNIMAGE32B)

boom32r:
	-cdir $(ODIR32RB)
	make $(DOOMFLAGS32RB) $(RUNIMAGE32RB)




# Aliases
normal:		doom8
high:		doom16
true:		doom32
resample:	doom32r
hight:		doom16t
highrt:		doom16rt
truet:		doom32t
resamplet:	doom32rt
boom:		boom8
boomh:		boom16
boomrh:		boom16r
boomt:		boom32
boomrt:		boom32r






# IMPORTANT: you must NOT use one of the targets below any more. Only use
# the targets doom#[rt] and boom#[r] specified above

$(RUNIMAGE):	o $(OBJS) $(OBJPATH)$(PLOTTERS).o
	-make -r -C $(FASTLZDIR)
	$(LINKER) $(LDFLAGS) $(LIBS) $(OBJS) $(OBJPATH)$(PLOTTERS).o -o $(RUNIMAGE)




mpp:	mpp.o
	$(CC) $(LDFLAGS) mpp.o -o mpp

mpp.o:	mpp.c


# Dependencies (auto-generated 06 Aug 01)
# Manual changes:
# hu_lib, st_lib: replace $(CFLAGS) with $(BASEFLAGS) $(SAFEOPT) (for old GCC)
# i_net: add $(NETINC)
# i_sound: add $(SOUNDFLAGS)
# i_system: add $(VARARGSFIX) (for old GCC)
# p_setup: replace $(CFLAGS) with $(BASEFLAGS) $(VERYSAFEOPT) (for old GCC)
# s_sound: some as versions have a bug. Fix like this:
#	-wipe $(OBJPATH)s_sound ~C~VF
#	$(CC) $(CFLAGS) -c -o $(OBJWRITE)s_sound s_sound.c
#	-rename $(OBJPATH)o_sound $(OBJPATH)s_sound


$(OBJBASE)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJBASE)/%.o : %.s
	$(AS) $(ASFLAGS) -c $< -o $@

$(OBJPATH)am_map.o:	am_map.c ROsupport.h doomtype.h doomdef.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h z_zone.h st_stuff.h d_event.h p_local.h r_local.h r_data.h r_state.h d_player.h d_items.h p_pspr.h d_ticcmd.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h w_wad.h g_game.h m_cheat.h i_system.h v_video.h i_video.h doomstat.h d_net.h r_context.h dstrings.h d_englsh.h am_map.h 

$(OBJPATH)dehacked.o:	c.dehacked ROsupport.h doomtype.h doomdef.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h d_event.h doomstat.h d_net.h d_player.h d_items.h p_pspr.h d_ticcmd.h d_main.h p_inter.h hu_stuff.h f_finale.h st_stuff.h wi_stuff.h am_map.h g_game.h p_spec.h p_local.h r_local.h r_data.h r_state.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h sounds.h m_swap.h m_cheat.h 

$(OBJPATH)doomdef.o:	doomdef.c doomdef.h 

$(OBJPATH)doomstat.o:	doomstat.c doomstat.h doomdata.h doomtype.h doomdef..h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h tables.h info.h d_think.h p_mobj.h d_ticcmd.h 

$(OBJPATH)dstrings.o:	dstrings.c dstrings.h d_englsh.h 

$(OBJPATH)d_items.o:	d_items.c info.h d_think.h d_items.h doomdef.h 

$(OBJPATH)d_main.o:	d_main.c ROsupport.h doomtype.h doomdef.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h GameSupp.h doomstat..h d_net.h d_player.h d_items.h p_pspr.h d_ticcmd.h dstrings.h d_englsh.h sounds.h z_zone.h w_wad.h s_sound.h v_video.h r_data.h r_state.h f_finale.h d_event.h f_wipe.h m_argv.h m_misc.h m_menu.h i_system.h i_sound.h i_video.h g_game.h hu_stuff.h wi_stuff.h st_stuff.h am_map.h p_setup.h r_local.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h r_context.h d_main.h 

$(OBJPATH)d_net.o:	d_net.c m_menu.h d_event.h doomtype.h doomdef.h i_system.h d_ticcmd.h i_video.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h i_net.h g_game.h d_main.h r_main.h d_player.h d_items.h p_pspr.h r_data.h r_state.h doomstat.h d_net.h 

$(OBJPATH)f_finale.o:	f_finale.c d_main.h d_event.h doomtype.h doomdef.h i_system.h d_ticcmd.h m_swap.h z_zone.h v_video.h r_data.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h w_wad.h s_sound.h hu_stuff.h dstrings.h d_englsh.h sounds.h doomstat.h d_net.h r_context.h i_video.h f_finale.h 

$(OBJPATH)f_wipe.o:	f_wipe.c z_zone.h i_video.h doomtype.h doomdef.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h i_system.h d_ticcmd.h d_event.h v_video.h r_data.h r_state.h d_player.h d_items.h p_pspr.h m_random.h f_wipe.h 

$(OBJPATH)g_game.o:	g_game.c doomdef.h doomstat.h doomdata.h doomtype.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h tables.h info.h d_think.h p_mobj.h d_ticcmd.h z_zone.h f_finale.h d_event.h m_argv.h m_misc.h m_menu.h m_random.h i_system.h p_local.h r_local.h r_data.h r_defs.h r_state.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h p_setup..h p_saveg.h p_saven.h p_tick.h d_main.h wi_stuff.h hu_stuff.h st_stuff.h am_map.h v_video.h w_wad.h s_sound.h dstrings.h d_englsh.h sounds.h r_sky.h g_game.h inl_pside.c inl_psub.c 

$(OBJPATH)hu_lib.o:	hu_lib.c doomdef.h v_video.h doomtype.h r_data.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h d_ticcmd.h m_swap.h d_event.h am_map.h hu_lib..h r_local.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h r_context.h

$(OBJPATH)hu_stuff.o:	hu_stuff.c doomdef.h z_zone.h m_swap.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h doomtype.h info.h hu_stuff.h d_event.h hu_lib.h w_wad.h m_menu.h s_sound.h doomstat.h d_net.h d_player.h d_items.h p_pspr.h d_ticcmd.h dstrings.h d_englsh.h sounds.h 

$(OBJPATH)info.o:	info.c sounds.h m_fixed.h info.h d_think.h p_mobj.h tables.h doomdata.h doomtype.h doomdef.h 

$(OBJPATH)inl_blitr.o:	inl_blitr.c 

$(OBJPATH)inl_btitr.o:	inl_btitr.c 

$(OBJPATH)inl_pside.o:	inl_pside.c 

$(OBJPATH)inl_psub.o:	inl_psub.c 

$(OBJPATH)i_main.o:	i_main.c doomdef.h m_argv.h d_main.h d_event.h doomtype.h 

#$(OBJPATH)i_net.o:

$(OBJPATH)i_net0.o:	i_net0.c i_system.h d_ticcmd.h doomtype.h doomdef.h d_event.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h tables.h info.h d_think.h p_mobj.h doomdata.h m_argv.h doomstat.h 

$(OBJPATH)i_sound.o:	i_sound.c ROsupport.h doomtype.h doomdef.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h GameSupp.h z_zone..h i_system.h d_ticcmd.h d_event.h i_sound.h doomstat.h d_net.h d_player.h d_items.h p_pspr.h sounds.h m_argv.h m_misc.h w_wad.h 

$(OBJPATH)i_system.o:	i_system.c ROsupport.h doomtype.h doomdef.h dstrings.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h GameSupp.h m_misc.h m_argv.h i_video.h i_sound.h doomstat.h d_net.h d_player.h d_items.h p_pspr.h d_ticcmd.h sounds.h i_net.h g_game.h d_event.h i_system.h 

$(OBJPATH)i_video.o:	i_video.c ROsupport.h doomtype.h doomdef.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h GameSupp.h doomstat.h d_net.h d_player.h d_items.h p_pspr.h d_ticcmd.h i_system.h d_event.h i_video.h i_sound.h sounds.h i_net.h v_video.h r_data.h r_state.h m_argv.h d_main.h r_draw.h r_context.h r_main.h z_zone.h w_wad.h 

$(OBJPATH)mpp.o:	mpp.c 

$(OBJPATH)m_argv.o:	m_argv.c ROsupport.h doomtype.h doomdef.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h 

$(OBJPATH)m_bbox.o:	m_bbox.c m_bbox.h doomtype.h doomdef.h m_fixed.h 

$(OBJPATH)m_cheat.o:	m_cheat.c doomstat.h doomdata.h doomtype.h doomdef.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h tables.h info.h d_think.h p_mobj.h d_ticcmd.h dstrings.h d_englsh.h g_game.h d_event.h m_cheat.h p_inter.h p_local.h r_local.h r_data.h r_defs.h r_state.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h st_stuff.h s_sound.h sounds.h 

$(OBJPATH)m_fixed.o:	m_fixed.c doomtype.h doomdef.h i_system.h d_ticcmd.h d_event.h m_fixed.h 

$(OBJPATH)m_menu.o:	m_menu.c doomdef.h dstrings.h d_englsh.h d_main.h d_event.h doomtype.h i_system.h d_ticcmd.h i_video.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h z_zone.h v_video.h r_data.h r_state.h d_player.h d_items.h p_pspr.h w_wad.h r_local.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h hu_stuff.h g_game.h m_argv.h m_swap..h s_sound.h doomstat.h d_net.h sounds.h m_menu.h am_map.h

$(OBJPATH)m_misc.o:	m_misc.c ROsupport.h doomtype.h doomdef.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h z_zone.h g_game.h d_event.h d_ticcmd.h m_swap.h m_argv.h m_menu.h w_wad.h i_system.h i_video..h i_sound.h doomstat.h d_net.h d_player.h d_items.h p_pspr.h sounds.h v_video.h r_data.h r_state.h s_sound.h hu_stuff.h dstrings.h d_englsh.h m_misc.h am_map.h

$(OBJPATH)m_random.o:	m_random.c 

$(OBJPATH)m_swap.o:	m_swap.c m_swap.h 

$(OBJPATH)p_ceilng.o:	p_ceilng.c i_system.h d_ticcmd.h doomtype.h doomdef..h d_event.h z_zone.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h s_sound.h doomstat.h d_net.h sounds.h 

$(OBJPATH)p_doors.o:	p_doors.c z_zone.h doomdef.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h doomtype.h info.h r_state.h d_player.h d_items.h p_pspr.h d_ticcmd.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h s_sound.h doomstat.h d_net.h dstrings.h d_englsh.h sounds.h p_inter.h 

$(OBJPATH)p_enemy.o:	p_enemy.c m_random.h doomtype.h doomdef.h i_system.h d_ticcmd.h d_event.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h s_sound.h g_game.h z_zone.h doomstat.h d_net.h sounds.h inl_btitr.c 

$(OBJPATH)p_floor.o:	p_floor.c z_zone.h doomdef.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h doomtype.h info.h r_state.h d_player.h d_items.h p_pspr.h d_ticcmd.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h s_sound.h doomstat.h d_net.h sounds.h 

$(OBJPATH)p_genlin.o:	p_genlin.c 

$(OBJPATH)p_inter.o:	p_inter.c doomdef.h dstrings.h d_englsh.h sounds.h doomstat.h doomdata.h doomtype.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h tables.h info.h d_think.h p_mobj.h d_ticcmd.h m_random.h i_system.h d_event.h am_map.h p_local.h r_local.h r_data.h r_defs.h r_state.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h s_sound.h p_inter.h 

$(OBJPATH)p_lights.o:	p_lights.c z_zone.h m_random.h doomtype.h doomdef.h doomstat.h doomdata.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h tables.h info.h d_think.h p_mobj.h d_ticcmd.h p_local.h r_local.h r_data.h r_defs.h r_state.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h 

$(OBJPATH)p_map.o:	p_map.c m_bbox.h doomtype.h doomdef.h m_fixed.h m_random.h i_system.h d_ticcmd.h d_event.h p_local.h r_local.h tables.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h s_sound.h z_zone.h doomstat.h d_net.h sounds.h inl_pside.c inl_psub.c inl_blitr.c inl_btitr.c 

$(OBJPATH)p_maputl.o:	p_maputl.c m_bbox.h doomtype.h doomdef.h m_fixed.h doomstat.h doomdata.h d_net.h d_player.h d_items.h p_pspr.h tables.h info..h d_think.h p_mobj.h d_ticcmd.h p_local.h r_local.h r_data.h r_defs.h r_state.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h z_zone.h i_system.h d_event.h inl_pside.c inl_psub.c inl_blitr.c inl_btitr.c 

$(OBJPATH)p_mobj.o:	p_mobj.c i_system.h d_ticcmd.h doomtype.h doomdef.h d_event.h z_zone.h m_random.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h sounds.h g_game.h st_stuff.h hu_stuff.h s_sound.h doomstat.h d_net.h p_setup.h ROsupport.h inl_pside.c inl_psub.c 

$(OBJPATH)p_plats.o:	p_plats.c i_system.h d_ticcmd.h doomtype.h doomdef.h d_event.h z_zone.h m_random.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h s_sound.h doomstat.h d_net.h sounds.h 

$(OBJPATH)p_pspr.o:	p_pspr.c doomdef.h d_event.h doomtype.h m_random.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h d_ticcmd.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h s_sound.h doomstat.h d_net.h sounds.h 

$(OBJPATH)p_saveg.o:	p_saveg.c i_system.h d_ticcmd.h doomtype.h doomdef.h d_event.h z_zone.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h p_saveg.h am_map.h doomstat.h d_net.h 

$(OBJPATH)p_saven.o:	p_saven.c i_system.h d_ticcmd.h doomtype.h doomdef.h d_event.h z_zone.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h p_saven.h p_saveg.h am_map.h doomstat.h d_net.h 

$(OBJPATH)p_setup.o:	p_setup.c ROsupport.h doomtype.h doomdef.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h z_zone.h m_swap.h m_bbox.h m_argv.h g_game.h d_event.h d_ticcmd.h i_system.h w_wad.h p_local.h r_local.h r_data.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h s_sound.h doomstat.h d_net.h p_setup.h 

$(OBJPATH)p_sight.o:	p_sight.c doomdef.h i_system.h d_ticcmd.h doomtype.h d_event.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h 

$(OBJPATH)p_speo.c:	p_spec.c doomdef.h doomstat.h doomdata.h doomtype.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h tables.h info.h d_think.h p_mobj.h d_ticcmd.h i_system.h d_event.h z_zone.h m_argv.h m_random.h m_swap.h m_bbox.h w_wad.h r_local.h r_data.h r_defs.h r_state.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_local.h p_spec.h p_inter.h g_game.h s_sound.h sounds.h 

$(OBJPATH)p_switch.o:	p_switch.c i_system.h d_ticcmd.h doomtype.h doomdef..h d_event.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h g_game.h s_sound.h sounds.h doomstat.h d_net.h 

$(OBJPATH)p_telept.o:	p_telept.c doomdef.h s_sound.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h doomtype.h info.h r_state.h d_player.h d_items.h p_pspr.h d_ticcmd.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h sounds.h 

$(OBJPATH)p_tick.o:	p_tick.c z_zone.h p_local.h r_local.h tables.h m_fixed.h doomdef.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h doomtype.h info.h r_state.h d_player.h d_items.h p_pspr.h d_ticcmd.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h doomstat.h d_net.h 

$(OBJPATH)p_user.o:	p_user.c doomdef.h d_event.h doomtype.h p_local.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h d_ticcmd.h r_main.h r_bsp..h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h doomstat.h d_net.h 

$(OBJPATH)ROsupport.o:	ROsupport.c ROsupport.h doomtype.h doomdef.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h 

$(OBJPATH)r_bsp.o:	r_bsp.c doomdef.h m_bbox.h doomtype.h m_fixed.h i_system.h d_ticcmd.h d_event.h r_main.h d_player.h d_items.h p_pspr.h tables.h info.h d_think.h p_mobj.h doomdata.h r_data.h r_defs.h r_state.h r_plane..h r_segs.h r_things.h doomstat.h d_net.h r_draw.h r_context.h inl_pside.c 

$(OBJPATH)r_data.o:	r_data.c i_system.h d_ticcmd.h doomtype.h doomdef.h d_event.h z_zone.h m_swap.h w_wad.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_local.h p_spec.h doomstat.h d_net.h r_sky.h ROsupport.h 

$(OBJPATH)r_draw.o:	r_draw.c doomdef.h i_system.h d_ticcmd.h doomtype.h d_event.h z_zone.h w_wad.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h r_context.h v_video.h doomstat.h d_net.h i_video.h ROsupport.h 

$(OBJPATH)r_main.o:	r_main.c ROsupport.h doomstat.h doomdata.h doomtype.h doomdef.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h tables.h info.h d_think.h p_mobj.h d_ticcmd.h m_bbox.h m_menu.h d_event.h r_local.h r_data.h r_defs.h r_state.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h r_sky.h r_context.h i_video.h p_spec.h z_zone.h 

$(OBJPATH)r_plane.o:	r_plane.c ROsupport.h doomtype.h doomdef.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h i_system.h d_ticcmd.h d_event.h i_video.h z_zone.h w_wad.h doomstat.h d_net.h d_player.h d_items.h p_pspr.h r_local.h r_data.h r_state.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h r_sky.h r_context.h 

$(OBJPATH)r_segs.o:	r_segs.c i_system.h d_ticcmd.h doomtype.h doomdef.h d_event.h doomstat.h doomdata.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h tables.h info.h d_think.h p_mobj.h r_local.h r_data.h r_defs.h r_state.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h r_context.h r_sky.h z_zone.h 

$(OBJPATH)r_sky.o:	r_sky.c m_fixed.h r_data.h r_defs.h doomdef.h d_think.h p_mobj.h tables.h doomdata.h doomtype.h info.h r_state.h d_player.h d_items.h p_pspr.h d_ticcmd.h r_sky.h 

$(OBJPATH)r_things.o:	r_things.c doomdef.h m_swap.h i_system.h d_ticcmd.h doomtype.h d_event.h i_video.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h z_zone.h w_wad.h r_local.h r_data.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h r_context.h st_stuff.h doomstat.h d_net.h ROsupport.h 

$(OBJPATH)sounds.o:	sounds.c doomtype.h doomdef.h sounds.h 

$(OBJPATH)st_lib.o:	st_lib.c doomdef.h z_zone.h v_video.h doomtype.h r_data.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h d_ticcmd.h m_swap.h i_system.h d_event.h w_wad.h st_stuff.h st_lib.h r_local.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h 

$(OBJPATH)st_stuff.o:	st_stuff.c i_system.h d_ticcmd.h doomtype.h doomdef..h d_event.h i_video.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h z_zone.h m_random.h w_wad.h hu_stuff.h g_game.h st_stuff.h st_lib.h r_local.h r_data.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_local.h p_spec.h p_inter.h am_map.h m_cheat.h s_sound.h v_video.h doomstat.h d_net.h dstrings.h d_englsh.h sounds.h 

$(OBJPATH)s_sound.o:	s_sound.c i_system.h d_ticcmd.h doomtype.h doomdef.h d_event.h i_sound.h doomstat.h doomdata.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h tables.h info.h d_think.h p_mobj.h sounds.h s_sound.h z_zone.h m_random.h w_wad.h p_local.h r_local.h r_data.h r_defs.h r_state.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h p_spec.h 

$(OBJPATH)tables.o:	tables.c tables.h m_fixed.h 

$(OBJPATH)v_video.o:	v_video.c i_system.h d_ticcmd.h doomtype.h doomdef.h d_event.h r_local.h tables.h m_fixed.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h r_context.h m_bbox.h m_swap.h v_video.h i_video.h ROsupport.h 

$(OBJPATH)wi_stuff.o:	wi_stuff.c z_zone.h m_random.h doomtype.h doomdef.h m_swap.h m_fixed.h i_system.h d_ticcmd.h d_event.h w_wad.h g_game.h r_local.h tables.h r_data.h r_defs.h d_think.h p_mobj.h doomdata.h info.h r_state.h d_player.h d_items.h p_pspr.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h s_sound.h doomstat.h d_net.h sounds.h v_video.h wi_stuff.h i_video.h 

$(OBJPATH)w_wad.o:	w_wad.c ROsupport.h doomtype.h doomdef.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h doomdata.h info.h doomstat.h d_net.h d_player.h d_items.h p_pspr.h d_ticcmd.h m_swap.h i_system.h d_event.h z_zone.h w_wad.h p_spec.h 

$(OBJPATH)z_zone.o:	z_zone.c z_zone.h i_system.h d_ticcmd.h doomtype.h doomdef.h d_event.h doomstat.h doomdata.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h tables.h info.h d_think.h p_mobj.h 






# Creating assembler files out of macro definitions
plot8.s:	plotterlib.m plotlib8.m sharedass.m mpp
	mpp $(MPPFLAGS) $(ASCCFLAGS) -o s.plot8 m.plotlib8

plot8b.s:	plotterlib.m plotlib8.m sharedass.m mpp
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYBOOM -o s.plot8b m.plotlib8

plot16.s:	plotterlib.m plotlib16.m sharedass.m mpp
	mpp $(MPPFLAGS) $(ASCCFLAGS) -o s.plot16 m.plotlib16

plot16r.s:	plotterlib.h plotlib16.m m.sharedass mpp
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYRESAMPLE -o s.plot16r plotlib16.m

plot16b.s:	plotterlib.h plotlib16.m sharedass.m mpp
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYBOOM -o s.plot16b plotlib16.m

plot16rb.s:	plotterlib.h plotlib16.m sharedass.m mpp
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYBOOM -DDIYRESAMPLE -o s.plot16rb plotlib16.m

plot32.s:	plotterlib.h m.plotlib32 sharedass.m mpp
	mpp $(MPPFLAGS) $(ASCCFLAGS) -o s.plot32 m.plotlib32

plot32r.s:	plotterlib.h m.plotlib32 sharedass.m mpp
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYRESAMPLE -o s.plot32r m.plotlib32

plot32b.s:	plotterlib.h m.plotlib32 sharedass.m mpp
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYBOOM -o s.plot32b m.plotlib32

plot32rb.s:	plotterlib.h m.plotlib32 sharedass.m mpp
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYBOOM -DDIYRESAMPLE -o s.plot32rb m.plotlib32

fixpoint.s:	fixpoint.m sharedass.m mpp
	mpp $(FIXPOINTFLAGS) -o s.fixpoint m.fixpoint

voices.s:	voices.m sharedass.m mpp 
	mpp $(VOICEFLAGS) -o s.voices m.voices

ROasm.s:	ROasm.m sharedass.m mpp
	mpp $(MPPFLAGS) $(ASCCFLAGS) -o s.ROasm m.ROasm

GameSupp.s:	GameSupp.m sharedass.m mpp
	mpp $(MPPFLAGS) $(ASCCFLAGS) -o s.GameSupp m.GameSupp



# Temporary assembler files to object files
$(OBJPATH)plot8.o:	plot8.s

$(OBJPATH)plot8b.o:	plot8b.s

$(OBJPATH)plot16.o:	plot16.s

$(OBJPATH)plot16r.o:	plot16r.s

$(OBJPATH)plot16b.o:	plot16b.s

$(OBJPATH)plot16rb.o:	plot16rb.s

$(OBJPATH)plot32.o:	plot32.s

$(OBJPATH)plot32r.o:	plot32r.s

$(OBJPATH)plot32b.o:	plot32b.s

$(OBJPATH)plot32rb.o:	plot32rb.s

fixpoint.o:	fixpoint.s

voices.o:	voices.s

ROasm.o:	ROasm.s

GameSupp.o:	GameSupp.s
