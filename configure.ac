dnl ----------------------------------------------------------------------------
dnl Copyright (C) 2010, Raditex AB
dnl All rights reserved.
dnl
dnl rSCADA 
dnl http://www.rSCADA.se
dnl info@rscada.se
dnl 
dnl ----------------------------------------------------------------------------

LT_CONFIG_LTDL_DIR([libltdl])

AC_INIT([libmbus], [0.9.0], [info@rscada.se], [libmbus], [http://www.rscada.se/libmbus/])
AC_CONFIG_AUX_DIR([libltdl/config])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AM_PROG_LIBTOOL
# fix for automake 1.11 & 1.12
m4_ifdef([AM_PROG_AR], [AM_PROG_AR]) 

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
