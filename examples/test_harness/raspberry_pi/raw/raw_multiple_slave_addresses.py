# coding=utf-8
# Copyright © 2020 Richard Gemmell
# Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

from time import sleep

from smbus2 import *


def main():
    address1 = 0x10
    address2 = 0x20
    bus = SMBus(1)
    print("Sending messages to addresses in range", hex(address1), "to", hex(address2))
    try:
        for address in range(address1, address2+1):
            send_message(bus, address)
    finally:
        bus.close()


def send_message(bus, address):
    try:
        echoed_address = bus.read_byte_data(address, address)
        if address == echoed_address:
            print(hex(address), "OK")
        else:
            print(hex(address), "Error: Teensy replied with", hex(echoed_address))
        sleep(0.1)
    except:
        print(hex(address), "Error: No response from Teensy")


if __name__ == '__main__':
    main()
