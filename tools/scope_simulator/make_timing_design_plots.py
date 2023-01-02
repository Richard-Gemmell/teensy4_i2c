from typing import Callable

from i2c_trace import I2CTrace

V_IH = 0.7
V_IL = 0.3
HIGH = 1
LOW = 0
V_MID = 0.5
TEENSY_RISE = 0.538  # Rising edge trigger point on Teensy with hysteresis enabled
TEENSY_FALL = 0.462  # Falling edge trigger point on Teensy with hysteresis enabled


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
    trace.sda.set_rise_time(1000).set_fall_time(300)\
        .low().rise_at(0).fall_at(5500)
    trace.scl.hide()
    p = -500
    trace.add_voltage_measurement("HIGH", p, 0.7, 1)
    trace.add_voltage_measurement("Either", p, 0.3, 0.7)
    trace.add_voltage_measurement("LOW", p, 0, 0.3)
    trace.measure_between_edges("$t_{r}$ Rise Time", left=['SDA', 0, V_IL], right=['SDA', 0, V_IH], y_pos=0.2)
    trace.measure_between_edges("$t_{f}$", left=['SDA', 1, V_IH], right=['SDA', 1, V_IL], y_pos=0.2)
    return trace


def illustrate_different_edge_detection_points():
    trace = I2CTrace("Different Devices See Different Intervals", -1300, 7000)
    trace.ax.set_yticks([0.3, 0.5, 0.7], ["0.3", "0.5", "0.7"], minor=True)

    trace.sda.set_rise_time(1000).set_fall_time(300)\
        .high().fall_at(0)
    trace.scl.set_rise_time(1000).set_fall_time(300)\
        .low().rise_at(5000)
    p = -500
    # trace.add_voltage_measurement("HIGH", p, 0.7, 1)
    # trace.add_voltage_measurement("Either", p, 0.3, 0.7)
    # trace.add_voltage_measurement("LOW", p, 0, 0.3)
    trace.measure_between_edges("$t_{SU;DAT}$ I2C Spec", left=['SDA', 0, V_IL], right=['SCL', 0, V_IH], y_pos=0.8)
    trace.measure_between_edges("$t_{SU;DAT}$ Bus Recorder No Hysteresis", left=['SDA', 0, HIGH/2], right=['SCL', 0, HIGH/2], y_pos=0.60)
    trace.measure_between_edges("$t_{SU;DAT}$ teensy4_i2c", left=['SDA', 0, 0.462], right=['SCL', 0, 0.538], y_pos=0.45)
    trace.measure_between_edges("$t_{SU;DAT}$ Other Device Worst Case", left=['SDA', 0, V_IL], right=['SCL', 0, V_IL], y_pos=0.25)
    trace.measure_between_edges("$t_{SU;DAT}$ Other Device Best Case", left=['SDA', 0, V_IH], right=['SCL', 0, V_IH], y_pos=0.1)
    return trace


def setup_start_time() -> I2CTrace:
    trace = I2CTrace("$t_{SU;STA}$ - Setup Time for Repeated Start", -700, 6000)
    trace.sda.set_fall_time(150).high().fall_at(5500)
    trace.scl.set_rise_time(1000).low().rise_at(0)
    trace.measure_between_edges("$t_{SU;STA}$: I2C Spec", left=['SCL', 0, V_IH], right=['SDA', 0, V_IH], y_pos=0.70)
    trace.measure_between_edges("Δt: BusRecorder", left=['SCL', 0, V_MID], right=['SDA', 0, V_MID], y_pos=0.5)
    trace.measure_between_edges("Worst Case: Slow SCL, Fast SDA", left=['SCL', 0, V_IH], right=['SDA', 0, V_IH], y_pos=0.3)
    trace.measure_between_edges("Nominal: Datasheet", left=['SCL', 0, LOW], right=['SDA', 0, HIGH], y_pos=0.1)
    return trace


def hold_start_time() -> I2CTrace:
    trace = I2CTrace("$t_{HD;STA}$ - Hold Start Time", -600, 5000)
    trace.sda.set_fall_time(150).high().fall_at(0)
    trace.scl.set_fall_time(150).high().fall_at(4000)
    trace.measure_between_edges("$t_{SU;STO}$: I2C Spec", left=['SDA', 0, V_IL], right=['SCL', 0, V_IH], y_pos=0.7)
    trace.measure_between_edges("Δt: BusRecorder", left=['SDA', 0, V_MID], right=['SCL', 0, V_MID], y_pos=0.5)
    trace.measure_between_edges("Worst Case: Slow SDA, Fast SCL", left=['SDA', 0, V_IL], right=['SCL', 0, V_IH], y_pos=0.3)
    trace.measure_between_edges("Nominal: Datasheet", left=['SDA', 0, HIGH], right=['SCL', 0, HIGH], y_pos=0.1)
    return trace


