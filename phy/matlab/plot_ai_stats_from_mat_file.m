clear all;close all;clc

% This is the template used to plot values regarding ACKs.
% ACK_PROCESSED: <timestamp_ns>, <mac_address>, <ack_success>, <num_retransmissions>, <old_mcs>, <old_gain>,                          <rssi>, <cqi>, <new_mcs>, <new_gain>
% ACK_PROCESSED:  timestamp_ns,   mac_address,   ack_success,   num_retransmissions,   old_mcs,   old_gain,    channel,    rx_gain,    rssi,   cqi,   new_mcs,   new_gain
% ACK_PROCESSED:  now,            mac,           success,       retries,               old_mcs,   old_gain,    channel,    rx_gain,    rssi,   cqi,   mcs,       gain

BATCH_ID = [20046];

figure_counter = 0;
filename = 'ai_mcs_stdout.txt';
for id_cnt=1:1:length(BATCH_ID)
    %folder = './scrimmage5_test/';
    %folder = '/home/zz4fap/Downloads/link_level_check/LBT/RESERVATION-15291/scatter-master-with-sw-lbt-srn47-RES15291/';
    folder = sprintf('/home/zz4fap/Downloads/link_level_check/LBT/OLD_6/RESERVATION-%d-all-logs/',BATCH_ID(id_cnt));
    
    subfolders = dir(folder);
    
    for k=3:1:length(subfolders)
        
        filename = subfolders(k).name;
               
        ret1 = strfind(filename,'-ai-stats-structs.mat');     
                    
        if(subfolders(k).isdir == 0 && ~isempty(ret1))
                      
            trial_name = filename;
            
            % Open file.
            full_path_to_file = sprintf('%s%s',folder,filename);
            
            clear acks_struct
            load(full_path_to_file);
            
            %% Now, plot things for each MAC address.
            LOWER_RSSI_BOUND = -37;
            UPPER_RSSI_BOUND = -7;
            
            number_of_acks = length(acks_struct);
            for i=1:1:number_of_acks
                figure_counter = figure_counter + 1;
                figure(figure_counter);
                mac_stats_struct = acks_struct(i);
                
                % Plot MCS and TX Gain.
                is_there_timeout = false;
                at_least_one_ack_received = false;
                ack_success = [];
                ack_success.timestamp_diff = [];
                ack_success.old_mcs = [];
                ack_success.new_mcs = [];
                ack_success.old_tx_gain = [];
                ack_success.new_tx_gain = [];
                ack_success.cqi = [];
                ack_success.rssi = [];
                timeout = [];
                timeout.timestamp_diff = [];
                timeout.new_mcs = [];
                timeout.new_tx_gain = [];
                for i = 1:1:length(mac_stats_struct.timestamp_diff)
                    if(mac_stats_struct.ack_success(i) == true)
                        at_least_one_ack_received = true;
                        ack_success.timestamp_diff = [ack_success.timestamp_diff, mac_stats_struct.timestamp_diff(i)];
                        ack_success.old_mcs = [ack_success.old_mcs, mac_stats_struct.mac_stats.old_mcs(i)];
                        ack_success.new_mcs = [ack_success.new_mcs, mac_stats_struct.mac_action.new_mcs(i)];
                        ack_success.old_tx_gain = [ack_success.old_tx_gain, mac_stats_struct.mac_stats.old_tx_gain(i)];
                        ack_success.new_tx_gain = [ack_success.new_tx_gain, mac_stats_struct.mac_action.new_tx_gain(i)];
                        ack_success.cqi = [ack_success.cqi, mac_stats_struct.mac_stats.cqi(i)];
                        ack_success.rssi = [ack_success.rssi, mac_stats_struct.mac_stats.rssi(i)];
                    else
                        is_there_timeout = true;
                        timeout.timestamp_diff = [timeout.timestamp_diff, mac_stats_struct.timestamp_diff(i)];
                        timeout.new_mcs = [timeout.new_mcs, mac_stats_struct.mac_action.new_mcs(i)];
                        timeout.new_tx_gain = [timeout.new_tx_gain, mac_stats_struct.mac_action.new_tx_gain(i)];
                    end
                end
                
                % Change the limits to analyse some specific part of the plot.
                minimum_time_diff = min(mac_stats_struct.timestamp_diff);
                maximum_time_diff = max(mac_stats_struct.timestamp_diff);
                % Figure out maximum and minimum time axis values.
                x_axis_bounds = minimum_time_diff:maximum_time_diff;
                
                if(at_least_one_ack_received)                 
                    % Plot RSSI.
                    plot(ack_success.timestamp_diff, ack_success.rssi,'b','linewidth',2);
                    hold on
                    
                    % Plot CQI.
                    plot(ack_success.timestamp_diff, ack_success.cqi,'r','linewidth',2);
                    % Plot Average CQI.
                    avg_cqi = sum(ack_success.cqi)/length(ack_success.cqi);
                    plot(x_axis_bounds, avg_cqi*ones(1,length(x_axis_bounds)),'--r','linewidth',2);
                    avg_cqi_legend_str = sprintf('Rxed Avg. CQI: %1.2f',avg_cqi);
                    
                    % Plot MCS for received ACK and timeout.
                    plot(ack_success.timestamp_diff, ack_success.old_mcs,'k','linewidth',2);
                    plot(ack_success.timestamp_diff, ack_success.new_mcs,'ko','linewidth',2);
                    if(is_there_timeout == true)
                        plot(timeout.timestamp_diff, timeout.new_mcs,'kx','linewidth',2);
                    end
                    % Plot TX Gain for received ACK and timeout.
                    plot(ack_success.timestamp_diff, ack_success.old_tx_gain,'g','linewidth',2);
                    plot(ack_success.timestamp_diff, ack_success.new_tx_gain,'go','linewidth',2);
                    if(is_there_timeout == true)
                        plot(timeout.timestamp_diff, timeout.new_tx_gain,'gx','linewidth',2);
                    end
                    
                    % Plot number of Re-Transmissions.
                    plot(mac_stats_struct.timestamp_diff, mac_stats_struct.number_of_retx,'linewidth',2);
                    
                    % Plot upper/lower bounds.
                    plot(x_axis_bounds,UPPER_RSSI_BOUND*ones(1,length(x_axis_bounds)),'linewidth',5);
                    plot(x_axis_bounds,LOWER_RSSI_BOUND*ones(1,length(x_axis_bounds)),'linewidth',5);
                    
                    xlim([minimum_time_diff maximum_time_diff]);
                    title_str = sprintf('Rxed ACK - %s - MAC: %d',trial_name(1:end-4),mac_stats_struct.mac);
                    xlabel('Time [s]');
                    title(title_str);
                    if(is_there_timeout == true)
                        legend('Rxed ACK RSSI','Rxed ACK CQI',avg_cqi_legend_str,'Rxed ACK MCS','Action New MCS','Timeout New MCS','Rxed ACK TX Gain','Action New Tx Gain','Timeout New TX Gain','Number of Re-Tx','UPPER RSSI BOUND','LOWER RSSI BOUND');
                    else
                        legend('Rxed ACK RSSI','Rxed ACK CQI',avg_cqi_legend_str,'Rxed ACK MCS','Action New MCS','Rxed ACK TX Gain','Action New Tx Gain','Number of Re-Tx','UPPER RSSI BOUND','LOWER RSSI BOUND');
                    end
                    grid on
                    hold off
                    
                    % Save figure.
                    scaleFactor = 1.6;
                    set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
                    saveas_file_name = sprintf('%s_mac_%d.png',trial_name,mac_stats_struct.mac);
                    saveas(gcf,saveas_file_name);                  
                else                   
                    % Plot MCS for received ACK and timeout.
                    plot(timeout.timestamp_diff, timeout.new_mcs,'kx','linewidth',2);
                    hold on
                    
                    % Plot TX Gain for received ACK and timeout.
                    plot(timeout.timestamp_diff, timeout.new_tx_gain,'g','linewidth',2);
                    
                    % Plot number of Re-Transmissions.
                    plot(mac_stats_struct.timestamp_diff, mac_stats_struct.number_of_retx,'linewidth',2);
                    
                    if(maximum_time_diff > minimum_time_diff)
                        xlim([minimum_time_diff maximum_time_diff]);
                    end
                    title_str = sprintf('Rxed ACK - %s - MAC: %d',trial_name(1:end-4),mac_stats_struct.mac);
                    xlabel('Time [s]');
                    title(title_str);
                    if(is_there_timeout == true)
                        legend('Timeout New MCS','Timeout New TX Gain','Number of Re-Tx');
                    end
                    grid on
                    hold off
                    
                    % Save figure.
                    scaleFactor = 1.6;
                    set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
                    saveas_file_name = sprintf('%s%s_mac_%d.png',folder,trial_name(1:end-4),mac_stats_struct.mac);
                    saveas(gcf,saveas_file_name);
                end
            end
        end
    end
end
