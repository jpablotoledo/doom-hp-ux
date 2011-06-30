################################################################
#
# Makefile for RISC OS using gcc and SharedCLibrary
#

CC	= gcc
LINK	= drlink
AS	= as
# Network stuff is Acorn's socketlib (on ftp.acorn.co.uk)
NETINC  = -ISocketlib:Include
# Link order is important!
#NETLINK = Socketlib:Libs/inetlib.o Socketlib:Libs/unixlib.o Socketlib:Libs/socklib.o
# Flags shared between gcc and mpp
ASCCFLAGS = -DDIYNOSHORT -DDIYARMASS
# MPP baseflags, add APCS switches here (APCS32/APCS26)
MPPBASEFLAGS =
# isolated MPP flags: DIYDEBUGPLOT, DIYNOUNROLL
MPPFLAGS = $(MPPBASEFLAGS) #-DDIYDEBUGPLOT
# MPP flags for building the fixpoint code: M_LONGMULT, M_FPA
FIXPOINTFLAGS	= $(MPPBASEFLAGS) -DM_LONGMULT
# MPP flags for building the voices module
VOICEFLAGS	= $(MPPBASEFLAGS)
# additional sound flags
SOUNDFLAGS	=
# switch to use Shared C Library for compiling and linking
LIBSCLSWITCH	= -mlibscl
# base flags, without optimization
BASEFLAGS	= -Wall -pedantic -ansi $(LIBSCLSWITCH) -mpoke-function-name -DDIYINLINE $(ASCCFLAGS) $(SPECIALFLAGS)
CFLAGS	= $(BASEFLAGS) -O3
# The patch to the fastlz library
FASTLZDIR = fastlz
# Assembler stuff
ASFLAGS = -gcc
LDFLAGS	= $(LIBSCLSWITCH) $(NETLINK)
LIBS	= -L$(FASTLZDIR) -lfastlz
LINKFLAGS =
LINKER	= drlink

WIPE	= wipe
WIPEFLAGS	= ~C~VF
WIPEDIRFLAGS	= $(WIPEFLAGS)R

# Fix varargs in some SCL versions
#VARARGSFIX = -DRISCOS_FIX_VARARGS
# Safe optimization level for some files
SAFEOPT	= -O3
VERYSAFEOPT = -O3


# OBJPATH =
# LDFLAGS	= -res
# LIBS	= GCCbin:arm-riscos.2_95_2.o.libgcc C:o.stubs


OBJPATH		= $(OBJBASE).
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

# to satisfy old make versions...
RUNIMAGE	= !NoImage


# Horrible hack, but this is the only way that works with the aged make...
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
OBJS=	$(OBJPATH)doomdef\
	$(OBJPATH)doomstat\
	$(OBJPATH)dstrings\
	$(OBJPATH)i_system\
	$(OBJPATH)i_sound\
	$(OBJPATH)i_main\
	$(OBJPATH)i_video\
	$(OBJPATH)i_net0\
	$(OBJPATH)tables\
	$(OBJPATH)f_finale\
	$(OBJPATH)f_wipe\
	$(OBJPATH)d_main\
	$(OBJPATH)d_net\
	$(OBJPATH)d_items\
	$(OBJPATH)g_game\
	$(OBJPATH)m_menu\
	$(OBJPATH)m_misc\
	$(OBJPATH)m_argv\
	$(OBJPATH)m_bbox\
	$(OBJPATH)m_fixed\
	$(OBJPATH)m_cheat\
	$(OBJPATH)m_random\
	$(OBJPATH)am_map\
	$(OBJPATH)p_ceilng\
	$(OBJPATH)p_doors\
	$(OBJPATH)p_enemy\
	$(OBJPATH)p_floor\
	$(OBJPATH)p_inter\
	$(OBJPATH)p_lights\
	$(OBJPATH)p_map\
	$(OBJPATH)p_maputl\
	$(OBJPATH)p_plats\
	$(OBJPATH)p_pspr\
	$(OBJPATH)p_setup\
	$(OBJPATH)p_sight\
	$(OBJPATH)p_spec\
	$(OBJPATH)p_switch\
	$(OBJPATH)p_mobj\
	$(OBJPATH)p_telept\
	$(OBJPATH)p_tick\
	$(OBJPATH)p_saveg\
	$(OBJPATH)p_saven\
	$(OBJPATH)p_user\
	$(OBJPATH)p_genlin\
	$(OBJPATH)r_bsp\
	$(OBJPATH)r_data\
	$(OBJPATH)r_draw\
	$(OBJPATH)r_main\
	$(OBJPATH)r_plane\
	$(OBJPATH)r_segs\
	$(OBJPATH)r_sky\
	$(OBJPATH)r_things\
	$(OBJPATH)w_wad\
	$(OBJPATH)wi_stuff\
	$(OBJPATH)v_video\
	$(OBJPATH)st_lib\
	$(OBJPATH)st_stuff\
	$(OBJPATH)hu_stuff\
	$(OBJPATH)hu_lib\
	$(OBJPATH)s_sound\
	$(OBJPATH)z_zone\
	$(OBJPATH)info\
	$(OBJPATH)sounds\
	$(OBJPATH)dehacked\
	$(OBJPATH)ROsupport\
	o.ROasm\
	o.fixpoint\
	o.GameSupp\
	o.voices



