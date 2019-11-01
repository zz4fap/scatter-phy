clear all;close all;clc



%% ------------------------------------------------------------------------
%% TX only

c_tx_only = categorical({'memcopy','fft processing','resource element mapping','bit interleave','memset','data transfer to USRP','volk multiplication','turbo encoding LUT','modulate bytes','CRC checksum'});

MCS_0_tx_only = [29.81, 14.91, 10.73, 9.25, 8.19, 6.66, 6.4, 0.86, 3.31, 0.22];

MCS_15_tx_only = [15.03, 7.49, 5.39, 47.82, 4.12, 3.35, 3.21, 4.19, 1.99, 2.23];

MCS_28_tx_only = [8.64, 4.01, 2.89, 64.65, 2.21, 1.79, 1.72, 5.64, 2.49, 3.01];

%% ------------------------------------------------------------------------
%% TX only

c_rx_only = categorical({'viterbi decoding','synchronization','memcopy','fft processing','predecoding','turbo decoding iteration','memset','volk index max'});

MCS_0_rx_only = [46.32, 27.85, 14.26, 10.57, 8.11, 4.65, 4.2, 2.91];

MCS_15_rx_only = [26.21, 31.84, 15.49, 11.59, 4.59, 27.27, 2.98, 3.41];

MCS_28_rx_only = [12.05, 42.68, 20.08, 15.13, 2.11, 36.45, 1.71, 4.65];

%% ------------------------------------------------------------------------

fdee_figure1 = figure('rend','painters','pos',[10 10 1100 1000]);

subplot(3,2,1)
bar(c_tx_only,MCS_0_tx_only)
ylabel('CPU %')
ylim([0 70])
yticks([0 20 40 60 70])
title('MCS 0 - Tx only')
grid on;

subplot(3,2,3)
bar(c_tx_only,MCS_15_tx_only)
ylabel('CPU %')
ylim([0 70])
yticks([0 20 40 60 70])
title('MCS 15 - Tx only')
grid on;

subplot(3,2,5)
bar(c_tx_only,MCS_28_tx_only)
ylabel('CPU %')
ylim([0 70])
yticks([0 20 40 60 70])
title('MCS 28 - Tx only')
grid on;

subplot(3,2,2)
bar(c_rx_only,MCS_0_rx_only)
ylabel('CPU %')
ylim([0 50])
yticks([0 20 40 60 70])
title('MCS 0 - Rx only')
grid on;

subplot(3,2,4)
bar(c_rx_only,MCS_15_rx_only)
ylabel('CPU %')
ylim([0 50])
yticks([0 20 40 60 70])
title('MCS 15 - Rx only')
grid on;

subplot(3,2,6)
bar(c_rx_only,MCS_28_rx_only)
ylabel('CPU %')
ylim([0 50])
yticks([0 20 40 60 70])
title('MCS 28 - Rx only')
grid on;

%fdee_figure1.PaperOrientation = 'landscape';

print('functions_cpu_profilling.pdf','-dpdf','-fillpage')

%print('functions_cpu_profilling2.pdf','-dpdf','-bestfit')

