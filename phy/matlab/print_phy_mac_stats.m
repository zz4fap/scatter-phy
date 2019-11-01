clear all;close all;clc

%BATCH_ID = [19999 20000 20001 20002 20003 20004];

%BATCH_ID = [20025 20027 20029 20030 20031 20032];

%BATCH_ID = [20038 20039 20040 20041 20042 20046];

%BATCH_ID = [20025 20027 20029 20030 20031 20032];

%BATCH_ID = [20038 20039 20040 20041 20042 20046];

BATCH_ID = [20182 20183 20184 20195 20196 20197];

for id_cnt=1:1:length(BATCH_ID)
    
    folder = sprintf('/home/zz4fap/Downloads/link_level_check/LBT/OLD_9/RESERVATION-%d-all-logs/',BATCH_ID(id_cnt));
    
    files_in_folder=dir(folder);
    
    total_number_of_transmitted_slots = 0;
    total_number_of_decoded_slots = 0;
    total_number_of_wrongly_decoded_slots = 0;
    total_number_of_wrongly_detected_slots = 0;
    total_number_of_correctly_received_and_sent_packets_to_app = 0;
    total_number_of_dropped_packet = 0;
    total_number_of_positive_segment_acks = 0;
    total_number_of_failed_acks = 0;
    total_number_of_sent_fragments = 0;
    
    for k=3:1:length(files_in_folder)
        
        tx_stats_struct = [];
        rx_stats_struct = [];
        wrong_rx_stats_struct = [];
        app_packets_stats_struct = [];
        
        filename = files_in_folder(k).name;
        
        ret1 = strfind(filename,'.mat');
        
        ret2 = strfind(filename,'.png');
        
        if(files_in_folder(k).isdir == 0 && ~isempty(ret1) && isempty(ret2))
            
            path_and_filename = sprintf('%s%s',folder,filename);
            load(path_and_filename);
            
            if(~isempty(tx_stats_struct) && ~isempty(rx_stats_struct) && ~isempty(wrong_rx_stats_struct))
                for j=1:1:length(tx_stats_struct)
                    tx_stats = tx_stats_struct(j);
                    if(tx_stats.channel == 0)
                        total_number_of_transmitted_slots = total_number_of_transmitted_slots + sum(tx_stats.number_of_slots);
                    end
                end
                
                for j=1:1:length(rx_stats_struct)
                    rx_stats = rx_stats_struct(j);
                    if(rx_stats.channel == 0)
                        total_number_of_decoded_slots = total_number_of_decoded_slots + rx_stats.total_number_of_slots(length(rx_stats.total_number_of_slots));
                        total_number_of_wrongly_decoded_slots = total_number_of_wrongly_decoded_slots + rx_stats.error(length(rx_stats.error));
                    end
                end
                
                for j=1:1:length(wrong_rx_stats_struct)
                    wrong_rx_stats = wrong_rx_stats_struct(j);
                    if(wrong_rx_stats.channel == 0)
                        total_number_of_wrongly_detected_slots = total_number_of_wrongly_detected_slots + wrong_rx_stats.number_of_slots(length(wrong_rx_stats.number_of_slots));
                    end
                end
            end
            
            if(~isempty(app_packets_stats_struct))

                % Packets received correctly and send to APP = 
                total_number_of_correctly_received_and_sent_packets_to_app = total_number_of_correctly_received_and_sent_packets_to_app + sum(app_packets_stats_struct.count_packet);
                total_number_of_dropped_packet = total_number_of_dropped_packet + sum(app_packets_stats_struct.count_dropped_packets);

                if(~isempty(app_packets_stats_struct.count_acks_success))
                    total_number_of_positive_segment_acks = total_number_of_positive_segment_acks + sum(app_packets_stats_struct.count_acks_success);
                end
                
                if(~isempty(app_packets_stats_struct.count_acks_failed))
                    total_number_of_failed_acks = total_number_of_failed_acks + sum(app_packets_stats_struct.count_acks_failed);
                end
                 
                if(~isempty(app_packets_stats_struct.fragments_sent))
                    total_number_of_sent_fragments = total_number_of_sent_fragments + sum(app_packets_stats_struct.fragments_sent);
                end
                
            end
        end
        
        if(~isempty(tx_stats_struct) && ~isempty(rx_stats_struct) && ~isempty(wrong_rx_stats_struct))
            clear tx_stats_struct rx_stats_struct wrong_rx_stats_struct
        end
        
        if(~isempty(app_packets_stats_struct))
           clear app_packets_stats_struct 
        end
        
    end
    
    if(total_number_of_transmitted_slots > 0 && 0)
        %fprintf(1,'PHY BATCH: %d - %d \t %d \t %d \t %d\n',BATCH_ID(id_cnt), total_number_of_transmitted_slots,total_number_of_decoded_slots,total_number_of_wrongly_decoded_slots,total_number_of_wrongly_detected_slots);
        fprintf(1,'%d \t %d \t %d \t %d\n', total_number_of_transmitted_slots,total_number_of_decoded_slots,total_number_of_wrongly_decoded_slots,total_number_of_wrongly_detected_slots);
    end
    
    if(total_number_of_sent_fragments > 0 && 1)
        %fprintf(1,'MAC BATCH: %d - %d\t%d\t%d\t%d\t%d\n',BATCH_ID(id_cnt),total_number_of_sent_fragments,total_number_of_positive_segment_acks,total_number_of_failed_acks,total_number_of_correctly_received_and_sent_packets_to_app,total_number_of_dropped_packet);
        fprintf(1,'%d\t%d\t%d\t%d\t%d\n',total_number_of_sent_fragments,total_number_of_positive_segment_acks,total_number_of_failed_acks,total_number_of_correctly_received_and_sent_packets_to_app,total_number_of_dropped_packet);
    end
    
end