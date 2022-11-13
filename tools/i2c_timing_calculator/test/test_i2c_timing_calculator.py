from unittest import TestCase

from i2c_timing_calculator.teensy_config import TeensyConfig, Parameter


class TestI2CTimingCalculator(TestCase):
    def build_config(self, scl_risetime=1000, sda_risetime=1000,
                     filtscl=5, filtsda=5,
                     frequency=24, prescale=1) -> TeensyConfig:
        return TeensyConfig(
            name="Test - Master 100k A",
            scl_risetime=scl_risetime,
            sda_risetime=sda_risetime,
            max_rise=1100,
            frequency=frequency, prescale=prescale,
            datavd=25, sethold=63,
            filtscl=filtscl, filtsda=filtsda,
            clkhi=55, clklo=59)

    def test_start_hold(self):
        # All results in this test case were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1384, 'scl_rise': 32, 'prescale': 1,
             'measured': Parameter(i2c=5320, nominal=5330, worst_case=5320),
             'expected': Parameter(i2c=5325, nominal=5333, worst_case=5325)},
            {'sda_rise': 1384, 'scl_rise': 32, 'prescale': 2,
             'measured': Parameter(i2c=10660, nominal=10680, worst_case=10660),
             'expected': Parameter(i2c=10658, nominal=10666, worst_case=10658)},
        ]
        for t in test_cases:
            config = self.build_config(prescale=t['prescale'], scl_risetime=t['sda_rise'], sda_risetime=t['scl_rise'])
            actual = config.start_hold()
            self.assert_i2c_nominal_equal(actual, t)

    def assert_i2c_nominal_equal(self, actual: Parameter, t):
        delta = 100  # nanoseconds
        measured: Parameter = t['measured']
        self.assertAlmostEqual(measured.i2c_value, actual.i2c_value, delta=delta)
        self.assertAlmostEqual(measured.nominal, actual.nominal, delta=delta)
        self.assertAlmostEqual(measured.worst_case, actual.worst_case, delta=delta)
        expected: Parameter = t['expected']
        self.assertEqual(expected.i2c_value, actual.i2c_value)
        self.assertEqual(expected.nominal, actual.nominal)
        self.assertEqual(expected.worst_case, actual.worst_case)


    def test_setup_repeated_start_depends_on_sda_rise_time(self):
        # All results in this test case were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1384, 'scl_rise': 32,
             'measured': Parameter(i2c=4860, nominal=6830, worst_case=5023),
             'expected': Parameter(i2c=4870, nominal=6833, worst_case=5023)},
            {'sda_rise': 434, 'scl_rise': 32,
             'measured': Parameter(i2c=5330, nominal=5910, worst_case=5023),
             'expected': Parameter(i2c=5386, nominal=6000, worst_case=5023)},
            {'sda_rise': 182, 'scl_rise': 32,
             'measured': Parameter(i2c=5510, nominal=5740, worst_case=5023),
             'expected': Parameter(i2c=5494, nominal=5750, worst_case=5023)},
            {'sda_rise': 31, 'scl_rise': 32,
             'measured': Parameter(i2c=5550, nominal=5590, worst_case=5023),
             'expected': Parameter(i2c=5542, nominal=5583, worst_case=5023)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=t['sda_rise'], sda_risetime=t['scl_rise'])
            actual = config.setup_repeated_start()
            self.assert_i2c_nominal_equal(actual, t)

    def test_setup_repeated_start_depends_on_filtscl(self):
        # All results in this test case were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1384, 'scl_rise': 32, 'filtscl': 5,
             'measured': Parameter(i2c=4860, nominal=6830, worst_case=5023),
             'expected': Parameter(i2c=4870, nominal=6833, worst_case=5023)},
            # ROUNDDOWN ensures that FILTSCL 4 is identical to FILTSCL 5
            {'sda_rise': 1384, 'scl_rise': 32, 'filtscl': 4,
             'measured': Parameter(i2c=4830, nominal=6820, worst_case=5023),
             'expected': Parameter(i2c=4870, nominal=6833, worst_case=5023)},
            {'sda_rise': 1384, 'scl_rise': 32, 'filtscl': 1,
             'measured': Parameter(i2c=4680, nominal=6660, worst_case=4856),
             'expected': Parameter(i2c=4703, nominal=6666, worst_case=4856)},
            # ROUNDDOWN ensures that FILTSCL 0 is identical to FILTSCL 1
            {'sda_rise': 1384, 'scl_rise': 32, 'filtscl': 0,
             'measured': Parameter(i2c=4680, nominal=6660, worst_case=4856),
             'expected': Parameter(i2c=4703, nominal=6666, worst_case=4856)},
        ]
        for t in test_cases:
            config = self.build_config(filtscl=t['filtscl'], scl_risetime=t['sda_rise'], sda_risetime=t['scl_rise'])
            actual = config.setup_repeated_start()
            self.assert_i2c_nominal_equal(actual, t)

    def test_setup_repeated_start_depends_on_prescale(self):
        # All results in this test case were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1384, 'scl_rise': 32, 'prescale': 1,
             'measured': Parameter(i2c=4860, nominal=6830, worst_case=5023),
             'expected': Parameter(i2c=4870, nominal=6833, worst_case=5023)},
            {'sda_rise': 1384, 'scl_rise': 32, 'prescale': 2,
             'measured': Parameter(i2c=10180, nominal=12160, worst_case=10273),
             'expected': Parameter(i2c=10203, nominal=12166, worst_case=10273)},
        ]
        for t in test_cases:
            config = self.build_config(prescale=t['prescale'], scl_risetime=t['sda_rise'], sda_risetime=t['scl_rise'])
            actual = config.setup_repeated_start()
            self.assert_i2c_nominal_equal(actual, t)

    def test_stop_setup_depends_on_sda_rise_time(self):
        # All results in this test case were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1384, 'scl_rise': 32,
             'measured': Parameter(i2c=4880, nominal=6840, worst_case=5020),
             'expected': Parameter(i2c=4880, nominal=6833, worst_case=5020)},
            {'sda_rise': 434, 'scl_rise': 32,
             'measured': Parameter(i2c=5340, nominal=5920, worst_case=5020),
             'expected': Parameter(i2c=5396, nominal=6000, worst_case=5020)},
            {'sda_rise': 182, 'scl_rise': 32,
             'measured': Parameter(i2c=5520, nominal=5750, worst_case=5020),
             'expected': Parameter(i2c=5504, nominal=5750, worst_case=5020)},
            {'sda_rise': 31, 'scl_rise': 32,
             'measured': Parameter(i2c=5560, nominal=5570, worst_case=5020),
             'expected': Parameter(i2c=5552, nominal=5583, worst_case=5020)},
        ]
        for t in test_cases:
            config = self.build_config(scl_risetime=t['sda_rise'], sda_risetime=t['scl_rise'])
            actual = config.stop_setup()
            self.assert_i2c_nominal_equal(actual, t)

    def test_stop_setup_depends_on_filtscl(self):
        # All results in this test case were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1384, 'scl_rise': 32, 'filtscl': 5,
             'measured': Parameter(i2c=4880, nominal=6840, worst_case=5020),
             'expected': Parameter(i2c=4880, nominal=6833, worst_case=5020)},
            # ROUNDDOWN ensures that FILTSCL 4 is identical to FILTSCL 5
            {'sda_rise': 1384, 'scl_rise': 32, 'filtscl': 4,
             'measured': Parameter(i2c=4880, nominal=6840, worst_case=5020),
             'expected': Parameter(i2c=4880, nominal=6833, worst_case=5020)},
            {'sda_rise': 1384, 'scl_rise': 32, 'filtscl': 1,
             'measured': Parameter(i2c=4730, nominal=6660, worst_case=4853),
             'expected': Parameter(i2c=4713, nominal=6666, worst_case=4853)},
            # ROUNDDOWN ensures that FILTSCL 0 is identical to FILTSCL 1
            {'sda_rise': 1384, 'scl_rise': 32, 'filtscl': 0,
             'measured': Parameter(i2c=4730, nominal=6660, worst_case=4853),
             'expected': Parameter(i2c=4713, nominal=6666, worst_case=4853)},
        ]
        for t in test_cases:
            config = self.build_config(filtscl=t['filtscl'], scl_risetime=t['sda_rise'], sda_risetime=t['scl_rise'])
            actual = config.stop_setup()
            self.assert_i2c_nominal_equal(actual, t)

    def test_stop_setup_depends_on_prescale(self):
        # All results in this test case were measured with an oscilloscope
        test_cases = [
            {'sda_rise': 1384, 'scl_rise': 32, 'prescale': 1,
             'measured': Parameter(i2c=4880, nominal=6840, worst_case=5020),
             'expected': Parameter(i2c=4880, nominal=6833, worst_case=5020)},
            {'sda_rise': 1384, 'scl_rise': 32, 'prescale': 2,
             'measured': Parameter(i2c=10220, nominal=12180, worst_case=10270),
             'expected': Parameter(i2c=10213, nominal=12166, worst_case=10270)},
        ]
        for t in test_cases:
            config = self.build_config(prescale=t['prescale'], scl_risetime=t['sda_rise'], sda_risetime=t['scl_rise'])
            actual = config.stop_setup()
            self.assert_i2c_nominal_equal(actual, t)
