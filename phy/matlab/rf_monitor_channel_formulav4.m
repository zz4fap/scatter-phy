clear all;close all;clc

%% Variable parameters.

competition_center_freq = 1e9;

competition_bw = 20e6;

phy_bw = 1.4e6;

%% Fix parameters.

sample_rate = 23.04e6;

NFFT = 2048;

num_of_fft_bins_in_subband = 16;

number_of_subbands = NFFT/num_of_fft_bins_in_subband;

%% Subband values.
[half_unused_bw_in_subbands, half_unused_bw, unused_bw_in_subbands, subband_bw] = get_unused_bw_in_num_of_subbands(competition_bw, sample_rate, NFFT, num_of_fft_bins_in_subband);

%% Channel number.

[possible_number_of_channels, phy_bw_in_subbands, unused_bw_due_to_phy_bw_mistach_in_subbands, piece_of_last_channel_in_upper_half_band_in_subbands] = get_number_of_channels(phy_bw, competition_bw, subband_bw);

indexes = get_allowed_indexes(half_unused_bw_in_subbands, number_of_subbands, unused_bw_due_to_phy_bw_mistach_in_subbands);

channel_counter = zeros(1,possible_number_of_channels);
for idx=1:1:length(indexes)

    subband_idx = indexes(idx);

    channel = get_channel_number(subband_idx, phy_bw_in_subbands, possible_number_of_channels, unused_bw_in_subbands, number_of_subbands, unused_bw_due_to_phy_bw_mistach_in_subbands, piece_of_last_channel_in_upper_half_band_in_subbands);

    fprintf(1,'idx: %d - channel: %d\n',subband_idx,channel);

    channel_counter(channel+1) = channel_counter(channel+1) + 1;

end

fprintf(1,'\nCount of subband bins per channel\n');
for i=1:1:possible_number_of_channels
    fprintf(1,'Channel[%d] bin counter: %d\n',i-1,channel_counter(i));
end
