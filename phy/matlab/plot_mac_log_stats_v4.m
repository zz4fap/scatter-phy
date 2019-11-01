clear all;close all;clc

% Size of outgoing APP packet is: 1042 (packets coming from darpa)
% Packet discarded, queue delay was HIGH (packets dropped cause of queue delay)
% ACK counter zeroed, dropping packet (packets dropped when max retries)
% Count of data packets sent to APP: (packets received correctly and send to APP)
% removed successfully from acks_map when receiving ACK  (got ack on fragment sent)
% added successfully from acks_needed_map! (send fragment with specific MCS and TX GAIN
% ACK timeout of dst:  (lost ack on fragment sent)

BATCH_ID = [20594 20595 20596 20597 20599 20600];

filename = 'mac_stdout.txt';

for id_cnt=1:1:length(BATCH_ID)
    
    %set your local path correctly here, preferably an absolut path
    full_path = sprintf('/home/zz4fap/Downloads/link_level_check/LBT/OLD_11/RESERVATION-%d-all-logs/',BATCH_ID(id_cnt));
    
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
            
            string1_to_look_for = '[info] Count of data packets sent to APP:';
            % RX ACK Message structure.
            app_packets_stats_struct = [];
            app_packets_stats_struct.timestamp_diff = [];
            app_packets_stats_struct.count_packet = [];
            packet_count = 0;
            
            string2_to_look_for = '[info] ACK counter zeroed, dropping packet';
            % RX ACK Message structure.
            %app_packets_stats_struct = [];
            app_packets_stats_struct.timestamp_diff_1 = [];
            app_packets_stats_struct.count_dropped_packets = [];
            %app_packets_stats_struct.total_time_diff = [];
            dropped_packet_count = 0;
            
            string3_to_look_for = 'removed successfully from acks_map when receiving ACK';
            % RX ACK Message structure.
            %app_packets_stats_struct = [];
            app_packets_stats_struct.timestamp_diff_2 = [];
            app_packets_stats_struct.count_acks_success = [];
            %app_packets_stats_struct.total_time_diff = [];
            acks_ok_count = 0;
            
            string4_to_look_for = 'ACK timeout of dst:';
            % RX ACK Message structure.
            %app_packets_stats_struct = [];
            app_packets_stats_struct.timestamp_diff_3 = [];
            app_packets_stats_struct.count_acks_failed = [];
            %app_packets_stats_struct.total_time_diff = [];
            acks_failed_count = 0;
            
            string5_to_look_for = 'added successfully from acks_needed_map!';
            % RX ACK Message structure.
            %app_packets_stats_struct = [];
            app_packets_stats_struct.timestamp_diff_4 = [];
            app_packets_stats_struct.fragments_sent = [];
            app_packets_stats_struct.mcs = [];
            app_packets_stats_struct.txgain = [];
            %app_packets_stats_struct.total_time_diff = [];
            fragments_count = 0;
            mcs_sum = 0;
            txgain_sum=0;
            
            % Open file.
            sub_folder = sub_folders(k).name;
            full_path_to_file = sprintf('%s/%s/%s',full_path,sub_folder,filename);
            fid = fopen(full_path_to_file);
            
            first_timestamp = -1;
            tline = fgetl(fid);
            while ischar(tline)
                % Find first timestamp.
                string_to_look_for_first_timestamp = ' [N12communicator11CommManagerE] [info] ';
                ret1 = strfind(tline,string_to_look_for_first_timestamp);
                if(~isempty(ret1) && first_timestamp==-1)
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    first_timestamp = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    prev_sec1 = str2double(secs_str);
                    prev_sec2 = prev_sec1;
                    prev_sec3 = prev_sec1;
                    prev_sec4 = prev_sec1;
                    prev_sec5 = prev_sec1;
                end
                
                % Find line with ACK report.
                ret = strfind(tline,string1_to_look_for);
                if(~isempty(ret))
                    % Find MAC address inside the line.
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    rxd_ack_timestamp1 = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    second_now = str2double(secs_str);
                    
                    %fprintf(1,'prev second : %d , second_now : %d\n',prev_sec, second_now);
                    %try to make measurements per second.
                    if(second_now==prev_sec1)
                        packet_count = packet_count + 1;
                    else
                        % Timestamp diff.
                        packet_count = packet_count + 1;
                        %fprintf(1,'Packets per second : %d\n',packet_count);
                        app_packets_stats_struct.count_packet = [app_packets_stats_struct.count_packet, packet_count];
                        time_diff = rxd_ack_timestamp1 - first_timestamp;
                        app_packets_stats_struct.timestamp_diff = [app_packets_stats_struct.timestamp_diff, time_diff];
                        packet_count = 0;
                        prev_sec1=second_now;
                    end
                    
                    %fprintf(1,'MAC: %d\n',mac);
                    % Add mac to struct.
                end
                
                %% Process file for packets discarded coming from APP darpa.
                % Find line with ACK report.
                ret = strfind(tline,string2_to_look_for);
                if(~isempty(ret))
                    % Find MAC address inside the line.
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    rxd_ack_timestamp2 = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    second_now = str2double(secs_str);
                    
                    %fprintf(1,'prev second : %d , second_now : %d\n',prev_sec, second_now);
                    %try to make measurements per second.
                    if(second_now==prev_sec2)
                        dropped_packet_count = dropped_packet_count + 1;
                    else
                        % Timestamp diff.
                        dropped_packet_count = dropped_packet_count + 1;
                        %fprintf(1,'Packets dropped per second : %d\n',dropped_packet_count);
                        app_packets_stats_struct.count_dropped_packets = [app_packets_stats_struct.count_dropped_packets, dropped_packet_count];
                        time_diff = rxd_ack_timestamp2 - first_timestamp;
                        app_packets_stats_struct.timestamp_diff_1 = [app_packets_stats_struct.timestamp_diff_1, time_diff];
                        dropped_packet_count = 0;
                        prev_sec2=second_now;
                    end
                    
                    %fprintf(1,'MAC: %d\n',mac);
                    % Add mac to struct.
                end
                
                %% Process file for acks success.
                % Find line with ACK report.
                ret = strfind(tline,string3_to_look_for);
                if(~isempty(ret))
                    % Find MAC address inside the line.
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    rxd_ack_timestamp3 = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    second_now = str2double(secs_str);
                    
                    %fprintf(1,'prev second : %d , second_now : %d\n',prev_sec, second_now);
                    %try to make measurements per second.
                    if(second_now==prev_sec3)
                        acks_ok_count = acks_ok_count + 1;
                    else
                        % Timestamp diff.
                        acks_ok_count = acks_ok_count + 1;
                        %fprintf(1,'Acks ok per second : %d\n',acks_ok_count);
                        app_packets_stats_struct.count_acks_success= [app_packets_stats_struct.count_acks_success, acks_ok_count];
                        time_diff = rxd_ack_timestamp3 - first_timestamp;
                        app_packets_stats_struct.timestamp_diff_2 = [app_packets_stats_struct.timestamp_diff_2, time_diff];
                        acks_ok_count = 0;
                        prev_sec3=second_now;
                    end
                    
                    %fprintf(1,'MAC: %d\n',mac);
                    % Add mac to struct.
                end
                
                %% Process file for acks failures.
                % Find line with ACK report.
                ret = strfind(tline,string4_to_look_for);
                if(~isempty(ret))
                    % Find MAC address inside the line.
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    rxd_ack_timestamp4 = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
                    second_now = str2double(secs_str);
                    
                    %fprintf(1,'prev second : %d , second_now : %d\n',prev_sec, second_now);
                    %try to make measurements per second.
                    if(second_now==prev_sec4)
                        acks_failed_count = acks_failed_count + 1;
                    else               
                        % Timestamp diff.
                        acks_failed_count = acks_failed_count + 1;
                        %fprintf(1,'Acks lost per second : %d\n',acks_failed_count);
                        app_packets_stats_struct.count_acks_failed= [app_packets_stats_struct.count_acks_failed, acks_failed_count];
                        time_diff = rxd_ack_timestamp4 - first_timestamp;
                        app_packets_stats_struct.timestamp_diff_3 = [app_packets_stats_struct.timestamp_diff_3, time_diff];
                        acks_failed_count = 0;
                        prev_sec4=second_now;                      
                    end
                    
                    %fprintf(1,'MAC: %d\n',mac);
                    % Add mac to struct.
                end
                
                %% Process file for statistics on fragment sent.
                % Find line with ACK report.
                ret = strfind(tline,string5_to_look_for);
                if(~isempty(ret))
                    % Find MAC address inside the line.
                    hours_start = find(tline==' ',1);
                    hours_end = find(tline==':',1);
                    hours_str = tline(hours_start(1)+1:hours_end(1)-1);
                    mins_str = tline(hours_end(1)+1:hours_end(1)+2);
                    secs_str = tline(hours_end(1)+4:hours_end(1)+5);
                    frac_secs_str = tline(hours_end(1)+6:hours_end(1)+15);
                    rxd_ack_timestamp5 = str2double(hours_str)*3600 + str2double(mins_str)*60 + str2double(secs_str) + str2double(frac_secs_str);
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
                    
                    %fprintf(1,'Fragments sent per second : %d\n',fragments_count);
                    %fprintf(1,'avg MCS : %d \n' , mcs_sum/fragments_count);
                    %fprintf(1,'avg TX Gain : %d \n' , txgain_sum/fragments_count);
                    
                    %fprintf(1,'prev second : %d , second_now : %d\n',prev_sec, second_now);
                    %try to make measurements per second.
                    if(second_now==prev_sec5)
                        fragments_count = fragments_count + 1;
                        mcs_sum = mcs_sum + mcs;
                        txgain_sum = txgain_sum + gain;
                    else
                        % Timestamp diff.
                        fragments_count = fragments_count + 1;
                        mcs_sum = mcs_sum +mcs;
                        txgain_sum = txgain_sum + gain;
                        
                        %fprintf(1,'Fragments sent per second : %d\n',fragments_count);
                        %fprintf(1,'avg MCS : %d \n' , mcs_sum/fragments_count);
                        %fprintf(1,'avg TX Gain : %d \n' , txgain_sum/fragments_count);
                        
                        app_packets_stats_struct.fragments_sent= [app_packets_stats_struct.fragments_sent, fragments_count];
                        app_packets_stats_struct.mcs= [app_packets_stats_struct.mcs, mcs_sum/fragments_count];
                        app_packets_stats_struct.txgain= [app_packets_stats_struct.txgain, txgain_sum/fragments_count];
                        
                        time_diff = rxd_ack_timestamp5 - first_timestamp;
                        app_packets_stats_struct.timestamp_diff_4 = [app_packets_stats_struct.timestamp_diff_4, time_diff];
                        fragments_count = 0;
                        mcs_sum = 0;
                        txgain_sum = 0;
                        prev_sec5 = second_now;
                    end
                    
                    %fprintf(1,'MAC: %d\n',mac);
                    % Add mac to struct.
                end
                tline = fgetl(fid);
            end
            fclose(fid);
            
            if(packet_count~=0)
                app_packets_stats_struct.count_packet = [app_packets_stats_struct.count_packet, packet_count];
                time_diff = rxd_ack_timestamp1 - first_timestamp;
                app_packets_stats_struct.timestamp_diff = [app_packets_stats_struct.timestamp_diff, time_diff];
            end
            
            if(dropped_packet_count~=0)
                app_packets_stats_struct.count_dropped_packets = [app_packets_stats_struct.count_dropped_packets, dropped_packet_count];
                time_diff = rxd_ack_timestamp2 - first_timestamp;
                app_packets_stats_struct.timestamp_diff_1 = [app_packets_stats_struct.timestamp_diff_1, time_diff];
            end
            
            if(acks_ok_count~=0)
                app_packets_stats_struct.count_acks_success = [app_packets_stats_struct.count_acks_success, acks_ok_count];
                time_diff = rxd_ack_timestamp3 - first_timestamp;
                app_packets_stats_struct.timestamp_diff_2 = [app_packets_stats_struct.timestamp_diff_2, time_diff];
            end
            
            if(acks_failed_count~=0)
                app_packets_stats_struct.count_acks_failed = [app_packets_stats_struct.count_acks_failed, acks_failed_count];
                time_diff = rxd_ack_timestamp4 - first_timestamp;
                app_packets_stats_struct.timestamp_diff_3 = [app_packets_stats_struct.timestamp_diff_3, time_diff];
            end
            
            if(fragments_count~=0)
                app_packets_stats_struct.fragments_sent = [app_packets_stats_struct.fragments_sent, fragments_count];
                app_packets_stats_struct.mcs = [app_packets_stats_struct.mcs, mcs_sum/fragments_count];
                app_packets_stats_struct.txgain = [app_packets_stats_struct.txgain, txgain_sum/fragments_count];
                time_diff = rxd_ack_timestamp5 - first_timestamp;
                app_packets_stats_struct.timestamp_diff_4 = [app_packets_stats_struct.timestamp_diff_4, time_diff];
            end
            
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
        if(~isempty(app_packets_stats_struct.count_packet))
            plot(app_packets_stats_struct.timestamp_diff, app_packets_stats_struct.count_packet,'k','linewidth',lineWidth);
            hold on;
        end
        plot(app_packets_stats_struct.timestamp_diff_4, app_packets_stats_struct.fragments_sent,'b','linewidth',lineWidth);
        if(isempty(app_packets_stats_struct.count_packet))
            hold on;
        end
        plot(app_packets_stats_struct.timestamp_diff_4, app_packets_stats_struct.mcs,'r','linewidth',lineWidth);
        plot(app_packets_stats_struct.timestamp_diff_4, app_packets_stats_struct.txgain,'g','linewidth',lineWidth);
        grid on
        hold off
        title_str = sprintf('TX related statistics - %s',sub_folder);
        title(title_str);
        xlabel('Time [s]');
        if(~isempty(app_packets_stats_struct.count_packet))
            legend('Received Packets','Fragments sent','MCS', 'Tx Gain');
        else
            legend('Fragments sent','MCS', 'Tx Gain');
        end
        
        subplot(2,1,2)
        plot(app_packets_stats_struct.timestamp_diff_1, app_packets_stats_struct.count_dropped_packets,'k','linewidth',lineWidth);
        hold on
        if(~isempty(app_packets_stats_struct.count_acks_success))
            plot(app_packets_stats_struct.timestamp_diff_2, app_packets_stats_struct.count_acks_success,'g','linewidth',lineWidth);
        end
        plot(app_packets_stats_struct.timestamp_diff_3, app_packets_stats_struct.count_acks_failed,'r','linewidth',lineWidth);
        grid on
        hold off
        title_str = sprintf('ACK related statistics');
        title(title_str);
        xlabel('Time [s]');
        if(~isempty(app_packets_stats_struct.count_acks_success))
            legend('Dropped-ACK packets', 'OK ACKs','Lost ACKs');
        else
            legend('Dropped-ACK packets','Lost ACKs');
        end
        
        scaleFactor = 1.6;
        set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
        saveas_file_name = sprintf('%s%s-mac.png',full_path,sub_folder);
        saveas(gcf,saveas_file_name);
        
        close all;
    end
end