all:	doom8


world:	doom8 doom16t doom16rt doom32t doom32rt boom8 boom16 boom16r boom32 boom32r




clean:
	-$(WIPE) o $(WIPEDIRFLAGS)
	-$(WIPE) $(ODIR8)* $(WIPEDIRFLAGS)
	-$(WIPE) $(ODIR16)* $(WIPEDIRFLAGS)
	-$(WIPE) $(ODIR32)* $(WIPEDIRFLAGS)
	-$(WIPE) $(ODIR8B)* $(WIPEDIRFLAGS)
	-$(WIPE) $(ODIR16B)* $(WIPEDIRFLAGS)
	-$(WIPE) $(ODIR32B)* $(WIPEDIRFLAGS)



doom8:
	-cdir $(ODIR8)
	make -r $(DOOMFLAGS8) $(RUNIMAGE8)


doom16:
	-cdir $(ODIR16)
	make -r $(DOOMFLAGS16) $(RUNIMAGE16)


doom32:
	-cdir $(ODIR32)
	make -r $(DOOMFLAGS32) $(RUNIMAGE32)


doom32r:
	-cdir $(ODIR32R)
	make -r $(DOOMFLAGS32R) $(RUNIMAGE32R)


doom16t:
	-cdir $(ODIR16T)
	make -r $(DOOMFLAGS16T) $(RUNIMAGE16T)


doom16rt:
	-cdir $(ODIR16RT)
	make -r $(DOOMFLAGS16RT) $(RUNIMAGE16RT)


doom32t:
	-cdir $(ODIR32T)
	make -r $(DOOMFLAGS32T) $(RUNIMAGE32T)


doom32rt:
	-cdir $(ODIR32RT)
	make -r $(DOOMFLAGS32RT) $(RUNIMAGE32RT)

# Boom targets

boom8:
	-cdir $(ODIR8B)
	make -r $(DOOMFLAGS8B) $(RUNIMAGE8B)

boom16:
	-cdir $(ODIR16B)
	make -r $(DOOMFLAGS16B) $(RUNIMAGE16B)

boom16r:
	-cdir $(ODIR16RB)
	make -r $(DOOMFLAGS16RB) $(RUNIMAGE16RB)

boom32:
	-cdir $(ODIR32B)
	make -r $(DOOMFLAGS32B) $(RUNIMAGE32B)

boom32r:
	-cdir $(ODIR32RB)
	make -r $(DOOMFLAGS32RB) $(RUNIMAGE32RB)




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

$(RUNIMAGE):	o $(OBJS) $(OBJPATH)$(PLOTTERS)
	-make -r -c $(FASTLZDIR)
	$(CC) $(LDFLAGS) $(LIBS) $(OBJS) $(OBJPATH)$(PLOTTERS) -o $(RUNIMAGE)




mpp:	o o.mpp
	$(CC) $(LDFLAGS) o.mpp -o mpp

o:
	-cdir o

s:
	-cdir s

o.mpp:	c.mpp
	$(CC) $(CFLAGS) -c -o o.mpp c.mpp




# Dependencies (auto-generated 06 Aug 01)
# Manual changes:
# hu_lib, st_lib: replace $(CFLAGS) with $(BASEFLAGS) $(SAFEOPT) (for old GCC)
# i_net: add $(NETINC)
# i_sound: add $(SOUNDFLAGS)
# i_system: add $(VARARGSFIX) (for old GCC)
# p_setup: replace $(CFLAGS) with $(BASEFLAGS) $(VERYSAFEOPT) (for old GCC)
# s_sound: some as versions have a bug. Fix like this:
#	-wipe $(OBJPATH)s_sound ~C~VF
#	$(CC) $(CFLAGS) -c -o $(OBJWRITE)s_sound c.s_sound
#	-rename $(OBJPATH)o_sound $(OBJPATH)s_sound
# w_wad: add -I$(FASTLZDIR) for libfastlz include

