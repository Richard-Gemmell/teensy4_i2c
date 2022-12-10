from i2c_timing_calculator.teensy_config import Parameter
from i2c_timing_calculator.test.timing_test_base import TimingTestBase


class TestI2CTimingCalculator(TimingTestBase):
    def test_start_hold(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'prescale': 1,
             'measured': Parameter(i2c=5320, nominal=5330, worst_case=5320),
             'expected': Parameter(i2c=5325, nominal=5333, worst_case=5325)},
            {'prescale': 2,
             'measured': Parameter(i2c=10660, nominal=10680, worst_case=10660),
             'expected': Parameter(i2c=10658, nominal=10666, worst_case=10658)},
        ]
        for t in test_cases:
            config = self.build_config(prescale=t['prescale'], scl_risetime=1384, sda_risetime=32)
            actual = config.start_hold()
            self.assert_i2c_nominal_equal(actual, t)

    def test_clock_low_depends_on_scl_rise_time(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'scl_rise': 1384,
             'measured': Parameter(i2c=5600, nominal=5000, worst_case=4988),
             'expected': Parameter(i2c=5571, nominal=5000, worst_case=4988)},
            {'scl_rise': 434,
             'measured': Parameter(i2c=5150, nominal=5000, worst_case=4988),
             'expected': Parameter(i2c=5171, nominal=5000, worst_case=4988)},
            {'scl_rise': 182,
             'measured': Parameter(i2c=5060, nominal=5020, worst_case=4988),
             'expected': Parameter(i2c=5065, nominal=5000, worst_case=4988)},
            {'scl_rise': 31,
             'measured': Parameter(i2c=4990, nominal=5000, worst_case=4988),
             'expected': Parameter(i2c=5001, nominal=5000, worst_case=4988)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=t['scl_rise'], sda_risetime=32)
            actual = config.clock_low()
            self.assert_i2c_nominal_equal(actual, t)

    def test_clock_low_depends_on_prescale(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'prescale': 0,
             'measured': Parameter(i2c=3084, nominal=2510, worst_case=2488),
             'expected': Parameter(i2c=3071, nominal=2500, worst_case=2488)},
            {'prescale': 2,
             'measured': Parameter(i2c=10580, nominal=10000, worst_case=9988),
             'expected': Parameter(i2c=10571, nominal=10000, worst_case=9988)},
        ]
        for t in test_cases:
            config = self.build_config(prescale=t['prescale'], scl_risetime=1384, sda_risetime=32)
            actual = config.clock_low()
            self.assert_i2c_nominal_equal(actual, t)

    def test_clock_high_depends_on_scl_rise_time(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'scl_rise': 1384,
             'measured': Parameter(i2c=4180, nominal=6150, worst_case=4356),
             'expected': Parameter(i2c=4203, nominal=6166, worst_case=4356)},
            {'scl_rise': 434,
             'measured': Parameter(i2c=4640, nominal=5240, worst_case=4356),
             'expected': Parameter(i2c=4719, nominal=5333, worst_case=4356)},
            {'scl_rise': 182,
             'measured': Parameter(i2c=4830, nominal=5080, worst_case=4356),
             'expected': Parameter(i2c=4828, nominal=5083, worst_case=4356)},
            {'scl_rise': 31,
             'measured': Parameter(i2c=4880, nominal=4920, worst_case=4356),
             'expected': Parameter(i2c=4875, nominal=4916, worst_case=4356)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=t['scl_rise'], sda_risetime=32)
            actual = config.clock_high()
            self.assert_i2c_nominal_equal(actual, t)

    def test_clock_high_depends_on_prescale(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'prescale': 0,
             'measured': Parameter(i2c=1910, nominal=3860, worst_case=2065),
             'expected': Parameter(i2c=1911, nominal=3875, worst_case=2065)},
            {'prescale': 2,
             'measured': Parameter(i2c=8860, nominal=10820, worst_case=8940),
             'expected': Parameter(i2c=8870, nominal=10833, worst_case=8940)},
        ]
        for t in test_cases:
            config = self.build_config(prescale=t['prescale'], scl_risetime=1384, sda_risetime=32)
            actual = config.clock_high()
            self.assert_i2c_nominal_equal(actual, t)

    def test_clock_high_depends_on_filtscl(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'filtscl': 9,
             'measured': Parameter(i2c=4350, nominal=6340, worst_case=4523),
             'expected': Parameter(i2c=4370, nominal=6333, worst_case=4523)},
            # ROUNDDOWN ensures that FILTSCL 8 is identical to FILTSCL 9
            {'filtscl': 8,
             'measured': Parameter(i2c=4360, nominal=6330, worst_case=4523),
             'expected': Parameter(i2c=4370, nominal=6333, worst_case=4523)},
            {'filtscl': 1,
             'measured': Parameter(i2c=4000, nominal=6000, worst_case=4190),
             'expected': Parameter(i2c=4036, nominal=6000, worst_case=4190)},
            # ROUNDDOWN ensures that FILTSCL 0 is identical to FILTSCL 1
            {'filtscl': 0,
             'measured': Parameter(i2c=3990, nominal=6000, worst_case=4190),
             'expected': Parameter(i2c=4036, nominal=6000, worst_case=4190)},
        ]
        for t in test_cases:
            config = self.build_config(filtscl=t['filtscl'], scl_risetime=1384, sda_risetime=32)
            actual = config.clock_high()
            self.assert_i2c_nominal_equal(actual, t)

    def test_clock_frequency_depends_on_scl_rise_time(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'scl_rise': 1384,
             'measured': Parameter(i2c=89605, nominal=89605, worst_case=100840),
             'expected': Parameter(i2c=89552, nominal=89552, worst_case=100840)},
            {'scl_rise': 434,
             'measured': Parameter(i2c=97465, nominal=97465, worst_case=100840),
             'expected': Parameter(i2c=96774, nominal=96774, worst_case=100840)},
            {'scl_rise': 182,
             'measured': Parameter(i2c=99206, nominal=99206, worst_case=100840),
             'expected': Parameter(i2c=99173, nominal=99173, worst_case=100840)},
            {'scl_rise': 31,
             'measured': Parameter(i2c=100806, nominal=100806, worst_case=100840),
             'expected': Parameter(i2c=100840, nominal=100840, worst_case=100840)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=t['scl_rise'], sda_risetime=32)
            actual = config.clock_frequency()
            self.assert_i2c_nominal_equal(actual, t, delta=1000)

    def test_clock_frequency_depends_on_prescale(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'prescale': 0,
             'measured': Parameter(i2c=156740, nominal=156740, worst_case=195121),
             'expected': Parameter(i2c=156862, nominal=156862, worst_case=195121)},
            {'prescale': 2,
             'measured': Parameter(i2c=48031, nominal=48031, worst_case=51282),
             'expected': Parameter(i2c=48000, nominal=48000, worst_case=51282)},
        ]
        for t in test_cases:
            config = self.build_config(prescale=t['prescale'], scl_risetime=1384, sda_risetime=32)
            actual = config.clock_frequency()
            self.assert_i2c_nominal_equal(actual, t, delta=1000)

    def test_clock_frequency_depends_on_filtscl(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'filtscl': 9,
             'measured': Parameter(i2c=88339, nominal=88339, worst_case=99173),
             'expected': Parameter(i2c=88235, nominal=88235, worst_case=99173)},
            # ROUNDDOWN ensures that FILTSCL 8 is identical to FILTSCL 9
            {'filtscl': 8,
             'measured': Parameter(i2c=88339, nominal=88339, worst_case=99173),
             'expected': Parameter(i2c=88235, nominal=88235, worst_case=99173)},
            {'filtscl': 1,
             'measured': Parameter(i2c=90909, nominal=90909, worst_case=102564),
             'expected': Parameter(i2c=90909, nominal=90909, worst_case=102564)},
            # ROUNDDOWN ensures that FILTSCL 0 is identical to FILTSCL 1
            {'filtscl': 0,
             'measured': Parameter(i2c=90909, nominal=90909, worst_case=102564),
             'expected': Parameter(i2c=90909, nominal=90909, worst_case=102564)},
        ]
        for t in test_cases:
            config = self.build_config(filtscl=t['filtscl'], scl_risetime=1384, sda_risetime=32)
            actual = config.clock_frequency()
            self.assert_i2c_nominal_equal(actual, t, delta=1000)

    def test_setup_repeated_start_depends_on_scl_rise_time(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'scl_rise': 1384,
             'measured': Parameter(i2c=4860, nominal=6830, worst_case=5023),
             'expected': Parameter(i2c=4870, nominal=6833, worst_case=5023)},
            {'scl_rise': 434,
             'measured': Parameter(i2c=5330, nominal=5910, worst_case=5023),
             'expected': Parameter(i2c=5386, nominal=6000, worst_case=5023)},
            {'scl_rise': 182,
             'measured': Parameter(i2c=5510, nominal=5740, worst_case=5023),
             'expected': Parameter(i2c=5494, nominal=5750, worst_case=5023)},
            {'scl_rise': 31,
             'measured': Parameter(i2c=5550, nominal=5590, worst_case=5023),
             'expected': Parameter(i2c=5542, nominal=5583, worst_case=5023)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=t['scl_rise'], sda_risetime=32)
            actual = config.setup_repeated_start()
            self.assert_i2c_nominal_equal(actual, t)

    def test_setup_repeated_start_depends_on_filtscl(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'filtscl': 5,
             'measured': Parameter(i2c=4860, nominal=6830, worst_case=5023),
             'expected': Parameter(i2c=4870, nominal=6833, worst_case=5023)},
            # ROUNDDOWN ensures that FILTSCL 4 is identical to FILTSCL 5
            {'filtscl': 4,
             'measured': Parameter(i2c=4830, nominal=6820, worst_case=5023),
             'expected': Parameter(i2c=4870, nominal=6833, worst_case=5023)},
            {'filtscl': 1,
             'measured': Parameter(i2c=4680, nominal=6660, worst_case=4856),
             'expected': Parameter(i2c=4703, nominal=6666, worst_case=4856)},
            # ROUNDDOWN ensures that FILTSCL 0 is identical to FILTSCL 1
            {'filtscl': 0,
             'measured': Parameter(i2c=4680, nominal=6660, worst_case=4856),
             'expected': Parameter(i2c=4703, nominal=6666, worst_case=4856)},
        ]
        for t in test_cases:
            config = self.build_config(filtscl=t['filtscl'], scl_risetime=1384, sda_risetime=32)
            actual = config.setup_repeated_start()
            self.assert_i2c_nominal_equal(actual, t)

    def test_setup_repeated_start_depends_on_prescale(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'prescale': 1,
             'measured': Parameter(i2c=4860, nominal=6830, worst_case=5023),
             'expected': Parameter(i2c=4870, nominal=6833, worst_case=5023)},
            {'prescale': 2,
             'measured': Parameter(i2c=10180, nominal=12160, worst_case=10273),
             'expected': Parameter(i2c=10203, nominal=12166, worst_case=10273)},
        ]
        for t in test_cases:
            config = self.build_config(prescale=t['prescale'], scl_risetime=1384, sda_risetime=32)
            actual = config.setup_repeated_start()
            self.assert_i2c_nominal_equal(actual, t)

    def test_stop_setup_depends_on_scl_rise_time(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'scl_rise': 1384,
             'measured': Parameter(i2c=4880, nominal=6840, worst_case=5020),
             'expected': Parameter(i2c=4880, nominal=6833, worst_case=5020)},
            {'scl_rise': 434,
             'measured': Parameter(i2c=5340, nominal=5920, worst_case=5020),
             'expected': Parameter(i2c=5396, nominal=6000, worst_case=5020)},
            {'scl_rise': 182,
             'measured': Parameter(i2c=5520, nominal=5750, worst_case=5020),
             'expected': Parameter(i2c=5504, nominal=5750, worst_case=5020)},
            {'scl_rise': 31,
             'measured': Parameter(i2c=5560, nominal=5570, worst_case=5020),
             'expected': Parameter(i2c=5552, nominal=5583, worst_case=5020)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=t['scl_rise'], sda_risetime=32)
            actual = config.stop_setup()
            self.assert_i2c_nominal_equal(actual, t)

    def test_stop_setup_depends_on_sda_rise_time(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1420,
             'measured': Parameter(i2c=6160, nominal=5590, worst_case=5020),
             'expected': Parameter(i2c=6135, nominal=5583, worst_case=5020)},
            {'sda_rise': 444,
             'measured': Parameter(i2c=5720, nominal=5600, worst_case=5020),
             'expected': Parameter(i2c=5724, nominal=5583, worst_case=5020)},
            {'sda_rise': 188,
             'measured': Parameter(i2c=5620, nominal=5580, worst_case=5020),
             'expected': Parameter(i2c=5617, nominal=5583, worst_case=5020)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, sda_risetime=t['sda_rise'])
            actual = config.stop_setup()
            self.assert_i2c_nominal_equal(actual, t)

    def test_stop_setup_depends_on_filtscl(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'filtscl': 5,
             'measured': Parameter(i2c=4880, nominal=6840, worst_case=5020),
             'expected': Parameter(i2c=4880, nominal=6833, worst_case=5020)},
            # ROUNDDOWN ensures that FILTSCL 4 is identical to FILTSCL 5
            {'filtscl': 4,
             'measured': Parameter(i2c=4880, nominal=6840, worst_case=5020),
             'expected': Parameter(i2c=4880, nominal=6833, worst_case=5020)},
            {'filtscl': 1,
             'measured': Parameter(i2c=4730, nominal=6660, worst_case=4853),
             'expected': Parameter(i2c=4713, nominal=6666, worst_case=4853)},
            # ROUNDDOWN ensures that FILTSCL 0 is identical to FILTSCL 1
            {'filtscl': 0,
             'measured': Parameter(i2c=4730, nominal=6660, worst_case=4853),
             'expected': Parameter(i2c=4713, nominal=6666, worst_case=4853)},
        ]
        for t in test_cases:
            config = self.build_config(filtscl=t['filtscl'], scl_risetime=1384, sda_risetime=32)
            actual = config.stop_setup()
            self.assert_i2c_nominal_equal(actual, t)

    def test_stop_setup_depends_on_prescale(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'prescale': 1,
             'measured': Parameter(i2c=4880, nominal=6840, worst_case=5020),
             'expected': Parameter(i2c=4880, nominal=6833, worst_case=5020)},
            {'prescale': 2,
             'measured': Parameter(i2c=10220, nominal=12180, worst_case=10270),
             'expected': Parameter(i2c=10213, nominal=12166, worst_case=10270)},
        ]
        for t in test_cases:
            config = self.build_config(prescale=t['prescale'], scl_risetime=1384, sda_risetime=32)
            actual = config.stop_setup()
            self.assert_i2c_nominal_equal(actual, t)
