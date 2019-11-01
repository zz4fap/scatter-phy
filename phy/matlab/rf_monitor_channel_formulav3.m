clear all;close all;clc

%% Variable parameters.
subband_idx = 36+28;

competition_center_freq = 1e9;

competition_bw = 10e6;

phy_bw = 5e6;

%% Fix parameters.

sample_rate = 23.04e6;

NFFT = 2048;

num_of_fft_bins_in_subband = 16;

%% Subband values.

subband_bw = num_of_fft_bins_in_subband*(sample_rate/NFFT);

unused_bw = sample_rate - competition_bw;

half_unused_bw = unused_bw/2;

half_unused_bw_in_subbands = half_unused_bw/subband_bw;

%% Channel number.

phy_bw_in_subbands = phy_bw/subband_bw;

possible_number_of_channels = floor(competition_bw/phy_bw);

channel_double = abs(subband_idx-half_unused_bw_in_subbands)/phy_bw_in_subbands;

channel = mod((fix(channel_double) + possible_number_of_channels/2), possible_number_of_channels);
