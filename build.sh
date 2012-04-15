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
	automake --add-missing
	autoreconf --install
	./configure
fi

make
