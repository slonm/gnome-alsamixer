## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)/gnome-alsamixer"\"        \
	-DPIXMAP_ICONDIR=\""$(datadir)/pixmaps/gnome-alsamixer"\"  \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DG_DISABLE_DEPRECATED					   \
	-DGDK_DISABLE_DEPRECATED				   \
	-DGTK_DISABLE_DEPRECATED				   \
	-DGNOME_DISABLE_DEPRECATED                                 \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = gnome-alsamixer

gnome_alsamixer_SOURCES = \
	gam-main.c \
	gam-app.c gam-app.h \
	gam-mixer.c gam-mixer.h \
	gam-slider.c gam-slider.h \
	gam-slider-pan.c gam-slider-pan.h \
	gam-slider-dual.c gam-slider-dual.h \
	gam-toggle.c gam-toggle.h \
	gam-enum.c gam-enum.h \
	gam-prefs-dlg.c gam-prefs-dlg.h \
	gam-props-dlg.c gam-props-dlg.h

gnome_alsamixer_LDADD = @PACKAGE_LIBS@
