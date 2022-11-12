from typing import List

import matplotlib as mpl
import numpy as np
from matplotlib import pyplot as plt
from matplotlib.ticker import ScalarFormatter, AutoLocator

from i2c_line import I2CLine

mpl.rcParams['lines.linewidth'] = 2
mpl.rcParams['font.size'] = 22


class I2CTrace:
    def __init__(self, title: str, start: int, stop: int):
        self.stop = stop
        self.start = start
        self.sda = I2CLine()
        self.scl = I2CLine()

        # A4 Size
        self.page_color = '#C2D7FF'
        plot_color = '#FEFDC3'
        self.fig, self.ax = plt.subplots(figsize=(15.98, 11.23/2), facecolor=self.page_color)
        self.ax.set_facecolor(plot_color)
        self.fig.tight_layout(rect=[0.04, 0.04, 1, 0.96])

        # General Styling
        self.ax.set_title(title, pad=15)
        self.ax.set_ylabel("Voltage (Vdd)")
        self.ax.set_xlabel("Time (nanoseconds)")
        self.ax.grid(visible=True)

        # Voltage ticks marks and grid lines
        self.ax.set_yticks([0, 1.0], ["GND", "Vdd"])
        self.ax.set_yticks([0.3, 0.5, 0.7], ["0.3", "0.5", "0.7"], minor=True)
        self.ax.grid(axis='y', which='major', color='black')
        self.ax.grid(axis='y', which='minor', color='grey', linestyle='--')

        # Time tick marks and grid lines
        self.ax.tick_params(axis='x', which='major')
        self.ax.tick_params(axis='x', which='minor', labelsize='small')
        self.ax.xaxis.set_minor_locator(AutoLocator())
        self.ax.xaxis.set_minor_formatter(ScalarFormatter())

    def plot(self):
        timestamps = np.arange(self.start, self.stop, 1)
        self.plot_line(timestamps, self.sda, "SDA", 'blue')
        self.plot_line(timestamps, self.scl, "SCL", 'red')
        self.ax.legend(facecolor=self.page_color, fontsize='small', loc='upper left', borderpad=0.2)

    def plot_line(self, timestamps, line: I2CLine, label: str, color: str):
        if not line.show:
            return
        voltages = np.zeros_like(timestamps, dtype=float)
        for i in range(len(timestamps)):
            voltages[i] = line.get_voltage_at(timestamps[i])
        self.ax.plot(timestamps, voltages, label=label, color=color)

    def set_events_from_edges(self):
        values = self.sda.get_edge_triggers() + self.scl.get_edge_triggers()
        labels = self.sda.get_edge_directions() + self.scl.get_edge_directions()
        self.set_events(values, labels)

    def set_events(self, values: List[int], labels: List[str]):
        self.ax.set_xticks(values, labels, minor=False)

    def show(self):
        self.fig.show()

    def save(self, filename: str):
        self.fig.savefig(filename, format='png')

    def add_voltage_measurement(self, title: str, x_pos: int, start: float, stop: float, lines: bool = False):
        if lines:
            self.ax.axhline(start, linewidth=1, color='grey')
            self.ax.axhline(stop, linewidth=1, color='grey')
        self.ax.annotate(
            '', xy=(x_pos, stop), xycoords='data',
            xytext=(x_pos, start), textcoords='data',
            arrowprops={'arrowstyle': '<->', 'shrinkA': 0, 'shrinkB': 0})
        self.ax.annotate(
            title, xy=(x_pos, ((stop-start)/2)+start), xycoords='data',
            xytext=(5, -5), textcoords='offset points', fontsize='x-small')

    def measure_between_edges(self, title: str, y_pos: float, left: [str, int, float], right: [str, int, float], lines: bool = True):
        start = self.get_time_from_edge(left)
        stop = self.get_time_from_edge(right)
        self.add_measurement(title, y_pos, start, stop, lines)

    def get_time_from_edge(self, edge: [int, float]) -> int:
        if edge[0] == 'SCL':
            return self.scl.get_time_from_edge(index=edge[1], v=edge[2])
        return self.sda.get_time_from_edge(index=edge[1], v=edge[2])

    def add_measurement(self, title: str, y_pos: float, start: int, stop: int, lines: bool = True):
        if lines:
            self.ax.axvline(start, linewidth=1, color='grey', linestyle='--')
            self.ax.axvline(stop, linewidth=1, color='grey', linestyle='--')
        self.ax.annotate(
            '', xy=(start, y_pos), xycoords='data',
            xytext=(stop, y_pos), textcoords='data',
            arrowprops={'arrowstyle': '<->', 'shrinkA': 0, 'shrinkB': 0})
        self.ax.text(((stop-start)/2)+start, y_pos+0.01, title,
             ha='center', va='bottom', fontsize='x-small')
