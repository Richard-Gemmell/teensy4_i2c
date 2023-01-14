# I2C Configuration Design
There are lots of possible configurations for the Teensy 4's I2C hardware
that would work. This document describes the configuration that ships
by default with the teensy4_i2c driver.

It describes the reasons for the design as well as the actual settings that
are used.

[I2C Timing on the i.MX RT1062 (Teensy 4)](i2c_timing_analysis.md) describes
the various settings and their effect on I2C.

# Goals and Principals
## Design Goals
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

## Timing Requirements
It's possible that SDA and SCL have very different rise and fall times.
In every case, the driver will meet the specification with the worst
possible combination.

For example, the worst case for the setup time for a repeated START t<sub>SU;STA</sub>
occurs when the SCL rise time is very long and the SDA fall time is very short.
In this case, the driver meets the I2C Specification even when SCL has the maximum
allowed rise time and SDA has a fall time of 0 nanoseconds.

## Supported I2C Modes
| I2C Mode       | Speed   | Min Rise Time | Max Rise Time | Min Fall Time | Max Fall Time (1) |
|----------------|---------|---------------|---------------|---------------|-------------------|
| Standard-mode  | 100 kHz | 30 ns         | 1000 ns       | 0             | 300               |
| Fast-mode      | 400 kHz | 20 ns         | 300 ns        | 0             | 300               |
| Fast-mode Plus | 1 MHz   | 10 ns         | 120 ns        | 0             | 120               |

1. The Teensy controls the fall time of SCL when it's the master. In this case
   the maximum fall time is 10 nanoseconds.

## Test Conditions
The driver will be tested with:
* rise time (t<sub>r</sub>) between 30 ns and 110% of the maximum
  allowed in the spec. e.g. 330 ns for Fast-mode
* fall time (t<sub>f</sub>) of approx 6 nanoseconds

# Design Decisions
## LPI2C Functional Clock Frequency and PRESCALE
### Background
In most cases, the `i.MX RT1062` register settings are multiplied by
the LPI2C functional clock period and  (2 ^ PRESCALE) to give a time in
nanoseconds. (PRESCALE is not use for filters).

A higher LPI2C functional clock frequency gives finer control of I2C timing.
A lower frequency gives a greater maximum range for these values.

### Decision
Set the clock to 60 MHz in all cases. This is the fastest possible clock
speed. This decision gives the best precision for Fast-mode Plus (1 MHz).

Set PRESCALE separately for Standard, Fast and Fast-Mode Plus. PRESCALE is set
to the lowest value that can achieve the required timing values. This maximises
the precision of each I2C timing value.

## Glitch Filters
### Background
The I2C Specification requires devices to suppress spikes (AKA glitches)
in Fast-mode and Fast-mode Plus. Spike suppression is not required in Standard
mode.

t<sub>SP</sub> is defined as having min and max values of 0 and 50 nanoseconds.
I've interpreted this means that all spikes up to 50 nanoseconds must be suppressed.
(An alternative interpretation would be that spike filters must be less than
50 nanoseconds in these modes.)

I'm not aware of any disadvantages for filtering larger spikes except that it
increases the latency of the `i.MX RT1062` I2C implementation.

### Decision
Spike suppression is provided by the `i.MX RT1062` glitch filter. These are
controlled by the FILTSCL and FILTSDA registers. These will be enabled in all
modes.

The glitch filters will be no more than 10% of the nominal clock period. This is
to ensure that they don't introduce any appreciable latency. There isn't a strong
reason for picking 10% specifically.

The SDA and SCL filters will have the same value as there's no reason to make
them different.

## Master Mode Timings

