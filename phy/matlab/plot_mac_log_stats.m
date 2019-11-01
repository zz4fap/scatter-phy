clear all;close all;clc

reservation_number = [19108];

for id_cnt=1:1:length(reservation_number)
    
    %set your local path correctly here, preferably an absolut path
    full_path = sprintf('/home/zz4fap/Downloads/link_level_check/LBT/RESERVATION-%d-phy-mac-logs/',reservation_number(id_cnt));
    
    filename = 'mac_stdout.txt';
    
    sub_folders = dir(full_path);
    figure_num = 0;
    
    for k=3:1:length(sub_folders)
        
        if(~sub_folders(k).isdir || isempty(strfind(sub_folders(k).name,'scatter-')))
            continue;
        end
        
        sub_folder = sub_folders(k).name;
        
        % Create the .mat file name.
        dot_mat_file_name = sprintf('%s%s-mac-stats-structs.mat',full_path,sub_folder);
        
        if(isempty(dir(dot_mat_file_name)))
            
            % Process file for received APP darpa packets.
            
            string_to_look_for = '[info] Count of data packets sent to APP:';
            % Size of outgoing APP packet is: 1042 (packets coming from darpa)
            % Packet discarded, queue delay was HIGH (packets dropped cause of queue delay)
            % ACK counter zeroed, dropping packet (packets dropped when max retries)
            % Count of data packets sent to APP: (packets received correctly and send to APP)
            % removed successfully from acks_map when receiving ACK  (got ack on fragment sent)
            % ACK timeout of dst:  (lost ack on fragment sent)
            
            sub_folder = sub_folders(k).name;
            
            % Open file.
            full_path_to_file = sprintf('%s/%s/%s',full_path,sub_folder,filename);
            fid = fopen(full_path_to_file);
            
            
            % RX ACK Message structure.
            app_packets_stats_struct = [];
            app_packets_stats_struct.timestamp_diff = [];
            app_packets_stats_struct.count_packet = [];
            
            packet_count = 0;
            
            tline = fgetl(fid);         
            while ischar(tline)
                % Find first timestamp.
                string_to_look_for_first_timestamp = ' [N12communicator11CommManagerE] [info] ';
                ret1 = strfind(tline,string_to_look_for_first_timestamp);
                if(~isempty(ret1))
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    first_timestamp = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    prev_sec = str2double(secs_str);
                end
                
                % Find line with ACK report.
                ret = strfind(tline,string_to_look_for);
                if(~isempty(ret))
                    % Find MAC address inside the line.                   
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    rxd_ack_timestamp = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    second_now = str2double(secs_str);
                    
                    %fprintf(1,'prev second : %d , second_now : %d\n',prev_sec, second_now);
                    %try to make measurements per second.
                    if(second_now==prev_sec)
                        packet_count = packet_count + 1;
                    else
                        % Timestamp diff.
                        packet_count = packet_count + 1;
                        %fprintf(1,'Packets per second : %d\n',packet_count);
                        app_packets_stats_struct.count_packet= [app_packets_stats_struct.count_packet, packet_count];
                        time_diff = rxd_ack_timestamp - first_timestamp;
                        app_packets_stats_struct.timestamp_diff = [app_packets_stats_struct.timestamp_diff, time_diff];
                        packet_count = 0;
                        prev_sec=second_now;
                    end
                    
                    %fprintf(1,'MAC: %d\n',mac);
                    % Add mac to struct.
                    
                end
                tline = fgetl(fid);
            end
            fclose(fid);
            
            %% Process file for packets discarded coming from APP darpa.
            
            string_to_look_for = '[info] ACK counter zeroed, dropping packet';
            % Size of outgoing APP packet is: 1042 (packets coming from darpa)
            % Packet discarded, queue delay was HIGH (packets dropped cause of queue delay)
            % ACK counter zeroed, dropping packet (packets dropped when max retries)
            % Count of data packets sent to APP: (packets received correctly and send to APP)
            % removed successfully from acks_map when receiving ACK  (got ack on fragment sent)
            % ACK timeout of dst:  (lost ack on fragment sent)
            
            % Open file.
            sub_folder = sub_folders(k).name;
            full_path_to_file = sprintf('%s/%s/%s',full_path,sub_folder,filename);
            fid = fopen(full_path_to_file);
            
            
            % RX ACK Message structure.
            %app_packets_stats_struct = [];
            app_packets_stats_struct.timestamp_diff_1 = [];
            app_packets_stats_struct.count_dropped_packets = [];
            %app_packets_stats_struct.total_time_diff = [];
            
            dropped_packet_count = 0;
            
            tline = fgetl(fid);            
            while ischar(tline)
                
                % Find first timestamp.
                string_to_look_for_first_timestamp = ' [N12communicator11CommManagerE] [info] ';
                ret1 = strfind(tline,string_to_look_for_first_timestamp);
                if(~isempty(ret1))
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    first_timestamp = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    prev_sec = str2double(secs_str);
                end             
                
                % Find line with ACK report.
                ret = strfind(tline,string_to_look_for);
                if(~isempty(ret))
                    % Find MAC address inside the line.
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    rxd_ack_timestamp = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    second_now = str2double(secs_str);
                    
                    %fprintf(1,'prev second : %d , second_now : %d\n',prev_sec, second_now);
                    %try to make measurements per second.
                    if(second_now==prev_sec)
                        dropped_packet_count = dropped_packet_count + 1;
                    else
                        % Timestamp diff.
                        dropped_packet_count = dropped_packet_count + 1;
                        %fprintf(1,'Packets dropped per second : %d\n',dropped_packet_count);
                        app_packets_stats_struct.count_dropped_packets= [app_packets_stats_struct.count_dropped_packets, dropped_packet_count];
                        time_diff = rxd_ack_timestamp - first_timestamp;
                        app_packets_stats_struct.timestamp_diff_1 = [app_packets_stats_struct.timestamp_diff_1, time_diff];
                        dropped_packet_count = 0;
                        prev_sec=second_now;
                    end
                    
                    %fprintf(1,'MAC: %d\n',mac);
                    % Add mac to struct.
                    
                end
                tline = fgetl(fid);
            end
            fclose(fid);
            
            
            %% Process file for acks success.
            
            string_to_look_for = 'removed successfully from acks_map when receiving ACK';
            % Size of outgoing APP packet is: 1042 (packets coming from darpa)
            % Packet discarded, queue delay was HIGH (packets dropped cause of queue delay)
            % ACK counter zeroed, dropping packet (packets dropped when max retries)
            % Count of data packets sent to APP: (packets received correctly and send to APP)
            % removed successfully from acks_map when receiving ACK  (got ack on fragment sent)
            % ACK timeout of dst:  (lost ack on fragment sent)
            
            % Open file.
            sub_folder = sub_folders(k).name;
            full_path_to_file = sprintf('%s/%s/%s',full_path,sub_folder,filename);
            fid = fopen(full_path_to_file);
            
            
            % RX ACK Message structure.
            %app_packets_stats_struct = [];
            app_packets_stats_struct.timestamp_diff_2 = [];
            app_packets_stats_struct.count_acks_success = [];
            %app_packets_stats_struct.total_time_diff = [];
            
            acks_ok_count = 0;
            
            tline = fgetl(fid);           
            while ischar(tline)
                
                % Find first timestamp.
                string_to_look_for_first_timestamp = ' [N12communicator11CommManagerE] [info] ';
                ret1 = strfind(tline,string_to_look_for_first_timestamp);
                if(~isempty(ret1))
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    first_timestamp = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    prev_sec = str2double(secs_str);
                end                 

                % Find line with ACK report.
                ret = strfind(tline,string_to_look_for);
                if(~isempty(ret))
                    % Find MAC address inside the line.
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    rxd_ack_timestamp = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    second_now = str2double(secs_str);
                    
                    %fprintf(1,'prev second : %d , second_now : %d\n',prev_sec, second_now);
                    %try to make measurements per second.
                    if(second_now==prev_sec)
                        acks_ok_count = acks_ok_count + 1;
                    else
                        % Timestamp diff.
                        acks_ok_count = acks_ok_count + 1;
                        %fprintf(1,'Acks ok per second : %d\n',acks_ok_count);
                        app_packets_stats_struct.count_acks_success= [app_packets_stats_struct.count_acks_success, acks_ok_count];
                        time_diff = rxd_ack_timestamp - first_timestamp;
                        app_packets_stats_struct.timestamp_diff_2 = [app_packets_stats_struct.timestamp_diff_2, time_diff];
                        acks_ok_count = 0;
                        prev_sec=second_now;
                    end
                    
                    %fprintf(1,'MAC: %d\n',mac);
                    % Add mac to struct.
                    
                end
                tline = fgetl(fid);
            end
            fclose(fid);
            
            
            %% Process file for acks failures.
            
            string_to_look_for = 'ACK timeout of dst:';
            % Size of outgoing APP packet is: 1042 (packets coming from darpa)
            % Packet discarded, queue delay was HIGH (packets dropped cause of queue delay)
            % ACK counter zeroed, dropping packet (packets dropped when max retries)
            % Count of data packets sent to APP: (packets received correctly and send to APP)
            % removed successfully from acks_map when receiving ACK  (got ack on fragment sent)
            % ACK timeout of dst:  (lost ack on fragment sent)
            
            % Open file.
            sub_folder = sub_folders(k).name;
            full_path_to_file = sprintf('%s/%s/%s',full_path,sub_folder,filename);
            fid = fopen(full_path_to_file);
            
            
            % RX ACK Message structure.
            %app_packets_stats_struct = [];
            app_packets_stats_struct.timestamp_diff_3 = [];
            app_packets_stats_struct.count_acks_failed = [];
            %app_packets_stats_struct.total_time_diff = [];
            
            acks_failed_count = 0;
            
            tline = fgetl(fid);           
            while ischar(tline)
                
                % Find first timestamp.
                string_to_look_for_first_timestamp = ' [N12communicator11CommManagerE] [info] ';
                ret1 = strfind(tline,string_to_look_for_first_timestamp);
                if(~isempty(ret1))
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    first_timestamp = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    prev_sec = str2double(secs_str);
                end            
                
                % Find line with ACK report.
                ret = strfind(tline,string_to_look_for);
                if(~isempty(ret))
                    % Find MAC address inside the line.
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    rxd_ack_timestamp = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    second_now = str2double(secs_str);
                    
                    %fprintf(1,'prev second : %d , second_now : %d\n',prev_sec, second_now);
                    %try to make measurements per second.
                    if(second_now==prev_sec)
                        acks_failed_count = acks_failed_count + 1;
                    else
                        % Timestamp diff.
                        acks_failed_count = acks_failed_count + 1;
                        %fprintf(1,'Acks lost per second : %d\n',acks_failed_count);
                        app_packets_stats_struct.count_acks_failed= [app_packets_stats_struct.count_acks_failed, acks_failed_count];
                        time_diff = rxd_ack_timestamp - first_timestamp;
                        app_packets_stats_struct.timestamp_diff_3 = [app_packets_stats_struct.timestamp_diff_3, time_diff];
                        acks_failed_count = 0;
                        prev_sec=second_now;
                    end
                    
                    %fprintf(1,'MAC: %d\n',mac);
                    % Add mac to struct.
                    
                end
                tline = fgetl(fid);
            end
            fclose(fid);
            
            %% Process file for statistics on fragment sent.
            
            string_to_look_for = 'added successfully from acks_needed_map!';
            % Size of outgoing APP packet is: 1042 (packets coming from darpa)
            % Packet discarded, queue delay was HIGH (packets dropped cause of queue delay)
            % ACK counter zeroed, dropping packet (packets dropped when max retries)
            % Count of data packets sent to APP: (packets received correctly and send to APP)
            % removed successfully from acks_map when receiving ACK  (got ack on fragment sent)
            % ACK timeout of dst:  (lost ack on fragment sent)
            % added successfully from acks_needed_map! (send fragment with specific MCS and TX GAIN
            
            % Open file.
            sub_folder = sub_folders(k).name;
            full_path_to_file = sprintf('%s/%s/%s',full_path,sub_folder,filename);
            fid = fopen(full_path_to_file);
            
            
            % RX ACK Message structure.
            %app_packets_stats_struct = [];
            app_packets_stats_struct.timestamp_diff_4 = [];
            app_packets_stats_struct.fragments_sent = [];
            app_packets_stats_struct.avg_mcs = [];
            app_packets_stats_struct.avg_txgain = [];
            
            %app_packets_stats_struct.total_time_diff = [];
            
            fragments_count = 0;
            mcs_sum = 0;
            txgain_sum=0;
            
            tline = fgetl(fid);          
            while ischar(tline)
                
                % Find first timestamp.
                string_to_look_for_first_timestamp = ' [N12communicator11CommManagerE] [info] ';
                ret1 = strfind(tline,string_to_look_for_first_timestamp);
                if(~isempty(ret1))
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    first_timestamp = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    prev_sec = str2double(secs_str);
                end                 
                
                % Find line with ACK report.
                ret = strfind(tline,string_to_look_for);
                if(~isempty(ret))
                    % Find MAC address inside the line.
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    rxd_ack_timestamp = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    second_now = str2double(secs_str);
                    
                    % MCS
                    ret = strfind(tline,'MCS');
                    aux_str = tline(ret+4:end);
                    end_of_desired_str = find(aux_str==',',1);
                    mcs_string = aux_str(1:end_of_desired_str-1);
                    mcs = str2double(mcs_string);
                    %fprintf(1,'MCS : %d \n' , mcs);
                    
                    % TX gain
                    ret = strfind(tline,'GAIN');
                    aux_str = tline(ret+5:end);
                    end_of_desired_str = find(aux_str==',',1);
                    gain_string = aux_str(1:end_of_desired_str-1);
                    gain = str2double(gain_string);
                    %fprintf(1,'Gain : %d \n' , gain);
                    
                    %fprintf(1,'prev second : %d , second_now : %d\n',prev_sec, second_now);
                    %try to make measurements per second.
                    if(second_now==prev_sec)
                        fragments_count = fragments_count + 1;
                        mcs_sum = mcs_sum +mcs;
                        txgain_sum = txgain_sum+gain;
                        %txgain_sum =
                    else
                        % Timestamp diff.
                        fragments_count = fragments_count + 1;
                        mcs_sum = mcs_sum +mcs;
                        txgain_sum = txgain_sum+gain;
                        
                        %fprintf(1,'Fragments sent per second : %d\n',fragments_count);
                        %fprintf(1,'avg MCS : %d \n' , mcs_sum/fragments_count);
                        %fprintf(1,'avg TX Gain : %d \n' , txgain_sum/fragments_count);
                        
                        app_packets_stats_struct.fragments_sent= [app_packets_stats_struct.fragments_sent, fragments_count];
                        app_packets_stats_struct.avg_mcs= [app_packets_stats_struct.avg_mcs, mcs_sum/fragments_count];
                        app_packets_stats_struct.avg_txgain= [app_packets_stats_struct.avg_txgain, txgain_sum/fragments_count];
                        
                        time_diff = rxd_ack_timestamp - first_timestamp;
                        app_packets_stats_struct.timestamp_diff_4 = [app_packets_stats_struct.timestamp_diff_4, time_diff];
                        fragments_count = 0;
                        mcs_sum=0;
                        txgain_sum=0;
                        prev_sec=second_now;
                    end
                    
                    %fprintf(1,'MAC: %d\n',mac);
                    % Add mac to struct.
                    
                end
                tline = fgetl(fid);
            end
            fclose(fid);
            
            %% Now, save .mat with all the variables.
            save(dot_mat_file_name,'app_packets_stats_struct');
            
        else
            clear app_packets_stats_struct
            load(dot_mat_file_name);
        end
        
        %% Now, plot things.
        lineWidth = 2;
        figure_num = figure_num + 1;
        fig=figure(figure_num);
        fig.Name = 'Station MAC status indicators';
        
        subplot(2,1,1)
        plot(app_packets_stats_struct.timestamp_diff, app_packets_stats_struct.count_packet,'k','linewidth',lineWidth);
        hold on;
        plot(app_packets_stats_struct.timestamp_diff_4, app_packets_stats_struct.fragments_sent,'b','linewidth',lineWidth);
        plot(app_packets_stats_struct.timestamp_diff_4, app_packets_stats_struct.avg_mcs,'r','linewidth',lineWidth);
        plot(app_packets_stats_struct.timestamp_diff_4, app_packets_stats_struct.avg_txgain,'g','linewidth',lineWidth);
        grid on
        hold off
        title_str = sprintf('TX related statistics');
        title(title_str);
        xlabel('Time [s]');
        legend('Received Packets','Fragmens sent','Avg. MCS', 'Avg. GAIN');
        
        subplot(2,1,2)
        plot(app_packets_stats_struct.timestamp_diff_1, app_packets_stats_struct.count_dropped_packets,'k','linewidth',lineWidth);
        hold on
        plot(app_packets_stats_struct.timestamp_diff_2, app_packets_stats_struct.count_acks_success,'g','linewidth',lineWidth);
        plot(app_packets_stats_struct.timestamp_diff_3, app_packets_stats_struct.count_acks_failed,'r','linewidth',lineWidth);
        grid on
        hold off
        title_str = sprintf('ACK related statistics');
        title(title_str);
        xlabel('Time [s]');
        legend('Dropped-ACK packets', 'OK ACKs','Lost ACKs');
        
        scaleFactor = 1.6;
        set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
        saveas_file_name = sprintf('%s%s-mac.png',full_path,sub_folder);
        saveas(gcf,saveas_file_name);
    end
end
