import math

time_to_rise_to_0_3_vdd = 0.421
time_to_rise_to_0_7_vdd = 1.421
time_to_rise_to_teensy_trigger_voltage = 0.911
time_to_fall_to_0_7_vdd = time_to_rise_to_0_3_vdd
time_to_fall_to_0_3_vdd = time_to_rise_to_0_7_vdd


class Parameter:
    def __init__(self, i2c: float, nominal: float, worst_case: float):
        self.i2c_value = int(i2c)
        self.nominal = int(nominal)
        self.worst_case = int(worst_case)


# Config parameters
class TeensyConfig:
    def __init__(self, name,
                 scl_risetime, sda_risetime, max_rise,
                 frequency, prescale,
                 datavd, sethold,
                 filtsda, filtscl,
                 clkhi, clklo):
        self.name = name
        self.frequency = frequency  # in MHz
        self.period = (1000.0 / self.frequency)
        self.PRESCALE = prescale
        self.scale = self.period * (2 ** self.PRESCALE)
        self.fall_time = 8  # Same for both pins when controlled by Teensy
        self.max_rise = max_rise
        self.scl_risetime = scl_risetime
        self.sda_risetime = sda_risetime
        self.SCL_RISETIME = (scl_risetime * time_to_rise_to_teensy_trigger_voltage) / self.period
        self.SDA_RISETIME = (sda_risetime * time_to_rise_to_teensy_trigger_voltage) / self.period
        self.DATAVD = datavd
        self.SETHOLD = sethold
        self.FILTSDA = filtsda
        self.FILTSCL = filtscl
        self.CLKHI = clkhi
        self.CLKLO = clklo

    def pre_rise(self, rise_time):
        return rise_time * 0.42

    def validate(self):
        if self.FILTSCL < 0 or self.FILTSCL > 15:
            raise ValueError("FILTSCL is out range")
        if self.FILTSDA < 0 or self.FILTSDA > 15:
            raise ValueError("FILTSDA is out range")

    def scl_latency(self):
        # Validated for tHIGH. Do NOT attempt to add SCL_RISETIME
        return math.trunc((2.0 + self.FILTSCL) / (2 ** self.PRESCALE))

    def SCL_LATENCY(self, scl_risetime):
        """As defined in the data sheet"""
        SCL_RISETIME = (scl_risetime * time_to_rise_to_teensy_trigger_voltage) / self.period
        value = (2.0 + self.FILTSCL + SCL_RISETIME) / (2 ** self.PRESCALE)
        return math.floor(value)

    def sda_latency(self):
        return math.trunc((2.0 + self.FILTSDA) / (2 ** self.PRESCALE))

    def data_hold(self):
        return self.scale * (self.DATAVD + 1)

    def data_setup(self):
        return self.scale * (self.sda_latency() + 1)

    def data_valid(self):
        return self.data_hold()

    def data_valid_rise(self):
        # Comes out a little low as it doesn't include the rise time from 0 to Vil
        return self.scale * (self.DATAVD + 1 + self.SDA_RISETIME)

    def start_hold(self):
        nominal = (self.SETHOLD + 1) * self.scale
        i2c_value = nominal - (self.fall_time * time_to_rise_to_0_7_vdd) + (self.fall_time * time_to_rise_to_0_3_vdd)
        # worst case is effectively identical to the i2c_value
        # as Teensy controls both fall times, and they're both very short
        return Parameter(i2c_value, nominal, i2c_value)

    def setup_repeated_start(self):
        # ideal = self.scale * (self.SETHOLD + 1 + self.scl_latency())
        # min_value = ideal - self.SCL_RISETIME
        # max_value = ideal + self.pre_rise(self.SDA_RISETIME)
        # return [ideal, min_value, max_value]
        nominal = self.scale * (self.SETHOLD + 1 + self.SCL_LATENCY(self.scl_risetime))
        i2c_value = nominal - (self.scl_risetime * time_to_rise_to_0_7_vdd) + (self.fall_time * time_to_fall_to_0_7_vdd)
        worst_case_nominal = self.scale * (self.SETHOLD + 1 + self.SCL_LATENCY(self.max_rise))
        worst_case = worst_case_nominal - (self.max_rise * time_to_rise_to_0_7_vdd) + (self.fall_time * time_to_fall_to_0_7_vdd)
        return Parameter(i2c_value, nominal, worst_case)

    def stop_setup(self):
        nominal = self.scale * (self.SETHOLD + 1 + self.SCL_LATENCY(self.scl_risetime))
        i2c_value = nominal - (self.scl_risetime * time_to_rise_to_0_7_vdd) + (self.sda_risetime * time_to_rise_to_0_3_vdd)
        worst_case_nominal = self.scale * (self.SETHOLD + 1 + self.SCL_LATENCY(self.max_rise))
        worst_case = worst_case_nominal - (self.max_rise * time_to_rise_to_0_7_vdd)
        return Parameter(i2c_value, nominal, worst_case)

    def clock_high_min(self):
        """Minimum value for tHIGH"""
        return self.clock_high_max() - self.SCL_RISETIME

    def clock_high_max(self):
        """Max value of tHIGH"""
        return self.scale * (self.CLKHI + 1 + self.scl_latency())

    def clock_low(self):
        # Tested only with 0 fall time
        return (self.scale * (self.CLKLO + 1)) + self.pre_rise(self.SCL_RISETIME)

    def bus_free(self):
        # Reality seems to be wildly different.
        # Calc says 5'600 nanos but actually comes out at 22'800 nanos
        return self.scale * (self.CLKLO + 1 + self.sda_latency())

    def sda_glitch_filter(self):
        return self.FILTSDA * self.period

    def scl_glitch_filter(self):
        return self.FILTSCL * self.period
