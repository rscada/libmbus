#!/bin/sh


rm -rf _build
mkdir _build
cd _build
cmake .. -DLIBMBUS_BUILD_EXAMPLES=ON -DLIBMBUS_BUILD_TESTS=ON
cmake --build . -j

# build deb

# rm -rf _build
# mkdir _build
# cd _build
# cmake .. -DLIBMBUS_PACKAGE_DEB=ON
# cpack ..
# dpkg -i *.deb