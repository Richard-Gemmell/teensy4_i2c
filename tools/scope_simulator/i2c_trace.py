from typing import List

import matplotlib as mpl
from matplotlib import pyplot as plt

mpl.rcParams['lines.linewidth'] = 2
mpl.rcParams['font.size'] = 22


class I2CTrace:
    def __init__(self, title: str, start: int, stop: int):
        self.stop = stop
        self.start = start
        self.title = title

        # A4 Size
        self.page_color = '#C2D7FF'
        plot_color = '#FEFDC3'
        self.fig, self.ax = plt.subplots(figsize=(15.98, 11.23/2), facecolor=self.page_color)
        self.ax.set_facecolor(plot_color)
        plt.tight_layout(rect=[0.04, 0.04, 1, 0.98])

        # General Styling
        self.ax.set_title(title)
        self.ax.set_ylabel("Voltage (Vdd)")
        self.ax.set_xlabel("Time (nanoseconds)")

        # Tick marks and Grid lines
        self.ax.set_yticks([0, 1.0], ["GND", "Vdd"])
        self.ax.set_yticks([0.3, 0.7], ["0.3", "0.7"], minor=True)
        plt.grid(axis='y', which='major', color='black')
        plt.grid(axis='y', which='minor', color='grey', linestyle='--')
        plt.grid(visible=True)

    def set_events(self, values: List[int], lables: List[str]):
        self.ax.set_xticks(values, lables)

    def sda(self, y_values: List[float], x_values: List[int]):
        self.ax.plot(x_values, y_values, label="SDA", color='blue')

    def scl(self, y_values: List[float], x_values: List[int]):
        self.ax.plot(x_values, y_values, label="SCL", color='red')

    def show(self):
        self.ax.legend(facecolor=self.page_color, fontsize='small', loc='upper left', borderpad=0.2)
        plt.show()

    def save(self, filename: str):
        plt.savefig(filename, format='png')

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
