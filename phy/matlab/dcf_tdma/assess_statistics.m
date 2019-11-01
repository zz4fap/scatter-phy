clear all;close all;clc

%% ------------------------------------------------------------------------
folder = '/home/zz4fap/work/dcf_tdma/scatter/phy/test_scripts';

mcs_vector = [28];

phy_id = 1;

cfo_mean_single_node = zeros(1,length(mcs_vector));
for idx=1:1:length(mcs_vector)
       
    fileName = sprintf('%s/rx_stats_mcs_%d_tx_20_rx_20_phy_id_%d.dat',folder,mcs_vector(idx),phy_id);
    
    results = load(fileName);
    
    cfo_mean_single_node(idx) = sum(results(:,6)) / length(results(:,6));
    
    fprintf('MCS: %f - CFO mean: %f\n', mcs_vector(idx), cfo_mean_single_node(idx));

end

fdee_figure1 = figure('rend','painters','pos',[10 10 700 600]);
plot(results(:,6),'r')
hold on
plot(cfo_mean_single_node*ones(1,length(results(:,6))),'b')
grid on
hold off
title('MCS: 28 - Tx gain: 20 dB - Rx Gain: 20 dB - PHY ID: 1')
%xticks([0 10 12 14 16 18 20 22 24 26 28])
%yticks([0 10 20 30 40 50 60 70 80 90 100])
xlim([1 length(results(:,6))])
%ylim([0 100])
xlabel('time')
ylabel('CFO [KHz]')

%% ------------------------------------------------------------------------
noise_mean_single_node = zeros(1,length(mcs_vector));
for idx=1:1:length(mcs_vector)
       
    fileName = sprintf('%s/rx_stats_mcs_%d_tx_20_rx_20_phy_id_%d.dat',folder,mcs_vector(idx),phy_id);
    
    results = load(fileName);
    
    noise_mean_single_node(idx) = sum(results(:,4)) / length(results(:,4));
    
    fprintf('MCS: %f - Noise mean: %f\n', mcs_vector(idx), noise_mean_single_node(idx));

end

fdee_figure1 = figure('rend','painters','pos',[10 10 700 600]);
plot(results(:,4),'r')
hold on
plot(noise_mean_single_node*ones(1,length(results(:,4))),'b')
grid on
hold off
title('MCS: 28 - Tx gain: 20 dB - Rx Gain: 20 dB - PHY ID: 1')
%xticks([0 10 12 14 16 18 20 22 24 26 28])
%yticks([0 10 20 30 40 50 60 70 80 90 100])
xlim([1 length(results(:,4))])
%ylim([0 100])
xlabel('time')
ylabel('Noise')

%% ------------------------------------------------------------------------
rssi_mean_single_node = zeros(1,length(mcs_vector));
for idx=1:1:length(mcs_vector)
       
    fileName = sprintf('%s/rx_stats_mcs_%d_tx_20_rx_20_phy_id_%d.dat',folder,mcs_vector(idx),phy_id);
    
    results = load(fileName);
    
    rssi_mean_single_node(idx) = sum(results(:,2)) / length(results(:,2));
    
    fprintf('MCS: %f - RSSI mean: %f [dBW]\n', mcs_vector(idx), rssi_mean_single_node(idx));

end

fdee_figure1 = figure('rend','painters','pos',[10 10 700 600]);
plot(results(:,2),'r')
hold on
plot(rssi_mean_single_node*ones(1,length(results(:,2))),'b')
grid on
hold off
title('MCS: 28 - Tx gain: 20 dB - Rx Gain: 20 dB - PHY ID: 1')
%xticks([0 10 12 14 16 18 20 22 24 26 28])
%yticks([0 10 20 30 40 50 60 70 80 90 100])
xlim([1 length(results(:,2))])
%ylim([0 100])
xlabel('time')
ylabel('RSSI [dBW]')

