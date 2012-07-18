#!/bin/sh
#

if [ -f Makefile ]; then
	#
	# use existing automake files
	#
	echo >> /dev/null
else
	#
	# regenerate automake files
	#

    autoheader \
        && aclocal \
        && libtoolize --ltdl --copy --force \
        && automake --add-missing --copy \
        && autoconf \
        && ./configure
fi

make
