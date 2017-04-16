# libmbus: M-bus Library from Raditex Control (http://www.rscada.se) <span style="float:right;"><a href="https://travis-ci.org/rscada/libmbus" style="border-bottom:none">![Build Status](https://travis-ci.org/rscada/libmbus.svg?branch=master)</a></span>

libmbus is an open source library for the M-bus (Meter-Bus) protocol.

The Meter-Bus is a standard for reading out meter data from electricity meters,
heat meters, gas meters, etc. The M-bus standard deals with both the electrical
signals on the M-Bus, and the protocol and data format used in transmissions on
the M-Bus. The role of libmbus is to decode/encode M-bus data, and to handle
the communication with M-Bus devices.

## Building

To configure and build the library simply run the build script

    ./build.sh

## Testing

After you've build the project you might run the tests

    cd test
    make check

For more information see http://www.rscada.se/libmbus
