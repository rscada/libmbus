This fork contains 3 new things

serial-mbus-send-raw binary (conversion from tcp)

function mbus_send_custom_text in mbus-protocol-aux.c/h
This works with the popular Elvaco CMa10, you can send text messages to the unit

function mbus_send_global_aes128_key in mbus-protocol-aux 
This works with the Elvaco CMi-Box, you can send global AES128 decryption key to module. Note; the length isn't enforced, although the key should be 16 bytes. Doublecheck the key before sending it.

# libmbus: M-bus Library from Raditex Control (http://www.rscada.se) <span style="float:right;"><a href="https://travis-ci.org/rscada/libmbus" style="border-bottom:none">![Build Status](https://travis-ci.org/rscada/libmbus.svg?branch=master)</a></span>

libmbus is an open source library for the M-bus (Meter-Bus) protocol.

The Meter-Bus is a standard for reading out meter data from electricity meters,
heat meters, gas meters, etc. The M-bus standard deals with both the electrical
signals on the M-Bus, and the protocol and data format used in transmissions on
the M-Bus. The role of libmbus is to decode/encode M-bus data, and to handle
the communication with M-Bus devices.

For more information see http://www.rscada.se/libmbus
