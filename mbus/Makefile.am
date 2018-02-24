# ------------------------------------------------------------------------------
# Copyright (C) 2010, Raditex AB
# All rights reserved.
#
# rSCADA 
# http://www.rSCADA.se
# info@rscada.se
#
# ------------------------------------------------------------------------------
PACKAGE		= @PACKAGE@
VERSION		= @VERSION@

AM_CPPFLAGS	= -I$(top_builddir) -I$(top_srcdir)

includedir = $(prefix)/include/mbus
include_HEADERS = mbus.h mbus-protocol.h mbus-tcp.h mbus-serial.h mbus-protocol-aux.h

lib_LTLIBRARIES	   = libmbus.la
libmbus_la_SOURCES = mbus.c mbus-protocol.c mbus-tcp.c mbus-serial.c mbus-protocol-aux.c

