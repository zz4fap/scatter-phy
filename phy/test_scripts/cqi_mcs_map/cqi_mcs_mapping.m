clear all;close all;clc

% BW Index - MCS - SNR - PSS PRR - PDSCH PRR

bw_idx = 1;

bw_index_vector = [1 2 3];

bw_vector = [7 15 25];

slot_vector = [1];

mcs_vector = 0:1:31;

SNR_vector = 27:-1:-1;

mapping_values = zeros(length(SNR_vector), 5, length(mcs_vector));
table = zeros(length(mcs_vector), length(SNR_vector));

folderName = sprintf('./dir_cqi_mcs_map_prb_%d/', bw_vector(bw_idx));

for mcs_idx = 0:1:31
    
    for nof_slots_idx = 1:1:length(slot_vector)
        
        fileName = sprintf('%scqi_mcs_map_phy_bw_%d_mcs_%d.dat', folderName, bw_index_vector(bw_idx), mcs_idx);
        
        ret = exist(fileName);
        
        if(ret > 0)
            
            map_aux = load(fileName);
            
            mapping_values(:,:,mcs_idx+1) = map_aux;
            
            aux = map_aux(:, 5).';
            
            table(mcs_idx+1, :) = aux;

        end
        
    end
    
end

xvalues = {'27','26','25','24','23','22','21','20','19','18','17','16','15','14','13','12','11','10','9','8','7','6','5','4','3','2','1','0','-1'};
yvalues = {'0','1','2','3', '4', '5', '6', '7', '8', '9' ,'10','11','12','13','14','15','16','17','18','19','20','21','22','23','24','25','26','27','28','29','30','31'};
h = heatmap(xvalues,yvalues,table);
titleStr = sprintf('NPRB: %d', bw_vector(1));
h.Title = titleStr;
h.XLabel = 'SNR [dB]';
h.YLabel = 'MCS';

