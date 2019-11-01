function [half_unused_bw_in_subbands, half_unused_bw, unused_bw_in_subbands, subband_bw] = get_unused_bw_in_num_of_subbands(competition_bw, sample_rate, NFFT, num_of_fft_bins_in_subband)

subband_bw = num_of_fft_bins_in_subband*(sample_rate/NFFT);

unused_bw = sample_rate - competition_bw;

unused_bw_in_subbands = unused_bw/subband_bw;

half_unused_bw = unused_bw/2;

half_unused_bw_in_subbands = half_unused_bw/subband_bw;

end