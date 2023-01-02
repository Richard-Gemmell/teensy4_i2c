from i2c_timing_calculator.teensy_config import Parameter
from i2c_timing_calculator.test.timing_test_base import TimingTestBase


# Data Setup Time is derived from Data Valid Time. This test just
# proves that it's derived correctly. It doesn't attempt to cover
# all the factors that affect the setup time.
class TestI2CTimingCalculatorSetupDataTime(TimingTestBase):
    def test_setup_data_time_depends_on_sda_rise_time_sda_rising_master(self):
        # Master controls SDA. SDA rises from LOW to HIGH
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1446,
             'measured': Parameter(i2c=800, nominal=2824, worst_case=1283),
             'expected': Parameter(i2c=792, nominal=2834, worst_case=1283)},
            {'sda_rise': 446,
             'measured': Parameter(i2c=2256, nominal=2828, worst_case=1283),
             'expected': Parameter(i2c=2213, nominal=2834, worst_case=1283)},
            {'sda_rise': 32,
             'measured': Parameter(i2c=2796, nominal=2828, worst_case=1284),
             'expected': Parameter(i2c=2802, nominal=2834, worst_case=1284)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, sda_risetime=t['sda_rise'])
            actual = config.data_setup(master=True, falling=False)
            self.assert_i2c_nominal_equal(actual, t)

    def test_setup_data_time_when_sda_falls_on_master(self):
        # Master controls SDA. SDA falls from HIGH to LOW
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [{
            'measured': Parameter(i2c=2832, nominal=2836, worst_case=2836),
            'expected': Parameter(i2c=2836, nominal=2834, worst_case=2836)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, sda_risetime=446)
            actual = config.data_setup(master=True, falling=True)
            self.assert_i2c_nominal_equal(actual, t)

    def test_setup_data_time_depends_on_sda_rise_time_sda_rising_slave(self):
        # Slave controls SDA. SDA rises from LOW to HIGH
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1446,
             'measured': Parameter(i2c=2240, nominal=4240, worst_case=2694),
             'expected': Parameter(i2c=2203, nominal=4250, worst_case=2694)},
            {'sda_rise': 446,
             'measured': Parameter(i2c=3640, nominal=4240, worst_case=2694),
             'expected': Parameter(i2c=3624, nominal=4250, worst_case=2694)},
            {'sda_rise': 32,
             'measured': Parameter(i2c=4220, nominal=4240, worst_case=2694),
             'expected': Parameter(i2c=4212, nominal=4250, worst_case=2694)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, datavd=10, sda_risetime=t['sda_rise'])
            actual = config.data_setup(master=False, falling=False)
            self.assert_i2c_nominal_equal(actual, t)

    def test_setup_data_time_when_sda_falls_on_slave(self):
        # Slave controls SDA. SDA falls from HIGH to LOW
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [{
            'measured': Parameter(i2c=4240, nominal=4250, worst_case=3823),
            'expected': Parameter(i2c=4246, nominal=4250, worst_case=3823)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, datavd=10)
            actual = config.data_setup(master=False, falling=True)
            self.assert_i2c_nominal_equal(actual, t)
