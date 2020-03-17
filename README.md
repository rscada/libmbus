# libmbus: M-bus Library from Raditex Control (http://www.rscada.se) <span style="float:right;"><a href="https://travis-ci.org/rscada/libmbus" style="border-bottom:none">

![Build Status](https://travis-ci.org/rscada/libmbus.svg?branch=master)</a></span>

libmbus is an open source library for the M-bus (Meter-Bus) protocol.

The Meter-Bus is a standard for reading out meter data from electricity meters,
heat meters, gas meters, etc. The M-bus standard deals with both the electrical
signals on the M-Bus, and the protocol and data format used in transmissions on
the M-Bus. The role of libmbus is to decode/encode M-bus data, and to handle
the communication with M-Bus devices.


## BUILD

with cmake

```bash
rm -rf _build
mkdir _build
cd _build
# configure
# e.g. on linux
cmake .. -DLIBMBUS_BUILD_EXAMPLES=ON
# e.g. for a target device
cmake .. -DLIBMBUS_BUILD_EXAMPLES=ON -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain/foo-bar-baz.cmake
# compile
cmake --build . -j
# install - optional
cmake --build . --target install
```

## CONSUME

```cmake
find_package(libmbus)
add_executable(my_app main.cpp)
target_link_libraries(my_app libmbus::libmbus)
```

For more information see http://www.rscada.se/libmbus
