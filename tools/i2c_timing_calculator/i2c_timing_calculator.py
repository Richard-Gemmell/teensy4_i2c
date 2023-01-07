from teensy_config import TeensyConfig, Parameter

master_400k_config = TeensyConfig(
    name="Master 400k",
    scl_risetime=350, sda_risetime=327, falltime=8,
    max_fall=300, max_rise=330,
    frequency=24, prescale=0,
    datavd=12, sethold=25, busidle=110,
    filtscl=2, filtsda=2,
    clkhi=26, clklo=28)

master_1M_config = TeensyConfig(
    name="Master 1M",
    scl_risetime=300, sda_risetime=320, falltime=8,
    max_fall=120, max_rise=330,
    frequency=24, prescale=0,
    datavd=4, sethold=10, busidle=44,
    filtscl=1, filtsda=1,
    clkhi=9, clklo=10)

master_example_400k_config = TeensyConfig(
    name="Master 400k Example 1",
    scl_risetime=320, sda_risetime=320, falltime=8,
    max_fall=300, max_rise=330,
    frequency=48, prescale=0,
    datavd=15, sethold=29, busidle=0,
    filtscl=1, filtsda=1,
    clkhi=53, clklo=62)

master_example_400k_config_2 = TeensyConfig(
    name="Master 400k Example 2",
    scl_risetime=320, sda_risetime=320, falltime=8,
    max_fall=300, max_rise=330,
    frequency=60, prescale=1,
    datavd=8, sethold=17, busidle=0,
    filtscl=2, filtsda=2,
    clkhi=31, clklo=40)

master_example_1M_config = TeensyConfig(
    name="Master 1M Example 1",
    scl_risetime=320, sda_risetime=320, falltime=8,
    max_fall=120, max_rise=132,
    frequency=48, prescale=2,
    datavd=4, sethold=3, busidle=0,
    filtscl=1, filtsda=1,
    clkhi=4, clklo=6)

master_example_1M_config_2 = TeensyConfig(
    name="Master 1M Example 2",
    scl_risetime=320, sda_risetime=320, falltime=8,
    max_fall=120, max_rise=132,
    frequency=60, prescale=1,
    datavd=1, sethold=7, busidle=0,
    filtscl=2, filtsda=2,
    clkhi=11, clklo=15)


def print_range(description, range):
    print(f"{description} {range[0]:.0f} ({range[1]:.0f} to {range[2]:.0f}) nanos")


def print_parameter(description, parameter: Parameter):
    print(f"{description} I2C {parameter.i2c_value:.0f} nanos (Worst Case: {parameter.worst_case:.0f}, Nominal {parameter.nominal:.0f})")


def print_master_timings(config: TeensyConfig):
    print(f"{config.name}")
    # print(f"Period {config.period:.0f}. Scale {config.scale:.0f} nanos")
    # print(f"SDA Rise Time {config.SDA_RISETIME:.2f} clocks")
    # print(f"SDA Latency {config.sda_latency():.0f} clocks")
    # print(f"SCL Latency {config.scl_latency():.0f} clocks {config.scl_latency() * config.period:.0f} nanos")
    # print_parameter(f"START Hold Time (tHD:STA)", config.start_hold())
    # print_parameter("Setup Repeated START (tSU:STA)", config.setup_repeated_start())
    # print_parameter("STOP Setup Time (tSU:STO)", config.stop_setup())
    # print_parameter(f"Data Hold Time (tHD:DAT) - Master 1->0", config.data_hold(master=True, falling=True))
    # print_parameter(f"Data Hold Time (tHD:DAT) - Master 0->1", config.data_hold(master=True, falling=False))
    # print_parameter(f"Data Hold Time (tHD:DAT) - Slave 1->0", config.data_hold(master=False, falling=True))
    # print_parameter(f"Data Hold Time (tHD:DAT) - Slave 0->1", config.data_hold(master=False, falling=False))
    # print_parameter(f"Data Valid Time (tVD:DAT) - Master 1->0", config.data_valid(master=True, falling=True))
    # print_parameter(f"Data Valid Time (tVD:DAT) - Master 0->1", config.data_valid(master=True, falling=False))
    # print_parameter(f"Data Valid Time (tVD:DAT) - Slave 1->0", config.data_valid(master=False, falling=True))
    # print_parameter(f"Data Valid Time (tVD:DAT) - Slave 0->1", config.data_valid(master=False, falling=False))
    # print_parameter(f"Data Setup Time (tSU:DAT) - Master 1->0", config.data_setup(master=True, falling=True))
    # print_parameter(f"Data Setup Time (tSU:DAT) - Master 0->1", config.data_setup(master=True, falling=False))
    print_parameter(f"Data Setup Time (tSU:DAT) - Slave 1->0", config.data_setup(master=False, falling=True))
    # print_parameter(f"Data Setup Time (tSU:DAT) - Slave 0->1", config.data_setup(master=False, falling=False))
    # print(f"Glitch filters. SDA: {config.sda_glitch_filter():.0f} nanos. SCL: {config.scl_glitch_filter():.0f} nanos")
    # print_parameter(f"Clock Low Time (tLOW)", config.clock_low())
    # print_parameter(f"Clock High Time (tHIGH)", config.clock_high())
    # print_parameter(f"SCL Clock Frequency (fSCL)", config.clock_frequency())
    # print_parameter(f"Bus Free Time (tBUF)", config.bus_free())
    print()


master_100k_config = TeensyConfig(
    name="Master 100k",

    # scl_risetime=490, sda_risetime=490,
    # scl_risetime=1100, sda_risetime=32,
    # scl_risetime=1384, sda_risetime=32,
    # scl_risetime=434, sda_risetime=32,
    # scl_risetime=182, sda_risetime=32,
    # scl_risetime=32, sda_risetime=32,

    scl_risetime=32, sda_risetime=1446,
    # scl_risetime=32, sda_risetime=1380,
    # scl_risetime=32, sda_risetime=1158,
    # scl_risetime=32, sda_risetime=770,
    # scl_risetime=32, sda_risetime=446,
    # scl_risetime=32, sda_risetime=179,
    # scl_risetime=32, sda_risetime=32,

    falltime=8, max_fall=300, max_rise=1100,
    frequency=24, prescale=1,
    datavd=25, sethold=63, busidle=6,
    filtscl=5, filtsda=5,
    clkhi=55, clklo=59)

if __name__ == '__main__':
    master_config = master_100k_config
    # master_config = master_400k_config
    # master_config = master_1M_config
    master_config.validate()
    print_master_timings(master_config)
