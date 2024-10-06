# libmbus: M-bus Library from Raditex Control (http://www.rscada.se) <span style="float:right;"><a href="https://travis-ci.org/rscada/libmbus" style="border-bottom:none">![Build Status](https://travis-ci.org/rscada/libmbus.svg?branch=master)</a></span>

libmbus is an open source library for the M-bus (Meter-Bus) protocol.

The Meter-Bus is a standard for reading out meter data from electricity meters,
heat meters, gas meters, etc. The M-bus standard deals with both the electrical
signals on the M-Bus, and the protocol and data format used in transmissions on
the M-Bus. The role of libmbus is to decode/encode M-bus data, and to handle
the communication with M-Bus devices.

For more information see http://www.rscada.se/libmbus

**END OF ORIGINAL REPO README**

# Build and installation info

The installation process is changed and the guide from the main repo is outdated. Now the installation is summed up into 1 (or 2 if you want to install a debian package) scripts. The `build.sh` and the `build-deb.sh`. Because of the copying of the executable files into the `usr/local/bin/` directory, there is no need to install build and install the .deb package unless you realy want to have the man pages and everyting that goes with it.

To build the libmbus library follow the next steps:

1. Tools requirements

```bash
sudo apt-get install build-essential autoconf automake libtool devscripts
```

2. Build the library

```bash
# use build-deb.sh if you want to build and install debian package
./build.sh
```

3. Install the library to your system **(Optional)**

```bash
make install
```

Now you can use the tools by going to the **./bin/** directory in the root of the repository and running the program you want, or, if you have installed the library to your machine, run the programs from anywhere because they are copied into the `/usr/local/bin/` folder.

#### Note: If you don't want to build and install debian package, you dont need to install the `devscripts`.
