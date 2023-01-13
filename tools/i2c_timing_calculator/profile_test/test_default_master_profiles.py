from unittest import TestCase

from i2c_timing_calculator.profile_test.i2c_specification import fast_mode_plus_spec, I2CSpecification, Range, \
    standard_mode_spec, fast_mode_spec, UNLIMITED
from i2c_timing_calculator.teensy_config import TeensyConfig, Parameter


standard_mode_design = I2CSpecification(
    # output_fall_time=Range(0, 250),
    spike_width=Range(250, 251),
    frequency=Range(90_000, 100_000),
    # start_hold_time=Range(4_000, UNLIMITED),
    scl_low_time=Range(4_850, UNLIMITED),
    scl_high_time=Range(4_150, UNLIMITED),
    start_setup_time=Range(4_700, UNLIMITED),
    data_hold_time=Range(1000, 1100),
    data_setup_time=Range(2000, UNLIMITED),
    # rise_time=Range(0, 1_000),
    # fall_time=Range(0, 300),
    # stop_setup_time=Range(4_000, UNLIMITED),
    # bus_free_time=Range(4_700, UNLIMITED),
    data_valid_time=Range(0, 3_450),
)

fast_mode_design = I2CSpecification(
    # output_fall_time=Range(12, 250),
    spike_width=Range(250, 251),
    frequency=Range(360_000, 400_000),
    # start_hold_time=Range(600, UNLIMITED),
    scl_low_time=Range(1_400, UNLIMITED),
    scl_high_time=Range(700, UNLIMITED),
    start_setup_time=Range(600, UNLIMITED),
    data_hold_time=Range(390, 430),
    data_setup_time=Range(700, UNLIMITED),
    # rise_time=Range(0, 300),
    # fall_time=Range(12, 300),
    # stop_setup_time=Range(600, UNLIMITED),
    # bus_free_time=Range(1_300, UNLIMITED),
    data_valid_time=Range(0, 900),
)

fast_mode_plus_design = I2CSpecification(
    # output_fall_time=Range(12,120),
    spike_width=Range(100, 100),
    frequency=Range(900_000, 1_000_000),
    # start_hold_time=Range(260,UNLIMITED),
    scl_low_time=Range(600, UNLIMITED),
    scl_high_time=Range(310, UNLIMITED),
    start_setup_time=Range(260, UNLIMITED),
    data_hold_time=Range(200, 225),
    data_setup_time=Range(250, UNLIMITED),
    # rise_time=Range(0,120),
    # fall_time=Range(12,120),
    # stop_setup_time=Range(260,UNLIMITED),
    # bus_free_time=Range(500,UNLIMITED),
    data_valid_time=Range(0, 450),
)


