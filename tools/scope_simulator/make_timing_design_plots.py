import math
from typing import Callable

from i2c_line import I2CLine
from i2c_trace import I2CTrace

SDA_1 = 0.985
SDA_0 = 0.015
SCL_1 = 1
SCL_0 = 0
tFall = 100
tRise = 1000


def setup_start_time() -> I2CTrace:
    start = -1000
    end = 7550
    trace = I2CTrace("$t_{SU;STA}$ - Setup Start Time", start, end)
    trace.sda.high().fall_at(0)
    trace.scl.high().fall_at(5330)
    trace.set_events([0, 5330], ['start', 'tSU;STA'])
    trace.save("generated/clock_high.png")
    return trace


def high_time() -> I2CTrace:
    rise_at = 0
    fall_at = 5000
    trace = I2CTrace("$t_{HIGH}$ - High Period of SCL Clock", -200, 5500)
    trace.sda.hide()
    trace.scl.low().rise_at(0).fall_at(5000)
    trace.set_events([rise_at, fall_at], ['↑', '↓'])
    trace.add_measurement("$t_{HIGH}$ Shortest", 0.6, 700, fall_at+30)
    trace.add_measurement("$t_{HIGH}$ Longest", 0.4, 300, fall_at+70)
    trace.add_measurement("$t_{HIGH}$ Nominal", 0.05, 0, fall_at, False)
    return trace


# def example() -> I2CTrace:
#     start = -200
#     end = 5500
#     trace = I2CTrace("$t_{HIGH}$ - High Period of SCL Clock", start, end)
#     rise_at = 0
#     fall_at = 5000
#     trace.trace_scl_deprecated([SCL_0, SCL_0, SCL_1, SCL_1, SCL_0, SCL_0], [start, rise_at, rise_at + tRise, fall_at, fall_at + tFall, end])
#     trace.set_events([rise_at, fall_at], ['↑', '↓'])
#     trace.add_voltage_measurement("HIGH", 1300, 0.7, 1)
#     trace.add_voltage_measurement("HIGH or LOW", 1300, 0.3, 0.7)
#     trace.add_voltage_measurement("LOW", 1300, 0, 0.3)
#     trace.add_measurement("$t_{HIGH}$ Shortest", 0.6, 700, fall_at+30)
#     trace.add_measurement("$t_{HIGH}$ Longest", 0.4, 300, fall_at+70)
#     trace.add_measurement("$t_{HIGH}$ Nominal", 0.05, 0, fall_at, False)
#     return trace


def data_bit_example() -> I2CTrace:
    trace = I2CTrace("$t_{HIGH}$ - High Period of SCL Clock", -200, 7500)
    trace.sda.tRise(300).tFall(10) \
        .high().fall_at(800).rise_at(5500)
    trace.scl.tRise(300).tFall(10)\
        .low().rise_at(1000).fall_at(5000)
    trace.set_events([800, 1000, 5000, 5500], ['↓', '↑', '↓', '↑'])
    return trace


def plot(probes: Callable[[], I2CTrace], filename: str, show: bool = False, save: bool = True):
    trace = probes()
    trace.plot()
    if save:
        trace.save(filename)
    if show:
        trace.show()


if __name__ == '__main__':
    output_dir = "generated";
    # plot(setup_start_time, f"{output_dir}/setup_start.png", show=True)
    plot(high_time, f"{output_dir}//clock_high.png", show=True)
    # plot(data_bit_example, f"{output_dir}/data_bit_example.png", show=True)

