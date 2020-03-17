#!/bin/bash

rm -rf _build
mkdir _build
cd _build
cmake .. -DLIBMBUS_BUILD_EXAMPLES=ON -DLIBMBUS_BUILD_TESTS=ON
cmake --build . -j


# conan create . gocarlos/testing --build missing
