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

noinst_HEADERS			= mbus-mock.h
check_PROGRAMS          = mbus_sendrecv
noinst_PROGRAMS			= mbus_parse mbus_parse_hex mbus_sendrecv

mbus_parse_LDFLAGS	= -L$(top_builddir)/mbus
mbus_parse_LDADD	= -lmbus -lm
mbus_parse_SOURCES	= mbus_parse.c

mbus_parse_hex_LDFLAGS	= -L$(top_builddir)/mbus
mbus_parse_hex_LDADD	= -lmbus -lm
mbus_parse_hex_SOURCES	= mbus_parse_hex.c

mbus_sendrecv_LDFLAGS	= -L$(top_builddir)/mbus
mbus_sendrecv_LDADD	= -lmbus -lm
mbus_sendrecv_SOURCES	= mbus-mock.c mbus_sendrecv.c

check-local: mbus_sendrecv
		$(LIBTOOL) --mode=execute $(builddir)/mbus_sendrecv && \
		./generate-xml.sh ./error-frames && \
		./generate-xml.sh ./test-frames && \
		./generate-xml.sh ./unsupported-frames
