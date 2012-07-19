#
#
#
PACKAGE		= @PACKAGE@
VERSION		= @VERSION@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmbus.pc

dist_doc_DATA = \
	README \
	COPYING \
	hardware/MBus_USB.pdf \
	hardware/MBus_USB.txt

SUBDIRS		= mbus bin
ACLOCAL		= aclocal -I .
ACLOCAL_AMFLAGS = -Werror -I m4
