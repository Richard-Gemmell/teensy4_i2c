# I2C Timing Registers for the i.MX RT1062
The Teensy 4 and Teensy 4.1 use the i.MX RT1062 processor.

This document describes how the processor's registers map to the I2C
Specification timings. All results have been confirmed by experiment
unless stated otherwise.

## References
### i.MX RT1062
Information on the i.MX RT1062 is taken from the datasheet;
[i.MX RT1060 Processor Reference Manual, Rev. 3 - 07/2021](../references/IMXRT1060RM_rev3.pdf).
References to this datasheet are given like this `47.5.1.24 Slave Configuration 2 (SCFGR2)`.

Relevant sections:
* `Chapter 47 - Low Power Inter-Integrated Circuit (LPI2C)`

### I2C Specification
Details of the I2C Specification are taken from the spec itself.
[I<sup>2</sup>C-bus specification and user manual Rev. 6 - 4 April 2014](../references/UM10204.v6.pdf)
References to the spec are given like this
`I2C Spec. 3.1 Standard-mode, Fast-mode and Fast-mode Plus I2C-bus protocols`.

There is a more [recent version of the spec](../references/UM10204.v7.pdf).
The only significant difference is that v7 replaces the terms "master" and "slave"
for "controller" and "target". I've decided to keep using the old terms as
they're so widely used.

## Symbols
All symbols are taken from the I2C Specification except for:
* t<sub>r0</sub> - time for signal to rise from 0 to 0.3 V<sub>dd</sub>
  - for an RC curve this = 0.4 x t<sub>r</sub> 
* t<sub>f1</sub> - time for signal to fall from V<sub>dd</sub> to 0.7 V<sub>dd</sub>
  - for an RC curve this = 0.4 x t<sub>f</sub> 
* T<sub>LPI2C</sub> - period of the LPI2C functional clock 
* T<sub>SCL</sub> - period of the SCL clock
* scale - scaling factor applied to I2C register values

## Units
All durations are given in nanoseconds (ns).

# Index
<!-- TOC -->
* [I2C Timing Registers for the i.MX RT1062](#i2c-timing-registers-for-the-imx-rt1062)
  * [References](#references)
    * [i.MX RT1062](#imx-rt1062)
    * [I2C Specification](#i2c-specification)
  * [Symbols](#symbols)
  * [Units](#units)
* [Index](#index)
* [i.MX RT 1060 Registers](#imx-rt-1060-registers)
  * [I2C Master Registers](#i2c-master-registers)
* [Calculations](#calculations)
  * [scale](#scale)
  * [t<sub>HD;STA</sub>](#t-sub-hdsta-sub)
  * [t<sub>SU;STO</sub>](#t-sub-susto-sub)
    * [t<sub>SU;STO</sub> Ideal Value](#t-sub-susto-sub-ideal-value)
    * [t<sub>SU;STO</sub> Minimum Value](#t-sub-susto-sub-minimum-value)
    * [t<sub>SU;STO</sub> Maximum Value](#t-sub-susto-sub-maximum-value)
  * [t<sub>low</sub>](#t-sub-low-sub)
  * [t<sub>high</sub>](#t-sub-high-sub)
    * [t<sub>high</sub> Minimum Value](#t-sub-high-sub-minimum-value)
    * [t<sub>high</sub> Max Value](#t-sub-high-sub-max-value)
  * [t<sub>fall</sub>](#t-sub-fall-sub)
<!-- TOC -->

# i.MX RT 1060 Registers
## I2C Master Registers
* MCCR0 `47.5.1.13 Master Clock Configuration 0 (MCCR0)`
  - CLKLO - controls t<sub>low</sub>, low period of the SCL clock pulse
  - CLKHI - controls t<sub>high</sub>, high period of the SCL clock pulse
  - DATAVD - controls t<sub>HD;DAT</sub>, data hold time
  - SETHOLD - controls 
    - t<sub>HD;STA</sub>, hold time for START condition
    - t<sub>SU;STA</sub>, setup time for repeated START condition
    - t<sub>SU;STO</sub>, setup time for STOP condition

# Calculations
## scale
**scale = (2 ^ PRESCALE) x T<sub>LPI2C</sub>**

## t<sub>HD;STA</sub>
**t<sub>HD;STA</sub> = (SETHOLD + 1) x scale**

I've been unable to establish whether the fall time (t<sub>f</sub>)
effects this or not.

## t<sub>SU;STO</sub>
Behaviour:
* the processor releases the SCL pin allowing SCL to rise
* when it detects that SCL has risen it waits for a time derived from SETHOLD
  * this provides limited compensation for different rise times
* when the time has passed it releases SDA allowing SDA to rise

Notes:
* The processor decides that SCL has risen at approximately 0.5 V<sub>dd</sub>.
* The I2C Specification defines t<sub>SU;STO</sub> as the time between SCL
reaching 0.7 V<sub>dd</sub> and SDA reaching 0.3 V<sub>dd</sub>. It specifies
a minimum value but not a maximum.
* The worst case scenario is that the SCL rise time is very long and the SDA
rise time is very short.
* The duration is affected by SCL_LATENCY as described in section `47.3.1.4 Timing Parameters`.

### t<sub>SU;STO</sub> Ideal Value
**t<sub>SU;STO</sub> = ((CLKHI + 1 + SCL_LATENCY) x scale)</sub>**

Applies only if t<sub>r</sub> is 0 for both lines.

### t<sub>SU;STO</sub> Minimum Value
**t<sub>SU;STO</sub> >= ((CLKHI + 1 + SCL_LATENCY) x scale) - t<sub>r</sub>**

Applies when SDA rises instantly but SCL rises slowly.

This is a slightly pessimistic estimate. It assumes the Teensy detects the
SCL rise at 0.3 V<sub>dd</sub> but it usually detects it closer to
0.5 V<sub>dd</sub>.

### t<sub>SU;STO</sub> Maximum Value
**t<sub>high</sub> <= ((CLKHI + 1 + SCL_LATENCY) x scale)</sub> + t<sub>r0</sub>**

Applies if t<sub>r</sub> is 0 for SCL but long for SDA.

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
According to the spec, t<sub>fall</sub> must have a minimum value of 12 ns.
The Teensy in loopback is much faster than that. It seems to vary between
4 and 8 ns. It's a hard to be sure as the actual value is approximately
the same as my oscilloscope's resolution. These figures are similar to
the ones given in section "7.2 Slew rate" of `AN5078.pdf`.

I searched the web and concluded that:
* fall times below 10ns are common
* the I2C spec figure is probably designed to reduce electrical noise such as undershoot spikes

SPEED, DSE and SRE are intended to control the "strength" of the pin.
I tried a lot of combinations of SPEED and DSE. If DSE is set to 1 then
the fall time is approx 8 ns. Otherwise it's 4 or 5 ns. SPEED seems to
have little effect although values 0 and 1 may be marginally slower.

I changed the default values to DSE(2) and SPEED(0). This still doesn't
meet the spec but it seems to reduce noise spikes. (Compared to the previous
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
very minor. For example 100Ω, increases fall time by a couple of
nanoseconds but increases V<sub>OL1</sub> to 500 mV with a 1k pull up
which exceeds the maximum allowed in the spec.
