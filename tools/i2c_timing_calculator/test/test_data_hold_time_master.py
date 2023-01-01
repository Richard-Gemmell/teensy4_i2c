from i2c_timing_calculator.teensy_config import Parameter
from i2c_timing_calculator.test.timing_test_base import TimingTestBase


# Tests calculation of Data Hold Time - tHD;DAT for a Master
# All 'measured' I2C and nominal values were measured with an oscilloscope
class TestI2CTimingCalculatorMasterDataHoldTime(TimingTestBase):
    def test_nominal_data_hold_time_depends_on_datavd(self):
        test_cases = [
            {'datavd': 25,
             'measured': Parameter(i2c=2164, nominal=2168, worst_case=2160),
             'expected': Parameter(i2c=2158, nominal=2166, worst_case=2158)},
            {'datavd': 10,
             'measured': Parameter(i2c=914, nominal=918, worst_case=914),
             'expected': Parameter(i2c=908, nominal=916, worst_case=908)},
            {'datavd': 1,
             'measured': Parameter(i2c=164, nominal=170, worst_case=164),
             'expected': Parameter(i2c=158, nominal=166, worst_case=158,)},
        ]
        for t in test_cases:
            config = self.build_config(datavd=t['datavd'])
            actual = config.data_hold(master=True, falling=True)
            self.assert_i2c_nominal_equal(actual, t)

    def test_i2c_data_hold_time_depends_on_scl_fall_time(self):
        test_cases = [
            {'falltime': 100,
             # These times were estimated not measured as I can't change the fall times
             'measured': Parameter(i2c=2066, nominal=2166, worst_case=2066),
             'expected': Parameter(i2c=2066, nominal=2166, worst_case=2066)},
            {'falltime': 8,
             # These times were measured
             'measured': Parameter(i2c=2164, nominal=2168, worst_case=2160),
             'expected': Parameter(i2c=2158, nominal=2166, worst_case=2158)},
        ]
        for t in test_cases:
            config = self.build_config(falltime=t['falltime'])
            actual = config.data_hold(master=True, falling=True)
            self.assert_i2c_nominal_equal(actual, t)

    def test_i2c_data_hold_time_depends_whether_sda_rises_or_falls(self):
        test_cases = [
            {'falling': True,
             'measured': Parameter(i2c=2164, nominal=2168, worst_case=2160),
             'expected': Parameter(i2c=2158, nominal=2166, worst_case=2158)},
            {'falling': False,
             'measured': Parameter(i2c=2770, nominal=2160, worst_case=2620),
             'expected': Parameter(i2c=2778, nominal=2166, worst_case=2618)},
        ]
        for t in test_cases:
            config = self.build_config(sda_risetime=1480)
            actual = config.data_hold(master=True, falling=t['falling'])
            self.assert_i2c_nominal_equal(actual, t)

    def test_i2c_data_hold_time_depends_on_sda_rise_time(self):
        test_cases = [
            {'sda_risetime': 1480,
             'measured': Parameter(i2c=2770, nominal=2160, worst_case=2620),
             'expected': Parameter(i2c=2778, nominal=2166, worst_case=2618)},
            {'sda_risetime': 440,
             'measured': Parameter(i2c=2320, nominal=2176, worst_case=2618),
             'expected': Parameter(i2c=2340, nominal=2166, worst_case=2618)},
        ]
        for t in test_cases:
            config = self.build_config(sda_risetime=t['sda_risetime'])
            actual = config.data_hold(master=True, falling=False)
            self.assert_i2c_nominal_equal(actual, t)

    def test_data_hold_time_depends_on_prescale(self):
        test_cases = [
            {'prescale': 2,
             'measured': Parameter(i2c=4330, nominal=4330, worst_case=4330),
             'expected': Parameter(i2c=4325, nominal=4333, worst_case=4325)},
            {'prescale': 3,
             'measured': Parameter(i2c=8670, nominal=8670, worst_case=8670),
             'expected': Parameter(i2c=8658, nominal=8666, worst_case=8658)},
        ]
        for t in test_cases:
            config = self.build_config(prescale=t['prescale'])
            actual = config.data_hold(master=True, falling=True)
            self.assert_i2c_nominal_equal(actual, t)
