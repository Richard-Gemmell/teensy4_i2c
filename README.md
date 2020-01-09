# teensy4_i2c
An I2C library for the [Teensy 4](https://www.pjrc.com/store/teensy40.html)
microcontroller.

The Teensy 4.0 uses the [NXP i.MXRT 1062](https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/i.mx-rt-series/i.mx-rt1060-crossover-processor-with-arm-cortex-m7-core:i.MX-RT1060)
microcontroller with n ARM Corex-M7 core.

This library is intended to be used as a drop in replacement for the
Wire library in [Teensyduino](https://www.pjrc.com/teensy/td_download.html).
The primary reason for writing it was to add support for Slave mode.

The driver implementations IMX_RT1060_I2CMaster and IMX_RT1060_I2CSlave have
no dependencies on Arduino itself and relatively few dependencies on the
Teensy code. This means that is should be possible to use the drivers with
other tool sets without having to change too much.

## Usage

### Use I2C Register Wrappers
I2CDevice and I2CRegisterSlave classes make it very simple to follow
the standard I2C pattern of reading or writing to "registers".
I recommend that you use this interface if it's suitable.

1. Download the code and put it in your include path.
1. &#35;include "i2c_device.h" if you have a master and wish
to read from a slave device.
1. &#35;include "i2c_register_slave.h" if you want to implement
a slave device to be read by a master.
1. See the examples in the examples/simple directory

### Use the Driver Directly
The driver interfaces are defined in i2c_driver.h. These provide
everything you need to use I2C without the limitations of the Wire
library. The key classes are I2CMaster and I2CSlave.

1. Download the code and put it in your include path.
1. &#35;include "imx_rt1060_i2c_driver.h"
1. See the examples in the examples/raw directory

### Replacing Wire.h
Follow these instructions if you have already written code to
use Wire.h and don't want to change it. I don't recommend using
the Wire API unless you have to. See below for alternatives.

1. Download the code and put it in your include path.
1. Change all #includes from Wire.h to i2c_driver_wire.h.
1. If any of your dependencies use Wire.h you'll have to
modify them to use i2c_driver_wire.h instead.
1. See the examples in the examples/wire directory

## Common Problems
Here are some of the common problems that will can break the I2C
connection or just make it very unreliable.
* the slave and the master must share a common ground
* you _must_ use external pullup resistors with the Teensy unless
these are provided by the slave. I recommend 2.2 kOhm or 1 kOhm.
* make sure the master and the slave agree on how many bytes to
send and what order they're in. Data alignment, padding and endianness
may cause issues.

## Features
* Supports all features that are required by the I2C specification
for master, multi-master and slave devices.
* Drop in replacement for the Wire library
* Master Mode
* Slave Mode
* Standard Mode (100 kbps)
* Fast Mode (400 kbps)
* Fast Mode Plus (1 Mbps)
* Multi-master support
* Clock stretching in Slave Mode
* Non-blocking API for Master Mode and Slave Mode
* Comprehensive error handling

## Not Tested
I haven't been able to test some features because of hardware and time
restrictions. These features *should* work but don't be surprised if
they don't. Please contact me if you encounter any problems.
* Multi-master configurations
* Noisy environments
* Cortex i.MX RT1050

## Not Implemented
The following features are supported by the NXP i.MXRT 1062 processor but
I haven't implemented them in this driver.
Please contact me if you need any of these features.
* Alternative pins for port 1
* Direct Memory Access (DMA)
* High Speed Mode (3.4 Mbps)
* Ultra Fast Mode (5 Mbps)
* Multiple addresses for a single slave
* 10 bit slave addresses
* Glitch filter (for slave mode)
* SMBus Alert
* General Call
* 4 pin I2C in Master mode
* Master reading more than 256 bytes in a single transfer

## Version History
| Version | Release Date      | Comment         |
| ------- |-------------------| ----------------|
| v0.9.0  | 7th November 2019 | Initial Version |
| v0.9.1  | 7th January 2020  | Fixed bug in i2c_driver_wire.h |
| v0.9.2  | 9th January 2020 | Can now probe for active slaves. |