$(OBJPATH)am_map:	c.am_map h.ROsupport h.doomtype h.doomdef h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.z_zone h.st_stuff h.d_event h.p_local h.r_local h.r_data h.r_state h.d_player h.d_items h.p_pspr h.d_ticcmd h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.w_wad h.g_game h.m_cheat h.i_system h.v_video h.i_video h.doomstat h.d_net h.r_context h.dstrings h.d_englsh h.am_map h.GameSupp
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)am_map c.am_map

$(OBJPATH)dehacked:	c.dehacked h.ROsupport h.doomtype h.doomdef h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.d_event h.doomstat h.d_net h.d_player h.d_items h.p_pspr h.d_ticcmd h.d_main h.p_inter h.hu_stuff h.f_finale h.st_stuff h.wi_stuff h.am_map h.g_game h.p_spec h.p_local h.r_local h.r_data h.r_state h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.sounds h.m_swap h.m_cheat 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)dehacked c.dehacked

$(OBJPATH)doomdef:	c.doomdef h.doomdef 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)doomdef c.doomdef

$(OBJPATH)doomstat:	c.doomstat h.doomstat h.doomdata h.doomtype h.doomdef h.d_net h.d_player h.d_items h.p_pspr h.m_fixed h.tables h.info h.d_think h.p_mobj h.d_ticcmd 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)doomstat c.doomstat

$(OBJPATH)dstrings:	c.dstrings h.dstrings h.d_englsh 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)dstrings c.dstrings

$(OBJPATH)d_items:	c.d_items h.info h.d_think h.d_items h.doomdef 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)d_items c.d_items

$(OBJPATH)d_main:	c.d_main h.ROsupport h.doomtype h.doomdef h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.GameSupp h.doomstat h.d_net h.d_player h.d_items h.p_pspr h.d_ticcmd h.dstrings h.d_englsh h.sounds h.z_zone h.w_wad h.s_sound h.v_video h.r_data h.r_state h.f_finale h.d_event h.f_wipe h.m_argv h.m_misc h.m_menu h.i_system h.i_sound h.i_video h.g_game h.hu_stuff h.wi_stuff h.st_stuff h.am_map h.p_setup h.r_local h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.r_context h.d_main 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)d_main c.d_main

$(OBJPATH)d_net:	c.d_net h.m_menu h.d_event h.doomtype h.doomdef h.i_system h.d_ticcmd h.i_video h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.i_net h.g_game h.d_main h.r_main h.d_player h.d_items h.p_pspr h.r_data h.r_state h.doomstat h.d_net 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)d_net c.d_net

$(OBJPATH)f_finale:	c.f_finale h.d_main h.d_event h.doomtype h.doomdef h.i_system h.d_ticcmd h.m_swap h.z_zone h.v_video h.r_data h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.w_wad h.s_sound h.hu_stuff h.dstrings h.d_englsh h.sounds h.doomstat h.d_net h.r_context h.i_video h.f_finale 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)f_finale c.f_finale

$(OBJPATH)f_wipe:	c.f_wipe h.z_zone h.i_video h.doomtype h.doomdef h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.i_system h.d_ticcmd h.d_event h.v_video h.r_data h.r_state h.d_player h.d_items h.p_pspr h.m_random h.f_wipe 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)f_wipe c.f_wipe

$(OBJPATH)g_game:	c.g_game h.doomdef h.doomstat h.doomdata h.doomtype h.d_net h.d_player h.d_items h.p_pspr h.m_fixed h.tables h.info h.d_think h.p_mobj h.d_ticcmd h.z_zone h.f_finale h.d_event h.m_argv h.m_misc h.m_menu h.m_random h.i_system h.p_local h.r_local h.r_data h.r_defs h.r_state h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.p_setup h.p_saveg h.p_saven h.p_tick h.d_main h.wi_stuff h.hu_stuff h.st_stuff h.am_map h.v_video h.w_wad h.s_sound h.dstrings h.d_englsh h.sounds h.r_sky h.g_game c.inl_pside c.inl_psub 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)g_game c.g_game

