clear all;close all;clc

% Size of outgoing APP packet is: 1042 (packets coming from darpa)
% Packet discarded, queue delay was HIGH (packets dropped cause of queue delay)
% ACK counter zeroed, dropping packet (packets dropped when max retries)
% Count of data packets sent to APP: (packets received correctly and send to APP)
% removed successfully from acks_map when receiving ACK  (got ack on fragment sent)
% added successfully from acks_needed_map! (send fragment with specific MCS and TX GAIN
% ACK timeout of dst:  (lost ack on fragment sent)

BATCH_ID = [19108 19109 19157 19158 19159 19160]; % v4

%BATCH_ID = [19171 19172 19173 19175 19176 19177 19179 19180 19181 19182 19183 19184 19163 19164 19166 19167 19168 19169];

BATCH_ID = [18611 18688];

filename = 'mac_stdout.txt';

for id_cnt=1:1:length(BATCH_ID)
    
    %set your local path correctly here, preferably an absolut path
    full_path = sprintf('/home/zz4fap/Downloads/link_level_check/LBT/OLD_13/RESERVATION-%d-all-logs/',BATCH_ID(id_cnt));
    
    sub_folders = dir(full_path);
    figure_num = 0;
    
    for k=3:1:length(sub_folders)
        
        if(~sub_folders(k).isdir || isempty(strfind(sub_folders(k).name,'scatter-')))
            continue;
        end
        
        sub_folder = sub_folders(k).name;
        
        % Create the .mat file name.
        dot_mat_file_name = sprintf('%s%s-mac-stats-structs.mat',full_path,sub_folder);
        
        if(~isempty(dir(dot_mat_file_name)))
            clear app_packets_stats_struct
            load(dot_mat_file_name);
        end
        
        %% Now, plot things.

        timestamp_vector = [app_packets_stats_struct.timestamp_diff app_packets_stats_struct.timestamp_diff_1 app_packets_stats_struct.timestamp_diff_2 app_packets_stats_struct.timestamp_diff_3 app_packets_stats_struct.timestamp_diff_4];
        % Change the limits to analyse some specific part of the plot.
        minimum_time_diff = min(timestamp_vector);
        maximum_time_diff = max(timestamp_vector);
        % Figure out maximum and minimum time axis values.
        x_axis_bounds = minimum_time_diff:maximum_time_diff;
        
        lineWidth = 2;
        figure_num = figure_num + 1;
        fig=figure(figure_num);
        fig.Name = 'Station MAC status indicators';
        
        subplot(2,1,1)
        if(~isempty(app_packets_stats_struct.count_packet))
            plot(app_packets_stats_struct.timestamp_diff, app_packets_stats_struct.count_packet,'k','linewidth',lineWidth);
            hold on;
        end
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
        xlim([minimum_time_diff maximum_time_diff]);
        if(~isempty(app_packets_stats_struct.count_packet))
            legend('Packets sent to APP','MCS', 'Tx Gain');
        else
            legend('MCS', 'Tx Gain');
        end
        
        subplot(2,1,2)
        semilogy(app_packets_stats_struct.timestamp_diff_4, app_packets_stats_struct.fragments_sent,'b','linewidth',lineWidth);
        hold on
        if(~isempty(app_packets_stats_struct.count_dropped_packets))
            semilogy(app_packets_stats_struct.timestamp_diff_1, app_packets_stats_struct.count_dropped_packets,'k','linewidth',lineWidth);
        end
        if(~isempty(app_packets_stats_struct.count_acks_success))
            plot(app_packets_stats_struct.timestamp_diff_2, app_packets_stats_struct.count_acks_success,'g','linewidth',lineWidth);
        end
        semilogy(app_packets_stats_struct.timestamp_diff_3, app_packets_stats_struct.count_acks_failed,'r','linewidth',lineWidth);
        grid on
        hold off
        title_str = sprintf('ACK related statistics');
        title(title_str);
        xlabel('Time [s]');
        xlim([minimum_time_diff maximum_time_diff]);
        if(~isempty(app_packets_stats_struct.count_acks_success) && ~isempty(app_packets_stats_struct.count_dropped_packets))
            legend('Sent fragments','Dropped-ACK packets', 'OK ACKs','Lost ACKs');
        else
            legend('Sent fragments','Dropped-ACK packets','Lost ACKs');
        end
        
        scaleFactor = 1.6;
        set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
        saveas_file_name = sprintf('%s%s-mac.png',full_path,sub_folder);
        saveas(gcf,saveas_file_name);
 
    end
end
