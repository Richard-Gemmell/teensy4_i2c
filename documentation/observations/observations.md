# Observations and Measurements

# Teensy 4 Loopback
## Test Configuration
* Master - Teensy 4 I2C port 0
* Slave - Teensy 4 I2C port 1 on the same device
* SCL connected to pin 22
* SDA connected to pin 23
* Owon VDS 1022 25 MHz oscilloscope hooked on to pins 22 and 23
* Measurements taken with oscilloscope unless stated otherwise

## Findings
### Fall Time (t<sub>f</sub>)
According to the I2C specification, the fall time is the time taken for
a line to fall from 0.7 V<sub>dd</sub> to 0.3 V<sub>dd</sub>. This is
called t<sub>f</sub> in the spec.

* SCL fall time approx 4 ns
* SDA fall time approx 6 ns

![Fall Times and Minimum Data Hold Time](fall_times.png)
### Hold Time (t<sub>HD;DAT</sub>)
See image for fall times.
* minimum approx 5 ns

### Rise Time (t<sub>r</sub>)
According to the I2C specification, the rise time is the time taken for
a line to rise from 0.3 V<sub>dd</sub> to 0.7 V<sub>dd</sub>. This is
called t<sub>r</sub> in the spec.

* SCL fall time approx 320 ns
* SDA fall time approx 320 ns

![Rise Times](rise_times.png)
