dnl ----------------------------------------------------------------------------
dnl Copyright (C) 2010, Raditex AB
dnl All rights reserved.
dnl
dnl FreeSCADA 
dnl http://www.FreeSCADA.com
dnl freescada@freescada.com
dnl 
dnl ----------------------------------------------------------------------------

AC_INIT([mbus], [0.7.0], [info@rscada.se], [mbus], [http://www.freescada.com/libmbus/])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_CONFIG_MACRO_DIR([m4])

AM_PROG_LIBTOOL

LDFLAGS="$LDFLAGS -version-info 0:7:0"

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
