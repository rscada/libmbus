# ------------------------------------------------------------------------------
#  Copyright (C) 2012, Robert Johansson <rob@raditex.nu>, Raditex Control AB
#  All rights reserved.
#
#  rSCADA
#  http://www.rSCADA.se
#  info@raditex.nu
#
# ------------------------------------------------------------------------------

# Default settings
BUILD_BINARY=1
BUILD_SOURCE=0
SIGN_KEY=

while [ $# -gt 0 ] && [ "$1" != "--" ]; do
    case "$1" in
        --build-source-pkg)
            BUILD_SOURCE=1
            ;;
        --skip-binary)
            BUILD_BINARY=0
            ;;
        --sign-key|-l)
            SIGN_KEY=$2
            shift
            ;;
        *)
            echo "Usage: $0 [--build-source-pkg] [--skip-binary]"
            echo "          [{--sign-key|-l} <GPG KEY ID>"
            echo "          [-- <dpkg-buildpackage options>"
            case "$1" in
                "-h"|"-?"|--help)
                    exit 0
                    ;;
                *)
                    echo "Unrecognised argument ${1}"
                    exit 1
                    ;;
            esac
    esac
    shift
done

if [ ! -f Makefile ]; then
    #
    # regenerate automake files
    #
    echo "Running autotools..."

    autoheader \
        && aclocal \
        && libtoolize --ltdl --copy --force \
        && automake --add-missing --copy \
        && autoconf
fi

# Determine what build options to pass for source-only,
# binary-only and binary+source builds.
BUILD_OPTS=
if [ ${BUILD_SOURCE} = 1 ]; then
    if [ ${BUILD_BINARY} = 0 ]; then
        BUILD_OPTS=-S
    fi
elif [ ${BUILD_BINARY} = 1 ]; then
    BUILD_OPTS=-b
fi

if [ -n "${SIGN_KEY}" ]; then
    BUILD_OPTS="${BUILD_OPTS} -k ${SIGN_KEY}"
else
    BUILD_OPTS="${BUILD_OPTS} -us -uc"
fi

debuild -i ${BUILD_OPTS} "$@"
#sudo pbuilder build $(NAME)_$(VERSION)-1.dsc
