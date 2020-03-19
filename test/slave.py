#!/usr/bin/env python3
# ./build/bin/mbus-serial-request-data -d -b 300 /dev/pty100 <address of device, e.g. 1-250>

import os
import codecs
from serial import Serial
from time import sleep
import binascii

devices = {
    "1": "68 5E 5E 68 08 05 72 91 64 00 08 65 32 99 06 DA 00 00 00 0C 13 00 00 00 00 0B 22 86 40 04 04 6D 24 0A 61 1C 32 6C 00 00 0C 78 91 64 00 08 06 FD 0C 0A 00 01 00 FA 01 0D FD 0B 05 31 32 48 46 57 01 FD 0E 00 4C 13 00 00 00 00 42 6C 5F 1C 0F 37 FD 17 00 00 00 00 00 00 00 00 02 7A 25 00 02 78 25 00 82 16",
    "2": "68 AE AE 68 28 01 72 95 08 12 11 83 14 02 04 17 00 00 00 84 00 86 3B 23 00 00 00 84 00 86 3C D1 01 00 00 84 40 86 3B 00 00 00 00 84 40 86 3C 00 00 00 00 85 00 5B 2B 4B AC 41 85 00 5F 20 D7 AC 41 85 40 5B 00 00 B8 42 85 40 5F 00 00 B8 42 85 00 3B 84 00 35 3F 85 40 3B 00 00 00 00 95 00 3B 95 CF B2 43 95 40 3B 00 00 00 00 85 00 2B 00 00 00 00 85 40 2B 00 00 00 00 95 00 2B D3 9F 90 46 95 40 2B 00 00 00 00 04 6D 19 0F 8A 17 84 00 7C 01 43 F3 0D 00 00 84 40 7C 01 43 9D 01 00 00 84 00 7C 01 63 01 00 00 00 84 40 7C 01 63 01 00 00 00 0F 2F 16"
}

ser = Serial("/dev/pty101")

response = []

def getDeviceBytes(address):
    return bytearray([int(x, 16) for x in devices[str(address)].split(" ")])

while True:
    buffer_length = int.from_bytes(ser.read(1), "big")
    print(buffer_length)

    frame = []
    for e in range(int(buffer_length / 4)):
        buffer = ser.read(1)
        decoded = binascii.hexlify(buffer)
        frame.append(decoded)
    response.append(frame)
    print(response)

    address = int(response[-1][1], 16)
    if address < 3 and address > 0:
        ser.write(getDeviceBytes(address))
