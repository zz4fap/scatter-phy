function [indexes] = get_allowed_indexes(half_unused_bw_in_subbands, number_of_subbands, unused_bw_due_to_phy_bw_mistach_in_subbands)

indexes = [0:1:(number_of_subbands/2)-(ceil(half_unused_bw_in_subbands+unused_bw_due_to_phy_bw_mistach_in_subbands)), (number_of_subbands/2)+ceil(half_unused_bw_in_subbands):1:number_of_subbands-1];

end