from unittest import TestCase

from i2c_timing_calculator.profile_test.i2c_specification import fast_mode_plus_spec, I2CSpecification, Range, \
    standard_mode_spec, fast_mode_spec
from i2c_timing_calculator.teensy_config import TeensyConfig, Parameter


# This test proves that default Slave configuration works with a Master
# that has tight timings.
class TestDefaultSlaveProfile(TestCase):
    bus_recorder_resolution = 140
    frequency = 60
    filtscl = 6
    filtsda = 6
    datavd = 3

    standard_mode: TeensyConfig = TeensyConfig(
        name="100 kHz - Standard-mode",
        scl_risetime=1100, sda_risetime=1100,
        falltime=4, max_fall=300, max_rise=1100,
        frequency=frequency, prescale=3,
        datavd=datavd, sethold=-1, busidle=-1,
        filtscl=filtscl, filtsda=filtsda,
        clkhi=35, clklo=35)

    fast_mode: TeensyConfig = TeensyConfig(
        name="400 kHz - Fast-mode",
        scl_risetime=330, sda_risetime=330,
        falltime=4, max_fall=300, max_rise=330,
        frequency=frequency, prescale=1,
        datavd=datavd, sethold=-1, busidle=-1,
        filtscl=filtscl, filtsda=filtsda,
        clkhi=39, clklo=39)

    fast_mode_plus: TeensyConfig = TeensyConfig(
        name="1 MHz - Fast-mode Plus",
        scl_risetime=132, sda_risetime=132,
        falltime=4, max_fall=120, max_rise=132,
        frequency=frequency, prescale=0,
        datavd=datavd, sethold=-1, busidle=-1,
        filtscl=filtscl, filtsda=filtsda,
        clkhi=30, clklo=30)

    def test_clock_low(self):
        def test(config: TeensyConfig, spec: I2CSpecification):
            self.assert_in_range(config.clock_low().worst_case, spec.scl_low_time)
        self.for_all_modes("tLOW", test)

    def test_spike_filter(self):
        def test(config: TeensyConfig, spec: I2CSpecification):
            # Meets I2C Specification
            self.assert_in_range(config.scl_glitch_filter(), spec.spike_width)
            self.assert_in_range(config.sda_glitch_filter(), spec.spike_width)
            # Meets design requirement
            design_spike_width = 100
            self.assertGreaterEqual(config.scl_glitch_filter(), design_spike_width)
            self.assertGreaterEqual(config.sda_glitch_filter(), design_spike_width)
        self.for_all_modes("tSP", test)

    def test_data_hold_time(self):
        def test(config: TeensyConfig, spec: I2CSpecification):
            # The worst case is the same whether SDA rises or falls
            worst_case = config.data_hold(False, True).worst_case
            nominal = config.data_hold(False, True).nominal
            # Meets I2C Specification
            self.assert_in_range(worst_case, spec.data_hold_time)
            # BusRecorder will record edges separately
            self.assertGreaterEqual(nominal, self.bus_recorder_resolution)
        self.for_all_modes("tHD;DAT", test)

    def test_data_setup_time(self):
        def test(config: TeensyConfig, spec: I2CSpecification):
            # Test with the shortest possible clock low time to get the worst case
            self.assertLessEqual(config.clock_low().worst_case, spec.scl_low_time.min * 1.05)
            # Meets I2C Specification
            self.assert_in_range(config.data_setup(False, True).worst_case, spec.data_setup_time)
            self.assert_in_range(config.data_setup(False, False).worst_case, spec.data_setup_time)
            # BusRecorder will record edges separately
            self.assertGreaterEqual(config.data_setup(False, True).worst_case, self.bus_recorder_resolution)
            self.assertGreaterEqual(config.data_setup(False, False).worst_case, self.bus_recorder_resolution)
        self.for_all_modes("tSU;DAT", test)

    def test_log_nominal_times(self):
        def print_parameter(description, parameter: Parameter):
            print(
                f"{description} I2C {parameter.i2c_value:.0f} nanos (Worst Case: {parameter.worst_case:.0f}, Nominal {parameter.nominal:.0f})")

        def test(config: TeensyConfig, spec: I2CSpecification):
            print(config.name)
            print_parameter(f"Clock Low Time (tLOW)", config.clock_low())
            print_parameter(f"Clock High Time (tHIGH)", config.clock_high())
            print_parameter(f"Data Hold Time (tHD:DAT) - Slave 1->0", config.data_hold(master=False, falling=True))
            # print_parameter(f"Data Valid Time (tVD:DAT) - Slave 1->0", config.data_valid(master=False, falling=True))
            print_parameter(f"Data Setup Time (tSU:DAT) - Slave 1->0", config.data_setup(master=False, falling=True))
            print(f"Glitch filters. SDA: {config.sda_glitch_filter():.0f} nanos. SCL: {config.scl_glitch_filter():.0f} nanos")
            print()

        self.for_all_modes("log times", test)

    def for_all_modes(self, message, test):
        for (mode, spec) in [
            [self.standard_mode, standard_mode_spec],
            [self.fast_mode, fast_mode_spec],
            [self.fast_mode_plus, fast_mode_plus_spec],
        ]:
            with self.subTest(msg=message, p1=mode, p2=spec):
                test(mode, spec)

    def assert_in_range(self, value: int, range: Range):
        self.assertGreaterEqual(value, range.min)
        self.assertLessEqual(value, range.max)
