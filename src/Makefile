################################################################
#
# Makefile for UNIX (Sun)
#

CC	= cc
#PROFILE	= -p -xprofile=tcov
OPTIONS	= -DFULL_NEW_FEATURES -DMAXSCREENWIDTH=1280 -DMAXSCREENHEIGHT=800
CFLAGS	= -DNORMALUNIX -DDIYINLINE $(BOOMFLAGS) $(COMPFLAGS) $(OPTIONS) $(PROFILE)
FASTLZDIR = fastlz
LDFLAGS	= 
LIBS	= 
LINBASEFLAGS = -DLINUX -DDIYNOSHORT -DVIDMODE_FULLSCREEN -DHAVE_LIBPNG -I/usr/X11R6/include
LINBASELINK = -L/usr/X11R6/lib -lX11 -lXext -lICE -lXmu -lXxf86vm -lpng -lz
BASELIBS = -L$(FASTLZDIR) -lfastlz
OBJPATH	= objects
DBGFLAG	= 

LD_PIXEL_DEPTH = 3
DEPTH_ID = 8


SUNOPT = -xO4

SUNFLAGS = COMPFLAGS='$(SUNOPT) -D__BIG_ENDIAN__ -DDIYNOSHORT -I/usr/demo/SOUND/include \
	-I/usr/local/DIR/X11/R6/include $(DBGFLAG) $$(SPECIALFLAGS)' \
	LDFLAGS='-L/usr/local/DIR/X11/R6/lib -lX11 -lXext -lICE -lXmu' \
	LIBS='$(BASELIBS) -lXext -lICE -lsocket -lnsl -L/usr/demo/SOUND/lib -laudio -lm'

HPFLAGS = COMPFLAGS='+O3 +Onolimit +DA2.0 +DS2.0 +Ofastaccess +e -Aa -D__BIG_ENDIAN__ -D_HPUX_SOURCE -I/usr/include -I/opt/audio/include $(DBGFLAG) $$(SPECIALFLAGS)' \
	LDFLAGS='-L/usr/lib/X11R6 -L/usr/contrib/X11R6/lib -L/opt/audio/lib -lX11 -lXext -lICE -lXmu -lAlib -lAt' \
	LIBS='$(BASELIBS) -lm'

LINUXFLAGS = CC=gcc COMPFLAGS='-O3 -fwritable-strings $(LINBASEFLAGS) $(DBGFLAG) $$(SPECIALFLAGS)' \
	LDFLAGS='$(LINBASELINK)' LIBS='$(BASELIBS) -lm'

INTELFLAGS = CC=icc COMPFLAGS='-O3 -mp -tpp6 -xiMK -ip -ansi -Xc $(LINBASEFLAGS) $(DBGFLAG) $$(SPECIALFLAGS)' \
	LDFLAGS='$(LINBASELINK)' LIBS='$(BASELIBS) -lm'



# Flags shared by many engines
DIYFLAGSR	= -DDIYRESAMPLE -DDIYRESAMPLETHINGS
DIYFLAGS16T	= -DLD_PIXEL_DEPTH=4 -DDIYTRANSPARENCY
DIYFLAGS16R	= -DLD_PIXEL_DEPTH=4 $(DIYFLAGSR)
DIYFLAGS16RT	= -DLD_PIXEL_DEPTH=4 -DDIYTRANSPARENCY $(DIYFLAGSR)
DIYFLAGS32T	= -DLD_PIXEL_DEPTH=5 -DDIYTRANSPARENCY
DIYFLAGS32R	= -DLD_PIXEL_DEPTH=5 $(DIYFLAGSR)
DIYFLAGS32RT	= -DLD_PIXEL_DEPTH=5 -DDIYTRANSPARENCY $(DIYFLAGSR)



