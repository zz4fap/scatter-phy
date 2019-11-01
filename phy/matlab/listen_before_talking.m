clear all;close all;clc

NFFT= 1024;

fs = 23040000;

delta_f = (fs/NFFT);

bw = 5e6; % PHY Bandwidth.

bw_in_fft_bins = floor(bw/delta_f);



%% ----------------------------------------------------------------------------------
center_freq = 1e9;

competition_bw = 20e6;

number_of_channels = competition_bw/bw;

offset = fs/2 - competition_bw/2;

offset_in_fft_bins = ceil(offset/delta_f);

fprintf(1,'*** Lower frequency bound: %1.3f ***\n',(center_freq-(competition_bw/2))/1000000.0);

for tx_ch=0:1:number_of_channels-1
    
    tx_channel_init_freq = tx_ch*bw;
    
    tx_channel_init_in_fft_bins = bw_in_fft_bins*tx_ch + offset_in_fft_bins;
    
    tx_channel_init_freq = tx_channel_init_freq + offset;
    
    fprintf(1,'channel: %d - TX Channel init freq: %1.3f - tx_channle_init_in_fft_bins: %1.3f - approximated init freq: %f\n',tx_ch,tx_channel_init_freq/1000000.0,tx_channel_init_in_fft_bins, tx_channel_init_in_fft_bins*delta_f);
    
end

fprintf(1,'*** Upper frequency bound: %1.3f ***\n\n\n',(center_freq+(competition_bw/2))/1000000.0);

