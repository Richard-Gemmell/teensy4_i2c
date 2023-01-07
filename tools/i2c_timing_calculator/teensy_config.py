import math

time_to_rise_to_0_3_vdd = 0.421
time_to_rise_to_0_7_vdd = 1.421
time_to_rise_to_teensy_trigger_voltage = 0.911
time_to_fall_to_0_7_vdd = time_to_rise_to_0_3_vdd
time_to_fall_to_0_3_vdd = time_to_rise_to_0_7_vdd
time_to_fall_from_trigger_to_0_3_vdd = 0.603


class Parameter:
    def __init__(self, i2c: float, nominal: float, worst_case: float):
        self.i2c_value = int(i2c)
        self.nominal = int(nominal)
        self.worst_case = int(worst_case)


# Config parameters
class TeensyConfig:
    def __init__(self, name,
                 scl_risetime, sda_risetime,
                 falltime, max_fall, max_rise,
                 frequency, prescale,
                 datavd, sethold, busidle,
                 filtsda, filtscl,
                 clkhi, clklo):
        self.name = name
        self.frequency = frequency  # in MHz
        self.period = (1000.0 / self.frequency)
        self.PRESCALE = prescale
        self.scale = self.period * (2 ** self.PRESCALE)
        self.fall_time = falltime  # Same for both pins when controlled by Teensy
        self.max_fall = max_fall
        self.max_rise = max_rise
        self.scl_risetime = scl_risetime
        self.sda_risetime = sda_risetime
        self.SCL_RISETIME = (scl_risetime * time_to_rise_to_teensy_trigger_voltage) / self.period
        self.SDA_RISETIME = (sda_risetime * time_to_rise_to_teensy_trigger_voltage) / self.period
        self.DATAVD = datavd
        self.SETHOLD = sethold
        self.FILTSDA = filtsda
        self.FILTSCL = filtscl
        self.BUSIDLE = busidle
        self.CLKHI = clkhi
        self.CLKLO = clklo

    def __repr__(self) -> str:
        return self.name

    def pre_rise(self, rise_time):
        return rise_time * 0.42

    def validate(self):
        if self.FILTSCL < 0 or self.FILTSCL > 15:
            raise ValueError("FILTSCL is out range")
        if self.FILTSDA < 0 or self.FILTSDA > 15:
            raise ValueError("FILTSDA is out range")

    def SCL_LATENCY(self, scl_risetime):
        """As defined in the data sheet"""
        SCL_RISETIME = (scl_risetime * time_to_rise_to_teensy_trigger_voltage) / self.period
        value = (2.0 + self.FILTSCL + SCL_RISETIME) / (2 ** self.PRESCALE)
        return math.floor(value)

    def SDA_LATENCY(self, sda_risetime):
        """As defined in the data sheet"""
        SDA_RISETIME = (sda_risetime * time_to_rise_to_teensy_trigger_voltage) / self.period
        value = (2.0 + self.FILTSDA + SDA_RISETIME) / (2 ** self.PRESCALE)
        return math.floor(value)

    def sda_latency(self):
        return math.trunc((2.0 + self.FILTSDA) / (2 ** self.PRESCALE))

    def data_hold(self, master: bool, falling: bool):
        adjustment = (self.fall_time * time_to_fall_to_0_7_vdd) if falling else (self.sda_risetime * time_to_rise_to_0_3_vdd)
        if master:
            nominal = self.scale * (self.DATAVD + 1)
            i2c_value = nominal - (self.fall_time * time_to_fall_to_0_3_vdd) + adjustment
            worst_case_adjustment = (self.fall_time * time_to_fall_to_0_7_vdd) if falling else (self.max_rise * time_to_rise_to_0_3_vdd)
            worst_case = nominal - (self.fall_time * time_to_fall_to_0_3_vdd) + worst_case_adjustment
        else:
            nominal = self.period * (self.FILTSCL + self.DATAVD + 3)
            i2c_value = nominal - (self.fall_time * time_to_fall_from_trigger_to_0_3_vdd) + adjustment
            worst_case = nominal - (self.max_fall * time_to_fall_from_trigger_to_0_3_vdd)
        return Parameter(i2c_value, nominal, worst_case)

    def data_setup(self, master: bool, falling: bool):
        # tLow = tVD;DAT + tSU;DAT
        low = self.clock_low()
        dv = self.data_valid(master, falling)
        i2c_value = low.i2c_value - dv.i2c_value
        nominal = low.nominal - dv.nominal
        if falling:
            worst_case = i2c_value if master else nominal - (self.max_fall * time_to_fall_to_0_3_vdd)
        else:
            worst_case = i2c_value - (self.max_rise - self.sda_risetime) * time_to_rise_to_0_7_vdd
        return Parameter(i2c_value, nominal, worst_case)

    def data_valid(self, master: bool, falling: bool):
        data_hold = self.data_hold(master, falling)
        nominal = data_hold.nominal
        adjustment = self.fall_time if falling else self.sda_risetime
        i2c_value = data_hold.i2c_value + adjustment
        worst_case = data_hold.worst_case + adjustment
        return Parameter(i2c_value, nominal, worst_case)

    def start_hold(self):
        nominal = (self.SETHOLD + 1) * self.scale
        i2c_value = nominal - (self.fall_time * time_to_rise_to_0_7_vdd) + (self.fall_time * time_to_rise_to_0_3_vdd)
        # worst case is effectively identical to the i2c_value
        # as Teensy controls both fall times, and they're both very short
        return Parameter(i2c_value, nominal, i2c_value)

    def setup_repeated_start(self):
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

    def clock_high(self):
        nominal = self.scale * (self.CLKHI + 1 + self.SCL_LATENCY(self.scl_risetime))
        i2c_value = nominal - (self.scl_risetime * time_to_rise_to_0_7_vdd) + (self.fall_time * time_to_fall_to_0_7_vdd)
        worst_case_nominal = self.scale * (self.CLKHI + 1 + self.SCL_LATENCY(self.max_rise))
        worst_case = worst_case_nominal - (self.max_rise * time_to_rise_to_0_7_vdd) + (self.fall_time * time_to_fall_to_0_7_vdd)
        return Parameter(i2c_value, nominal, worst_case)

    def clock_low(self):
        nominal = self.scale * (self.CLKLO + 1)
        worst_case = nominal - (self.fall_time * time_to_fall_to_0_3_vdd)
        i2c_value = worst_case + (self.scl_risetime * time_to_rise_to_0_3_vdd)
        return Parameter(i2c_value, nominal, worst_case)

    def clock_frequency(self):
        period_nominal = self.scale * (self.CLKHI + self.CLKLO + 2 + self.SCL_LATENCY(self.scl_risetime))
        worst_case_period_nominal = self.scale * (self.CLKHI + self.CLKLO + 2 + self.SCL_LATENCY(0))
        nominal = 1.0 / (period_nominal / 1e9)
        worst_case = 1.0 / (worst_case_period_nominal / 1e9)
        return Parameter(nominal, nominal, worst_case)

    def bus_free(self):
        # I've no idea what the actual algorithm is, but it definitely involves capping rise time
        # and some sort of rounding to integers
        def get_nominal_and_i2c_value(rise_time: int) -> tuple[int, int]:
            if rise_time > 1000:
                offset = (((rise_time - 1000) * time_to_rise_to_0_7_vdd) / self.scale) + 1
            else:
                offset = 2
                if self.BUSIDLE > 1:
                    offset = self.BUSIDLE + 1
            nominal = 1000 + (self.CLKLO + 1 + offset) * self.scale
            i2c_value = nominal - (rise_time * time_to_rise_to_0_7_vdd) + (self.fall_time * time_to_fall_to_0_7_vdd)
            return nominal, i2c_value
        nominal, i2c_value = get_nominal_and_i2c_value(self.sda_risetime)
        worst_case_nominal, worst_case_i2c_value = get_nominal_and_i2c_value(self.max_rise)
        return Parameter(i2c_value, nominal, worst_case_i2c_value)

    def sda_glitch_filter(self):
        return self.FILTSDA * self.period

    def scl_glitch_filter(self):
        return self.FILTSCL * self.period
