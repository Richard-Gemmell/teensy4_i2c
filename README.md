# teensy4_i2c
An I2C library for the [Teensy 4](https://www.pjrc.com/store/teensy40.html)
microcontroller.

The Teensy 4.0 uses the [NXP i.MXRT 1062](https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/i.mx-rt-series/i.mx-rt1060-crossover-processor-with-arm-cortex-m7-core:i.MX-RT1060)
microcontroller with an ARM Corex-M7 core.

This library can be used as a drop in replacement for the Wire library
in [Teensyduino](https://www.pjrc.com/teensy/td_download.html). It has
native APIs that are more flexible than Wire and simpler to use in
some cases. The primary reason for creating this library was to add
support for Slave mode.

The driver implementations IMX_RT1060_I2CMaster and IMX_RT1060_I2CSlave have
no dependencies on Arduino itself and relatively few dependencies on the
Teensy code. This means that is should be possible to use the drivers with
other tool sets without having to change too much.

## Features
* Supports all features that are required by the I2C specification
for master, multi-master and slave devices.
* Optional drop in replacement for the Wire library in i2c_driver_wire.h
* Master Mode
* Slave Mode
* Standard Mode (100 kbps)
* Fast Mode (400 kbps)
* Fast Mode Plus (1 Mbps)
* Multi-master support
* Clock stretching in Slave Mode
* Non-blocking API for Master Mode and Slave Mode
* Comprehensive error handling
* Can tune the Teensy's electrical configuration for you application
* A single slave can handle multiple I2C addresses

## Installation
* [Arduino Instructions](documentation/installation/arduino_installation.md)
* [PlatformIO Instructions](documentation/installation/platformio_installation.md)

### GitHub Help
If you're wondering how to get code out of GitHub then you're not alone!
Here are some [instructions for downloading or cloning](documentation/installation/github_help.md)
this library. The page also explains how to download a different branch.

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
the Wire API unless you have to.

1. Download the code and put it in your include path.
1. Change all #includes from Wire.h to i2c_driver_wire.h.
1. If any of your dependencies use Wire.h you'll have to
modify them to use i2c_driver_wire.h instead.
1. If you depend on any libraries that use Wire.h then you'll
have to replace _all_ references to Wire.h to i2c_driver_wire.h
in that library. This is because Arduino compiles all .cpp files
in a library whether you reference them or not.
1. See the examples in the examples/wire directory

If you miss a reference to Wire.h then you'll see compilation errors
like this:-

`... Wire/WireIMXRT.cpp:10: multiple definition of 'Wire'`

You may also see linker errors like this:-

`... bin/ld.exe: Warning: size of symbol 'Wire' changed from 116 in ... teensy4_i2c\i2c_driver_wire.cpp.o to 112 in ... libraries\Wire\WireIMXRT.cpp.o`

## Ports and Pins
This table lists the objects that you should use to handle each I2C port.

| Port | Pins               | imx_rt1060_i2c_driver.h | i2c_driver_wire.h |
| ---- |--------------------| ------------------|-------|
| 0    | SCL0(19), SDA0(18) | Master or Slave   | Wire  |
| 1    | SCL1(16), SDA1(17) | Master1 or Slave1 | Wire1 |
| 2    | SCL2(24), SDA2(25) | Master2 or Slave2 | Wire2 |

## Pull Up Resistors
The I2C protocol uses open drain pins to pull signal voltages low and
pull up resistors to pull voltages back up again. This library enables
the Teensy's 22 kΩ internal pull up resistor by default. Many sensors
that support I2C often have internal pullups as well. These may
provide enough resistance for your application.

If you don't have any internal pullups or the internal resistors aren't
big enough for your application then you'll need to use external pullups.
I recommend 2.2 kΩ or 1 kΩ.

## Common Problems
Here are some of the common problems that will can break the I2C
connection or just make it very unreliable.
* the slave and the master must share a common ground
* you may need external pullup resistors (see above)
* if you're upgrading your project from a Teensy 3 with 5V IO to a Teensy 4
and you already have 4.7 kΩ pullup resistors then you may need to swap them
to 2.2 kΩ. This is because the Teensy 4 IO pins run at 3.3V.
* you may need to tune the Teensy's pad configuration to match the
I2C pin driver impedance to match your I2C wiring impedance. Depending
on which API you use you'll need to call setPadControlConfiguration() or
set_pad_control_configuration(). There are examples in raw_configure_pad.ino
and configure_pad.ino.
* make sure the master and the slave agree on how many bytes to
send and what order they're in. Data alignment, padding and endianness
may cause issues.
* the Teensy's I2C implementation is inherently asynchronous. Watch out
for bugs where your application code is reading (or writing) to the buffer
that holds your I2C data while the driver is busy transmitting or receiving
the data. This causes a device to see part of one message and part of a later
message. (A partial read bug).

## Data Sheets and References
* https://www.i2c-bus.org/
* https://www.pjrc.com/teensy/IMXRT1060RM_rev2.pdf
* https://www.nxp.com/docs/en/application-note/AN5078.pdf
* https://www.nxp.com/docs/en/application-note/AN10216.pdf

## Not Tested
I haven't been able to test some features because of hardware and time
restrictions. These features *should* work but don't be surprised if
they don't. Please contact me if you encounter any problems.
* Multi-master configurations
* Cortex i.MX RT1050
* Teensy 4.1

## Not Implemented
The following features are supported by the NXP i.MXRT 1062 processor but
I haven't implemented them in this driver.
Please contact me if you need any of these features.
* Alternative pins for port 1
* Direct Memory Access (DMA)
* High Speed Mode (3.4 Mbps)
* Ultra Fast Mode (5 Mbps)
* 10 bit slave addresses
* Glitch filter (for slave mode)
* SMBus Alert
* General Call
* 4 pin I2C in Master mode
* Master reading more than 256 bytes in a single transfer

## Version History
| Version | Release Date      | Comment         |
| ------- |-------------------| ----------------|
| v1.1.0  | 12th Aug 2020 | I2C slave can now have many I2C addresses.
| v1.0.1  | 11th Aug 2020 | Adjusted timings to improve responsiveness.
| v1.0.0  | 19th May 2020 | Promoted v0.9.5 to 1.0 as it seems stable.
| v0.9.5  | 19th May 2020 | Improved pad control configuration to reduce errors from noise. (Changed default config from 0xF0B0 to 0x1F830)
| v0.9.4  | 15th May 2020 | Added function overloads to I2CDriverWire to avoid ambiguous calls from existing code. |
| v0.9.3  | 17th February 2020 | I2CDriverWire::requestFrom() now reports correct number of bytes. |
| v0.9.2  | 9th January 2020 | Can now probe for active slaves. |
| v0.9.1  | 7th January 2020  | Fixed bug in i2c_driver_wire.h |
| v0.9.0  | 7th November 2019 | Initial Version |
