# Teensy I2C Design

# Pin Configuration

## Drive Strength
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

## Pullup Configuration
Enable the 22k internal pullup by default. Give the user the option
to disable it to use external pullups only.

### To Use Internal Pull Up
* PUS(3) - Pull Up Select
  * selects 22k internal pullup
  * this allows a 100k bus to work without external pullups 
  * the other options are too weak at any speed
* PUE(1) - Pull / Keep Select
  * must be 1 to enable internal pullup
* PKE(1) - Pull / Keep Enable
  * must be 1 to enable internal pullup

### For External Pullups Only
It would be nice if the user had the option to disable internal pullups.
In that case select 
* PUS(0)
* PUE(0)
* PKE(0)
  * Could also use PKE(1) but it doesn't seem to add any benefit

### Open Drain Enable
I2C is an open drain system so this setting is required. Apparently
removing will cause a short if one device pulls high when the other
pulls low.
* ODE(1)

### Hysteresis
Significantly reduces the chance that the device interprets noise as an edge. 
* HYS(1)

### Slew Rate Enable
Setting it to 1 would increase the fall time which is already too fast. 
* SRE(0)