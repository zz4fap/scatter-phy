clear all;close all;clc

% 1 - result
% 2 - channel
% 3 - mcs_idx
% 4 - rssi
% 5 - cqi
% 6 - noise
% 7 - sinr
% 8 - total_nof_detected
% 9 - nof_detection_errors
% 10 - nof_decoding_errors
% 11 - total_nof_synchronized
% 12 - nof_decoded_pkts

%% ------------------------------------------------------------------------
folder = '/home/zz4fap/Downloads/ccnf_mcs';

mcs_vector = [0 10 15 20 23 25 26 27 28];

prr_single_node = zeros(1,length(mcs_vector));
prr_percentage_single_node = zeros(1,length(mcs_vector));
for idx=1:1:length(mcs_vector)
       
    fileName = sprintf('%s/ccnf_mcs_%d.txt',folder,mcs_vector(idx));
    
    results = load(fileName);
    
    total_pkts =  results(length(results),11) - results(1,11) + 1;
    
    correctly_decoded = results(length(results), 8);
    
    prr_single_node(idx) = correctly_decoded / total_pkts;
    
    prr_percentage_single_node(idx) = prr_single_node(idx)*100;
    
    valid_result = find(results(:,1) == 100);
    
    if(isempty(valid_result))
        mcs_idx = -1;
        fprintf(1,'The file only contains decoding error information. No packet was decoded....\n')
    else
        mcs_idx = results(valid_result(length(valid_result)), 3);
    end
    
    fprintf('MCS: %d - Correctly decoded : %d - PRR: %f - PRR: %f %%\n', mcs_idx, correctly_decoded, prr_single_node(idx),prr_percentage_single_node(idx));

end