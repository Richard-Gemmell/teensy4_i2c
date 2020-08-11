# coding=utf-8
# Copyright © 2020 Richard Gemmell
# Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

import struct

from smbus2 import *


def main():
    address = 0x2D
    bus = SMBus(1)
    register = 0xFF
    data = [0x01, 0x02, 0x03]
    # data = [0x01, 0x02, 0x03, 0x04, 0x05]   # Uncomment to trigger an overflow exception
    try:
        bus.write_i2c_block_data(address, register, data)
    except:
        print("Error talking to slave.\nCheck that the wiring is correct and you're using the correct pins.")
    finally:
        bus.close()
    # Note that the register appears as data to a simple slave reader
    # as "register" isn't part of the I2C spec. It's just a programming
    # convention.
    all_bytes = [register]
    all_bytes.extend(data)
    print("Sent", all_bytes)


if __name__ == '__main__':
    main()