$(OBJPATH)hu_lib:	c.hu_lib h.doomdef h.v_video h.doomtype h.r_data h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.d_ticcmd h.m_swap h.d_event h.am_map h.hu_lib h.r_local h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.r_context 
	$(CC) $(BASEFLAGS) $(SAFEOPT) -c -o $(OBJWRITE)hu_lib c.hu_lib

$(OBJPATH)hu_stuff:	c.hu_stuff h.doomdef h.z_zone h.m_swap h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.doomtype h.info h.hu_stuff h.d_event h.hu_lib h.w_wad h.m_menu h.s_sound h.doomstat h.d_net h.d_player h.d_items h.p_pspr h.d_ticcmd h.dstrings h.d_englsh h.sounds 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)hu_stuff c.hu_stuff

$(OBJPATH)info:	c.info h.sounds h.m_fixed h.info h.d_think h.p_mobj h.tables h.doomdata h.doomtype h.doomdef 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)info c.info

$(OBJPATH)inl_blitr:	c.inl_blitr 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)inl_blitr c.inl_blitr

$(OBJPATH)inl_btitr:	c.inl_btitr 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)inl_btitr c.inl_btitr

$(OBJPATH)inl_pside:	c.inl_pside 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)inl_pside c.inl_pside

$(OBJPATH)inl_psub:	c.inl_psub 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)inl_psub c.inl_psub

$(OBJPATH)i_main:	c.i_main h.doomdef h.m_argv h.d_main h.d_event h.doomtype 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)i_main c.i_main

$(OBJPATH)i_net:	
	$(CC) $(CFLAGS) $(NETINC) -c -o $(OBJWRITE)i_net c.i_net

$(OBJPATH)i_net0:	c.i_net0 h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.d_net h.d_player h.d_items h.p_pspr h.m_fixed h.tables h.info h.d_think h.p_mobj h.doomdata h.m_argv h.doomstat 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)i_net0 c.i_net0

$(OBJPATH)i_sound:	c.i_sound h.ROsupport h.doomtype h.doomdef h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.GameSupp h.z_zone h.i_system h.d_ticcmd h.d_event h.i_sound h.doomstat h.d_net h.d_player h.d_items h.p_pspr h.sounds h.m_argv h.m_misc h.w_wad 
	$(CC) $(CFLAGS) $(SOUNDFLAGS) -c -o $(OBJWRITE)i_sound c.i_sound

$(OBJPATH)i_system:	c.i_system h.ROsupport h.doomtype h.doomdef h.dstrings h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.GameSupp h.m_misc h.m_argv h.i_video h.i_sound h.doomstat h.d_net h.d_player h.d_items h.p_pspr h.d_ticcmd h.sounds h.i_net h.g_game h.d_event h.i_system 
	$(CC) $(CFLAGS) $(VARARGSFIX) -c -o $(OBJWRITE)i_system c.i_system

$(OBJPATH)i_video:	c.i_video h.ROsupport h.doomtype h.doomdef h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.GameSupp h.doomstat h.d_net h.d_player h.d_items h.p_pspr h.d_ticcmd h.i_system h.d_event h.i_video h.i_sound h.sounds h.i_net h.v_video h.r_data h.r_state h.m_argv h.d_main h.r_draw h.r_context h.r_main h.z_zone h.w_wad h.m_menu
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)i_video c.i_video

$(OBJPATH)mpp:	c.mpp 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)mpp c.mpp

$(OBJPATH)m_argv:	c.m_argv h.ROsupport h.doomtype h.doomdef h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)m_argv c.m_argv

$(OBJPATH)m_bbox:	c.m_bbox h.m_bbox h.doomtype h.doomdef h.m_fixed 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)m_bbox c.m_bbox

$(OBJPATH)m_cheat:	c.m_cheat h.doomstat h.doomdata h.doomtype h.doomdef h.d_net h.d_player h.d_items h.p_pspr h.m_fixed h.tables h.info h.d_think h.p_mobj h.d_ticcmd h.dstrings h.d_englsh h.g_game h.d_event h.m_cheat h.p_inter h.p_local h.r_local h.r_data h.r_defs h.r_state h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.st_stuff h.s_sound h.sounds 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)m_cheat c.m_cheat

$(OBJPATH)m_fixed:	c.m_fixed h.doomtype h.doomdef h.i_system h.d_ticcmd h.d_event h.m_fixed 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)m_fixed c.m_fixed

