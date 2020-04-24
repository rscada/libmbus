#!/bin/sh


rm -rf build
mkdir build
cd build
cmake .. -DLIBMBUS_BUILD_EXAMPLES=ON -DLIBMBUS_BUILD_TESTS=ON
cmake --build .

# build deb

# rm -rf build
# mkdir build
# cd build
# cmake .. -DLIBMBUS_PACKAGE_DEB=ON
# cpack ..
# dpkg -i *.deb

# build doc

# mkdir build
# cd build
# cmake .. -DLIBMBUS_BUILD_DOCS=ON
# cmake --build . --target doc
