# coding=utf-8
# Copyright © 2020 Richard Gemmell
# Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

import struct
from time import sleep

from smbus2 import *


def main():
    address = 0x2D
    bus = SMBus(1)
    try:
        # Read the initial values
        read_values(bus, address)

        # Change the sensor configuration and read again
        safe_write_byte(bus, address, 0, 10) # => Change the temperature offset
        sleep(1)
        read_values(bus, address)

        safe_write_byte(bus, address, 1, 2) # => Change the scaling
        sleep(1)
        read_values(bus, address)

        # Reset config
        safe_write_byte(bus, address, 0, -40) # => Change the temperature offset
        safe_write_byte(bus, address, 1, 10) # => Change the scaling

    finally:
        bus.close()


def read_values(bus, address):
    flags = safe_read(bus, address, 2, 1)[0]
    print("Flags:", flags)
    temp = safe_read(bus, address, 3, 1)[0]
    print("Temp:", temp)
    voltage = to_long(safe_read(bus, address, 6, 4))
    print("Voltage:", voltage)
    voltage = to_long(safe_read(bus, address, 10, 4))
    print("Current:", voltage)


def safe_read(bus, address, register, num_bytes):
    data = []
    try:
        data = bus.read_i2c_block_data(address, register, num_bytes)
    except:
        print("Error reading from slave.\nCheck that the wiring is correct and you're using the correct pins.")
    return data


def safe_write_byte(bus, address, register, value):
    try:
        bus.write_byte_data(address, register, value)
    except:
        print("Error writing to slave.\nCheck that the wiring is correct and you're using the correct pins.")


def to_long(data):
    return struct.unpack('<L', bytes(data))[0]


if __name__ == '__main__':
    main()
