from unittest import TestCase

from i2c_timing_calculator.teensy_config import TeensyConfig, Parameter


class TimingTestBase(TestCase):
    def build_config(self, scl_risetime=1000, sda_risetime=1000,
                     falltime=8,
                     filtscl=5, filtsda=5,
                     frequency=24, prescale=1,
                     datavd=25, sethold=63,
                     busidle=248) -> TeensyConfig:
        return TeensyConfig(
            name="Test - Master 100k A",
            scl_risetime=scl_risetime,
            sda_risetime=sda_risetime,
            falltime=falltime,
            max_fall=300,
            max_rise=1100,
            frequency=frequency, prescale=prescale,
            datavd=datavd, sethold=sethold,
            busidle=busidle,
            filtscl=filtscl, filtsda=filtsda,
            clkhi=55, clklo=59)

    def assert_i2c_nominal_equal(self, actual: Parameter, t, delta: int = 100):
        measured: Parameter = t['measured']
        self.assertAlmostEqual(measured.i2c_value, actual.i2c_value, delta=delta)
        self.assertAlmostEqual(measured.nominal, actual.nominal, delta=delta)
        self.assertAlmostEqual(measured.worst_case, actual.worst_case, delta=delta)
        expected: Parameter = t['expected']
        self.assertEqual(expected.i2c_value, actual.i2c_value)
        self.assertEqual(expected.nominal, actual.nominal)
        self.assertEqual(expected.worst_case, actual.worst_case)
