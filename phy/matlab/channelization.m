clear all;close all;clc

center_freq = 1e9;

competition_bw = 20e6;

bw = 5e6; % PHY Bandwidth.

numebr_of_channels = competition_bw/bw;

fprintf(1,'*** Lower frequency bound: %1.3f ***\n',(center_freq-(competition_bw/2))/1000000.0);

for tx_ch=0:1:numebr_of_channels-1
    
    
    tx_channel_center_freq = center_freq - (competition_bw/2.0) + (bw/2.0) + tx_ch*bw;
    
    
    fprintf(1,'channel: %d - tx_channel_center_freq: %1.3f\n',tx_ch,tx_channel_center_freq/1000000.0);
    
end

fprintf(1,'*** Upper frequency bound: %1.3f ***\n\n\n',(center_freq+(competition_bw/2))/1000000.0);

%% ----------------------------------------------------------------------------------------------
bw = 5e6; % PHY Bandwidth.

center_freq = 1e9 + bw/2;

competition_bw = 20e6;

numebr_of_channels = competition_bw/bw;

fprintf(1,'*** Lower frequency bound: %1.3f ***\n',(center_freq - (competition_bw/2) - (bw/2))/1000000.0);

for tx_ch=0:1:numebr_of_channels-1
    
    
    tx_channel_center_freq = center_freq - (competition_bw/2.0) + (bw/2.0) + tx_ch*bw;
    
    
    fprintf(1,'channel: %d - tx_channel_center_freq: %1.3f\n',tx_ch,tx_channel_center_freq/1000000.0);
    
end

fprintf(1,'*** Upper frequency bound: %1.3f ***\n\n\n',(center_freq + (competition_bw/2) - (bw/2))/1000000.0);