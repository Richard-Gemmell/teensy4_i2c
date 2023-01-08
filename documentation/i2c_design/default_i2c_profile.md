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
* filter out larger noise glitches

### Timing Requirements
It's possible that SDA and SCL have very different rise and fall times.
In every case, the driver will meet the specification with the worst
possible combination.

For example, the worst case for the setup time for a repeated START t<sub>SU;STA</sub>
occurs when the SCL rise time is very long and the SDA fall time is very short.
In this case, the driver meets the I2C Specification even when SCL has the maximum
allowed rise time and SDA has a fall time of 0 nanoseconds.

### Supported I2C Modes
| I2C Mode       | Speed   | Min Rise Time | Max Rise Time | Min Fall Time | Max Fall Time (1) |
|----------------|---------|---------------|---------------|---------------|-------------------|
| Standard-mode  | 100 kHz | 30 ns         | 1000 ns       | 0             | 300               |
| Fast-mode      | 400 kHz | 20 ns         | 300 ns        | 0             | 300               |
| Fast-mode Plus | 1 MHz   | 10 ns         | 120 ns        | 0             | 120               |

1. The Teensy controls the fall time of SCL when it's the master. In this case
   the maximum fall time is 10 nanoseconds.

### Test Conditions
The driver will be tested with:
* rise time (t<sub>r</sub>) between 30 ns and 110% of the maximum
  allowed in the spec. e.g. 330 ns for Fast-mode
* fall time (t<sub>f</sub>) of approx 6 nanoseconds

## Design Decisions
### Master Mode Timings
#### f<sub>SCL</sub> SCL Clock Frequency
* the clock frequency must meet the I2C with the [minimum rise and fall times](#supported-i2c-modes) 

#### t<sub>LOW</sub> LOW Period of the SCL Clock
* the low period should be larger than the minimum period in order to increase
  t<sub>SU;DAT</sub>

#### t<sub>HIGH</sub> HIGH Period of the SCL Clock
* no additional requirements

### Slave Mode Timings
By default, slave mode uses the same set of timing parameters for all I2C modes.
This removes the need for the developer to specify the bus mode.

#### t<sub>HD;DAT</sub> Data Hold Time
* the nominal value must be > the maximum t<sub>f</sub> * 0.603 to avoid breaking
  the spec when SCL falls slowing and SDA falls very fast
* the nominal value should be > 140 ns to enable the BusRecorder to capture
  the SCL falling edge and the SDA edge as separate events

#### t<sub>VD;DAT</sub> Data Valid Time
* value is determined by requirements for t<sub>HD;DAT</sub> and t<sub>SU;DAT</sub>

#### t<sub>SU;DAT</sub> Data Setup Time
* should be > 140 ns to enable the BusRecorder to capture the SDA edge and
  the SCL rising edge as separate events

### LPI2C Functional Clock Frequency and PRESCALE
#### Background
In most cases, the `i.MX RT1062` register settings are multiplied by
the LPI2C functional clock period and  (2 ^ PRESCALE) to give a time in
nanoseconds. (PRESCALE is not use for filters).

A higher LPI2C functional clock frequency gives finer control of I2C timing.
A lower frequency gives a greater maximum range for these values.

#### Decision
Set the clock to 60 MHz in all cases. This is the fastest possible clock
speed. This decision gives the best precision for Fast-mode Plus (1 MHz).

Set PRESCALE separately for Standard, Fast and Fast-Mode Plus. PRESCALE is set
to the lowest value that can achieve the required timing values. This maximises
the precision of each I2C timing value.

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
