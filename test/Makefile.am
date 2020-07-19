# ------------------------------------------------------------------------------
# Copyright (C) 2010, Raditex AB
# All rights reserved.
#
# rSCADA 
# http://www.rSCADA.se
# info@rscada.se
#
# ------------------------------------------------------------------------------  
PACKAGE			= @PACKAGE@
VERSION			= @VERSION@

AM_CPPFLAGS	= -I$(top_builddir) -I$(top_srcdir) -I$(top_srcdir)/mbus

noinst_HEADERS			= 
noinst_PROGRAMS			= mbus_parse mbus_parse_hex

mbus_parse_LDFLAGS	= -L$(top_builddir)/mbus
mbus_parse_LDADD	= -lmbus -lm
mbus_parse_SOURCES	= mbus_parse.c

mbus_parse_hex_LDFLAGS	= -L$(top_builddir)/mbus
mbus_parse_hex_LDADD	= -lmbus -lm
mbus_parse_hex_SOURCES	= mbus_parse_hex.c