all:
	# You must select a target for your platform.
	# Handles for currently supported platforms are:
	# hp: PA-RISC (HPUX), very basic support
	# sun: Ultra Spark (Solaris)
	# linux: x86 (Linux) with gcc
	# intel: x86 (Linux) with icc (Intel compiler)
	# Target names have the form <platform>[b]<depth>[r][t] where b enables Boom,
	# r enables resampling and t enables transparency. Not all combinations are
	# present in this Makefile (look under ``Available engine targets'').
	# Examples: linux8, sun32rt, intelb32t



$(OBJPATH):
	-mkdir $(OBJPATH)


# not too sophisticated dependency
OBJS=	$(OBJPATH)doomdef.o \
	$(OBJPATH)doomstat.o \
	$(OBJPATH)dstrings.o \
	$(OBJPATH)i_system.o \
	$(OBJPATH)i_main.o \
	$(OBJPATH)i_video.o \
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
	$(OBJPATH)m_swap.o \
	$(OBJPATH)m_cheat.o \
	$(OBJPATH)m_random.o \
	$(OBJPATH)am_map.o \
	$(OBJPATH)p_ceilng.o \
	$(OBJPATH)p_doors.o \
	$(OBJPATH)p_enemy.o \
	$(OBJPATH)p_floor.o \
	$(OBJPATH)p_inter.o \
	$(OBJPATH)p_genlin.o \
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
	$(OBJPATH)i_net.o \
	$(OBJPATH)i_sound.o \
	$(OBJPATH)simpleAudio.o \
	$(OBJPATH)hp_music.o



clean:
	-rm *.o

clean_sun:
	-rm -r -f objects_sun*

clean_hp:
	-rm -r -f objects_hp*

clean_linux:
	-rm -r -f objects_linux*

clean_intel:
	-rm -r -f objects_intel*


libfastlz:
	cd $(FASTLZDIR) && make CC=$(CC) CFLAGS="$(COMPFLAGS)" libfastlz.a



###
### Available engine targets
###

sun8:
	make $(SUNFLAGS) OBJPATH=objects_sun8/ doomengine
	mv doomengine sundiy8

sun16:
	make $(SUNFLAGS) OBJPATH=objects_sun16/ SPECIALFLAGS='-DLD_PIXEL_DEPTH=4' doomengine
	mv doomengine sundiy16

sun32:
	make $(SUNFLAGS) OBJPATH=objects_sun32/ SPECIALFLAGS='-DLD_PIXEL_DEPTH=5' doomengine
	mv doomengine sundiy32

sun32r:
	make $(SUNFLAGS) OBJPATH=objects_sun32r/ SPECIALFLAGS='$(DIYFLAGS32R)' doomengine
	mv doomengine sundiy32r

sun32t:
	make $(SUNFLAGS) OBJPATH=objects_sun32t/ SPECIALFLAGS='$(DIYFLAGS32T)' doomengine
	mv doomengine sundiy32t

sun32rt:
	make $(SUNFLAGS) OBJPATH=objects_sun32rt/ SPECIALFLAGS='$(DIYFLAGS32RT)' doomengine
	mv doomengine sundiy32rt


hp8:
	make $(HPFLAGS) OBJPATH=objects_hp8/ doomengine
	mv doomengine hpdiy8

hp16:
	make $(HPFLAGS) OBJPATH=objects_hp16/ SPECIALFLAGS='-DLD_PIXEL_DEPTH=4' doomengine
	mv doomengine hpdiy16

hp32:
	make $(HPFLAGS) OBJPATH=objects_hp32/ SPECIALFLAGS='-DLD_PIXEL_DEPTH=5' doomengine
	mv doomengine hpdiy32

hp32r:
	make $(HPFLAGS) OBJPATH=objects_hp32r/ SPECIALFLAGS='$(DIYFLAGS32R)' doomengine
	mv doomengine hpdiy32r


linux8:
	make $(LINUXFLAGS) OBJPATH=objects_linux8/ doomengine
	mv doomengine linuxdiy8

linux16:
	make $(LINUXFLAGS) OBJPATH=objects_linux16/ SPECIALFLAGS='-DLD_PIXEL_DEPTH=4' doomengine
	mv doomengine linuxdiy16