$(OBJPATH)m_menu:	c.m_menu h.doomdef h.dstrings h.d_englsh h.d_main h.d_event h.doomtype h.i_system h.d_ticcmd h.i_video h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.z_zone h.v_video h.r_data h.r_state h.d_player h.d_items h.p_pspr h.w_wad h.r_local h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.hu_stuff h.g_game h.m_argv h.m_swap h.s_sound h.doomstat h.d_net h.sounds h.m_menu h.am_map
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)m_menu c.m_menu

$(OBJPATH)m_misc:	c.m_misc h.ROsupport h.doomtype h.doomdef h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.z_zone h.g_game h.d_event h.d_ticcmd h.m_swap h.m_argv h.m_menu h.w_wad h.i_system h.i_video h.i_sound h.doomstat h.d_net h.d_player h.d_items h.p_pspr h.sounds h.v_video h.r_data h.r_state h.s_sound h.hu_stuff h.dstrings h.d_englsh h.m_misc h.am_map
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)m_misc c.m_misc

$(OBJPATH)m_random:	c.m_random 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)m_random c.m_random

$(OBJPATH)m_swap:	c.m_swap h.m_swap 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)m_swap c.m_swap

$(OBJPATH)p_ceilng:	c.p_ceilng h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.z_zone h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.s_sound h.doomstat h.d_net h.sounds 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_ceilng c.p_ceilng

$(OBJPATH)p_doors:	c.p_doors h.z_zone h.doomdef h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.doomtype h.info h.r_state h.d_player h.d_items h.p_pspr h.d_ticcmd h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.s_sound h.doomstat h.d_net h.dstrings h.d_englsh h.sounds h.p_inter 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_doors c.p_doors

$(OBJPATH)p_enemy:	c.p_enemy h.m_random h.doomtype h.doomdef h.i_system h.d_ticcmd h.d_event h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.s_sound h.g_game h.z_zone h.doomstat h.d_net h.sounds c.inl_btitr 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_enemy c.p_enemy

$(OBJPATH)p_floor:	c.p_floor h.z_zone h.doomdef h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.doomtype h.info h.r_state h.d_player h.d_items h.p_pspr h.d_ticcmd h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.s_sound h.doomstat h.d_net h.sounds 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_floor c.p_floor

$(OBJPATH)p_genlin:	c.p_genlin 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_genlin c.p_genlin

$(OBJPATH)p_inter:	c.p_inter h.doomdef h.dstrings h.d_englsh h.sounds h.doomstat h.doomdata h.doomtype h.d_net h.d_player h.d_items h.p_pspr h.m_fixed h.tables h.info h.d_think h.p_mobj h.d_ticcmd h.m_random h.i_system h.d_event h.am_map h.p_local h.r_local h.r_data h.r_defs h.r_state h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.s_sound h.p_inter 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_inter c.p_inter

$(OBJPATH)p_lights:	c.p_lights h.z_zone h.m_random h.doomtype h.doomdef h.doomstat h.doomdata h.d_net h.d_player h.d_items h.p_pspr h.m_fixed h.tables h.info h.d_think h.p_mobj h.d_ticcmd h.p_local h.r_local h.r_data h.r_defs h.r_state h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_lights c.p_lights

$(OBJPATH)p_map:	c.p_map h.m_bbox h.doomtype h.doomdef h.m_fixed h.m_random h.i_system h.d_ticcmd h.d_event h.p_local h.r_local h.tables h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.s_sound h.z_zone h.doomstat h.d_net h.sounds c.inl_pside c.inl_psub c.inl_blitr c.inl_btitr 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_map c.p_map

$(OBJPATH)p_maputl:	c.p_maputl h.m_bbox h.doomtype h.doomdef h.m_fixed h.doomstat h.doomdata h.d_net h.d_player h.d_items h.p_pspr h.tables h.info h.d_think h.p_mobj h.d_ticcmd h.p_local h.r_local h.r_data h.r_defs h.r_state h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.z_zone h.i_system h.d_event c.inl_pside c.inl_psub c.inl_blitr c.inl_btitr 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_maputl c.p_maputl

$(OBJPATH)p_mobj:	c.p_mobj h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.z_zone h.m_random h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.sounds h.g_game h.st_stuff h.hu_stuff h.s_sound h.doomstat h.d_net h.p_setup h.ROsupport c.inl_pside c.inl_psub 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_mobj c.p_mobj

$(OBJPATH)p_plats:	c.p_plats h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.z_zone h.m_random h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.s_sound h.doomstat h.d_net h.sounds 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_plats c.p_plats

