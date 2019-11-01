from gi.repository import Gtk
from matplotlib.figure import Figure
import matplotlib.pyplot as plt
import numpy as np
import time
import math
import _thread
import threading
import signal, os
import queue
from matplotlib.backends.backend_gtk3cairo import FigureCanvasGTK3Cairo as FigureCanvas
from gi.repository import GObject as gobject
import radio
import measure_lbt_performance

class SenSysDemo2():
    def __init__(self, lc, ylim_correct=[-42,20], ylim_wrong=[-42,2], cmds=[]):

        self.lc = lc

        self.commands = cmds;

        self.create_queues()

        self.window = Gtk.Window(title="SenSys'17 - Packetized-LTE PHY")
        self.window.set_default_size(1200, 900)

        self.superbox = Gtk.Box()
        self.window.add(self.superbox)

        self.boxplot = Gtk.Box()
        self.superbox.pack_start(self.boxplot, True, True, 0)

        self.grid = Gtk.Grid()
        self.superbox.pack_start(self.grid, False, False, 0)

        self.boxbuttons = Gtk.Box()
        self.grid.add(self.boxbuttons)

        self.boxstats = Gtk.Box()
        self.grid.attach_next_to(self.boxstats, self.boxbuttons, Gtk.PositionType.BOTTOM, 1, 2)

        button = Gtk.Button.new_with_label("Single Link")
        button.connect("clicked", self.on_single_radio)
        self.boxbuttons.pack_start(button, True, True, 0)

        button = Gtk.Button.new_with_label("LBT Disabled")
        button.connect("clicked", self.on_lbt_disabled)
        self.boxbuttons.pack_start(button, True, True, 0)

        button = Gtk.Button.new_with_label("LBT Enabled")
        button.connect("clicked", self.on_lbt_enabled)
        self.boxbuttons.pack_start(button, True, True, 0)

        self.box = Gtk.Box()
        self.boxplot.pack_start(self.box, True, True, 0)

        scale = 10
        self.fig = Figure(figsize=(4*scale,3*scale))
        self.ax1 = self.fig.add_subplot(2, 1, 1)
        plt.grid(True)
        self.ax2 = self.fig.add_subplot(2, 1, 2)
        plt.grid(True)
        # draw and show it
        self.ax1.set_title("Successful Statistics")
        self.line1_correct, = self.ax1.plot(np.zeros(measure_lbt_performance.CORRECT_ARRAY_SIZE),'b',visible=True,label='RSSI [dBm]')
        self.line2_correct, = self.ax1.plot(np.zeros(measure_lbt_performance.CORRECT_ARRAY_SIZE),'k',visible=True,label='CQI')
        self.line3_correct, = self.ax1.plot(np.zeros(measure_lbt_performance.CORRECT_ARRAY_SIZE),'g',visible=True,label='Noise [dBm]')
        self.ax1.legend()
        self.ax1.grid(True)
        self.ax1.set_ylim(ylim_correct)
        self.ax2.set_title("Unsuccessful Statistics")
        self.line1_wrong, = self.ax2.plot(np.zeros(measure_lbt_performance.ERROR_ARRAY_SIZE),'b',visible=True,label='RSSI [dBm]')
        self.line2_wrong, = self.ax2.plot(np.zeros(measure_lbt_performance.ERROR_ARRAY_SIZE),'g',visible=True,label='Noise [dBm]')
        self.ax2.legend()
        self.ax2.grid(True)
        self.ax2.set_ylim(ylim_wrong)
        self.canvas = FigureCanvas(self.fig)
        self.canvas.show()
        self.box.pack_start(self.canvas, True, True, 0)
        plt.ion()

        self.liststore_correct_rx = Gtk.ListStore(str, float)
        self.liststore_correct_rx.append(["In sequence %", 0.0])
        self.liststore_correct_rx.append(["Out of sequence  %", 0.0])
        self.liststore_correct_rx.append(["Correct %", 0.0])
        self.liststore_correct_rx.append(["Error %", 0.0])
        self.liststore_correct_rx.append(["Channel free %", 0.0])
        self.liststore_correct_rx.append(["Channel busy %", 0.0])
        self.liststore_correct_rx.append(["Channel free pwr", 0.0])
        self.liststore_correct_rx.append(["Channel busy pwr", 0.0])
        self.liststore_correct_rx.append(["Avg. coding time", 0.0])
        self.liststore_correct_rx.append(["Avg. # TX slots", 0.0])
        treeview = Gtk.TreeView(model=self.liststore_correct_rx)
        renderer_text = Gtk.CellRendererText()
        column_text = Gtk.TreeViewColumn("Statistics", renderer_text, text=0)
        treeview.append_column(column_text)
        renderer_spin = Gtk.CellRendererSpin()
        renderer_spin.set_property("editable", False)
        column_spin = Gtk.TreeViewColumn("Value", renderer_spin, text=1)
        treeview.append_column(column_spin)
        self.boxstats.pack_start(treeview, True, True, 0)

        gobject.idle_add(self.update_graph)

    def create_queues(self):
        self.rx_success_error_queue = queue.Queue()
        self.rx_success_plot_queue = queue.Queue()
        self.rx_error_plot_queue = queue.Queue()
        self.tx_stats_queue = queue.Queue()
        self.decoded_stats_queue = queue.Queue()

    def clear_queues(self):
        while(self.rx_success_error_queue.qsize() > 0): dumb = self.rx_success_error_queue.get()
        while(self.rx_success_plot_queue.qsize() > 0): dumb = self.rx_success_plot_queue.get()
        while(self.rx_error_plot_queue.qsize() > 0): dumb = self.rx_error_plot_queue.get()
        while(self.tx_stats_queue.qsize() > 0): dumb = self.tx_stats_queue.get()
        while(self.decoded_stats_queue.qsize() > 0): dumb = self.decoded_stats_queue.get()

    def close_all(self, widget=None, *data):
        # Do some other clean up before killing the loop.
        radio.stop_radio()
        Gtk.main_quit()
        os.system("sudo killall -9 python3")

    def init(self):
        self.window.connect("delete-event", self.close_all)
        self.window.show_all()

    def set_commands(self, cmds):
        self.commands = cmds

    def on_single_radio(self, button):
        print("Single Link")
        radio.stop_radio()
        print(self.commands[0])
        self.clear_queues()
        radio.start_radio(self.commands[0],self.lc,self)

    def on_lbt_disabled(self, button):
        print("Two Links: LBT disabled")
        radio.stop_radio()
        print(self.commands[1])
        self.clear_queues()
        radio.start_radio(self.commands[1],self.lc,self)

    def on_lbt_enabled(self, button):
        print("Two Links: LBT enabled")
        radio.stop_radio()
        print(self.commands[2])
        self.clear_queues()
        radio.start_radio(self.commands[2],self.lc,self)

    def update_graph(self):

        if(self.decoded_stats_queue.empty() == False):
            try:
                decoded = self.decoded_stats_queue.get_nowait()
                self.liststore_correct_rx[0][1] = decoded.in_sequence_percentage # In sequence
                self.liststore_correct_rx[1][1] = decoded.out_of_sequence_percentage # Out of sequence
            except queue.Empty:
                pass

        if(self.rx_success_error_queue.empty() == False):
            try:
                internal = self.rx_success_error_queue.get_nowait()
                self.liststore_correct_rx[2][1] = internal.correct # Correct
                self.liststore_correct_rx[3][1] = internal.error # Error
            except queue.Empty:
                pass

        if(self.tx_stats_queue.empty() == False):
            try:
                internal = self.tx_stats_queue.get_nowait()
                self.liststore_correct_rx[8][1] = internal.average_coding_time # average coding Time
                self.liststore_correct_rx[9][1] = internal.average_num_of_tx_slots # average number of tx slots
                self.liststore_correct_rx[4][1] = internal.channel_free_percentage # LBT Free
                self.liststore_correct_rx[5][1] = internal.channel_busy_percentage # LBT Busy
                self.liststore_correct_rx[6][1] = internal.channel_free_avg_power # Channle Free Power
                self.liststore_correct_rx[7][1] = internal.channel_busy_avg_power # Channle Busy Power
            except queue.Empty:
                pass

        if(self.rx_success_plot_queue.empty() == False):
            try:
                internal = self.rx_success_plot_queue.get_nowait()
                self.plot_correct_rx_stats(internal)
            except queue.Empty:
                pass

        if(self.rx_error_plot_queue.empty() == False):
            try:
                internal = self.rx_error_plot_queue.get_nowait()
                self.plot_wrong_rx_stats(internal)
            except queue.Empty:
                pass

        return True

    # Plot figure with correct decoding statistics.
    def plot_correct_rx_stats(self, internal):
        # set the new data.
        self.line1_correct.set_ydata(internal.correct_rssi_vector)
        self.line2_correct.set_ydata(internal.correct_cqi_vector)
        self.line3_correct.set_ydata(internal.correct_noise_vector)
        # Draw stats.
        self.fig.canvas.draw()

    # Plot figure with wrong decoding/dectection statistics.
    def plot_wrong_rx_stats(self, internal):
        # set the new data.
        self.line1_wrong.set_ydata(internal.error_rssi_vector)
        self.line2_wrong.set_ydata(internal.error_noise_vector)
        # Draw stats.
        self.fig.canvas.draw()

