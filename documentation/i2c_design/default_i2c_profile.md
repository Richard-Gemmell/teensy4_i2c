# I2C Configuration Design
There are lots of possible configurations for the Teensy 4's I2C hardware
that would work. This document describes the configuration that ships
by default with the teensy4_i2c driver.

It describes the reasons for the design as well as the actual settings that
are used.

[I2C Timing on the i.MX RT1062 (Teensy 4)](i2c_timing_analysis.md) describes
the various settings and their effect on I2C.

## Goals and Principals
### Design Goals
* 100% compatible with the I2C Specification
  * except that fall time (t<sub>f</sub>) is too fast
* work with any device that also meets the I2C specification
* all timing requirements are met with worst case combinations of rise and fall times
* support other devices with instantaneous falling edges
    * many devices have fall times < 10 nanoseconds
* avoid unnecessary latency
  * but reliability is more important
* SCL clock frequency is reduced where necessary to ensure that timings
  are compliant under all conditions
* when operating as a slave, the timings will be compatible with
  Standard-mode, Fast-mode and Fast-mode Plus masters by default

Note that other configurations could:
* reduce latency for specific circuits
* support longer rise times
* support higher SCL clock speeds for specific circuits

### Timing Requirements
It's possible that SDA and SCL have very different rise and fall times.
In every case, the driver will meet the specification with the worst
possible combination.

For example, the worst case for the setup time for a repeated START t<sub>SU;STA</sub>
occurs when the SCL rise time is very long and the SDA fall time is very short.
In this case, the driver meets the I2C Specification even when SCL has the maximum
allowed rise time and SDA has a fall time of 0 nanoseconds.

### Test Conditions
The driver will be tested with:
* rise time (t<sub>r</sub>) between 30 ns and 105% of the maximum
  allowed in the spec. e.g. 315 ns for Fast-mode
* Standard-mode at 100 kHz, Fast-mode at 400 kHz and Fast-mode Plus at 1 MHz
  * slower speeds are guaranteed to be compliant with the spec as well

## Design Decisions

## Target Values for I2C Timing Parameters

## i.MX RT1062 Register Settings Used

~~~~~~~~~~~~~~~~~
## SCL Clock Frequency
### f<sub>SCL</sub> SCL Clock Frequency
### t<sub>LOW</sub> LOW Period of the SCL Clock
### t<sub>HIGH</sub> HIGH Period of the SCL Clock

## Start and Stop Conditions
### t<sub>SU;STA</sub> Setup Time for a Repeated START Condition
### t<sub>HD;STA</sub> Hold Time for a START or Repeated START Condition
### t<sub>SU;STO</sub> Setup Time for STOP Condition
### t<sub>BUF</sub> Minimum Bus Free Time Between a STOP and START Condition

## Data Bits
### t<sub>SU;DAT</sub> Data Setup Time
### t<sub>HD;DAT</sub> Data Hold Time
### t<sub>VD;DAT</sub> Data Valid Time

## ACKs and Spikes
### t<sub>VD;ACK</sub> Data Valid Acknowledge Time
### t<sub>SP</sub> Pulse Width of Spikes that must be Suppressed by the Input Filter
