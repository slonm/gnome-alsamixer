#!/bin/sh
# Run this to generate all the initial makefiles, etc.

PROJECT=gnome-alsamixer

srcdir=`dirname $0`
test #! /bin/sh

export WANT_AUTOMAKE_1_6=1

rm -rf autom4te.cache

aclocal || exit 1

glib-gettextize --force || exit $?

autoheader || exit $?

automake --gnu --add-missing || exit $?

autoconf || exit $?

./configure --enable-maintainer-mode --enable-compile-warnings "$@" || exit $?

echo 
echo "Now type 'make' to compile $PROJECT."
