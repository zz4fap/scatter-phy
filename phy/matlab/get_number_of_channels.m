function [possible_number_of_channels, phy_bw_in_subbands, unused_bw_due_to_phy_bw_mistach_in_subbands, piece_of_last_channel_in_upper_half_band_in_subbands] = get_number_of_channels(phy_bw, competition_bw, subband_bw)

phy_bw_in_subbands = (phy_bw/subband_bw);

possible_number_of_channels = floor(competition_bw/phy_bw);

unused_bw_due_to_phy_bw_mistach = competition_bw - (possible_number_of_channels*phy_bw);

unused_bw_due_to_phy_bw_mistach_in_subbands = unused_bw_due_to_phy_bw_mistach/subband_bw;

number_of_channels_in_half_competiton_bw = (competition_bw/2)/phy_bw;

piece_of_last_channel_in_upper_half_band = (competition_bw/2) - floor(number_of_channels_in_half_competiton_bw)*phy_bw;

piece_of_last_channel_in_upper_half_band_in_subbands = piece_of_last_channel_in_upper_half_band/subband_bw;

end