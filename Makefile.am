## Process this file with automake to produce Makefile.in

SUBDIRS = po src

EXTRA_DIST = autogen.sh gnome-alsamixer.schemas

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_DATA = gnome-alsamixer.schemas

install-data-local:
	@$(NORMAL_INSTALL)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	gconftool-2 --makefile-install-rule $(srcdir)/$(schema_DATA)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	    fi \
	  done \
	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi
	for spec in *.spec; do \
	  if test -f $$spec; then \
	    cp -p $$spec $(distdir); \
	  fi \
	done
