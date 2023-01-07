# Teensy I2C Design

## I2C Signal Timing Design
This section describes how I've decided to interpret the I2C Specification
in order to choose various I2C timings. It's written in terms of the I2C
spec, not the Teensy's configuration.

The next section converts these decisions into configuration values for the
Teensy's I2C hardware.

### t<sub>HD;STA</sub>
Hold time for START and repeated START. See [I2C Timing Registers](i2c_timing_registers.old.md#tsubhdstasub).

The I2C Specification sets a minimum hold time. Devices **must not** interpret
shorter hold times as a START. Longer times allow devices more time
to notice that a START has occurred.

## Teensy I2C Configuration
This section describes how I've converted the [I2C Signal Timing Design](#i2c-signal-timing-design)
above into the values that'll be used to configure the Teensy's hardware.

### SETHOLD - Master
SETHOLD controls the START hold time, [t<sub>HD;STA</sub>](i2c_timing_registers.old.md#tsubhdstasub),
and the setup STOP time, [t<sub>SU;STO</sub>](i2c_timing_registers.old.md#tsubsustosub).

#### t<sub>HD;STA</sub> - hold time for START condition
The master sends a START condition by dropping SDA whilst SCL is HIGH.
t<sub>HD;STA</sub> is the delay between SDA going LOW and SDA ceasing to
be HIGH. i.e. the time between dropping to 0.3 V<sub>dd</sub> and SCL
reaching 0.7 V<sub>dd</sub>.

![Start Hold Time (Standard Mode)](images/start_hold.png)

* start with the minimum value from the I2C Specification
* add 25% of the minimum to give other I2C devices a nice window to react
* assume the fall time for SDA reduces the window so add the maximum
  allowed fall time

| Frequency | Min Setup Time | 25%   | Max Fall Time | Target Total |
|-----------|----------------|-------|---------------|--------------|
| 100 kHz   | 4'000          | 1'000 | 300           | 5'300        |
| 400 kHz   | 600            | 150   | 300           | 1'050        |
| 1 MHz     | 260            | 65    | 120           | 445          |

#### t<sub>SU;STA</sub> - setup time for a repeated START
* the worst case scenario is that the SCL rise time is very long
  and the SDA fall time is very short
* start with the minimum value from the I2C Specification
* add 25% of the minimum to give other I2C devices a nice window to react
* add 60% of SCL t<sub>r</sub> to allow for Teensy trigger voltage
* make no allowance for SDA fall time in case it's very short

| Frequency | Min Setup Time | 25%   | SCL Rise | Target Total | Min   |
|-----------|----------------|-------|----------|--------------|-------|
| 100 kHz   | 4'700          | 1'175 | 600      | 6'475        | 5'875 |
| 400 kHz   | 600            | 150   | 180      | 930          | 750   |
| 1 MHz     | 260            | 65    | 72       | 397          | 325   |

For a STOP setup time:
* the worst case scenario is that the SCL rise time is very long
  and the SDA rise time is very short
* start with the minimum value from the I2C Specification
* add 60% of SCL t<sub>r</sub> to allow for Teensy trigger voltage
* make no allowance for SDA rise in case it's very short

| Frequency | Min Setup Time | 25%   | SCL Rise | Target Total | Min   |
|-----------|----------------|-------|----------|--------------|-------|
| 100 kHz   | 4'000          | 1'000 | 600      | 5'600        | 5'000 |
| 400 kHz   | 600            | 150   | 180      | 930          | 750   |
| 1 MHz     | 260            | 65    | 72       | 397          | 325   |

