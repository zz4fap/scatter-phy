from gi.repository import Gtk
import matplotlib.pyplot as plt
import numpy as np
import time
import math
from matplotlib.backends.backend_gtk3cairo import FigureCanvasGTK3Cairo as FigureCanvas

class SenSysDemo:

    figure = []
    ax1 = ax2 = []
    line1_correct = line2_correct = line3_correct = []
    line1_wrong = line2_wrong = []
    updated_correct_values_cnt = updated_wrong_values_cnt = 0

    # Constants and global variables.
    NUMBER_OF_CORRECT_VALUES = 10
    CORRECT_ARRAY_SIZE = 100
    correct_rssi_vector = np.zeros(CORRECT_ARRAY_SIZE)
    correct_cqi_vector = np.zeros(CORRECT_ARRAY_SIZE)
    correct_noise_vector = np.zeros(CORRECT_ARRAY_SIZE)
    correct_xaxis_vector = np.arange(0,CORRECT_ARRAY_SIZE)

    NUMBER_OF_WRONG_VALUES = 10
    ERROR_ARRAY_SIZE = 100
    error_rssi_vector = np.zeros(ERROR_ARRAY_SIZE)
    error_noise_vector = np.zeros(ERROR_ARRAY_SIZE)
    error_xaxis_vector = np.arange(0,ERROR_ARRAY_SIZE)

    def __init__(self, ylim_correct=[-50,16], ylim_wrong=[-50,2]):
        scale = 3

        myfirstwindow = Gtk.Window()
        myfirstwindow.connect("delete-event", Gtk.main_quit)
        myfirstwindow.connect('destroy', lambda win: Gtk.main_quit())
        myfirstwindow.set_default_size(800, 600)

        self.updated_correct_values_cnt = 0
        self.updated_wrong_values_cnt = 0

        self.figure = plt.figure(figsize=(4*scale,3*scale))
        self.figure.suptitle("SenSys'17 - Packetized-LTE PHY", fontsize=16)
        self.ax1 = self.figure.add_subplot(2, 1, 1)
        plt.grid(True)
        self.ax2 = self.figure.add_subplot(2, 1, 2)
        plt.grid(True)
        # draw and show it
        self.ax1.set_title("Successful Statistics")
        self.line1_correct, = self.ax1.plot(self.correct_rssi_vector,visible=True,label='RSSI [dBm]')
        self.line2_correct, = self.ax1.plot(self.correct_cqi_vector,visible=True,label='CQI')
        self.line3_correct, = self.ax1.plot(self.correct_noise_vector,visible=True,label='Noise [dBm]')
        self.ax1.legend()
        self.ax1.set_ylim(ylim_correct)
        self.ax2.set_title("Unsuccessful Statistics")
        self.line1_wrong, = self.ax2.plot(self.error_rssi_vector,visible=True,label='RSSI [dBm]')
        self.line2_wrong, = self.ax2.plot(self.error_noise_vector,visible=True,label='Noise [dBm]')
        self.ax2.legend()
        self.ax2.set_ylim(ylim_wrong)
        self.figure.canvas.draw()
        plt.ion()

        sw = Gtk.ScrolledWindow()
        myfirstwindow.add(sw)

        canvas = FigureCanvas(self.figure)
        canvas.set_size_request(1600, 1200)
        sw.add_with_viewport(canvas)

        myfirstwindow.show_all()
        Gtk.main()

    def plot_correct_rx_stats(self, rssi, cqi, noise, update_rate=1):
        # Shift the data for RSSI.
        self.correct_rssi_vector[:-1] = self.correct_rssi_vector[1:]
        self.correct_rssi_vector[-1:] = rssi
        # Shift the data for CQI.
        self.correct_cqi_vector[:-1] = self.correct_cqi_vector[1:]
        self.correct_cqi_vector[-1:] = cqi
        # Shift the data for Noise.
        self.correct_noise_vector[:-1] = self.correct_noise_vector[1:]
        self.correct_noise_vector[-1:] = 10*math.log10(noise)
        # Update counter.
        self.updated_correct_values_cnt = self.updated_correct_values_cnt + 1
        if(self.updated_correct_values_cnt >= update_rate):
            self.updated_correct_values_cnt = 0
            # set the new data.
            self.line1_correct.set_ydata(self.correct_rssi_vector)
            self.line2_correct.set_ydata(self.correct_cqi_vector)
            self.line3_correct.set_ydata(self.correct_noise_vector)
            # Draw stats.
            self.figure.canvas.draw()

    def plot_wrong_rx_stats(self, rssi, noise, update_rate=1):
        # Shift the data for RSSI.
        self.error_rssi_vector[:-1] = self.error_rssi_vector[1:]
        self.error_rssi_vector[-1:] = rssi
        # Shift the data for CQI.
        self.error_noise_vector[:-1] = self.error_noise_vector[1:]
        self.error_noise_vector[-1:] = 10*math.log10(noise)
        # Update counter.
        self.updated_wrong_values_cnt = self.updated_wrong_values_cnt + 1
        if(self.updated_wrong_values_cnt >= update_rate):
            self.updated_wrong_values_cnt = 0
            # set the new data.
            self.line1_wrong.set_ydata(self.error_rssi_vector)
            self.line2_wrong.set_ydata(self.error_noise_vector)
            # Draw stats.
            self.figure.canvas.draw()

def test_sensys_demo():
    stats_fig = SenSysDemo()
    while True:
        try:
            rssi = np.random.randn(1)
            cqi = np.random.randn(1)
            noise = np.random.randn(1)
            stats_fig.plot_correct_rx_stats(rssi, cqi, noise)
            stats_fig.plot_wrong_rx_stats(rssi, noise)
            time.sleep(0.1)
        except KeyboardInterrupt:
            break

if __name__ == '__main__':

    stats_fig = SenSysDemo()

    while True:
        try:
            rssi = np.random.rand(1)
            cqi = np.random.rand(1)
            noise = np.random.rand(1)
            stats_fig.plot_correct_rx_stats(rssi, cqi, noise)
            stats_fig.plot_wrong_rx_stats(rssi, noise)
            time.sleep(0.1)
        except KeyboardInterrupt:
            break