class TestDefaultMasterProfiles(TestCase):
    fall_time = 8   # The master controls the fall time. We don't need to worry about high fall times.
    bus_recorder_resolution = 140
    frequency = 60

    standard_mode: TeensyConfig = TeensyConfig(
        name="100 kHz - Standard-mode",
        # scl_risetime=30, sda_risetime=30,
        scl_risetime=1100, sda_risetime=1100,
        falltime=fall_time, max_fall=fall_time, max_rise=1100,
        frequency=frequency, prescale=3,
        datavd=7, sethold=39, busidle=4,
        filtscl=15, filtsda=15,
        clkhi=34, clklo=37)

    fast_mode: TeensyConfig = TeensyConfig(
        name="400 kHz - Fast-mode",
        # scl_risetime=30, sda_risetime=30,
        scl_risetime=330, sda_risetime=330,
        falltime=fall_time, max_fall=fall_time, max_rise=330,
        frequency=frequency, prescale=1,
        datavd=11, sethold=32, busidle=4,
        filtscl=15, filtsda=15,
        clkhi=23, clklo=42)

    fast_mode_plus: TeensyConfig = TeensyConfig(
        name="1 MHz - Fast-mode Plus",
        # scl_risetime=30, sda_risetime=30,
        scl_risetime=132, sda_risetime=132,
        falltime=fall_time, max_fall=fall_time, max_rise=132,
        frequency=frequency, prescale=0,
        datavd=12, sethold=25, busidle=3,
        filtscl=6, filtsda=6,
        clkhi=14, clklo=36)

    def test_clock_low(self):
        def test(config: TeensyConfig, spec: I2CSpecification, design: I2CSpecification):
            self.assert_in_range(config.clock_low().worst_case, spec.scl_low_time)
            self.assert_in_range(config.clock_low().worst_case, design.scl_low_time)
        self.for_all_modes("tLOW", test)

    def test_clock_high(self):
        def test(config: TeensyConfig, spec: I2CSpecification, design: I2CSpecification):
            self.assert_in_range(config.clock_high().worst_case, spec.scl_high_time)
            self.assert_in_range(config.clock_high().worst_case, design.scl_high_time)
        self.for_all_modes("tHIGH", test)

    def test_frequency(self):
        def test(config: TeensyConfig, spec: I2CSpecification, design: I2CSpecification):
            self.assert_in_range(config.clock_frequency().worst_case, spec.frequency)
            self.assert_in_range(config.clock_frequency().worst_case, design.frequency)
        self.for_all_modes("tHIGH", test)

    def test_spike_filter(self):
        def test(config: TeensyConfig, spec: I2CSpecification, design: I2CSpecification):
            # Meets I2C Specification
            self.assert_in_range(config.scl_glitch_filter(), spec.spike_width)
            self.assert_in_range(config.sda_glitch_filter(), spec.spike_width)
            # Meets design requirement
            self.assert_in_range(config.scl_glitch_filter(), design.spike_width)
            self.assert_in_range(config.sda_glitch_filter(), design.spike_width)
        self.for_all_modes("tSP", test)

    def test_data_hold_time(self):
        def test(config: TeensyConfig, spec: I2CSpecification, design: I2CSpecification):
            # BusRecorder will record edges separately
            nominal = config.data_hold(master=True, falling=True).nominal
            self.assertGreaterEqual(nominal, self.bus_recorder_resolution)
            # The worst case is the same whether SDA rises or falls
            worst_case = config.data_hold(master=True, falling=True).worst_case
            # Meets I2C Specification
            self.assert_in_range(worst_case, spec.data_hold_time)
            # Meets design requirement
            self.assert_in_range(worst_case, design.data_hold_time)
        self.for_all_modes("tHD;DAT", test)

    def test_data_setup_time(self):
        def test(config: TeensyConfig, spec: I2CSpecification, design: I2CSpecification):
            rising_worst_case = config.data_setup(master=True, falling=True).worst_case
            falling_worst_case = config.data_setup(master=True, falling=False).worst_case
            # Meets I2C Specification
            self.assert_in_range(rising_worst_case, spec.data_setup_time)
            self.assert_in_range(falling_worst_case, spec.data_setup_time)
            # Meets design requirement
            self.assert_in_range(rising_worst_case, design.data_setup_time)
            self.assert_in_range(falling_worst_case, design.data_setup_time)
            # BusRecorder will record edges separately
            self.assertGreaterEqual(rising_worst_case, self.bus_recorder_resolution)
            self.assertGreaterEqual(falling_worst_case, self.bus_recorder_resolution)
        self.for_all_modes("tSU;DAT", test)

    def test_log_nominal_times(self):
        def print_parameter(description, parameter: Parameter):
            print(
                f"{description} I2C {parameter.i2c_value:.0f} nanos (Worst Case: {parameter.worst_case:.0f}, Nominal {parameter.nominal:.0f})")

        def test(config: TeensyConfig, spec: I2CSpecification, design: I2CSpecification):
            print(config.name)
            print_parameter(f"Clock Low Time (tLOW)", config.clock_low())
            print_parameter(f"Clock High Time (tHIGH)", config.clock_high())
            print_parameter(f"SCL Clock Frequency (fSCL)", config.clock_frequency())
            print_parameter(f"Data Hold Time (tHD:DAT) - Master 1->0", config.data_hold(master=True, falling=True))
            print_parameter(f"Data Hold Time (tHD:DAT) - Master 0->1", config.data_hold(master=True, falling=False))
            # print_parameter(f"Data Valid Time (tVD:DAT) - Master 1->0", config.data_valid(master=FaTruese, falling=True))
            print_parameter(f"Data Setup Time (tSU:DAT) - Master 1->0", config.data_setup(master=True, falling=True))
            print_parameter(f"Data Setup Time (tSU:DAT) - Master 0->1", config.data_setup(master=True, falling=False))
            print(f"Glitch filters. SDA: {config.sda_glitch_filter():.0f} nanos. SCL: {config.scl_glitch_filter():.0f} nanos")
            print()

        self.for_all_modes("log times", test)

    def for_all_modes(self, message, test):
        for (mode, spec, design) in [
            [self.standard_mode, standard_mode_spec, standard_mode_design],
            [self.fast_mode, fast_mode_spec, fast_mode_design],
            [self.fast_mode_plus, fast_mode_plus_spec, fast_mode_plus_design],
        ]:
            with self.subTest(msg=message, p1=mode, p2=spec, p3=design):
                test(mode, spec, design)

    def assert_in_range(self, value: int, range: Range):
        self.assertGreaterEqual(value, range.min)
        self.assertLessEqual(value, range.max)
