AC_PREREQ(2.57)
AC_INIT(liboousb2k, 0.0.0, sawinski@users.sourceforge.net)
AC_CONFIG_SRCDIR([oousb2k.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([m], [pow])
AC_CHECK_LIB([usb], [usb_init])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_HEADER_STDC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([atexit memset strerror strtol])

libversion=`./ltversion.sh --version`
AC_SUBST(libversion)

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES(oousb2k-config, chmod 0755 oousb2k-config)
AC_OUTPUT