class Decoded():
    in_sequence_percentage = 0
    out_of_sequence_percentage = 0

    def __init__(self, in_sequence_percentage, out_of_sequence_percentage):
        self.in_sequence_percentage = in_sequence_percentage
        self.out_of_sequence_percentage = out_of_sequence_percentage

class RxSuccessError():
    correct = 0
    error = 0

    def __init__(self, correct, error):
        self.correct = correct
        self.error = error

class SuccessRxForPlot():
    correct_rssi_vector = []
    correct_cqi_vector = []
    correct_noise_vector = []

    def __init__(self, correct_rssi_vector, correct_cqi_vector, correct_noise_vector):
        self.correct_rssi_vector = correct_rssi_vector
        self.correct_cqi_vector = correct_cqi_vector
        self.correct_noise_vector = correct_noise_vector

class ErrorRxForPlot():
    error_rssi_vector = []
    error_noise_vector = []

    def __init__(self, error_rssi_vector, error_noise_vector):
        self.error_rssi_vector = error_rssi_vector
        self.error_noise_vector = error_noise_vector

class TxLbtStats():
    average_coding_time = 0
    average_num_of_tx_slots = 0
    channel_free_percentage = 0
    channel_busy_percentage = 0
    channel_free_avg_power = 0
    channel_busy_avg_power = 0

    def __init__(self, average_coding_time, average_num_of_tx_slots, channel_free_percentage, channel_busy_percentage, channel_free_avg_power, channel_busy_avg_power):
        self.average_coding_time = average_coding_time
        self.average_num_of_tx_slots = average_num_of_tx_slots
        self.channel_free_percentage = channel_free_percentage
        self.channel_busy_percentage = channel_busy_percentage
        self.channel_free_avg_power = channel_free_avg_power
        self.channel_busy_avg_power = channel_busy_avg_power

def start_demo_plot():
    mc = SenSysDemo2()
    mc.window.connect("delete-event", Gtk.main_quit)
    mc.window.show_all()
    Gtk.main()

if __name__ == '__main__':
    mc = SenSysDemo2()
    mc.window.connect("delete-event", Gtk.main_quit)
    mc.window.show_all()
    Gtk.main()