def setup_stop_time() -> I2CTrace:
    trace = I2CTrace("$t_{SU;STO}$ - Setup Stop Time", -600, 5000)
    trace.sda.set_rise_time(200).low().rise_at(4000)
    trace.scl.set_rise_time(1000).low().rise_at(0)
    trace.measure_between_edges("$t_{SU;STO}$: I2C Spec", left=['SCL', 0, V_IH], right=['SDA', 0, V_IL], y_pos=0.70)
    trace.measure_between_edges("Δt: BusRecorder", left=['SCL', 0, V_MID], right=['SDA', 0, V_MID], y_pos=0.5)
    trace.measure_between_edges("Worst Case: Slow SCL, Fast SDA", left=['SCL', 0, V_IH], right=['SDA', 0, V_IL], y_pos=0.3)
    trace.measure_between_edges("Nominal: Datasheet", left=['SCL', 0, LOW], right=['SDA', 0, LOW], y_pos=0.1)
    return trace


def low_time() -> I2CTrace:
    trace = I2CTrace("$t_{LOW}$ - Low Period of SCL Clock", -700, 6500)
    trace.sda.hide()
    trace.scl.set_fall_time(150).high().fall_at(0).rise_at(5000)
    trace.measure_between_edges("$t_{LOW}$: I2C Spec", left=['SCL', 0, V_IL], right=['SCL', 1, V_IL], y_pos=0.70)
    trace.measure_between_edges("Δt: BusRecorder", left=['SCL', 0, V_MID], right=['SCL', 1, V_MID], y_pos=0.5)
    trace.measure_between_edges("Worst Case: Slow Fall, Fast Rise", left=['SCL', 0, V_IL], right=['SCL', 1, V_IL], y_pos=0.3)
    trace.measure_between_edges("Nominal: Datasheet", left=['SCL', 0, HIGH], right=['SCL', 1, LOW], y_pos=0.1)
    return trace


def high_time() -> I2CTrace:
    trace = I2CTrace("$t_{HIGH}$ - High Period of SCL Clock", -200, 6000)
    trace.sda.hide()
    trace.scl.set_fall_time(150).low().rise_at(0).fall_at(5000)
    trace.measure_between_edges("$t_{HIGH}$: I2C Spec", left=['SCL', 0, V_IH], right=['SCL', 1, V_IH], y_pos=0.70)
    trace.measure_between_edges("Δt: BusRecorder", left=['SCL', 0, V_MID], right=['SCL', 1, V_MID], y_pos=0.5)
    trace.measure_between_edges("Worst Case: Slow Rise, Fast Fall", left=['SCL', 0, V_IH], right=['SCL', 1, V_IH], y_pos=0.3)
    trace.measure_between_edges("Nominal: Datasheet", left=['SCL', 0, LOW], right=['SCL', 1, HIGH], y_pos=0.1)
    return trace


def data_bit_example() -> I2CTrace:
    trace = I2CTrace("Typical Data Bit", -200, 7500)
    trace.sda.set_rise_time(300).set_fall_time(10) \
        .high().fall_at(800).rise_at(5500)
    trace.scl.set_rise_time(300).set_fall_time(10)\
        .low().rise_at(1000).fall_at(5000)
    return trace


def frequency() -> I2CTrace:
    trace = I2CTrace("$f_{SCL}$ - SCL Clock Frequency", -800, 11000)
    trace.sda.hide()
    trace.scl.set_fall_time(150).set_rise_time(500)\
        .high().fall_at(0).rise_at(5000).fall_at(10000)
    trace.measure_between_edges("1/$f_{SCL}$: I2C Spec", left=['SCL', 0, V_IL], right=['SCL', 2, V_IL], y_pos=0.70)
    trace.measure_between_edges("Δt: BusRecorder", left=['SCL', 0, V_MID], right=['SCL', 2, V_MID], y_pos=0.5)
    trace.measure_between_edges("Nominal: Datasheet", left=['SCL', 0, HIGH], right=['SCL', 2, HIGH], y_pos=0.1)
    return trace


