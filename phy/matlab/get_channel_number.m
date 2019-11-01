function [channel] = get_channel_number(subband_idx, phy_bw_in_subbands, possible_number_of_channels, unused_bw_in_subbands, number_of_subbands, unused_bw_due_to_phy_bw_mistach_in_subbands, piece_of_last_channel_in_upper_half_band_in_subbands)

if(subband_idx > number_of_subbands/2)
    offset = -1*((unused_bw_in_subbands) + (unused_bw_due_to_phy_bw_mistach_in_subbands)) + piece_of_last_channel_in_upper_half_band_in_subbands;
else
    offset = (piece_of_last_channel_in_upper_half_band_in_subbands);
end

channel_double = abs(subband_idx+(offset))/phy_bw_in_subbands;

channel = mod((fix(channel_double + (possible_number_of_channels/2))), possible_number_of_channels);

end