linux16t:
	make $(LINUXFLAGS) OBJPATH=objects_linux16t/ SPECIALFLAGS='$(DIYFLAGS16T)' doomengine
	mv doomengine linuxdiy16t

linux16rt:
	make $(LINUXFLAGS) OBJPATH=objects_linux16rt/ SPECIALFLAGS='$(DIYFLAGS16RT)' doomengine
	mv doomengine linuxdiy16rt

linux32:
	make $(LINUXFLAGS) OBJPATH=objects_linux32/ SPECIALFLAGS='-DLD_PIXEL_DEPTH=5' doomengine
	mv doomengine linuxdiy32

linux32r:
	make $(LINUXFLAGS) OBJPATH=objects_linux32r/ SPECIALFLAGS='$(DIYFLAGS32R)' doomengine
	mv doomengine linuxdiy32r

linux32t:
	make $(LINUXFLAGS) OBJPATH=objects_linux32t/ SPECIALFLAGS='$(DIYFLAGS32T)' doomengine
	mv doomengine linuxdiy32t

linux32rt:
	make $(LINUXFLAGS) OBJPATH=objects_linux32rt/ SPECIALFLAGS='$(DIYFLAGS32RT)' doomengine
	mv doomengine linuxdiy32rt

linuxb32t:
	make $(LINUXFLAGS) OBJPATH=objects_linuxb32r/ SPECIALFLAGS='-DDIYBOOM $(DIYFLAGS32T)' doomengine
	mv doomengine linuxboom32t

linuxb32rt:
	make $(LINUXFLAGS) OBJPATH=objects_linuxb32rt/ SPECIALFLAGS='-DDIYBOOM $(DIYFLAGS32RT)' doomengine
	mv doomengine linuxboom32rt

intel8:
	make $(INTELFLAGS) OBJPATH=objects_intel8/ SPECIALFLAGS="-DLD_PIXEL_DEPTH=3" doomengine
	mv doomengine inteldiy8

intel32t:
	make $(INTELFLAGS) OBJPATH=objects_intel32t/ SPECIALFLAGS='$(DIYFLAGS32T)' doomengine
	mv doomengine inteldiy32t

intel32r:
	make $(INTELFLAGS) OBJPATH=objects_intel32r/ SPECIALFLAGS='$(DIYFLAGS32R)' doomengine
	mv doomengine inteldiy32r

intel32rt:
	make $(INTELFLAGS) OBJPATH=objects_intel32rt/ SPECIALFLAGS='$(DIYFLAGS32RT)' doomengine
	mv doomengine inteldiy32rt

intelb32t:
	make $(INTELFLAGS) OBJPATH=objects_intelb32r/ SPECIALFLAGS='-DDIYBOOM $(DIYFLAGS32T)' doomengine
	mv doomengine intelboom32t

intelb32rt:
	make $(INTELFLAGS) OBJPATH=objects_intelb32rt/ SPECIALFLAGS='-DDIYBOOM $(DIYFLAGS32RT)' doomengine
	mv doomengine intelboom32rt




doomengine:	$(OBJPATH) $(OBJS) libfastlz
	$(PURIFY) $(CC) $(PROFILE) $(LIBS) $(OBJS) -o $@ $(LDFLAGS) $(LIBS)






# Dependencies (auto-generated 1 Mar 98)

$(OBJPATH)am_map.o:	am_map.c \
	am_map.h d_englsh.h d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h dstrings.h i_system.h i_video.h info.h m_cheat.h m_fixed.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h st_stuff.h tables.h v_video.h w_wad.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)am_map.o am_map.c

$(OBJPATH)doomdef.o:	doomdef.c \
	doomdef.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)doomdef.o doomdef.c

$(OBJPATH)doomstat.o:	doomstat.c \
	d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h info.h m_fixed.h p_mobj.h p_pspr.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)doomstat.o doomstat.c

