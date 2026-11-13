dnl ----------------------------------------------------------------------------
dnl Copyright (C) 2010, Raditex AB
dnl All rights reserved.
dnl
dnl rSCADA 
dnl https://github.com/rscada/libmbus/
dnl info@rscada.se
dnl 
dnl ----------------------------------------------------------------------------

AC_CONFIG_MACRO_DIRS([m4])

AC_INIT([libmbus], [0.10.4], [libmbus@gmx.net], [libmbus], [https://github.com/rscada/libmbus/])
AM_INIT_AUTOMAKE([foreign -Wall])

# fix for automake 1.11 & 1.12
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT

dnl ----------------------
dnl 
AC_PROG_CC

dnl socket(), connect() and getaddrinfo() are in libsocket on some systems (QNX)
AC_SEARCH_LIBS([socket], [socket])
AC_SEARCH_LIBS([getaddrinfo], [socket])

dnl after the link tests: the compiler itself does not know this libtool option
LDFLAGS="$LDFLAGS -version-info 0:10:0"

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile mbus/Makefile test/Makefile bin/Makefile libmbus.pc])
AC_OUTPUT


echo \
"----------------------------------------------------------
Configuration:

	Source location:	${srcdir}
	Compile:		${CC}
	Compiler flags:		${CFLAGS}
	Linker flags:		${LDFLAGS}
	Host system type:	${host}
	Install path:		${prefix}

	See config.h for further configuration.
----------------------------------------------------------"
