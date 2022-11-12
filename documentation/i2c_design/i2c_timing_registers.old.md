## t<sub>low</sub>
**t<sub>low</sub> = ((CLKLO + 1) x scale) + t<sub>r0</sub>**
* probably affected by t<sub>f</sub> but I haven't tested it yet
* NOT affected by FILTSCL in any appreciable way at 100kHz 
* confirmed that it's not affected by:
  - FILTSCL
  - FILTSDA
  - CLKHI
  - DATAVD
  - SETHOLD
  - IOMUXC_PAD_HYS

## t<sub>high</sub>
I've found it impossible to relate the master config directly to t<sub>high</sub>.
Testing with the builtin 100kΩ, 47kΩ and 22kΩ pull up resistors shows:-
* t<sub>high</sub> is affected by:
  * CLKHI - as expected
  * PRESCALE - as expected
  * FILTSCL
  * SCL rise time
  * IOMUXC_PAD_HYS
* WARNING: t<sub>high</sub> is probably affected by t<sub>f</sub> but I haven't tested it yet
* t<sub>high</sub> is not affected by:
  - FILTSDA
  - CLKLO
  - DATAVD
  - SETHOLD
  - BUSIDLE
  - Slave settings

It appears that the CLKHI period begins after SCL reaches 0.3 V<sub>dd</sub>
and before it reaches 0.7 V<sub>dd</sub>. If so, then the exact trigger point
depends heavily on the SCL rise time and anything that affects it.

I've therefore given the minimum and maximum values for t<sub>high</sub> instead
of trying to specify an exact value. 

Tests with the Teensy loopback are fairly
close to ((CLKHI + 1 + SCL_LATENCY) x scale) - (t<sub>r</sub> * 0.5) but I
suspect this isn't useful as it relies on the exact shape of the rise time curve.

### t<sub>high</sub> Minimum Value
**t<sub>high</sub> <= ((CLKHI + 1 + SCL_LATENCY) x scale) - t<sub>r</sub>**

### t<sub>high</sub> Max Value
**t<sub>high</sub> <= ((CLKHI + 1 + SCL_LATENCY) x scale)</sub>**

## t<sub>fall</sub>
Almost entirely determined by the [pin configuration](pin_configuration.md).
Probably affected by bus capacitance and pull-up strength as well.
Different for each device. 
