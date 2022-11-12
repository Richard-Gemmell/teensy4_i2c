# Pin Configuration
The pin configuration determines the electrical behaviour of the I2C pins.

This document describes which settings I've chosen and why. It shows
which settings are involved and contains the results of some experiments.

The settings are hard coded into the driver. See `PAD_CONTROL_CONFIG` in
[imx_rt1060_i2c_driver.cpp](../../src/imx_rt1060/imx_rt1060_i2c_driver.cpp)
for the latest settings.

## Table of Contents
<!-- TOC -->
* [Pin Configuration](#pin-configuration)
  * [Table of Contents](#table-of-contents)
  * [Drive Strength](#drive-strength)
    * [Settings](#settings)
  * [Pullup Configuration](#pullup-configuration)
    * [To Use Internal Pull-Up](#to-use-internal-pull-up)
    * [To Use External Pull-ups Only](#to-use-external-pull-ups-only)
    * [Open Drain Enable](#open-drain-enable)
  * [Hysteresis](#hysteresis)
  * [Slew Rate Enable](#slew-rate-enable)
  * [Appendix - Affect of Drive Strength on Fall Time (t<sub>fall</sub>)](#appendix---affect-of-drive-strength-on-fall-time-tsubfallsub)
<!-- TOC -->

## Drive Strength
The drive strength determines the fall time of the I2C lines.
This is defined as t<sub>fall</sub> in the I2C Specification.

I tested various drive strength settings to see the effect on the
fall time. See the [Appendix](#appendix---affect-of-drive-strength-on-fall-time-tsubfallsub)
for the results. 

### Settings
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
Enable the 22 kΩ internal pullups by default. These are suitable for a 100 kHz
bus with 1 or 2 devices all of which have internal pullups.

User can choose to use 47 kΩ or 100 kΩ internal pullups instead although
these are too weak to be useful on their own.

User can disable the internal pullups to use external pullups only.

See `I2CDriver::set_internal_pullups()` and `I2CDriverWire::setInternalPullups()`.

### To Use Internal Pull-Up
* PUS(3) - Pull Up Select
    * selects 22k internal pullup
    * this allows a 100k bus to work without external pullups in ideal circumstances
    * the other options are too weak at any speed
    * set to 2 to use 100 kΩ and 1 to use 47 kΩ internal pullups.
* PUE(1) - Pull / Keep Select
    * must be 1 to enable internal pullup
* PKE(1) - Pull / Keep Enable
    * must be 1 to enable internal pullup

### To Use External Pull-ups Only
This disables the internal pull-ups.

Set:
* PUS(0)
* PUE(0)
* PKE(0)
    * Could also use PKE(1) but it doesn't seem to add any benefit

### Open Drain Enable
I2C is an open drain system so this setting is required. Apparently
removing will cause a short if one device pulls high when the other
pulls low.
* ODE(1)

## Hysteresis
Significantly reduces the chance that the device interprets noise as an edge.
The I2C Specification requires hysteresis (V<sub>hys</sub>)
to be 0.05 V<sub>dd</sub> (165 mV) or longer in Fast-mode and Fast-mode Plus.
We know that the Teensy tends to generate rare glitches without it.

[4.2 HYS of AN5078.pdf](../references/AN5078.pdf) says that the hysteresis
is 0.25 V. This meets the I2C Specification requirement.

See [12.4.2.1.1 Schmitt trigger](../references/IMXRT1060RM_rev3.pdf) for
the datasheet's description of hysteresis.

Set:
* HYS(1) - Enable hysteresis

| Item                        | Calculation                  | Value   |
|-----------------------------|------------------------------|---------|
| I2C Requirement             | at least 165 mV              |         |
| Hysteresis                  | 250 mV                       | 250 mV  |
| LOW to HIGH trigger voltage | 0.5 V<sub>dd</sub> + 0.125 V | 1.775 V |
| HIGH to LOW trigger voltage | 0.5 V<sub>dd</sub> - 0.125 V | 1.525 V |


## Slew Rate Enable
Setting it to 1 would increase the fall time which is already too fast.
* SRE(0)

## Appendix - Affect of Drive Strength on Fall Time (t<sub>fall</sub>)
According to the [I2C Specification](../references/UM10204.v6.pdf),
t<sub>fall</sub> must have a minimum value of 12 ns at 3.3 V.

The Teensy in loopback is much faster than that. It seems to vary between
4 and 8 ns. It's a hard to be sure as the actual value is approximately
the same as my oscilloscope's resolution. These figures are similar to
the ones given in section [7.2 Slew rate (SRE) of AN5078.pdf](../references/AN5078.pdf).

I searched the web and concluded that:
* fall times below 10ns are common
* the I2C spec figure is probably designed to reduce electrical noise such as undershoot spikes

SPEED, DSE and SRE are intended to control the "strength" of the pin.
I tried a lot of combinations of SPEED and DSE. If DSE is set to 1 then
the fall time is approx 8 ns; otherwise, it's 4 or 5 ns. SPEED seems to
have little effect although values 0 and 1 may be marginally slower.

I changed the default values to DSE(2) and SPEED(0). This still doesn't
meet the spec, but it seems to reduce noise spikes. (Compared to the previous
values of DES(4) and SPEED(1)).

This table shows:
* DSE value
* size of falling edges undershoot spike with 22k pullup
* LOW voltage (V<sub>OL</sub>) with 1k external pullup

| DSE | t<sub>fall</sub> | Undershoot | V<sub>OL</sub> |
|-----|------------------|------------|----------------|
| 1   | 8 ns             | -180 mV    | 290 mV         |
| 2   | 5 ns             | -150 mV    | 130 mV         |
| 3   | 5 ns             | -160 mV    | 90 mV          |
| 4   | 5 ns             | -270 mV    | 70 mV          |

Adding a series resistor increases t<sub>fall</sub> but the effect is
very minor. For example, 100Ω increases fall time by a couple of
nanoseconds but increases V<sub>OL1</sub> to 500 mV with a 1k pull up
which exceeds the maximum allowed in the spec.
