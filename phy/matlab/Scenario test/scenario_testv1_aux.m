clear all;close all;clc

folder = '/home/zz4fap/work/mac_redesign/';

%filename = 'tx_channel_1_rx_channel_0_tx_20_rx_13.dat';
filename = 'tx_channel_0_rx_channel_1_tx_28_rx_07_scrimmage7-62-hwfir.dat';

pdf_filename = filename(1:end-4);

k = strfind(filename,'_tx_');

tx_gain = str2num(filename(k+4:k+4+1));

k = strfind(filename,'_rx_');

rx_gain = str2num(filename(k(2)+4:k(2)+4+1));

titleStr = sprintf('Tx gain: %d [dB] - Rx gain: %d [dB]', tx_gain, rx_gain);

results = load(sprintf('%s%s',folder,filename));

rssi = results(:,7);

cqi = results(:,8);

cfo = results(:,6);

noise = results(:,9);

status = results(:,2);

time = 0:1:length(results(:,1))-1;

figure;
subplot(5,1,1)
plot(time, rssi)
xlabel('Time [s]')
ylabel('RSSI [dBW]')
xlim([time(1) time(length(time))])
grid on
title(titleStr);

subplot(5,1,2)
plot(time, cqi)
xlabel('Time [s]')
ylabel('CQI')
xlim([time(1) time(length(time))])
grid on

subplot(5,1,3)
plot(time, cfo)
xlabel('Time [s]')
ylabel('CFO [KHz]')
xlim([time(1) time(length(time))])
grid on

subplot(5,1,4)
semilogy(time, noise)
xlabel('Time [s]')
ylabel('Noise')
yticks([1e-6 1e-4 1e-3])
xlim([time(1) time(length(time))])
grid on

subplot(5,1,5)
plot(time, status,'*')
xlabel('Time [s]')
ylabel('Status')
yticks([100 101])
xlim([time(1) time(length(time))])
grid on

print(pdf_filename,'-dpdf','-fillpage')

cfo_filename = sprintf('cfo_%s',pdf_filename);
figure;
plot(time, cfo)
xlabel('Time [s]')
ylabel('CFO [KHz]')
xlim([time(1) time(length(time))])
yticks([-8:1:8])
grid on
print(cfo_filename,'-dpdf','-fillpage')