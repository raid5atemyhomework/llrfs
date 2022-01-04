#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libllrfs], [0.1A], [raid5atemyhomework@protonmail.com])
AC_CONFIG_AUX_DIR([auxdir])
AM_INIT_AUTOMAKE([subdir-objects tar-ustar foreign])
AC_CONFIG_SRCDIR([libllrfs.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

# Checks for programs.
AC_PROG_CC_C99
AM_PROG_AR
AC_PROG_RANLIB

AX_VALGRIND_CHECK()
AM_CONDITIONAL([USE_VALGRIND], [test x"$enable_valgrind" = xyes])

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
