from unittest import skip

from i2c_timing_calculator.teensy_config import Parameter
from i2c_timing_calculator.test.timing_test_base import TimingTestBase


@skip("Don't know how to calculate setup data time yet. Try again when Data Hold and Data Valid are defined.")
class TestI2CTimingCalculatorSetupDataTime(TimingTestBase):
    def test_setup_data_time_depends_on_sda_rise_time_sda_rising_master(self):
        # Master controls SDA. SDA rises from LOW to HIGH
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1446,
             #
             'measured': Parameter(i2c=800, nominal=2824, worst_case=1),
             'expected': Parameter(i2c=1, nominal=1, worst_case=1)},
            {'sda_rise': 446,
             #
             'measured': Parameter(i2c=2256, nominal=2828, worst_case=1),
             'expected': Parameter(i2c=1, nominal=1, worst_case=1)},
            {'sda_rise': 32,
             #
             'measured': Parameter(i2c=2796, nominal=2828, worst_case=1),
             'expected': Parameter(i2c=1, nominal=1, worst_case=1)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, sda_risetime=t['sda_rise'])
            actual = config.data_setup()
            self.assert_i2c_nominal_equal(actual, t)

    def test_setup_data_time_when_sda_falls_on_master(self):
        # Master controls SDA. SDA falls from HIGH to LOW
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1446,
             #
             'measured': Parameter(i2c=2828, nominal=2840, worst_case=1),
             'expected': Parameter(i2c=1, nominal=1, worst_case=1)},
            {'sda_rise': 446,
             #
             'measured': Parameter(i2c=2832, nominal=2836, worst_case=1),
             'expected': Parameter(i2c=1, nominal=1, worst_case=1)},
            {'sda_rise': 32,
             #
             'measured': Parameter(i2c=2832, nominal=2840, worst_case=1),
             'expected': Parameter(i2c=1, nominal=1, worst_case=1)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, sda_risetime=t['sda_rise'])
            actual = config.data_setup()
            self.assert_i2c_nominal_equal(actual, t)

    def test_setup_data_time_depends_on_sda_rise_time_sda_rising_slave(self):
        # Master controls SDA. SDA rises from LOW to HIGH
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1446,
             #
             'measured': Parameter(i2c=1, nominal=1, worst_case=1),
             'expected': Parameter(i2c=1, nominal=1, worst_case=1)},
            {'sda_rise': 446,
             #
             'measured': Parameter(i2c=4380, nominal=4980, worst_case=1),
             'expected': Parameter(i2c=1, nominal=1, worst_case=1)},
            {'sda_rise': 32,
             #
             'measured': Parameter(i2c=1, nominal=1, worst_case=1),
             'expected': Parameter(i2c=1, nominal=1, worst_case=1)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, sda_risetime=t['sda_rise'])
            actual = config.data_setup()
            self.assert_i2c_nominal_equal(actual, t)

    def test_setup_data_time_when_sda_falls_on_slave(self):
        # Slave controls SDA. SDA falls from HIGH to LOW
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 446,
             #
             'measured': Parameter(i2c=4620, nominal=4610, worst_case=1),
             'expected': Parameter(i2c=1, nominal=1, worst_case=1)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, sda_risetime=t['sda_rise'])
            actual = config.data_setup()
            self.assert_i2c_nominal_equal(actual, t)