$(OBJPATH)dstrings.o:	dstrings.c \
	d_englsh.h dstrings.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)dstrings.o dstrings.c

$(OBJPATH)d_items.o:	d_items.c \
	d_items.h d_think.h doomdef.h info.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)d_items.o d_items.c

$(OBJPATH)d_main.o:	d_main.c \
	am_map.h d_englsh.h d_event.h d_items.h d_main.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h dstrings.h f_finale.h f_wipe.h g_game.h hu_stuff.h i_sound.h i_system.h i_video.h info.h m_argv.h m_fixed.h m_menu.h m_misc.h p_mobj.h p_pspr.h p_setup.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h  s_sound.h sounds.h st_stuff.h tables.h v_video.h w_wad.h wi_stuff.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)d_main.o d_main.c

$(OBJPATH)d_net.o:	d_net.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h g_game.h i_net.h i_system.h i_video.h info.h m_fixed.h m_menu.h p_mobj.h p_pspr.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)d_net.o d_net.c

$(OBJPATH)f_finale.o:	f_finale.c \
	d_englsh.h d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h dstrings.h hu_stuff.h i_system.h info.h m_fixed.h m_swap.h p_mobj.h p_pspr.h r_data.h r_defs.h r_state.h s_sound.h sounds.h tables.h v_video.h w_wad.h z_zone.h i_video.h
	$(CC) $(CFLAGS) -c -o $(OBJPATH)f_finale.o f_finale.c

$(OBJPATH)f_wipe.o:	f_wipe.c \
	d_items.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomtype.h f_wipe.h i_video.h info.h m_fixed.h m_random.h p_mobj.h p_pspr.h r_data.h r_defs.h r_state.h tables.h v_video.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)f_wipe.o f_wipe.c

$(OBJPATH)g_game.o:	g_game.c \
	am_map.h d_englsh.h d_event.h d_items.h d_main.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h dstrings.h f_finale.h g_game.h hu_stuff.h i_system.h info.h m_argv.h m_fixed.h m_menu.h m_misc.h m_random.h p_local.h p_mobj.h p_pspr.h p_saveg.h p_setup.h p_spec.h p_tick.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_sky.h r_state.h r_things.h s_sound.h sounds.h st_stuff.h tables.h v_video.h w_wad.h wi_stuff.h z_zone.h inl_pside.c inl_psub.c
	$(CC) $(CFLAGS) -c -o $(OBJPATH)g_game.o g_game.c

$(OBJPATH)hu_lib.o:	hu_lib.c \
	d_items.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomtype.h hu_lib.h info.h m_fixed.h m_swap.h p_mobj.h p_pspr.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h tables.h v_video.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)hu_lib.o hu_lib.c

$(OBJPATH)hu_stuff.o:	hu_stuff.c \
	d_englsh.h d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h dstrings.h hu_lib.h hu_stuff.h info.h m_fixed.h m_swap.h p_mobj.h p_pspr.h r_defs.h s_sound.h sounds.h tables.h w_wad.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)hu_stuff.o hu_stuff.c

$(OBJPATH)info.o:	info.c \
	d_think.h doomdata.h doomdef.h doomtype.h info.h m_fixed.h p_mobj.h sounds.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)info.o info.c

$(OBJPATH)i_main.o:	i_main.c \
	d_event.h d_main.h doomdef.h doomtype.h m_argv.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)i_main.o i_main.c

$(OBJPATH)i_net.o:	i_net.c \
	i_net.h d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_system.h info.h m_argv.h m_fixed.h p_mobj.h p_pspr.h tables.h 
	$(CC) $(CFLAGS) $(NETINC) -c -o $(OBJPATH)i_net.o i_net.c

$(OBJPATH)i_sound.o:	i_sound.c \
	 d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_sound.h i_system.h info.h m_argv.h m_fixed.h m_misc.h p_mobj.h p_pspr.h sounds.h tables.h w_wad.h z_zone.h
	$(CC) $(CFLAGS) -c -o $(OBJPATH)i_sound.o i_sound.c

