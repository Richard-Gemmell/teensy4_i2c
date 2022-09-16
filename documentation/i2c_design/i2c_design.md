# Teensy I2C Design

## I2C Signal Timing Design
This section describes how I've decided to interpret the I2C Specification
in order to choose various I2C timings. It's written in terms of the I2C
spec, not the Teensy's configuration.

The next section converts these decisions into configuration values for the
Teensy's I2C hardware.

### t<sub>HD;STA</sub>
Hold time for START and repeated START. See [I2C Timing Registers](i2c_timing_registers.md#tsubhdstasub).

The I2C Specification sets a minimum hold time. Devices **must not** interpret
shorter hold times as a START. Longer times allow devices more time
to notice that a START has occurred.

## Teensy I2C Configuration
This section describes how I've converted the [I2C Signal Timing Design](#i2c-signal-timing-design)
above into the values that'll be used to configure the Teensy's hardware.

### SETHOLD - Master
SETHOLD controls the start hold time, [t<sub>HD;STA</sub>](i2c_timing_registers.md#tsubhdstasub),
and the setup STOP time, [t<sub>SU;STO</sub>](i2c_timing_registers.md#tsubsustosub).

For a START:
* start with the minimum value from the I2C Specification
* add 25% of the minimum to give other I2C devices a nice window to react
* assume the fall time for SDA reduces the window so add the maximum
  allowed fall time

| Frequency | Min Setup Time | 25%   | Max Fall Time | Target Total |
|-----------|----------------|-------|---------------|--------------|
| 100 kHz   | 4'000          | 1'000 | 300           | 5'300        |
| 400 kHz   | 600            | 150   | 300           | 1'050        |
| 1 MHz     | 260            | 65    | 120           | 445          |

For a repeated START
* ???

For a STOP setup time:
* the worst case scenario is that the SCL rise time is very long
  and the SDA rise time is very short
* start with the minimum value from the I2C Specification
* add the maximum value of t<sub>r</sub>
* make no allowance for SDA rise (usually very short anyway)

| Frequency | Min Setup Time | 25%   | Max Rise Time | Target Total |
|-----------|----------------|-------|---------------|--------------|
| 100 kHz   | 4'000          | 1'000 | 1000          | 6'000        |
| 400 kHz   | 600            | 150   | 300           | 1'050        |
| 1 MHz     | 260            | 65    | 120           | 445          |

## Pin Configuration

### Drive Strength
See the t<sub>fall</sub> section of [I2C Timing Registers](i2c_timing_registers.md#tsubfallsub)
for test data.

Set the drive strength register (DSE) to:
* DSE(2) - Drive Strength Enable
  * minimises undershoot spikes
  * small increase in V<sub>OL</sub> with large pullup resistors
  * DSE(1) would maximise t<sub>fall</sub>
    * but I believe the I2C requirement is designed to minimise noise and DSE(2) does this
* SPEED(0) - Speed
  * SPEED doesn't seem to make much difference but the docs say that lower values
    * produce less noise
    * reduce current consumption
* SRE(0) - Slew Rate Enable
  * we don't need to increase the slew rate
  * documentation advises leaving it disabled if possible

### Pullup Configuration
Enable the 22 kΩ internal pullups by default. These are suitable for a 100 kHz
bus with 1 or 2 devices all of which have internal pullups.

User can choose to use 47 kΩ or 100 kΩ internal pullups instead although
these are too weak to be useful on their own.

User can disable the internal pullups to use external pullups only.

See `I2CDriver::set_internal_pullups()` and `I2CDriverWire::setInternalPullups()`. 

#### To Use Internal Pull Up
* PUS(3) - Pull Up Select
  * selects 22k internal pullup
  * this allows a 100k bus to work without external pullups in ideal circumstances 
  * the other options are too weak at any speed
  * set to 2 to use 100 kΩ and 1 to use 47 kΩ internal pullups.
* PUE(1) - Pull / Keep Select
  * must be 1 to enable internal pullup
* PKE(1) - Pull / Keep Enable
  * must be 1 to enable internal pullup

#### For External Pullups Only
Set:
* PUS(0)
* PUE(0)
* PKE(0)
  * Could also use PKE(1) but it doesn't seem to add any benefit

#### Open Drain Enable
I2C is an open drain system so this setting is required. Apparently
removing will cause a short if one device pulls high when the other
pulls low.
* ODE(1)

#### Hysteresis
Significantly reduces the chance that the device interprets noise as an edge. 
* HYS(1)

#### Slew Rate Enable
Setting it to 1 would increase the fall time which is already too fast. 
* SRE(0)