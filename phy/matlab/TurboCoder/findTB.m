clear all;close all;clc

bw_idx = 2;

subframe5_max_nof_bits = [4464, 5424, 13104, 22704, 46704, 70704, 94704];

mcs_28_bits = [4840, 5736, 11576, 19336, 4904*8, 7327*8, 10035*8];

tbLen = mcs_28_bits(bw_idx);

for iter=1:1:100000
    
    tbLen = tbLen + 8;

    [C, Kplus, validK, bByC, b] = CblkSegParams(tbLen);
    
    if(bByC == Kplus && b < subframe5_max_nof_bits(bw_idx))
        fprintf(1, 'C: %d - Kplus: %d - bByC: %1.2f - tbLen: %d - total: %d - b: %d - TB in bytes: %d\n', C, Kplus, bByC, tbLen, C*Kplus, b, tbLen/8);
    end

end

% Total number of bits in subframe #5
% 4464 - 1.4 MHz (6 RBs)
% 5424 - 1.4 MHz (7 RBs)
% 13104 - 3 MHz
% 22704 - 5 MHz

% Total number of bits in subframe #6
% 5760 - 1.4 MHz
