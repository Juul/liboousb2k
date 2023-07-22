#!/bin/sh

NEED_LTDL=no

#--------------------------------------------------------------------------
# autoconf 2.50 or newer
#
ac_version=`${AUTOCONF:-autoconf} --version 2>/dev/null|head -n1| sed -e 's/^[^0-9]*//' -e 's/[a-z]* *$//'`
if test -z "$ac_version"; then
  echo "autoconf not found."
  echo "You need autoconf version 2.50 or newer installed."
  exit 1
fi
IFS=.; set $ac_version; IFS=' '
if test "$1" = "2" -a "$2" -lt "50" || test "$1" -lt "2"; then
  echo "autoconf version $ac_version found."
  echo "You need autoconf version 2.50 or newer installed."
  echo "If you have a sufficient autoconf installed, but it"
  echo "is not named 'autoconf', then try setting the"
  echo "AUTOCONF environment variable.  (See the INSTALL file"
  echo "for details.)"
  exit 1
fi

echo "autoconf version $ac_version (ok)"

#--------------------------------------------------------------------------
# autoheader 2.50 or newer
#
ah_version=`${AUTOHEADER:-autoheader} --version 2>/dev/null|head -n1| sed -e 's/^[^0-9]*//' -e 's/[a-z]* *$//'`
if test -z "$ah_version"; then
  echo "autoheader not found."
  echo "You need autoheader version 2.50 or newer installed."
  exit 1
fi
IFS=.; set $ah_version; IFS=' '
if test "$1" = "2" -a "$2" -lt "50" || test "$1" -lt "2"; then
  echo "autoheader version $ah_version found."
  echo "You need autoheader version 2.50 or newer installed."
  echo "If you have a sufficient autoheader installed, but it"
  echo "is not named 'autoheader', then try setting the"
  echo "AUTOHEADER environment variable.  (See the INSTALL file"
  echo "for details.)"
  exit 1
fi

echo "autoheader version $ah_version (ok)"

#--------------------------------------------------------------------------
# automake 1.7 or newer
#
am_version=`${AUTOMAKE:-automake} --version 2>/dev/null|head -n1| sed -e 's/^[^0-9]*//' -e 's/[a-z]* *$//'`
if test -z "$am_version"; then
  echo "automake not found."
  echo "You need automake version 1.7 or newer installed."
  exit 1
fi
IFS=.; set $am_version; IFS=' '
if test "$1" = "1" -a "$2" -lt "7" || test "$1" -lt "1"; then
  echo "automake version $am_version found."
  echo "You need automake version 2.50 or newer installed."
  echo "If you have a sufficient automake installed, but it"
  echo "is not named 'automake', then try setting the"
  echo "AUTOMAKE environment variable.  (See the INSTALL file"
  echo "for details.)"
  exit 1
fi

echo "automake version $am_version (ok)"

#--------------------------------------------------------------------------
# aclocal 1.7 or newer
#
acl_version=`${ACLOCAL:-aclocal} --version 2>/dev/null|head -n1| sed -e 's/^[^0-9]*//' -e 's/[a-z]* *$//'`
if test -z "$acl_version"; then
  echo "aclocal not found."
  echo "You need aclocal version 1.7 or newer installed."
  exit 1
fi
IFS=.; set $acl_version; IFS=' '
if test "$1" = "1" -a "$2" -lt "7" || test "$1" -lt "1"; then
  echo "aclocal version $acl_version found."
  echo "You need aclocal version 2.50 or newer installed."
  echo "If you have a sufficient aclocal installed, but it"
  echo "is not named 'aclocal', then try setting the"
  echo "ACLOCAL environment variable.  (See the INSTALL file"
  echo "for details.)"
  exit 1
fi

echo "aclocal version $acl_version (ok)"

#--------------------------------------------------------------------------
# libtool 1.4 or newer
#
libtool=`which glibtool 2>/dev/null`
if test ! -x "$libtool"; then
  libtool=`which libtool`
fi
lt_pversion=`$libtool --version 2>/dev/null|sed -e 's/^[^0-9]*//' -e 's/[- ].*//'`
if test -z "$lt_pversion"; then
  echo "libtool not found."
  echo "You need libtool version 1.4 or newer installed"
  exit 1
fi
lt_version=`echo $lt_pversion|sed -e 's/\([a-z]*\)$/.\1/'`
IFS=.; set $lt_version; IFS=' '
lt_status="good"
if test "$1" = "1"; then
   if test "$2" -lt "4"; then
      lt_status="bad"
   fi
fi
if test $lt_status != "good"; then
  echo "libtool version $lt_pversion found."
  echo "You need libtool version 1.4 or newer installed"
  exit 1
fi

echo "libtool version $lt_pversion (ok)"

#--------------------------------------------------------------------------
echo
echo
echo
#--------------------------------------------------------------------------
# check for macro paths
test -d "macros" && MACROS="-I macros"
test -n "$GNOME" || test -d $GNOME/share/aclocal && MACROS="$MACROS -I $GNOME/share/aclocal"

#--------------------------------------------------------------------------
# generate files
#

# remove previous configs
rm -f aclocal.m4
rm -f config.log
rm -f config.guess
rm -f config.sub
rm -f ltmain.sh
rm -rf autom4te.cache
rm -rf libltdl

# generate aclocal.m4
${ACLOCAL:-aclocal} $MACROS

# run libtoolize (check if we need to create libltdl)
if test x$NEED_LTDL = xyes; then
    ${LIBTOOLIZE:-libtoolize} --ltdl
else
    ${LIBTOOLIZE:-libtoolize}
fi

# update aclocal.m4
${ACLOCAL:-aclocal} $MACROS

# generate makefiles
${AUTOMAKE:-automake} --add-missing --gnu

# generate headers
${AUTOHEADER:-autoheader}

# create "configure" script
${AUTOCONF:-autoconf} $MACROS

# @FIXME remove for release:
BUILDHOST="`./config.guess`"
./configure --enable-maintainer-mode

# cleanup
rm -rf autom4te.cache
