clear all;close all;clc

channel_center_freq = 992.5e6;

competition_center_freq = 1e9;

competition_bw = 20e6;

sample_rate = 23.04e6;

NFFT = 2048;

num_of_fft_bins_in_subband = 16;

subband_bw = num_of_fft_bins_in_subband*(sample_rate/NFFT);

half_unused_bw = ((sample_rate - competition_bw)/2);

half_unused_bw_in_subbands = half_unused_bw/subband_bw;

pos_in_subbands = (channel_center_freq - (competition_center_freq - competition_bw/2) + half_unused_bw)/subband_bw;

%% ***** PHY BW ******
phy_bw = 5e6;

phy_bw_in_subbands = phy_bw/subband_bw;

channel_double = (pos_in_subbands-half_unused_bw_in_subbands)/phy_bw_in_subbands;

channel = fix(channel_double)


