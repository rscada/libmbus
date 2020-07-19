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
                          mbus-serial-scan mbus-serial-request-data mbus-serial-request-data-multi-reply \
                          mbus-serial-select-secondary mbus-serial-scan-secondary \
                          mbus-serial-switch-baudrate mbus-tcp-raw-send mbus-tcp-application-reset \
                          mbus-serial-set-address

# tcp
mbus_tcp_scan_LDFLAGS	= -L$(top_builddir)/mbus
mbus_tcp_scan_LDADD		= -lmbus -lm
mbus_tcp_scan_SOURCES	= mbus-tcp-scan.c 

mbus_tcp_request_data_LDFLAGS = -L$(top_builddir)/mbus
mbus_tcp_request_data_LDADD	  = -lmbus -lm
mbus_tcp_request_data_SOURCES = mbus-tcp-request-data.c

mbus_tcp_request_data_multi_reply_LDFLAGS = -L$(top_builddir)/mbus
mbus_tcp_request_data_multi_reply_LDADD	  = -lmbus -lm
mbus_tcp_request_data_multi_reply_SOURCES = mbus-tcp-request-data-multi-reply.c

mbus_tcp_select_secondary_LDFLAGS = -L$(top_builddir)/mbus
mbus_tcp_select_secondary_LDADD	  = -lmbus -lm
mbus_tcp_select_secondary_SOURCES = mbus-tcp-select-secondary.c

mbus_tcp_scan_secondary_LDFLAGS = -L$(top_builddir)/mbus
mbus_tcp_scan_secondary_LDADD	= -lmbus -lm
mbus_tcp_scan_secondary_SOURCES = mbus-tcp-scan-secondary.c

mbus_tcp_raw_send_LDFLAGS = -L$(top_builddir)/mbus
mbus_tcp_raw_send_LDADD	  = -lmbus -lm
mbus_tcp_raw_send_SOURCES = mbus-tcp-raw-send.c

mbus_tcp_application_reset_LDFLAGS = -L$(top_builddir)/mbus
mbus_tcp_application_reset_LDADD   = -lmbus -lm
mbus_tcp_application_reset_SOURCES = mbus-tcp-application-reset.c

# serial
mbus_serial_scan_LDFLAGS	= -L$(top_builddir)/mbus
mbus_serial_scan_LDADD		= -lmbus -lm
mbus_serial_scan_SOURCES	= mbus-serial-scan.c 

mbus_serial_request_data_LDFLAGS = -L$(top_builddir)/mbus
mbus_serial_request_data_LDADD	 = -lmbus -lm
mbus_serial_request_data_SOURCES = mbus-serial-request-data.c

mbus_serial_request_data_multi_reply_LDFLAGS = -L$(top_builddir)/mbus
mbus_serial_request_data_multi_reply_LDADD   = -lmbus -lm
mbus_serial_request_data_multi_reply_SOURCES = mbus-serial-request-data-multi-reply.c

mbus_serial_select_secondary_LDFLAGS = -L$(top_builddir)/mbus
mbus_serial_select_secondary_LDADD	 = -lmbus -lm
mbus_serial_select_secondary_SOURCES = mbus-serial-select-secondary.c

mbus_serial_scan_secondary_LDFLAGS = -L$(top_builddir)/mbus
mbus_serial_scan_secondary_LDADD   = -lmbus -lm
mbus_serial_scan_secondary_SOURCES = mbus-serial-scan-secondary.c

mbus_serial_switch_baudrate_LDFLAGS = -L$(top_builddir)/mbus
mbus_serial_switch_baudrate_LDADD   = -lmbus -lm
mbus_serial_switch_baudrate_SOURCES = mbus-serial-switch-baudrate.c

mbus_serial_set_address_LDFLAGS = -L$(top_builddir)/mbus
mbus_serial_set_address_LDADD   = -lmbus -lm
mbus_serial_set_address_SOURCES = mbus-serial-set-address.c

# man pages
dist_man_MANS = libmbus.1 \
                mbus-tcp-scan.1 \
                mbus-tcp-request-data.1 \
                mbus-tcp-request-data-multi-reply.1 \
                mbus-tcp-select-secondary.1 \
                mbus-tcp-scan-secondary.1 \
                mbus-tcp-raw-send.1 \
                mbus-serial-scan.1 \
                mbus-serial-request-data.1 \
                mbus-serial-request-data-multi-reply.1 \
                mbus-serial-select-secondary.1 \
                mbus-serial-scan-secondary.1 \
                mbus-serial-switch-baudrate.1

.pod.1:
	pod2man --release=$(VERSION) --center=$(PACKAGE) $< \
		>.pod2man.tmp.$$$$ 2>/dev/null && mv -f .pod2man.tmp.$$$$ $@ || true
	@if grep '\<POD ERRORS\>' $@ >/dev/null 2>&1; \
	then \
		echo "$@ has some POD errors!"; false; \
	fi
