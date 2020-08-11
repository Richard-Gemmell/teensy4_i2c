# coding=utf-8
# Copyright © 2020 Richard Gemmell
# Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

from smbus2 import *


def main():
    address = 0x09
    bus = SMBus(1)
    register = 0xFF
    try:
        value = 90
        bus.write_byte(address, value)
    except:
        print("Error talking to slave.\nCheck that the wiring is correct and you're using the correct pins.")
    finally:
        bus.close()
    print("Sent", value)


if __name__ == '__main__':
    main()