$(OBJPATH)simpleAudio.o:	simpleAudio.c simpleAudio.h
	$(CC) $(CFLAGS) -I/opt/audio/include -c -o $(OBJPATH)simpleAudio.o simpleAudio.c

$(OBJPATH)hp_music.o:	hp_music.c hp_music.h
	$(CC) $(CFLAGS) -c -o $(OBJPATH)hp_music.o hp_music.c

$(OBJPATH)i_system.o:	i_system.c \
	 d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h g_game.h i_sound.h i_system.h i_video.h i_net.h info.h m_fixed.h m_misc.h p_mobj.h p_pspr.h sounds.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)i_system.o i_system.c

$(OBJPATH)i_video.o:	i_video.c \
	 d_event.h d_items.h d_main.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_sound.h i_system.h i_video.h info.h m_argv.h m_fixed.h p_mobj.h p_pspr.h r_data.h r_defs.h r_draw.h r_state.h sounds.h tables.h v_video.h r_main.h z_zone.h w_wad.h m_menu.h
	$(CC) $(CFLAGS) -c -o $(OBJPATH)i_video.o i_video.c

$(OBJPATH)m_argv.o:	m_argv.c \
	 doomtype.h m_fixed.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)m_argv.o m_argv.c

$(OBJPATH)m_bbox.o:	m_bbox.c \
	doomtype.h m_bbox.h m_fixed.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)m_bbox.o m_bbox.c

$(OBJPATH)m_cheat.o:	m_cheat.c \
	m_cheat.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)m_cheat.o m_cheat.c

$(OBJPATH)m_fixed.o:	m_fixed.c \
	d_event.h d_ticcmd.h doomtype.h i_system.h m_fixed.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)m_fixed.o m_fixed.c

$(OBJPATH)m_menu.o:	m_menu.c \
	d_englsh.h d_event.h d_items.h d_main.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h dstrings.h g_game.h hu_stuff.h i_system.h i_video.h info.h m_argv.h m_fixed.h m_menu.h m_swap.h p_mobj.h p_pspr.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h v_video.h w_wad.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)m_menu.o m_menu.c

$(OBJPATH)m_misc.o:	m_misc.c \
	 d_englsh.h d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h dstrings.h hu_stuff.h i_system.h i_video.h info.h m_argv.h m_fixed.h m_misc.h m_swap.h p_mobj.h p_pspr.h r_data.h r_defs.h r_state.h tables.h v_video.h w_wad.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)m_misc.o m_misc.c

$(OBJPATH)m_random.o:	m_random.c
	$(CC) $(CFLAGS) -c -o $(OBJPATH)m_random.o m_random.c

$(OBJPATH)m_swap.o:	m_swap.c \
	m_swap.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)m_swap.o m_swap.c

$(OBJPATH)p_ceilng.o:	p_ceilng.c \
	d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h info.h m_fixed.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_ceilng.o p_ceilng.c

$(OBJPATH)p_doors.o:	p_doors.c \
	d_englsh.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h dstrings.h info.h m_fixed.h p_inter.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_doors.o p_doors.c

$(OBJPATH)p_enemy.o:	p_enemy.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h g_game.h i_system.h info.h m_fixed.h m_random.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h inl_btitr.c
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_enemy.o p_enemy.c

$(OBJPATH)p_floor.o:	p_floor.c \
	d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h info.h m_fixed.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_floor.o p_floor.c

$(OBJPATH)p_genlin.o:	p_genlin.c p_spec.h
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_genlin.o p_genlin.c

$(OBJPATH)p_inter.o:	p_inter.c \
	am_map.h d_englsh.h d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h dstrings.h i_system.h info.h m_fixed.h m_random.h p_inter.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_inter.o p_inter.c

$(OBJPATH)p_lights.o:	p_lights.c \
	d_items.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomtype.h info.h m_fixed.h m_random.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h tables.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_lights.o p_lights.c