### SCL Clock Frequency
#### f<sub>SCL</sub> SCL Clock Frequency
* the clock frequency must meet the I2C with the [minimum rise and fall times](#supported-i2c-modes) 

#### t<sub>LOW</sub> LOW Period of the SCL Clock
* the low period should be larger than the minimum period in order to increase
  t<sub>SU;DAT</sub>

#### t<sub>HIGH</sub> HIGH Period of the SCL Clock
* no additional requirements

### Start and Stop Conditions
All 3 START and STOP condition times depend on SETHOLD, so they need to be
tuned together.

There's no reason I know of to make these times significantly longer than
the specification. Keeping them short reduces latency for an I2C message.

#### t<sub>SU;STA</sub> Setup Time for a Repeated START Condition
* All times will be between 120% and 150% of the minimum value in the
  I2C Specification for the worst case.

#### t<sub>HD;STA</sub> Hold Time for a START or Repeated START Condition
* All times will be between 120% and 150% of the minimum value in the
  I2C Specification for the worst case.

#### t<sub>SU;STO</sub> Setup Time for STOP Condition
* All times will be between 120% and 150% of the minimum value in the
  I2C Specification for the worst case.

#### t<sub>BUF</sub> Minimum Bus Free Time Between a STOP and START Condition
I'm not aware of any reason to make the bus free time significantly longer than
the minimum required by the I2C Specification. As such, I chose to make it
slightly larger than the minimum in the worst case to ensure that the
specification is met.

The bus free time is affected by the clock low period. This can make the bus free
time quite large at higher clock speeds. This is acceptable.

BUSIDLE must be > 0 to ensure that the `i.MX RT1062` recovers if another device
fails to end a transaction but leaves the bus free. See `BusRecoveryTest`.

* BUSIDLE must be > 0
* value will be at least 25% larger than the minimum required for the worst case scenario
* value will be no larger than required
* I won't reduce the clock low time (t<sub>LOW</sub>) in order to reduce the bus
  free time

### Data Bits
#### t<sub>HD;DAT</sub> Data Hold Time
* the nominal value should be > 140 ns to enable the BusRecorder to capture
  the SCL falling edge and the SDA edge as separate events
* t<sub>SU;DAT</sub> is more critical than t<sub>HD;DAT</sub> so timings should
  be more generous to t<sub>HD;DAT</sub>

I've picked the following target values to meet these design requirements. The
times are very arbitrary.

| Mode           | Nominal Clock Period (ns) | Worst Case Data Hold Time (ns) |
|----------------|---------------------------|--------------------------------|
| Standard-mode  | 10,000                    | 1000                           |
| Fast-mode      | 2,500                     | 400                            |
| Fast-mode Plus | 1,000                     | 200                            |

#### t<sub>VD;DAT</sub> Data Valid Time
Data valid time is constrained by the data hold time (t<sub>HD;DAT</sub>) and
the data setup time (t<sub>SU;DAT</sub>). There is no need to design it directly.

#### t<sub>SU;DAT</sub> Data Setup Time
* nominal should be > 140 ns to enable the BusRecorder to capture the SDA edge
  and the SCL rising edge as separate events
* the worst case will be at lest 2x the time required by the spec as there's no
  reason to have a very tight time

### ACKs and Spikes
#### t<sub>VD;ACK</sub> Data Valid Acknowledge Time
Requirements are identical to those for t<sub>VD;DAT</sub>.

#### t<sub>SP</sub> Pulse Width of Spikes that must be Suppressed by the Input Filter
* the glitch filters will be enabled in all modes
* the SDA and SCL filters will be set to the same value
* the glitch filters will be <= 10% of the nominal clock period
* the maximum glitch filter for a 60 MHz clock is 250 nanoseconds. This limits
  the size of the glitch filter for standard mode.

| Mode           | Nominal Clock Period (ns) | Glitch Filter (ns) |
|----------------|---------------------------|--------------------|
| Standard-mode  | 10,000                    | 250                |
| Fast-mode      | 2,500                     | 250                |
| Fast-mode Plus | 1,000                     | 100                |

## Slave Mode Timings
Slave mode uses the same set of timing parameters for all I2C modes.
This removes the need for the developer to specify the I2C bus mode.

### Data Bits
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
* I2C Specification must be met with the shortest possible clock low period,
  t<sub>LOW</sub> not just the clock low period set for the Teensy master mode

### ACKs and Spikes
#### t<sub>VD;ACK</sub> Data Valid Acknowledge Time
Requirements are identical to those for t<sub>VD;DAT</sub>.

#### t<sub>SP</sub> Pulse Width of Spikes that must be Suppressed by the Input Filter
* the glitch filters will be enabled in all modes
* the SDA and SCL filters will be set to the same value
* set the glitch filters to 100 nanoseconds as this is 10% of the nominal
  Fast-mode Plus clock period

~~~~~~~~~~~~~~~~~
### SCL Clock Frequency
#### f<sub>SCL</sub> SCL Clock Frequency
#### t<sub>LOW</sub> LOW Period of the SCL Clock
#### t<sub>HIGH</sub> HIGH Period of the SCL Clock

### Start and Stop Conditions
#### t<sub>SU;STA</sub> Setup Time for a Repeated START Condition
#### t<sub>HD;STA</sub> Hold Time for a START or Repeated START Condition
#### t<sub>SU;STO</sub> Setup Time for STOP Condition
#### t<sub>BUF</sub> Minimum Bus Free Time Between a STOP and START Condition

### Data Bits
#### t<sub>SU;DAT</sub> Data Setup Time
#### t<sub>HD;DAT</sub> Data Hold Time
#### t<sub>VD;DAT</sub> Data Valid Time

### ACKs and Spikes
#### t<sub>VD;ACK</sub> Data Valid Acknowledge Time
#### t<sub>SP</sub> Pulse Width of Spikes that must be Suppressed by the Input Filter
