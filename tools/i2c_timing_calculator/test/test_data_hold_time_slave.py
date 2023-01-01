from i2c_timing_calculator.teensy_config import Parameter
from i2c_timing_calculator.test.timing_test_base import TimingTestBase


# Tests calculation of Data Hold Time - tHD;DAT for a Slave
# All 'measured' I2C and nominal values were measured with an oscilloscope
class TestI2CTimingCalculatorSlaveDataHoldTime(TimingTestBase):
    def test_nominal_data_hold_time_depends_on_datavd(self):
        test_cases = [
            {'datavd': 25,
             'measured': Parameter(i2c=1368, nominal=1374, worst_case=1187),
             'expected': Parameter(i2c=1373, nominal=1375, worst_case=1194)},
            {'datavd': 10,
             'measured': Parameter(i2c=744, nominal=750, worst_case=569),
             'expected': Parameter(i2c=748, nominal=750, worst_case=569)},
            {'datavd': 1,
             'measured': Parameter(i2c=369, nominal=375, worst_case=188),
             'expected': Parameter(i2c=373, nominal=375, worst_case=194)},
        ]
        for t in test_cases:
            config = self.build_config(datavd=t['datavd'])
            actual = config.data_hold(master=False, falling=True)
            self.assert_i2c_nominal_equal(actual, t)

    def test_nominal_data_hold_time_depends_on_filtscl(self):
        test_cases = [
            {'filtscl': 10,
             'measured': Parameter(i2c=1578, nominal=1584, worst_case=1402),
             'expected': Parameter(i2c=1581, nominal=1583, worst_case=1402)},
            {'filtscl': 5,
             'measured': Parameter(i2c=1368, nominal=1374, worst_case=1187),
             'expected': Parameter(i2c=1373, nominal=1375, worst_case=1194)},
        ]
        for t in test_cases:
            config = self.build_config(filtscl=t['filtscl'])
            actual = config.data_hold(master=False, falling=True)
            self.assert_i2c_nominal_equal(actual, t)

    def test_i2c_data_hold_time_depends_on_scl_fall_time(self):
        test_cases = [
            {'falltime': 100,
             # These times were estimated not measured as I can't change the fall times
             'measured': Parameter(i2c=1356, nominal=1375, worst_case=1194),
             'expected': Parameter(i2c=1356, nominal=1375, worst_case=1194)},
            {'falltime': 8,
             # These times were measured
             'measured': Parameter(i2c=1368, nominal=1374, worst_case=1187),
             'expected': Parameter(i2c=1373, nominal=1375, worst_case=1194)},
        ]
        for t in test_cases:
            config = self.build_config(falltime=t['falltime'])
            actual = config.data_hold(master=False, falling=True)
            self.assert_i2c_nominal_equal(actual, t)

    def test_i2c_data_hold_time_depends_whether_sda_rises_or_falls(self):
        test_cases = [
            {'falling': True,
             'measured': Parameter(i2c=1368, nominal=1374, worst_case=1187),
             'expected': Parameter(i2c=1373, nominal=1375, worst_case=1194)},
            {'falling': False,
             'measured': Parameter(i2c=1976, nominal=1388, worst_case=1194),
             'expected': Parameter(i2c=1993, nominal=1375, worst_case=1194)},
        ]
        for t in test_cases:
            config = self.build_config(sda_risetime=1480)
            actual = config.data_hold(master=False, falling=t['falling'])
            self.assert_i2c_nominal_equal(actual, t)

    def test_i2c_data_hold_time_depends_on_sda_rise_time(self):
        test_cases = [
            {'sda_risetime': 1480,
             'measured': Parameter(i2c=1976, nominal=1388, worst_case=1194),
             'expected': Parameter(i2c=1993, nominal=1375, worst_case=1194)},
            {'sda_risetime': 440,
             'measured': Parameter(i2c=1524, nominal=1372, worst_case=1194),
             'expected': Parameter(i2c=1555, nominal=1375, worst_case=1194)},
        ]
        for t in test_cases:
            config = self.build_config(sda_risetime=t['sda_risetime'])
            actual = config.data_hold(master=False, falling=False)
            self.assert_i2c_nominal_equal(actual, t)
