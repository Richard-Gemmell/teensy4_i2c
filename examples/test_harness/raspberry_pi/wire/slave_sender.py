# coding=utf-8
# Copyright © 2020 Richard Gemmell
# Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

from smbus2 import *


def main():
    address = 0x08
    bus = SMBus(1)
    register = 0x00
    data = []
    try:
        data = bus.read_i2c_block_data(address, register, 6)
    except:
        print("Error talking to slave.\nCheck that the wiring is correct and you're using the correct pins.")
    finally:
        bus.close()
    print("Teensy says", bytes(data).decode('ASCII'))


if __name__ == '__main__':
    main()
