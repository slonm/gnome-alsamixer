dnl Process this file with autoconf to produce a configure script.

AC_INIT(gnome-alsamixer, 0.9.7)
AC_CONFIG_SRCDIR(src/gam-main.c)
AM_INIT_AUTOMAKE(dist-bzip2)
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_HEADER_STDC
AC_C_CONST
AC_CHECK_FUNCS([rint])

AC_ARG_ENABLE(debug, [  --enable-debug=[no/yes] turn on debugging [default=no]],,enable_debug=no)

if test "x$enable_debug" = "xyes"; then
  AC_DEFINE_UNQUOTED(DEBUG, , "")
else
  AC_DEFINE_UNQUOTED(G_DISABLE_ASSERT, , "")
  AC_DEFINE_UNQUOTED(G_DISABLE_CHECKS, , "")
  AC_DEFINE_UNQUOTED(G_DISABLE_CAST_CHECKS, , "")
fi

GNOME_COMPILE_WARNINGS

PKG_CHECK_MODULES(DBUS_GTK, gtk+-2.0 >= 2.6.0, have_gtk26=yes, have_gtk26=no)

if test x$have_gtk26 = xyes ; then
    AC_DEFINE_UNQUOTED(HAVE_GTK26, , "")
fi

pkg_modules="libgnomeui-2.0 gconf-2.0 alsa"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

AM_GCONF_SOURCE_2

GETTEXT_PACKAGE=gnome-alsamixer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", "")
ALL_LINGUAS="az ca cs de dz el en_CA en_GB es eu fi fr gl hr hu ja mn mk nb ne nl pa pt pt_BR sk sq sr sr@Latn sv th uk vi zh_CN zh_TW"
AM_GLIB_GNU_GETTEXT

AC_CONFIG_FILES([Makefile
                 gnome-alsamixer.spec
                 po/Makefile.in
                 src/Makefile])

AC_OUTPUT