$(OBJPATH)p_map.o:	p_map.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_system.h info.h m_bbox.h m_fixed.h m_random.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h inl_blitr.c inl_btitr.c inl_pside.c inl_psub.c
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_map.o p_map.c

$(OBJPATH)p_maputl.o:	p_maputl.c \
	d_items.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomtype.h info.h m_bbox.h m_fixed.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h tables.h inl_blitr.c inl_btitr.c inl_pside.c inl_psub.c
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_maputl.o p_maputl.c

$(OBJPATH)p_mobj.o:	p_mobj.c \
	 d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h hu_stuff.h i_system.h info.h m_fixed.h m_random.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h st_stuff.h tables.h z_zone.h p_setup.h inl_pside.c inl_psub.c
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_mobj.o p_mobj.c

$(OBJPATH)p_plats.o:	p_plats.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_system.h info.h m_fixed.h m_random.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_plats.o p_plats.c

$(OBJPATH)p_pspr.o:	p_pspr.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h info.h m_fixed.h m_random.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_pspr.o p_pspr.c

$(OBJPATH)p_saveg.o:	p_saveg.c \
	p_saveg.h d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_system.h info.h m_fixed.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h tables.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_saveg.o p_saveg.c

$(OBJPATH)p_saven.o:	p_saven.c \
	p_saveg.h p_saven.h d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_system.h info.h m_fixed.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h tables.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_saven.o p_saven.c

$(OBJPATH)p_setup.o:	p_setup.c \
	 d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h g_game.h i_system.h info.h m_bbox.h m_fixed.h m_swap.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h tables.h w_wad.h z_zone.h p_setup.h
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_setup.o p_setup.c

$(OBJPATH)p_sight.o:	p_sight.c \
	d_event.h d_items.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomtype.h i_system.h info.h m_fixed.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_sight.o p_sight.c

$(OBJPATH)p_spec.o:	p_spec.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h g_game.h i_system.h info.h m_argv.h m_fixed.h m_random.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h w_wad.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_spec.o p_spec.c

$(OBJPATH)p_switch.o:	p_switch.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h g_game.h i_system.h info.h m_fixed.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_switch.o p_switch.c

$(OBJPATH)p_telept.o:	p_telept.c \
	d_items.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomtype.h info.h m_fixed.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_telept.o p_telept.c

$(OBJPATH)p_tick.o:	p_tick.c \
	d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h info.h m_fixed.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h tables.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_tick.o p_tick.c

$(OBJPATH)p_user.o:	p_user.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h info.h m_fixed.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)p_user.o p_user.c

$(OBJPATH)r_bsp.o:	r_bsp.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_system.h info.h m_bbox.h m_fixed.h p_mobj.h p_pspr.h r_data.h r_defs.h r_main.h r_plane.h r_state.h r_things.h tables.h inl_pside.c
	$(CC) $(CFLAGS) -c -o $(OBJPATH)r_bsp.o r_bsp.c

$(OBJPATH)r_data.o:	r_data.c \
	 d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_system.h info.h m_fixed.h m_swap.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_sky.h r_state.h r_things.h tables.h w_wad.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)r_data.o r_data.c

$(OBJPATH)r_draw.o:	r_draw.c \
	 d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_system.h info.h m_fixed.h p_mobj.h p_pspr.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h tables.h v_video.h w_wad.h z_zone.h i_video.h
	$(CC) $(CFLAGS) -c -o $(OBJPATH)r_draw.o r_draw.c

$(OBJPATH)r_main.o:	r_main.c \
	d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomtype.h info.h m_bbox.h m_fixed.h p_mobj.h p_pspr.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_sky.h r_state.h r_things.h tables.h inl_pside.c inl_psub.c
	$(CC) $(CFLAGS) -c -o $(OBJPATH)r_main.o r_main.c