def bus_free() -> I2CTrace:
    trace = I2CTrace("$t_{BUF}$ - Bus Free Time", -1000, 8000)
    trace.sda.set_fall_time(150).set_rise_time(500)\
        .low().rise_at(0).fall_at(6700)
    # trace.scl.hide()
    trace.scl.set_fall_time(30).set_rise_time(100)\
        .low().rise_at(-500).fall_at(7500)
    trace.measure_between_edges("1/$t_{BUF}$: I2C Spec", left=['SDA', 0, V_IH], right=['SDA', 1, V_IH], y_pos=0.70)
    trace.measure_between_edges("Δt: BusRecorder", left=['SDA', 0, V_MID], right=['SDA', 1, V_MID], y_pos=0.5)
    trace.measure_between_edges("Worst Case: Slow Rise, Fast Fall", left=['SDA', 0, V_IH], right=['SDA', 1, V_IH], y_pos=0.3)
    trace.measure_between_edges("Nominal: Datasheet", left=['SDA', 0, LOW], right=['SDA', 1, HIGH], y_pos=0.1)
    return trace


def setup_data_time_low_to_high() -> I2CTrace:
    trace = I2CTrace("$t_{SU;DAT}$ - Setup Data Time (LOW to HIGH)", -10, 700)
    trace.sda.set_rise_time(50).set_fall_time(10) \
        .low().rise_at(50)
    trace.scl.set_rise_time(50).set_fall_time(10)\
        .low().rise_at(500)
    trace.measure_between_edges("$t_{SU;DAT}$: I2C Spec", left=['SDA', 0, V_IH], right=['SCL', 0, V_IL], y_pos=0.70)
    trace.measure_between_edges("Δt: BusRecorder", left=['SDA', 0, V_MID], right=['SCL', 0, V_MID], y_pos=0.5)
    trace.measure_between_edges("Worst Case: SDA Slow Rise, SCL Fast Rise", left=['SDA', 0, V_IH], right=['SCL', 0, V_IL], y_pos=0.3)
    trace.measure_between_edges("Nominal: Datasheet", left=['SDA', 0, LOW], right=['SCL', 0, LOW], y_pos=0.1)
    return trace


def setup_data_time_high_to_low() -> I2CTrace:
    trace = I2CTrace("$t_{SU;DAT}$ - Setup Data Time (HIGH to LOW)", -10, 700)
    trace.sda.set_rise_time(50).set_fall_time(10) \
        .high().fall_at(50)
    trace.scl.set_rise_time(50).set_fall_time(10)\
        .low().rise_at(500)
    trace.measure_between_edges("$t_{SU;DAT}$: I2C Spec", left=['SDA', 0, V_IL], right=['SCL', 0, V_IL], y_pos=0.70)
    trace.measure_between_edges("Δt: BusRecorder", left=['SDA', 0, V_MID], right=['SCL', 0, V_MID], y_pos=0.5)
    trace.measure_between_edges("Worst Case: SDA Slow Fall, SCL Fast Rise", left=['SDA', 0, V_IL], right=['SCL', 0, V_IL], y_pos=0.3)
    trace.measure_between_edges("Nominal: Datasheet", left=['SDA', 0, HIGH], right=['SCL', 0, LOW], y_pos=0.1)
    return trace


def data_hold_master_low_to_high() -> I2CTrace:
    trace = I2CTrace("$t_{HD;DAT}$ - Data Hold Time (Master LOW to HIGH)", -10, 700)
    trace.scl.set_fall_time(8).high().fall_at(50)
    trace.sda.set_rise_time(100).low().rise_at(500)
    trace.measure_between_edges("$t_{HD;DAT}$: I2C Spec", left=['SCL', 0, V_IL], right=['SDA', 0, V_IL], y_pos=0.70)
    trace.measure_between_edges("Δt: BusRecorder", left=['SCL', 0, V_MID], right=['SDA', 0, V_MID], y_pos=0.5)
    trace.measure_between_edges("Worst Case: SCL Slow Fall, SDA Fast Fall or Rise", left=['SCL', 0, V_IL], right=['SDA', 0, V_IL], y_pos=0.30)
    trace.measure_between_edges("Nominal: Datasheet", left=['SCL', 0, HIGH], right=['SDA', 0, LOW], y_pos=0.1)
    return trace