$(OBJPATH)p_pspr:	c.p_pspr h.doomdef h.d_event h.doomtype h.m_random h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.d_ticcmd h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.s_sound h.doomstat h.d_net h.sounds 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_pspr c.p_pspr

$(OBJPATH)p_saveg:	c.p_saveg h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.z_zone h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.p_saveg h.am_map h.doomstat h.d_net 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_saveg c.p_saveg

$(OBJPATH)p_saven:	c.p_saven h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.z_zone h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.p_saven h.p_saveg h.am_map h.doomstat h.d_net 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_saven c.p_saven

$(OBJPATH)p_setup:	c.p_setup h.ROsupport h.doomtype h.doomdef h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.z_zone h.m_swap h.m_bbox h.m_argv h.g_game h.d_event h.d_ticcmd h.i_system h.w_wad h.p_local h.r_local h.r_data h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.s_sound h.doomstat h.d_net h.p_setup 
	$(CC) $(BASEFLAGS) $(VERYSAFEOPT) -c -o $(OBJWRITE)p_setup c.p_setup

$(OBJPATH)p_sight:	c.p_sight h.doomdef h.i_system h.d_ticcmd h.doomtype h.d_event h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_sight c.p_sight

$(OBJPATH)p_spec:	c.p_spec h.doomdef h.doomstat h.doomdata h.doomtype h.d_net h.d_player h.d_items h.p_pspr h.m_fixed h.tables h.info h.d_think h.p_mobj h.d_ticcmd h.i_system h.d_event h.z_zone h.m_argv h.m_random h.m_swap h.m_bbox h.w_wad h.r_local h.r_data h.r_defs h.r_state h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_local h.p_spec h.p_inter h.g_game h.s_sound h.sounds 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_spec c.p_spec

$(OBJPATH)p_switch:	c.p_switch h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.g_game h.s_sound h.sounds h.doomstat h.d_net 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_switch c.p_switch

$(OBJPATH)p_telept:	c.p_telept h.doomdef h.s_sound h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.doomtype h.info h.r_state h.d_player h.d_items h.p_pspr h.d_ticcmd h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.sounds 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_telept c.p_telept

$(OBJPATH)p_tick:	c.p_tick h.z_zone h.p_local h.r_local h.tables h.m_fixed h.doomdef h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.doomtype h.info h.r_state h.d_player h.d_items h.p_pspr h.d_ticcmd h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.doomstat h.d_net 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_tick c.p_tick

$(OBJPATH)p_user:	c.p_user h.doomdef h.d_event h.doomtype h.p_local h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.d_ticcmd h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec h.doomstat h.d_net 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)p_user c.p_user

$(OBJPATH)ROsupport:	c.ROsupport h.ROsupport h.doomtype h.doomdef h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)ROsupport c.ROsupport

$(OBJPATH)r_bsp:	c.r_bsp h.doomdef h.m_bbox h.doomtype h.m_fixed h.i_system h.d_ticcmd h.d_event h.r_main h.d_player h.d_items h.p_pspr h.tables h.info h.d_think h.p_mobj h.doomdata h.r_data h.r_defs h.r_state h.r_plane h.r_segs h.r_things h.doomstat h.d_net h.r_draw h.r_context c.inl_pside 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)r_bsp c.r_bsp

$(OBJPATH)r_data:	c.r_data h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.z_zone h.m_swap h.w_wad h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_local h.p_spec h.doomstat h.d_net h.r_sky h.ROsupport h.GameSupp
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)r_data c.r_data

$(OBJPATH)r_draw:	c.r_draw h.doomdef h.i_system h.d_ticcmd h.doomtype h.d_event h.z_zone h.w_wad h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.r_context h.v_video h.doomstat h.d_net h.i_video h.ROsupport 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)r_draw c.r_draw

$(OBJPATH)r_main:	c.r_main h.ROsupport h.doomstat h.doomdata h.doomtype h.doomdef h.d_net h.d_player h.d_items h.p_pspr h.m_fixed h.tables h.info h.d_think h.p_mobj h.d_ticcmd h.m_bbox h.m_menu h.d_event h.r_local h.r_data h.r_defs h.r_state h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.r_sky h.r_context h.i_video h.p_spec h.z_zone 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)r_main c.r_main

$(OBJPATH)r_plane:	c.r_plane h.ROsupport h.doomtype h.doomdef h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.i_system h.d_ticcmd h.d_event h.i_video h.z_zone h.w_wad h.doomstat h.d_net h.d_player h.d_items h.p_pspr h.r_local h.r_data h.r_state h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.r_sky h.r_context 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)r_plane c.r_plane

