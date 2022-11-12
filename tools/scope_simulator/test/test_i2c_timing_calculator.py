from unittest import TestCase

from i2c_timing_calculator import TeensyConfig


class TestI2CTimingCalculator(TestCase):
    def build_config(self, scl_risetime=1000, sda_risetime=1000,
                     filtscl=5, filtsda=5,
                     frequency=24, prescale=1) -> TeensyConfig:
        return TeensyConfig(
            name="Test - Master 100k A",
            scl_risetime=scl_risetime,
            sda_risetime=sda_risetime,
            frequency=frequency, prescale=prescale,
            datavd=25, sethold=63,
            filtscl=filtscl, filtsda=filtsda,
            clkhi=55, clklo=59)

    def test_stop_setup_depends_on_sda_rise_time(self):
        # All results in this test case were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1384, 'scl_rise': 32,
             'measured': {'i2c': 4880, 'nominal': 6840},
             'expected': {'i2c': 4880, 'nominal': 6833}},
            {'sda_rise': 434, 'scl_rise': 32,
             'measured': {'i2c': 5340, 'nominal': 5920},
             'expected': {'i2c': 5396, 'nominal': 6000}},
            {'sda_rise': 182, 'scl_rise': 32,
             'measured': {'i2c': 5520, 'nominal': 5750},
             'expected': {'i2c': 5504, 'nominal': 5750}},
            {'sda_rise': 31, 'scl_rise': 32,
             'measured': {'i2c': 5560, 'nominal': 5570},
             'expected': {'i2c': 5552, 'nominal': 5583}},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=t['sda_rise'], sda_risetime=t['scl_rise'])
            actual = config.stop_setup()
            self.assert_i2c_nominal_equal(actual, t)

    def assert_i2c_nominal_equal(self, actual, t):
        self.assertEqual(actual, [t['expected']['i2c'], t['expected']['nominal']])
        self.assertAlmostEqual(actual[0], t['measured']['i2c'], delta=100)
        self.assertAlmostEqual(actual[1], t['measured']['nominal'], delta=100)

    def test_stop_setup_depends_on_filtscl(self):
        # All results in this test case were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1384, 'scl_rise': 32, 'filtscl': 5,
             'measured': {'i2c': 4880, 'nominal': 6840},
             'expected': {'i2c': 4880, 'nominal': 6833}},
            # ROUNDDOWN ensures that FILTSCL 4 is identical to FILTSCL 5
            {'sda_rise': 1384, 'scl_rise': 32, 'filtscl': 4,
             'measured': {'i2c': 4880, 'nominal': 6840},
             'expected': {'i2c': 4880, 'nominal': 6833}},
            {'sda_rise': 1384, 'scl_rise': 32, 'filtscl': 1,
             'measured': {'i2c': 4730, 'nominal': 6660},
             'expected': {'i2c': 4713, 'nominal': 6666}},
            # ROUNDDOWN ensures that FILTSCL 0 is identical to FILTSCL 1
            {'sda_rise': 1384, 'scl_rise': 32, 'filtscl': 0,
             'measured': {'i2c': 4730, 'nominal': 6660},
             'expected': {'i2c': 4713, 'nominal': 6666}},
        ]
        for t in test_cases:
            config = self.build_config(filtscl=t['filtscl'], scl_risetime=t['sda_rise'], sda_risetime=t['scl_rise'])
            actual = config.stop_setup()
            self.assert_i2c_nominal_equal(actual, t)

    def test_stop_setup_depends_on_prescale(self):
        # All results in this test case were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1384, 'scl_rise': 32, 'prescale': 1,
             'measured': {'i2c': 4880, 'nominal': 6840},
             'expected': {'i2c': 4880, 'nominal': 6833}},
            {'sda_rise': 1384, 'scl_rise': 32, 'prescale': 2,
             'measured': {'i2c': 10220, 'nominal': 12180},
             'expected': {'i2c': 10213, 'nominal': 12166}},
        ]
        for t in test_cases:
            config = self.build_config(prescale=t['prescale'], scl_risetime=t['sda_rise'], sda_risetime=t['scl_rise'])
            actual = config.stop_setup()
            self.assert_i2c_nominal_equal(actual, t)
