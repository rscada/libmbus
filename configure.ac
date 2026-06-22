dnl ----------------------------------------------------------------------------
dnl Copyright (C) 2010, Raditex AB
dnl All rights reserved.
dnl
dnl rSCADA 
dnl http://www.rSCADA.se
dnl info@rscada.se
dnl 
dnl ----------------------------------------------------------------------------

AC_CONFIG_MACRO_DIRS([m4])

AC_INIT([libmbus], [0.9.0], [info@rscada.se], [libmbus], [http://www.rscada.se/libmbus/])
AM_INIT_AUTOMAKE([foreign -Wall])

# fix for automake 1.11 & 1.12
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT

LDFLAGS="$LDFLAGS -version-info 0:9:0"

dnl ----------------------
dnl 
AC_PROG_CC

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
