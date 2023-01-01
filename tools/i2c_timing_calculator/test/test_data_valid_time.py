from i2c_timing_calculator.test.timing_test_base import TimingTestBase


# Tests calculation of Data Valid Time - tVD;DAT
class TestI2CTimingCalculatorDataValidTime(TimingTestBase):
    def test_data_valid_time_equals_data_hold_time_plus_rise_time(self):
        config = self.build_config(sda_risetime=1000)

        data_valid_master = config.data_valid(master=True, falling=False)
        data_hold_master = config.data_hold(master=True, falling=False)
        self.assertEqual(data_hold_master.i2c_value + 1000, data_valid_master.i2c_value)
        self.assertEqual(data_hold_master.nominal, data_valid_master.nominal)
        self.assertEqual(data_hold_master.worst_case + 1000, data_valid_master.worst_case)

        data_valid_slave = config.data_valid(master=False, falling=False)
        data_hold_slave = config.data_hold(master=False, falling=False)
        self.assertEqual(data_hold_slave.i2c_value + 1000, data_valid_slave.i2c_value)
        self.assertEqual(data_hold_slave.nominal, data_valid_slave.nominal)
        self.assertEqual(data_hold_slave.worst_case + 1000, data_valid_slave.worst_case)

    def test_data_valid_time_equals_data_hold_time_plus_fall_time(self):
        config = self.build_config(falltime=123)

        data_valid_master = config.data_valid(master=True, falling=True)
        data_hold_master = config.data_hold(master=True, falling=True)
        self.assertEqual(data_hold_master.i2c_value + 123, data_valid_master.i2c_value)
        self.assertEqual(data_hold_master.nominal, data_valid_master.nominal)
        self.assertEqual(data_hold_master.worst_case + 123, data_valid_master.worst_case)

        data_valid_slave = config.data_valid(master=False, falling=True)
        data_hold_slave = config.data_hold(master=False, falling=True)
        self.assertEqual(data_hold_slave.i2c_value + 123, data_valid_slave.i2c_value)
        self.assertEqual(data_hold_slave.nominal, data_valid_slave.nominal)
        self.assertEqual(data_hold_slave.worst_case + 123, data_valid_slave.worst_case)
