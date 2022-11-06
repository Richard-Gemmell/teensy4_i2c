import math


class I2CLine:
    def __init__(self, rise_time: int = 700, fall_time: int = 10):
        self.HIGH = 1.0
        self.LOW = 0.0
        self.events = []
        self.fall_time = fall_time
        self.rise_time = rise_time
        self.tau_ratio = math.log(0.7, math.e) - math.log(0.3, math.e)
        self.show = True

    def hide(self) -> 'I2CLine':
        self.show = False
        return self

    def tRise(self, rise_time: int) -> 'I2CLine':
        self.rise_time = rise_time
        return self

    def tFall(self, fall_time: int) -> 'I2CLine':
        self.fall_time = fall_time
        return self

    def high(self, at: int = 0) -> 'I2CLine':
        self.events.append([at, lambda t: self.HIGH])
        return self

    def low(self, at: int = 0) -> 'I2CLine':
        self.events.append([at, lambda t: self.LOW])
        return self

    def fall_at(self, at: int) -> 'I2CLine':
        tau = self.fall_time / self.tau_ratio
        self.events.append([at, lambda t: self.rc(tau, t - at)])
        return self

    def rise_at(self, at: int) -> 'I2CLine':
        tau = self.rise_time / self.tau_ratio
        self.events.append([at, lambda t: self.HIGH - self.rc(tau, t - at)])
        return self

    def get_voltage_at(self, at: int) -> float:
        num_events = len(self.events)
        for i in range(num_events):
            event = self.events[num_events - i - 1]
            if at >= event[0]:
                return event[1](at)
        if len(self.events) > 0:
            return self.events[0][1](at)
        return -1

    def rc(self, tau: float, at: int):
        return self.HIGH * math.exp(-at/tau)