def data_hold_slave_high_to_low() -> I2CTrace:
    trace = I2CTrace("$t_{HD;DAT}$ - Data Hold Time (Slave HIGH to LOW)", -10, 1650)
    trace.scl.set_fall_time(100).high().fall_at(150)
    trace.sda.set_fall_time(30).high().fall_at(1200)
    trace.measure_between_edges("$t_{HD;DAT}$: I2C Spec", left=['SCL', 0, V_IL], right=['SDA', 0, V_IH], y_pos=0.70)
    trace.measure_between_edges("Δt: BusRecorder", left=['SCL', 0, V_MID], right=['SDA', 0, V_MID], y_pos=0.5)
    trace.measure_between_edges("Worst Case: SCL Slow Fall, SDA Fast Fall or Rise", left=['SCL', 0, V_IL], right=['SDA', 0, V_IH], y_pos=0.30)
    trace.measure_between_edges("Nominal: Datasheet", left=['SCL', 0, HIGH], right=['SDA', 0, HIGH], y_pos=0.1)
    return trace


def data_valid_master_low_to_high() -> I2CTrace:
    trace = I2CTrace("$t_{VD;DAT}$ - Data Valid Time (Master LOW to HIGH)", -10, 800)
    trace.scl.set_fall_time(8).high().fall_at(50)
    trace.sda.set_rise_time(100).low().rise_at(500)
    trace.measure_between_edges("$t_{VD;DAT}$: I2C Spec", left=['SCL', 0, V_IL], right=['SDA', 0, V_IH], y_pos=0.70)
    trace.measure_between_edges("Δt: BusRecorder", left=['SCL', 0, V_MID], right=['SDA', 0, V_MID], y_pos=0.5)
    trace.measure_between_edges("Worst Case: SDA Slow Rise, SCL Fast Fall or Rise", left=['SCL', 0, V_IL], right=['SDA', 0, V_IH], y_pos=0.30)
    trace.measure_between_edges("Nominal: Datasheet", left=['SCL', 0, HIGH], right=['SDA', 0, LOW], y_pos=0.1)
    return trace


def data_valid_slave_high_to_low() -> I2CTrace:
    trace = I2CTrace("$t_{VD;DAT}$ - Data Valid Time (Slave HIGH to LOW)", -10, 1650)
    trace.scl.set_fall_time(100).high().fall_at(150)
    trace.sda.set_fall_time(50).high().fall_at(1200)
    trace.measure_between_edges("$t_{VD;DAT}$: I2C Spec", left=['SCL', 0, V_IL], right=['SDA', 0, V_IL], y_pos=0.70)
    trace.measure_between_edges("Δt: BusRecorder", left=['SCL', 0, V_MID], right=['SDA', 0, V_MID], y_pos=0.5)
    trace.measure_between_edges("Worst Case: SCL Slow Fall, SDA Fast Fall or Rise", left=['SCL', 0, V_IL], right=['SDA', 0, V_IL], y_pos=0.30)
    trace.measure_between_edges("Nominal: Datasheet", left=['SCL', 0, HIGH], right=['SDA', 0, HIGH], y_pos=0.1)
    return trace


if __name__ == '__main__':
    output_dir = "../../documentation/i2c_design/images"
    # plot(illustrate_rise_and_fall_times, f"{output_dir}/rise_and_fall_times.png", show=False)
    # plot(illustrate_different_edge_detection_points, f"{output_dir}/different_intervals_for_different_devices.png", show=True)
    # plot(setup_start_time, f"{output_dir}/setup_start.png", show=True)
    # plot(hold_start_time, f"{output_dir}/hold_start.png", show=True)
    # plot(setup_stop_time, f"{output_dir}/setup_stop.png", show=True)
    # plot(low_time, f"{output_dir}/clock_low.png", show=True)
    # plot(high_time, f"{output_dir}/clock_high.png", show=True)
    # plot(frequency, f"{output_dir}/frequency.png", show=True)
    # plot(data_bit_example, f"{output_dir}/data_bit_example.png", show=False)
    # plot(bus_free, f"{output_dir}/bus_free.png", show=True)
    # plot(setup_data_time_low_to_high, f"{output_dir}/setup_data_low_to_high.png", show=True)
    # plot(setup_data_time_high_to_low, f"{output_dir}/setup_data_high_to_low.png", show=True)
    # plot(data_hold_master_low_to_high, f"{output_dir}/data_hold_master_low_to_high.png", show=True)
    # plot(data_hold_slave_high_to_low, f"{output_dir}/data_hold_slave_high_to_low.png", show=True)
    plot(data_valid_master_low_to_high, f"{output_dir}/data_valid_master_low_to_high.png", show=True)
    plot(data_valid_slave_high_to_low, f"{output_dir}/data_valid_slave_high_to_low.png", show=True)