$(OBJPATH)r_plane.o:	r_plane.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_system.h info.h m_fixed.h p_mobj.h p_pspr.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_sky.h r_state.h r_things.h tables.h w_wad.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)r_plane.o r_plane.c

$(OBJPATH)r_segs.o:	r_segs.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_system.h info.h m_fixed.h p_mobj.h p_pspr.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_sky.h r_state.h r_things.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)r_segs.o r_segs.c

$(OBJPATH)r_sky.o:	r_sky.c \
	d_items.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomtype.h info.h m_fixed.h p_mobj.h p_pspr.h r_data.h r_defs.h r_sky.h r_state.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)r_sky.o r_sky.c

$(OBJPATH)r_things.o:	r_things.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_system.h info.h m_fixed.h m_swap.h p_mobj.h p_pspr.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h tables.h w_wad.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)r_things.o r_things.c

$(OBJPATH)sounds.o:	sounds.c \
	doomtype.h sounds.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)sounds.o sounds.c

$(OBJPATH)st_lib.o:	st_lib.c \
	d_event.h d_items.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomtype.h i_system.h info.h m_fixed.h m_swap.h p_mobj.h p_pspr.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h st_lib.h st_stuff.h tables.h v_video.h w_wad.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)st_lib.o st_lib.c

$(OBJPATH)st_stuff.o:	st_stuff.c \
	am_map.h d_englsh.h d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h dstrings.h g_game.h i_system.h i_video.h info.h m_cheat.h m_fixed.h m_random.h p_inter.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h st_lib.h st_stuff.h tables.h v_video.h w_wad.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)st_stuff.o st_stuff.c

$(OBJPATH)s_sound.o:	s_sound.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h i_sound.h i_system.h info.h m_fixed.h m_random.h p_local.h p_mobj.h p_pspr.h p_spec.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h w_wad.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)s_sound.o s_sound.c

$(OBJPATH)tables.o:	tables.c \
	m_fixed.h tables.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)tables.o tables.c

$(OBJPATH)v_video.o:	v_video.c \
	d_event.h d_items.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomtype.h i_system.h info.h m_bbox.h m_fixed.h m_swap.h p_mobj.h p_pspr.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h tables.h v_video.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)v_video.o v_video.c

$(OBJPATH)wi_stuff.o:	wi_stuff.c \
	d_event.h d_items.h d_net.h d_player.h d_think.h d_ticcmd.h doomdata.h doomdef.h doomstat.h doomtype.h g_game.h i_system.h info.h m_fixed.h m_random.h m_swap.h p_mobj.h p_pspr.h r_bsp.h r_data.h r_defs.h r_draw.h r_local.h r_main.h r_plane.h r_segs.h r_state.h r_things.h s_sound.h sounds.h tables.h v_video.h w_wad.h wi_stuff.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)wi_stuff.o wi_stuff.c

$(OBJPATH)w_wad.o:	w_wad.c \
	 d_event.h d_ticcmd.h doomtype.h i_system.h m_fixed.h m_swap.h w_wad.h z_zone.h 
	$(CC) $(CFLAGS) -I$(FASTLZDIR) -c -o $(OBJPATH)w_wad.o w_wad.c

$(OBJPATH)z_zone.o:	z_zone.c \
	d_event.h d_ticcmd.h doomdef.h doomtype.h i_system.h z_zone.h 
	$(CC) $(CFLAGS) -c -o $(OBJPATH)z_zone.o z_zone.c

$(OBJPATH)dehacked.o:	dehacked.c \
	d_main.h p_inter.h hu_stuff.h d_player.h d_items.h p_pspr.h p_mobj.h d_ticcmd.h doomdef.h m_fixed.h tables.h info.h d_think.h doomdata.h doomtype.h f_finale.h st_stuff.h st_lib.h d_event.h r_defs.h g_game.h p_spec.h sounds.h wi_stuff.h p_local.h p_pspr.h am_map.h doomstat.h
	$(CC) $(CFLAGS) -c -o $(OBJPATH)dehacked.o dehacked.c

