from typing import Callable

from i2c_trace import I2CTrace


def plot(probes: Callable[[], I2CTrace], filename: str, show: bool = False, save: bool = True):
    trace = probes()
    trace.set_events_from_edges()
    trace.plot()
    if save:
        trace.save(filename)
    if show:
        trace.show()


def illustrate_rise_and_fall_times():
    trace = I2CTrace("I2C Rise and Fall Times", -1300, 7000)
    fall_at = 5500
    trace.sda.set_rise_time(1000).set_fall_time(300)\
        .low().rise_at(0).fall_at(fall_at)
    trace.scl.hide()
    p = -500
    trace.add_voltage_measurement("HIGH", p, 0.7, 1)
    trace.add_voltage_measurement("Either", p, 0.3, 0.7)
    trace.add_voltage_measurement("LOW", p, 0, 0.3)
    trace.add_measurement("$t_{r}$ Rise Time", 0.2, 400, 1400)
    trace.add_measurement("$t_{f}$", 0.2, fall_at + 120, fall_at + 360)
    return trace


def setup_start_time() -> I2CTrace:
    start = -1000
    end = 7550
    trace = I2CTrace("$t_{SU;STA}$ - Setup Start Time", start, end)
    trace.sda.high().fall_at(0)
    trace.scl.high().fall_at(5330)
    trace.save("generated/clock_high.png")
    return trace


def high_time() -> I2CTrace:
    rise_at = 0
    fall_at = 5000
    trace = I2CTrace("$t_{HIGH}$ - High Period of SCL Clock", -200, 5500)
    trace.sda.hide()
    trace.scl.low().rise_at(0).fall_at(5000)
    trace.add_measurement("$t_{HIGH}$ Shortest", 0.6, 700, fall_at+30)
    trace.add_measurement("$t_{HIGH}$ Longest", 0.4, 300, fall_at+70)
    trace.add_measurement("$t_{HIGH}$ Nominal", 0.05, 0, fall_at, False)
    return trace


def data_bit_example() -> I2CTrace:
    trace = I2CTrace("Typical Data Bit", -200, 7500)
    trace.sda.set_rise_time(300).set_fall_time(10) \
        .high().fall_at(800).rise_at(5500)
    trace.scl.set_rise_time(300).set_fall_time(10)\
        .low().rise_at(1000).fall_at(5000)
    return trace


if __name__ == '__main__':
    output_dir = "generated";
    plot(illustrate_rise_and_fall_times, f"{output_dir}/rise_and_fall_times.png", show=False)
    plot(setup_start_time, f"{output_dir}/setup_start.png", show=False)
    plot(high_time, f"{output_dir}//clock_high.png", show=False)
    plot(data_bit_example, f"{output_dir}/data_bit_example.png", show=True)
    # TODO: measurements need calculated times for 0.3 and 0.7 Vdd
