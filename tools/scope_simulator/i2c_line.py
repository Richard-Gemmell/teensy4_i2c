import math

RISING = '↑'
FALLING = '↓'


class I2CLine:
    def __init__(self, rise_time: int = 700, fall_time: int = 10):
        self.HIGH = 1.0
        self.LOW = 0.0
        self.events = []
        self.fall_time = fall_time
        self.rise_time = rise_time
        self.tau_ratio = math.log(0.7, math.e) - math.log(0.3, math.e)
        self.show = True
        self.edges = []
        self.edge_directions = []

    def hide(self) -> 'I2CLine':
        self.show = False
        return self

    def set_rise_time(self, rise_time: int) -> 'I2CLine':
        self.rise_time = rise_time
        return self

    def set_fall_time(self, fall_time: int) -> 'I2CLine':
        self.fall_time = fall_time
        return self

    def high(self, at: int = 0) -> 'I2CLine':
        self.events.append([at, lambda t: self.HIGH])
        return self

    def low(self, at: int = 0) -> 'I2CLine':
        self.events.append([at, lambda t: self.LOW])
        return self

    def get_edge_triggers(self):
        return list(map(lambda r: r[0], self.edges))

    def get_edge_directions(self):
        return self.edge_directions

    def fall_at(self, at: int) -> 'I2CLine':
        self.edge_directions.append(FALLING)
        tau = self.fall_time / self.tau_ratio
        edge = [at, lambda t: self.rc(tau, t - at)]
        self.edges.append(edge)
        self.events.append(edge)
        return self

    def rise_at(self, at: int) -> 'I2CLine':
        self.edge_directions.append(RISING)
        tau = self.rise_time / self.tau_ratio
        edge = [at, lambda t: self.HIGH - self.rc(tau, t - at)]
        self.edges.append(edge)
        self.events.append(edge)
        return self

    def get_voltage_at(self, at: int) -> float:
        num_events = len(self.events)
        for i in range(num_events):
            event = self.events[num_events - i - 1]
            if at > event[0]:
                return event[1](at)
        if len(self.events) > 0:
            return self.events[0][1](at)
        return -1

    def rc(self, tau: float, at: int):
        return self.HIGH * math.exp(-at/tau)

    def get_time_from_edge(self, index: int, v: float) -> int:
        edge = self.edges[index]
        if self.edge_directions[index] == RISING:
            tau = self.rise_time / self.tau_ratio
            ratio = (self.HIGH - v) / self.HIGH
        else:
            tau = self.fall_time / self.tau_ratio
            ratio = v / self.HIGH
        # t = -log((V-Vc)/V)R*C
        offset = -math.log(ratio) * tau
        return edge[0] + offset
