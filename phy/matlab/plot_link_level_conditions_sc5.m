clear all;close all;clc

% This is the template used to plot values regarding ACKs.
% ACK_PROCESSED: <timestamp_ns>, <mac_address>, <ack_success>, <num_retransmissions>, <old_mcs>, <old_gain>,                          <rssi>, <cqi>, <new_mcs>, <new_gain>
% ACK_PROCESSED:  timestamp_ns,   mac_address,   ack_success,   num_retransmissions,   old_mcs,   old_gain,    channel,    rx_gain,    rssi,   cqi,   new_mcs,   new_gain
% ACK_PROCESSED:  now,            mac,           success,       retries,               old_mcs,   old_gain,    channel,    rx_gain,    rssi,   cqi,   mcs,       gain

BATCH_ID = [19108];

filename = 'ai_mcs_stdout.txt';
for id_cnt=1:1:length(BATCH_ID)
    %folder = './scrimmage5_test/';
    %folder = '/home/zz4fap/Downloads/link_level_check/LBT/RESERVATION-15291/scatter-master-with-sw-lbt-srn47-RES15291/';
    folder = sprintf('/home/zz4fap/Downloads/link_level_check/LBT/AI/RESERVATION-%d-ai-logs/',BATCH_ID(id_cnt));
    
    subfolders = dir(folder);
    
    figure_counter = 0;
    for k=3:1:length(subfolders)
        
        if(subfolders(k).isdir == 1)
            
            subfoldername = subfolders(k).name;
            
            trial_name = subfoldername;
            
            %% Process file for received ACK message.
            string_to_look_for = ' [info] ACK_PROCESSED: ';
            
            % Open file.
            full_path_to_file = sprintf('%s%s/%s',folder,subfoldername,filename);
            fid = fopen(full_path_to_file);
            
            % RX ACK Message structure.
            acks_struct = [];
            mac_stats_struct = [];
            
            tline = fgetl(fid);
            first_timestamp = str2double(tline(13:14))*3600 + str2double(tline(16:17))*60 + str2double(tline(19:20)) + str2double(tline(21:30));
            while ischar(tline)
                % Find line with ACK report.
                ret = strfind(tline,string_to_look_for);
                if(~isempty(ret))
                    %fprintf(1,'%s\n',tline);
                    % *** Get log timestamp. ***
                    rxd_ack_timestamp = str2double(tline(13:14))*3600 + str2double(tline(16:17))*60 + str2double(tline(19:20)) + str2double(tline(21:30));
                    
                    % *** Retrieve MAC address from log. ***
                    end_of_desired_str = find(tline==',',1);
                    aux_str = tline(end_of_desired_str+1:end);
                    end_of_desired_str = find(aux_str==',',1);
                    mac_string = aux_str(1:end_of_desired_str-1);
                    mac = str2num(['int64(' mac_string ')']);
                    %fprintf(1,'MAC: %d\n',mac);
                    
                    % *** Add mac to struct. ***
                    if(isempty(acks_struct))
                        mac_stats_struct.mac = mac;
                        mac_stats_struct.ack_success = [];
                        mac_stats_struct.timestamp_diff = [];
                        mac_stats_struct.timestamp = [];
                        mac_stats_struct.number_of_retx = [];
                        % Received MAC stats.
                        mac_stats_struct.mac_stats.rssi = [];
                        mac_stats_struct.mac_stats.cqi = [];
                        mac_stats_struct.mac_stats.old_mcs = [];
                        mac_stats_struct.mac_stats.old_tx_gain = [];
                        mac_stats_struct.mac_stats.rx_channel = [];
                        mac_stats_struct.mac_stats.rx_gain = [];
                        % Action taken by AI either because of Timeout or reception of ACK.
                        mac_stats_struct.mac_action.new_mcs = [];
                        mac_stats_struct.mac_action.new_tx_gain = [];
                        acks_struct = [acks_struct, mac_stats_struct];
                        index = 1;
                    else
                        number_of_acks = length(acks_struct);
                        index = -1;
                        for i=1:1:number_of_acks
                            mac_stats_struct = acks_struct(i);
                            if(mac_stats_struct.mac == mac)
                                index = i;
                                break;
                            end
                        end
                        if(index < 0)
                            mac_stats_struct.mac = mac;
                            mac_stats_struct.ack_success = [];
                            mac_stats_struct.timestamp_diff = [];
                            mac_stats_struct.timestamp = [];
                            mac_stats_struct.number_of_retx = [];
                            % Received MAC stats.
                            mac_stats_struct.mac_stats.rssi = [];
                            mac_stats_struct.mac_stats.cqi = [];
                            mac_stats_struct.mac_stats.old_mcs = [];
                            mac_stats_struct.mac_stats.old_tx_gain = [];
                            mac_stats_struct.mac_stats.rx_channel = [];
                            mac_stats_struct.mac_stats.rx_gain = [];
                            % Action taken by AI either because of Timeout or reception of ACK.
                            mac_stats_struct.mac_action.new_mcs = [];
                            mac_stats_struct.mac_action.new_tx_gain = [];
                            acks_struct = [acks_struct, mac_stats_struct];
                            index = number_of_acks + 1;
                        end
                    end
                    
                    % *** Retrieve information on timeout. ***
                    end_of_desired_str = find(tline==',',3);
                    ack_success_string = tline(end_of_desired_str(2)+1:end_of_desired_str(3)-1);
                    if(strcmp(ack_success_string,'true') == true)
                        ack_success = true;
                    else
                        ack_success = false;
                    end
                    mac_stats_struct.ack_success = [mac_stats_struct.ack_success, ack_success];
                    
                    % *** Now parse the values: Timestamp, RSSI, MCS, CQI and TX gain. ***
                    % Timestamp
                    start_of_desired_str = find(tline==':',3);
                    end_of_desired_str = find(tline==',',1);
                    timestamp_str = tline(start_of_desired_str(3)+2:end_of_desired_str-1);
                    timestamp = str2num(['int64(' timestamp_str ')']);
                    timestamp = timestamp * 1e-9; % convert to seconds.
                    mac_stats_struct.timestamp = [mac_stats_struct.timestamp, timestamp];
                    %fprintf(1,'\t Timestamp: %d\n',timestamp);
                    
                    % Number of retransmissions.
                    desired_str_delimiters = find(tline==',',4);
                    num_of_retx_str = tline(desired_str_delimiters(3)+1:desired_str_delimiters(4)-1);
                    num_of_retx = str2num(['int64(' num_of_retx_str ')']);
                    mac_stats_struct.number_of_retx = [mac_stats_struct.number_of_retx, num_of_retx];
                    %fprintf(1,'\t Number of retransmissions: %d\n',num_of_retx);
                    
                    % Old MCS
                    desired_str_delimiters = find(tline==',',5);
                    old_mcs_string = tline(desired_str_delimiters(4)+1:desired_str_delimiters(5)-1);
                    old_mcs = str2num(['int8(' old_mcs_string ')']);
                    mac_stats_struct.mac_stats.old_mcs = [mac_stats_struct.mac_stats.old_mcs, old_mcs];
                    %fprintf(1,'\t Old MCS: %d\n',old_mcs);
                    
                    % Old TX Gain
                    desired_str_delimiters = find(tline==',',6);
                    old_tx_gain_string = tline(desired_str_delimiters(5)+1:desired_str_delimiters(6)-1);
                    old_tx_gain = str2num(['int8(' old_tx_gain_string ')']);
                    mac_stats_struct.mac_stats.old_tx_gain = [mac_stats_struct.mac_stats.old_tx_gain, old_tx_gain];
                    %fprintf(1,'\t Old TX Gain: %d\n',old_tx_gain);
                    
                    % RX Channel
                    desired_str_delimiters = find(tline==',',7);
                    rx_channel_string = tline(desired_str_delimiters(6)+1:desired_str_delimiters(7)-1);
                    rx_channel = str2num(['int64(' rx_channel_string ')']);
                    mac_stats_struct.mac_stats.rx_channel = [mac_stats_struct.mac_stats.rx_channel, rx_channel];
                    %fprintf(1,'\t RX Channel: %d\n',rx_channel);
                    
                    % RX Gain
                    desired_str_delimiters = find(tline==',',8);
                    rx_gain_string = tline(desired_str_delimiters(7)+1:desired_str_delimiters(8)-1);
                    rx_gain = str2num(['int64(' rx_gain_string ')']);
                    mac_stats_struct.mac_stats.rx_gain = [mac_stats_struct.mac_stats.rx_gain, rx_gain];
                    %fprintf(1,'\t RX Gain: %d\n',rx_gain);
                    
                    % RSSI
                    desired_str_delimiters = find(tline==',',9);
                    rssi_string = tline(desired_str_delimiters(8)+1:desired_str_delimiters(9)-1);
                    rssi = str2num(['int64(' rssi_string ')']);
                    mac_stats_struct.mac_stats.rssi = [mac_stats_struct.mac_stats.rssi, rssi];
                    %fprintf(1,'\t RSSI: %d\n',rssi);
                    
                    % CQI
                    desired_str_delimiters = find(tline==',',10);
                    cqi_string = tline(desired_str_delimiters(9)+1:desired_str_delimiters(10)-1);
                    cqi = str2num(['int8(' cqi_string ')']);
                    mac_stats_struct.mac_stats.cqi = [mac_stats_struct.mac_stats.cqi, cqi];
                    %fprintf(1,'\t CQI: %d\n',cqi);
                    
                    % New MCS
                    desired_str_delimiters = find(tline==',',11);
                    new_mcs_string = tline(desired_str_delimiters(10)+1:desired_str_delimiters(11)-1);
                    new_mcs = str2num(['int8(' new_mcs_string ')']);
                    mac_stats_struct.mac_action.new_mcs = [mac_stats_struct.mac_action.new_mcs, new_mcs];
                    %fprintf(1,'\t New MCS: %d\n',new_mcs);
                    
                    % New TX Gain
                    desired_str_delimiters = find(tline==',',12);
                    new_tx_gain_string = tline(desired_str_delimiters(11)+1:end);
                    new_tx_gain = str2num(['int8(' new_tx_gain_string ')']);
                    mac_stats_struct.mac_action.new_tx_gain = [mac_stats_struct.mac_action.new_tx_gain, new_tx_gain];
                    %fprintf(1,'\t New TX Gain: %d\n',new_tx_gain);
                    
                    % Timestamp diff.
                    time_diff = rxd_ack_timestamp - first_timestamp;
                    mac_stats_struct.timestamp_diff = [mac_stats_struct.timestamp_diff, time_diff];
                    
                    % Add MAC struct back to QUEUE.
                    acks_struct(index) = mac_stats_struct;
                end
                tline = fgetl(fid);
            end
            fclose(fid);
            
            %% Now, plot things for each MAC address.
            LOWER_RSSI_BOUND = -37;
            UPPER_RSSI_BOUND = -7;
            
            % Plot MCS and TX Gain.
            is_there_timeout = false;
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
            
            number_of_acks = length(acks_struct);
            for i=1:1:number_of_acks
                figure(i);
                mac_stats_struct = acks_struct(i);
                
                % Change the limits to analyse some specific part of the plot.
                minimum_time_diff = min(mac_stats_struct.timestamp_diff);
                maximum_time_diff = max(mac_stats_struct.timestamp_diff);
                % Figure out maximum and minimum time axis values.
                x_axis_bounds = minimum_time_diff:maximum_time_diff;
                
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
                title_str = sprintf('Rxed ACK - Match: %s - MAC: %d',trial_name,mac_stats_struct.mac);
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
            end
        end
    end
end
