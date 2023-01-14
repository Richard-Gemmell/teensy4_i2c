from dataclasses import dataclass


@dataclass
class Range:
    min: int
    max: int


@dataclass
class I2CSpecification:
    # output_fall_time: Range
    spike_width: Range
    frequency: Range
    start_hold_time: Range
    scl_low_time: Range
    scl_high_time: Range
    start_setup_time: Range
    data_hold_time: Range
    data_setup_time: Range
    # rise_time: Range
    # fall_time: Range
    stop_setup_time: Range
    bus_free_time: Range
    data_valid_time: Range


UNLIMITED = 1_000_000

standard_mode_spec = I2CSpecification(
    # output_fall_time=Range(0, 250),
    spike_width=Range(0, UNLIMITED),  # Not applicable for standard mode
    frequency=Range(0, 100_000),
    start_hold_time=Range(4_000, UNLIMITED),
    scl_low_time=Range(4_700, UNLIMITED),
    scl_high_time=Range(4_000, UNLIMITED),
    start_setup_time=Range(4_700, UNLIMITED),
    data_hold_time=Range(0, UNLIMITED),
    data_setup_time=Range(250, UNLIMITED),
    # rise_time=Range(0, 1_000),
    # fall_time=Range(0, 300),
    stop_setup_time=Range(4_000, UNLIMITED),
    bus_free_time=Range(4_700, UNLIMITED),
    data_valid_time=Range(0, 3_450),
)

fast_mode_spec = I2CSpecification(
    # output_fall_time=Range(12, 250),
    spike_width=Range(50, UNLIMITED),
    frequency=Range(0, 400_000),
    start_hold_time=Range(600, UNLIMITED),
    scl_low_time=Range(1_300, UNLIMITED),
    scl_high_time=Range(600, UNLIMITED),
    start_setup_time=Range(600, UNLIMITED),
    data_hold_time=Range(0, UNLIMITED),
    data_setup_time=Range(100, UNLIMITED),
    # rise_time=Range(0, 300),
    # fall_time=Range(12, 300),
    stop_setup_time=Range(600, UNLIMITED),
    bus_free_time=Range(1_300, UNLIMITED),
    data_valid_time=Range(0, 900),
)

fast_mode_plus_spec = I2CSpecification(
    # output_fall_time=Range(12,120),
    spike_width=Range(50, UNLIMITED),
    frequency=Range(0, 1_000_000),
    start_hold_time=Range(260, UNLIMITED),
    scl_low_time=Range(500, UNLIMITED),
    scl_high_time=Range(260, UNLIMITED),
    start_setup_time=Range(260, UNLIMITED),
    data_hold_time=Range(0, UNLIMITED),
    data_setup_time=Range(50, UNLIMITED),
    # rise_time=Range(0, 120),
    # fall_time=Range(12, 120),
    stop_setup_time=Range(260, UNLIMITED),
    bus_free_time=Range(500, UNLIMITED),
    data_valid_time=Range(0, 450),
)
