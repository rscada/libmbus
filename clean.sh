#!/bin/sh

set -x

true \
    && rm -f Makefile \
    && rm -f Makefile.in \
    && rm -f aclocal.m4 \
    && rm -f config.guess \
    && rm -f config.h \
    && rm -f config.h.in \
    && rm -f config.log \
    && rm -f config.status \
    && rm -f config.sub \
    && rm -f configure \
    && rm -f depcomp \
    && rm -f install-sh \
    && rm -f libmbus.pc \
    && rm -f libtool \
    && rm -f ltmain.sh \
    && rm -f missing \
    && rm -f stamp-h1 \
    && rm -f -r autom4te.cache \
    && rm -f -r libltdl \
    && rm -f -r m4 \
    && rm -f -r bin/.libs \
    && rm -f bin/Makefile \
    && rm -f bin/Makefile.in \
    && rm -f bin/*.o \
    && rm -f bin/libmbus.1 \
    && rm -f bin/mbus-serial-request-data \
    && rm -f bin/mbus-serial-request-data-multi-reply \
    && rm -f bin/mbus-serial-scan \
    && rm -f bin/mbus-serial-scan-secondary \
    && rm -f bin/mbus-serial-select-secondary \
    && rm -f bin/mbus-serial-switch-baudrate \
    && rm -f bin/mbus-tcp-request-data \
    && rm -f bin/mbus-tcp-request-data-multi-reply \
    && rm -f bin/mbus-tcp-scan \
    && rm -f bin/mbus-tcp-scan-secondary \
    && rm -f bin/mbus-tcp-select-secondary \
    && rm -f -r bin/.libs \
    && rm -f mbus/Makefile \
    && rm -f mbus/Makefile.in \
    && rm -f mbus/*.o \
    && rm -f mbus/*.lo \
    && rm -f mbus/*.la \
    && rm -f -r debian/libmbus-dev \
    && rm -f -r debian/libmbus1 \
    && rm -f -r debian/tmp \
    && rm -f -r debian/*.log \
    && rm -f -r debian/*.substvars \
    && rm -f -r debian/*.debhelper


