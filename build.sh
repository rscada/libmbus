#!/bin/sh
#

if [ -f Makefile ]; then
    # use existing automake files
    echo >> /dev/null
else
    # regenerate automake files
    echo "Running autotools..."

    autoheader \
        && aclocal \
        && case \
          $(uname) in Darwin*) glibtoolize --ltdl --copy --force ;; \
          *) libtoolize --ltdl --copy --force ;; esac \
        && automake --add-missing --copy \
        && autoconf \
        && ./configure
fi

make
