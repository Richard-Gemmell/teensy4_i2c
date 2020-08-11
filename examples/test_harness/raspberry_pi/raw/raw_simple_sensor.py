# coding=utf-8
# Copyright © 2020 Richard Gemmell
# Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

import struct

from smbus2 import *


def main():
    address = 0x2D
    bus = SMBus(1)
    try:
        data = bus.read_i2c_block_data(address, 0, 2)
        temp = struct.unpack('<h', bytes(data))[0]
        print("Temperature is", temp)
    except:
        print("Error talking to slave.\nCheck that the wiring is correct and you're using the correct pins.")
    finally:
        bus.close()


if __name__ == '__main__':
    main()
