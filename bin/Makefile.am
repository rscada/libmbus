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

AM_CPPFLAGS	= -I$(top_builddir) -I$(top_srcdir) -I$(top_srcdir)/src

noinst_HEADERS			= 
bin_PROGRAMS			= mbus-tcp-scan mbus-tcp-request-data mbus-tcp-request-data-multi-reply \
                          mbus-tcp-select-secondary mbus-tcp-scan-secondary \
                          mbus-serial-scan mbus-serial-request-data \
                          mbus-serial-select-secondary mbus-serial-scan-secondary \
                          mbus-serial-switch-baudrate

# tcp
mbus_tcp_scan_LDFLAGS	= -L$(top_builddir)/mbus
mbus_tcp_scan_LDADD		= -lmbus 
mbus_tcp_scan_SOURCES	= mbus-tcp-scan.c 

mbus_tcp_request_data_LDFLAGS = -L$(top_builddir)/mbus
mbus_tcp_request_data_LDADD	  = -lmbus
mbus_tcp_request_data_SOURCES = mbus-tcp-request-data.c

mbus_tcp_request_data_multi_reply_LDFLAGS = -L$(top_builddir)/mbus
mbus_tcp_request_data_multi_reply_LDADD	  = -lmbus
mbus_tcp_request_data_multi_reply_SOURCES = mbus-tcp-request-data-multi-reply.c

mbus_tcp_select_secondary_LDFLAGS = -L$(top_builddir)/mbus
mbus_tcp_select_secondary_LDADD	  = -lmbus
mbus_tcp_select_secondary_SOURCES = mbus-tcp-select-secondary.c

mbus_tcp_scan_secondary_LDFLAGS = -L$(top_builddir)/mbus
mbus_tcp_scan_secondary_LDADD	= -lmbus
mbus_tcp_scan_secondary_SOURCES = mbus-tcp-scan-secondary.c

# serial
mbus_serial_scan_LDFLAGS	= -L$(top_builddir)/mbus
mbus_serial_scan_LDADD		= -lmbus 
mbus_serial_scan_SOURCES	= mbus-serial-scan.c 

mbus_serial_request_data_LDFLAGS = -L$(top_builddir)/mbus
mbus_serial_request_data_LDADD	 = -lmbus
mbus_serial_request_data_SOURCES = mbus-serial-request-data.c

mbus_serial_select_secondary_LDFLAGS = -L$(top_builddir)/mbus
mbus_serial_select_secondary_LDADD	 = -lmbus
mbus_serial_select_secondary_SOURCES = mbus-serial-select-secondary.c

mbus_serial_scan_secondary_LDFLAGS = -L$(top_builddir)/mbus
mbus_serial_scan_secondary_LDADD   = -lmbus
mbus_serial_scan_secondary_SOURCES = mbus-serial-scan-secondary.c

mbus_serial_switch_baudrate_LDFLAGS = -L$(top_builddir)/mbus
mbus_serial_switch_baudrate_LDADD   = -lmbus
mbus_serial_switch_baudrate_SOURCES = mbus-serial-switch-baudrate.c