$(OBJPATH)r_segs:	c.r_segs h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.doomstat h.doomdata h.d_net h.d_player h.d_items h.p_pspr h.m_fixed h.tables h.info h.d_think h.p_mobj h.r_local h.r_data h.r_defs h.r_state h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.r_context h.r_sky h.z_zone 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)r_segs c.r_segs

$(OBJPATH)r_sky:	c.r_sky h.m_fixed h.r_data h.r_defs h.doomdef h.d_think h.p_mobj h.tables h.doomdata h.doomtype h.info h.r_state h.d_player h.d_items h.p_pspr h.d_ticcmd h.r_sky 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)r_sky c.r_sky

$(OBJPATH)r_things:	c.r_things h.doomdef h.m_swap h.i_system h.d_ticcmd h.doomtype h.d_event h.i_video h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.z_zone h.w_wad h.r_local h.r_data h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.r_context h.st_stuff h.doomstat h.d_net h.ROsupport 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)r_things c.r_things

$(OBJPATH)sounds:	c.sounds h.doomtype h.doomdef h.sounds 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)sounds c.sounds

$(OBJPATH)st_lib:	c.st_lib h.doomdef h.z_zone h.v_video h.doomtype h.r_data h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.d_ticcmd h.m_swap h.i_system h.d_event h.w_wad h.st_stuff h.st_lib h.r_local h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw 
	$(CC) $(BASEFLAGS) $(SAFEOPT) -c -o $(OBJWRITE)st_lib c.st_lib

$(OBJPATH)st_stuff:	c.st_stuff h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.i_video h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.z_zone h.m_random h.w_wad h.hu_stuff h.g_game h.st_stuff h.st_lib h.r_local h.r_data h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_local h.p_spec h.p_inter h.am_map h.m_cheat h.s_sound h.v_video h.doomstat h.d_net h.dstrings h.d_englsh h.sounds 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)st_stuff c.st_stuff

$(OBJPATH)s_sound:	c.s_sound h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.i_sound h.doomstat h.doomdata h.d_net h.d_player h.d_items h.p_pspr h.m_fixed h.tables h.info h.d_think h.p_mobj h.sounds h.s_sound h.z_zone h.m_random h.w_wad h.p_local h.r_local h.r_data h.r_defs h.r_state h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.p_spec 
	-$(WIPE) $(OBJPATH)s_sound $(WIPEFLAGS)
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)s_sound c.s_sound
	-rename $(OBJPATH)o_sound $(OBJPATH)s_sound

$(OBJPATH)tables:	c.tables h.tables h.m_fixed 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)tables c.tables

$(OBJPATH)v_video:	c.v_video h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.r_local h.tables h.m_fixed h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.r_context h.m_bbox h.m_swap h.v_video h.i_video h.ROsupport 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)v_video c.v_video

$(OBJPATH)wi_stuff:	c.wi_stuff h.z_zone h.m_random h.doomtype h.doomdef h.m_swap h.m_fixed h.i_system h.d_ticcmd h.d_event h.w_wad h.g_game h.r_local h.tables h.r_data h.r_defs h.d_think h.p_mobj h.doomdata h.info h.r_state h.d_player h.d_items h.p_pspr h.r_main h.r_bsp h.r_segs h.r_plane h.r_things h.r_draw h.s_sound h.doomstat h.d_net h.sounds h.v_video h.wi_stuff h.i_video 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)wi_stuff c.wi_stuff

$(OBJPATH)w_wad:	c.w_wad h.ROsupport h.doomtype h.doomdef h.r_defs h.m_fixed h.d_think h.p_mobj h.tables h.doomdata h.info h.doomstat h.d_net h.d_player h.d_items h.p_pspr h.d_ticcmd h.m_swap h.i_system h.d_event h.z_zone h.w_wad h.p_spec 
	$(CC) $(CFLAGS) -I$(FASTLZDIR) -c -o $(OBJWRITE)w_wad c.w_wad

$(OBJPATH)z_zone:	c.z_zone h.z_zone h.i_system h.d_ticcmd h.doomtype h.doomdef h.d_event h.doomstat h.doomdata h.d_net h.d_player h.d_items h.p_pspr h.m_fixed h.tables h.info h.d_think h.p_mobj 
	$(CC) $(CFLAGS) -c -o $(OBJWRITE)z_zone c.z_zone






# True assembler files (only two left ;-) )
o.ROasm:	s.ROasm
	$(AS) $(ASFLAGS) -o o.ROasm s.ROasm

