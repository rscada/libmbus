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
    echo "Running autotools..."

    autoheader \
        && aclocal \
        && libtoolize --ltdl --copy --force \
        && automake --add-missing --copy \
        && autoconf \
        && ./configure
fi

make
