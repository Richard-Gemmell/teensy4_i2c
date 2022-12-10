from i2c_timing_calculator.teensy_config import Parameter
from i2c_timing_calculator.test.timing_test_base import TimingTestBase


class TestI2CTimingCalculatorBusFreeTime(TimingTestBase):
    def test_bus_free_time_no_busidle_depends_on_sda_rise_time(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        # GIVEN BUSIDLE is 0 - so we use the tBUF calculation
        test_cases = [
            {'sda_rise': 1446, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 5544 adjusted 4698
             'measured': Parameter(i2c=4680, nominal=6720, worst_case=4665),
             'expected': Parameter(i2c=4665, nominal=6717, worst_case=4665)},
            {'sda_rise': 1158, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 5496 adjusted 4650
             'measured': Parameter(i2c=4650, nominal=6297, worst_case=4665),
             'expected': Parameter(i2c=4665, nominal=6307, worst_case=4665)},
            {'sda_rise': 770, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 5468 adjusted 5000
             'measured': Parameter(i2c=5000, nominal=6080, worst_case=4665),
             'expected': Parameter(i2c=5075, nominal=6166, worst_case=4665)},
            {'sda_rise': 446, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 5468 adjusted 5000
             'measured': Parameter(i2c=5510, nominal=6150, worst_case=4665),
             'expected': Parameter(i2c=5536, nominal=6166, worst_case=4665)},
            {'sda_rise': 184, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 5961 adjusted 5844
             'measured': Parameter(i2c=5850, nominal=6080, worst_case=4665),
             'expected': Parameter(i2c=5908, nominal=6166, worst_case=4665)},
            {'sda_rise': 32, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 6153 adjusted 6149
             'measured': Parameter(i2c=6120, nominal=6160, worst_case=4665),
             'expected': Parameter(i2c=6124, nominal=6166, worst_case=4665)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, sda_risetime=t['sda_rise'], busidle=t['busidle'])
            actual = config.bus_free()
            self.assert_i2c_nominal_equal(actual, t)

    def test_bus_free_time_no_busidle_depends_on_prescale(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope
        # GIVEN BUSIDLE is 0 - so we use the tBUF calculation
        test_cases = [
            # Behaviour when rise time > 1,000 nanos is different
            {'sda_rise': 1446, 'prescale': 0, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 2953 adjusted 2107
             'measured': Parameter(i2c=2092, nominal=4160, worst_case=2124),
             'expected': Parameter(i2c=2124, nominal=4175, worst_case=2124)},
            {'sda_rise': 1446, 'prescale': 2, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 10616 adjusted 9770
             'measured': Parameter(i2c=9800, nominal=11780, worst_case=9749),
             'expected': Parameter(i2c=9749, nominal=11800, worst_case=9749)},
           {'sda_rise': 1446, 'prescale': 3, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 20761 adjusted 19915
             'measured': Parameter(i2c=19840, nominal=21920, worst_case=19915),
             'expected': Parameter(i2c=19915, nominal=21967, worst_case=19915)},
            # Time is more or less fixed if rise time  < 1000 ns
            {'sda_rise': 418, 'prescale': 0, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 3261 adjusted 3005
             'measured': Parameter(i2c=3008, nominal=3580, worst_case=2124),
             'expected': Parameter(i2c=2992, nominal=3583, worst_case=2124)},
            {'sda_rise': 439, 'prescale': 2, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 11094 adjusted 10826
             'measured': Parameter(i2c=10743, nominal=11340, worst_case=9749),
             'expected': Parameter(i2c=10712, nominal=11333, worst_case=9749)},
            {'sda_rise': 439, 'prescale': 3, 'busidle': 0, 'delta': 250,
             # Bus Free Time (tBUF) raw: 21593 adjusted 21325
             'measured': Parameter(i2c=21283, nominal=21880, worst_case=19915),
             'expected': Parameter(i2c=21046, nominal=21666, worst_case=19915)},
            {'sda_rise': 180, 'prescale': 0, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 3379 adjusted 3262
             'measured': Parameter(i2c=3260, nominal=3500, worst_case=2124),
             'expected': Parameter(i2c=3330, nominal=3583, worst_case=2124)},
            {'sda_rise': 179, 'prescale': 2, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 11211 adjusted 11099
             # Setting rise to 175 gets very close to measured
             'measured': Parameter(i2c=11100, nominal=11320, worst_case=9749),
             'expected': Parameter(i2c=11082, nominal=11333, worst_case=9749)},
            {'sda_rise': 179, 'prescale': 3, 'busidle': 0, 'delta': 190,
             # Bus Free Time (tBUF) raw: 21711 adjusted 21599
             'measured': Parameter(i2c=21600, nominal=21840, worst_case=19915),
             'expected': Parameter(i2c=21415, nominal=21666, worst_case=19915)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, sda_risetime=t['sda_rise'], prescale=t['prescale'], busidle=t['busidle'])
            actual = config.bus_free()
            if 'delta' in t:
                self.assert_i2c_nominal_equal(actual, t, delta=t['delta'])
            else:
                self.assert_i2c_nominal_equal(actual, t)

    def test_bus_free_time_ignores_busidle_when_sda_rise_time_is_large(self):
        # 'measured' I2C and nominal values were measured with an oscilloscope
        # for 1410 case but not for 1158 times. 1158 case is achieved by
        # disconnecting oscilloscope from the test board.

        # GIVEN SDA rise time > 1000 nanoseconds
        test_cases = [
            {'sda_rise': 1410, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 5543 adjusted 4689
             'measured': Parameter(i2c=4770, nominal=6740, worst_case=4665),
             'expected': Parameter(i2c=4665, nominal=6665, worst_case=4665)},
            {'sda_rise': 1410, 'busidle': 1,
             # Bus Free Time (tBUF) raw: 5543 adjusted 4689
             'measured': Parameter(i2c=4770, nominal=6750, worst_case=4665),
             'expected': Parameter(i2c=4665, nominal=6665, worst_case=4665)},
            {'sda_rise': 1410, 'busidle': 250,
             # Bus Free Time (tBUF) raw: 5541 adjusted 4687
             'measured': Parameter(i2c=4760, nominal=6730, worst_case=4665),
             'expected': Parameter(i2c=4665, nominal=6665, worst_case=4665)},
            {'sda_rise': 1158, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 5503 adjusted 4649
             'measured': Parameter(i2c=4649, nominal=6297, worst_case=4665),
             'expected': Parameter(i2c=4665, nominal=6307, worst_case=4665)},
            {'sda_rise': 1158, 'busidle': 1,
             # Bus Free Time (tBUF) raw: 5503 adjusted 4649
             'measured': Parameter(i2c=4649, nominal=6297, worst_case=4665),
             'expected': Parameter(i2c=4665, nominal=6307, worst_case=4665)},
            {'sda_rise': 1158, 'busidle': 250,
             # Bus Free Time (tBUF) raw: 5503 adjusted 4649
             'measured': Parameter(i2c=4649, nominal=6297, worst_case=4665),
             'expected': Parameter(i2c=4665, nominal=6307, worst_case=4665)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, sda_risetime=t['sda_rise'], busidle=t['busidle'])
            actual = config.bus_free()
            self.assert_i2c_nominal_equal(actual, t, delta=110)

    def test_bus_free_time_depends_on_busidle_when_sda_rise_time_is_small(self):
        # All 'measured' I2C and nominal values were measured with an oscilloscope

        # GIVEN SDA rise time < 1000 nanoseconds
        test_cases = [
            {'sda_rise': 439, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 5801 adjusted 5533
             'measured': Parameter(i2c=5540, nominal=6110, worst_case=4665),
             'expected': Parameter(i2c=5546, nominal=6166, worst_case=4665)},
            {'sda_rise': 439, 'busidle': 1,
             # Bus Free Time (tBUF) raw: 5803 adjusted 5535
             'measured': Parameter(i2c=5540, nominal=6120, worst_case=4665),
             'expected': Parameter(i2c=5546, nominal=6166, worst_case=4665)},
            {'sda_rise': 439, 'busidle': 20,
             # Bus Free Time (tBUF) raw: 7384 adjusted 7116
             'measured': Parameter(i2c=7110, nominal=7700, worst_case=4665),
             'expected': Parameter(i2c=7129, nominal=7750, worst_case=4665)},
            {'sda_rise': 439, 'busidle': 40,
             # Bus Free Time (tBUF) raw: 9053 adjusted 8785
             'measured': Parameter(i2c=8790, nominal=9370, worst_case=4665),
             'expected': Parameter(i2c=8796, nominal=9416, worst_case=4665)},
            {'sda_rise': 31, 'busidle': 0,
             # Bus Free Time (tBUF) raw: 6151 adjusted 6129
             'measured': Parameter(i2c=6130, nominal=6170, worst_case=4665),
             'expected': Parameter(i2c=6125, nominal=6166, worst_case=4665)},
            {'sda_rise': 31, 'busidle': 1,
             # Bus Free Time (tBUF) raw: 6151 adjusted 6129
             'measured': Parameter(i2c=6130, nominal=6170, worst_case=4665),
             'expected': Parameter(i2c=6125, nominal=6166, worst_case=4665)},
            {'sda_rise': 31, 'busidle': 20,
             # Bus Free Time (tBUF) raw: 7734 adjusted 7712
             'measured': Parameter(i2c=7720, nominal=7750, worst_case=4665),
             'expected': Parameter(i2c=7709, nominal=7750, worst_case=4665)},
            {'sda_rise': 31, 'busidle': 40,
             # Bus Free Time (tBUF) raw: 9359 adjusted 9337
             'measured': Parameter(i2c=9340, nominal=9370, worst_case=4665),
             'expected': Parameter(i2c=9375, nominal=9416, worst_case=4665)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=32, sda_risetime=t['sda_rise'], busidle=t['busidle'])
            actual = config.bus_free()
            self.assert_i2c_nominal_equal(actual, t)