o.GameSupp:	s.GameSupp
	$(AS) $(ASFLAGS) -o o.GameSupp s.GameSupp


# Creating assembler files out of macro definitions
s.plot8:	m.plotterlib m.plotlib8 m.sharedass mpp Makefile
	mpp $(MPPFLAGS) $(ASCCFLAGS) -o s.plot8 m.plotlib8

s.plot8b:	m.plotterlib m.plotlib8 m.sharedass mpp Makefile
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYBOOM -o s.plot8b m.plotlib8

s.plot16:	m.plotterlib m.plotlib16 m.sharedass mpp Makefile
	mpp $(MPPFLAGS) $(ASCCFLAGS) -o s.plot16 m.plotlib16

s.plot16r:	m.plotterlib m.plotlib16 m.sharedass mpp Makefile
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYRESAMPLE -o s.plot16r m.plotlib16

s.plot16b:	m.plotterlib m.plotlib16 m.sharedass mpp Makefile
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYBOOM -o s.plot16b m.plotlib16

s.plot16rb:	m.plotterlib m.plotlib16 m.sharedass mpp Makefile
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYBOOM -DDIYRESAMPLE -o s.plot16rb m.plotlib16

s.plot32:	m.plotterlib m.plotlib32 m.sharedass mpp Makefile
	mpp $(MPPFLAGS) $(ASCCFLAGS) -o s.plot32 m.plotlib32

s.plot32r:	m.plotterlib m.plotlib32 m.sharedass mpp Makefile
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYRESAMPLE -o s.plot32r m.plotlib32

s.plot32b:	m.plotterlib m.plotlib32 m.sharedass mpp Makefile
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYBOOM -o s.plot32b m.plotlib32

s.plot32rb:	m.plotterlib m.plotlib32 m.sharedass mpp Makefile
	mpp $(MPPFLAGS) $(ASCCFLAGS) -DDIYBOOM -DDIYRESAMPLE -o s.plot32rb m.plotlib32

s.fixpoint:	m.fixpoint m.sharedass mpp Makefile
	mpp $(FIXPOINTFLAGS) -o s.fixpoint m.fixpoint

s.voices:	m.voices m.sharedass mpp Makefile
	mpp $(VOICEFLAGS) -o s.voices m.voices

s.ROasm:	m.ROasm m.sharedass mpp Makefile
	mpp $(MPPFLAGS) $(ASCCFLAGS) -o s.ROasm m.ROasm

s.GameSupp:	m.GameSupp m.sharedass mpp Makefile
	mpp $(MPPFLAGS) $(ASCCFLAGS) -o s.GameSupp m.GameSupp


# Temporary assembler files to object files
$(OBJPATH)plot8:	s.plot8
	$(AS) $(ASFLAGS) -o $(OBJWRITE)plot8 s.plot8

$(OBJPATH)plot8b:	s.plot8b
	$(AS) $(ASFLAGS) -o $(OBJWRITE)plot8b s.plot8b

$(OBJPATH)plot16:	s.plot16
	$(AS) $(ASFLAGS) -o $(OBJWRITE)plot16 s.plot16

$(OBJPATH)plot16r:	s.plot16r
	$(AS) $(ASFLAGS) -o $(OBJWRITE)plot16r s.plot16r

$(OBJPATH)plot16b:	s.plot16b
	$(AS) $(ASFLAGS) -o $(OBJWRITE)plot16b s.plot16b

$(OBJPATH)plot16rb:	s.plot16rb
	$(AS) $(ASFLAGS) -o $(OBJWRITE)plot16rb s.plot16rb

$(OBJPATH)plot32:	s.plot32
	$(AS) $(ASFLAGS) -o $(OBJWRITE)plot32 s.plot32

$(OBJPATH)plot32r:	s.plot32r
	$(AS) $(ASFLAGS) -o $(OBJWRITE)plot32r s.plot32r

$(OBJPATH)plot32b:	s.plot32b
	$(AS) $(ASFLAGS) -o $(OBJWRITE)plot32b s.plot32b

$(OBJPATH)plot32rb:	s.plot32rb
	$(AS) $(ASFLAGS) -o $(OBJWRITE)plot32rb s.plot32rb

o.fixpoint:	s.fixpoint
	$(AS) $(ASFLAGS) -o o.fixpoint s.fixpoint

o.voices:	s.voices
	$(AS) $(ASFLAGS) -o o.voices s.voices

