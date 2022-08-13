import math


# Config parameters
class TeensyConfig:
    def __init__(self, name,
                 scl_risetime, sda_risetime,
                 frequency, prescale,
                 datavd, sethold,
                 filtsda, filtscl,
                 clkhi, clklo):
        self.name = name
        self.frequency = frequency  # in MHz
        self.period = (1000.0 / self.frequency)
        self.PRESCALE = prescale
        self.scale = self.period * (2 ** self.PRESCALE)
        self.SCL_RISETIME = scl_risetime
        self.SDA_RISETIME = sda_risetime
        self.DATAVD = datavd
        self.SETHOLD = sethold
        self.FILTSDA = filtsda
        self.FILTSCL = filtscl
        self.CLKHI = clkhi
        self.CLKLO = clklo

    def validate(self):
        if self.FILTSCL < 0 or self.FILTSCL > 15:
            raise ValueError("FILTSCL is out range")
        if self.FILTSDA < 0 or self.FILTSDA > 15:
            raise ValueError("FILTSDA is out range")

    def scl_latency(self):
        # Validated for tHIGH. Do NOT attempt to add SCL_RISETIME
        return math.trunc((2.0 + self.FILTSCL) / (2 ** self.PRESCALE))

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
        return (self.SETHOLD + 1) * self.scale

    def repeated_start_setup(self):
        return (self.SETHOLD + 1 + self.scl_latency()) * self.scale

    def stop_setup(self):
        return self.repeated_start_setup()

    def clock_high_min(self):
        """Minimum value for tHIGH"""
        return self.scale * (self.CLKHI + 1 + self.scl_latency()) - self.SCL_RISETIME

    def clock_high_max(self):
        """Max value of tHIGH"""
        return self.scale * (self.CLKHI + 1 + self.scl_latency())

    def clock_low(self):
        # Tested only with 0 fall time
        return (self.scale * (self.CLKLO + 1)) + (self.SCL_RISETIME * 0.42)

    def bus_free(self):
        # Reality seems to be wildly different.
        # Calc says 5'600 nanos but actually comes out at 22'800 nanos
        return self.scale * (self.CLKLO + 1 + self.sda_latency())


master_100k_config = TeensyConfig(
    name="Master 100k",
    # scl_risetime=1980, sda_risetime=1980,
    # scl_risetime=1000, sda_risetime=1000,
    scl_risetime=490, sda_risetime=490,
    # scl_risetime=40, sda_risetime=40,
    frequency=24, prescale=1,
    datavd=25, sethold=40,
    # filtscl=15, filtsda=15,
    # filtscl=10, filtsda=10,
    filtscl=5, filtsda=5,
    # filtscl=2, filtsda=2,
    # filtscl=0, filtsda=0,
    # clkhi=55, clklo=59)
    clkhi=55, clklo=59)

master_400k_config = TeensyConfig(
    name="Master 400k",
    scl_risetime=300, sda_risetime=320,
    frequency=24, prescale=0,
    datavd=12, sethold=18,
    filtscl=2, filtsda=2,
    clkhi=26, clklo=28)

master_1M_config = TeensyConfig(
    name="Master 1M",
    scl_risetime=300, sda_risetime=320,
    frequency=24, prescale=0,
    datavd=4, sethold=7,
    filtscl=1, filtsda=1,
    clkhi=9, clklo=10)

master_example_400k_config = TeensyConfig(
    name="Master 400k Example 1",
    scl_risetime=320, sda_risetime=320,
    frequency=48, prescale=0,
    datavd=15, sethold=29,
    filtscl=1, filtsda=1,
    clkhi=53, clklo=62)

master_example_400k_config_2 = TeensyConfig(
    name="Master 400k Example 2",
    scl_risetime=320, sda_risetime=320,
    frequency=60, prescale=1,
    datavd=8, sethold=17,
    filtscl=2, filtsda=2,
    clkhi=31, clklo=40)

master_example_1M_config = TeensyConfig(
    name="Master 1M Example 1",
    scl_risetime=320, sda_risetime=320,
    frequency=48, prescale=2,
    datavd=4, sethold=3,
    filtscl=1, filtsda=1,
    clkhi=4, clklo=6)

master_example_1M_config_2 = TeensyConfig(
    name="Master 1M Example 2",
    scl_risetime=320, sda_risetime=320,
    frequency=60, prescale=1,
    datavd=1, sethold=7,
    filtscl=2, filtsda=2,
    clkhi=11, clklo=15)


def print_master_timings(config: TeensyConfig):
    print(f"{config.name}")
    print(f"Period {config.period:.0f}. Scale {config.scale:.0f} nanos")
    # print(f"SDA Rise Time {config.SDA_RISETIME:.2f} clocks")
    # print(f"SDA Latency {config.sda_latency():.0f} clocks")
    print(f"SCL Latency {config.scl_latency():.0f} clocks")
    print(f"START Hold Time (tHD:STA) {config.start_hold():.0f} nanos")
    print(f"Repeated START / STOP Setup Time (tSU:STA) {config.repeated_start_setup():.0f} nanos")
    print(f"Data Setup Time (tSU:DAT) {config.data_setup():.0f} nanos")
    print(f" Data Hold Time (tHD:DAT) {config.data_hold():.0f} nanos")
    print(f"Data Valid 0->1 (tVD:DAT) {config.data_valid_rise():.0f} nanos")
    print(f"Data Valid 1->0 (tVD:DAT) {config.data_valid():.0f} nanos")

    print(f"Clock High Time (tHIGH) {config.clock_high_min():.0f} to {config.clock_high_max():.0f} nanos")
    print(f"Clock Low Time (tLOW) {config.clock_low():.0f} nanos")
    print(f"Bus Free Time (tBUF) {config.bus_free():.0f} nanos")
    print()


if __name__ == '__main__':
    master_config = master_100k_config
    master_config.validate()
    print_master_timings(master_config)
