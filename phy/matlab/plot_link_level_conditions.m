clear all;close all;clc

%folder = './n-tower-scatter-only/';
%folder = './match-141/';
%folder = './match-135/';
folder = './match-006/';

filename = 'ai_mcs_stdout.txt';

%% Process file for received ACK message.
string_to_look_for = '[trace] Received RX Stat with';

string_to_look_for_mac = 'MAC Address ';

% Open file.
full_path_to_file = strcat(folder,filename);
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
        % Find MAC address inside the line.
        ret = strfind(tline,string_to_look_for_mac);
        if(~isempty(ret))
            rxd_ack_timestamp = str2double(tline(13:14))*3600 + str2double(tline(16:17))*60 + str2double(tline(19:20)) + str2double(tline(21:30));
            mac_string = tline(ret+12:end);
            mac = str2num(['int64(' mac_string ')']);
            %fprintf(1,'MAC: %d\n',mac);
            % Add mac to struct.
            if(isempty(acks_struct))
                mac_stats_struct.mac = mac;
                mac_stats_struct.total_time_diff = [];
                % Received MAC stats.
                mac_stats_struct.mac_stats.rssi = [];
                mac_stats_struct.mac_stats.mcs = [];
                mac_stats_struct.mac_stats.cqi = [];
                mac_stats_struct.mac_stats.tx_gain = [];
                mac_stats_struct.mac_stats.timestamp_diff = [];
                % Action taken by AI.
                mac_stats_struct.mac_action.new_mcs = [];
                mac_stats_struct.mac_action.new_tx_gain = [];
                mac_stats_struct.mac_action.timestamp_diff = [];
                % ACK timeout.
                mac_stats_struct.mac_timeout.new_mcs = [];
                mac_stats_struct.mac_timeout.new_tx_gain = [];
                mac_stats_struct.mac_timeout.timestamp_diff = [];
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
                    mac_stats_struct.total_time_diff = [];
                    % Received MAC stats.
                    mac_stats_struct.mac_stats.rssi = [];
                    mac_stats_struct.mac_stats.mcs = [];
                    mac_stats_struct.mac_stats.cqi = [];
                    mac_stats_struct.mac_stats.tx_gain = [];
                    mac_stats_struct.mac_stats.timestamp_diff = [];
                    % Action taken by AI.
                    mac_stats_struct.mac_action.new_mcs = [];
                    mac_stats_struct.mac_action.new_tx_gain = [];
                    mac_stats_struct.mac_action.timestamp_diff = [];
                    % ACK timeout.
                    mac_stats_struct.mac_timeout.new_mcs = [];
                    mac_stats_struct.mac_timeout.new_tx_gain = [];
                    mac_stats_struct.mac_timeout.timestamp_diff = [];
                    acks_struct = [acks_struct, mac_stats_struct];
                    index = number_of_acks + 1;
                end
            end
            % Now parse the values: RSSI, MCS, CQI and TX gain.
            % RSSI
            ret = strfind(tline,'RSSI');
            aux_str = tline(ret:end);
            end_of_desired_str = find(aux_str==',',1);
            rssi_string = aux_str(6:end_of_desired_str-1);
            rssi = str2num(['int64(' rssi_string ')']);
            mac_stats_struct.mac_stats.rssi = [mac_stats_struct.mac_stats.rssi, rssi];
            %fprintf(1,'\t RSSI: %d\n',rssi);

            % MCS
            ret = strfind(tline,'MCS');
            aux_str = tline(ret(2):end);
            end_of_desired_str = find(aux_str==',',1);
            mcs_string = aux_str(5:end_of_desired_str-1);
            mcs = str2num(['int8(' mcs_string ')']);
            mac_stats_struct.mac_stats.mcs = [mac_stats_struct.mac_stats.mcs, mcs];
            %fprintf(1,'\t MCS: %d\n',mcs);

            % CQI
            ret = strfind(tline,'CQI');
            aux_str = tline(ret:end);
            end_of_desired_str = find(aux_str=='a',1);
            cqi_string = aux_str(5:end_of_desired_str-2);
            cqi = str2num(['int8(' cqi_string ')']);
            mac_stats_struct.mac_stats.cqi = [mac_stats_struct.mac_stats.cqi, cqi];
            %fprintf(1,'\t CQI: %d\n',cqi);

            % TX Gain
            ret = strfind(tline,'TX gain');
            aux_str = tline(ret:end);
            end_of_desired_str = find(aux_str=='f',1);
            tx_gain_string = aux_str(9:end_of_desired_str-2);
            tx_gain = str2num(['int8(' tx_gain_string ')']);
            mac_stats_struct.mac_stats.tx_gain = [mac_stats_struct.mac_stats.tx_gain, tx_gain];
            %fprintf(1,'\t TX Gain: %d\n',tx_gain);

            % Timestamp diff.
            time_diff = rxd_ack_timestamp - first_timestamp;
            mac_stats_struct.mac_stats.timestamp_diff = [mac_stats_struct.mac_stats.timestamp_diff, time_diff];
            mac_stats_struct.total_time_diff = [mac_stats_struct.total_time_diff, time_diff];
        end

       acks_struct(index) = mac_stats_struct;
    end
    tline = fgetl(fid);
end
fclose(fid);

%% process file for action taken upon received ACK.
string_to_look_for = '[trace] Received PHY rx stats for MAC address';

string_to_look_for_mac = 'for MAC address ';

% Open file.
full_path_to_file = strcat(folder,filename);
fid = fopen(full_path_to_file);

tline = fgetl(fid);
while ischar(tline)
    % Find line with ACK report.
    ret = strfind(tline,string_to_look_for);
    if(~isempty(ret))
        % Find MAC address inside the line.
        ret = strfind(tline,string_to_look_for_mac);
        if(~isempty(ret))
            mac_action_timestamp = str2double(tline(13:14))*3600 + str2double(tline(16:17))*60 + str2double(tline(19:20)) + str2double(tline(21:30));
            mac_string = tline(ret+16:end);
            end_of_desired_str = find(mac_string=='.',1);
            mac_string = mac_string(1:end_of_desired_str-1);
            mac = str2num(['int64(' mac_string ')']);
            %fprintf(1,'MAC: %d\n',mac);

            % Add mac to struct.
            number_of_acks = length(acks_struct);
            index = -1;
            for i=1:1:number_of_acks
                mac_stats_struct = acks_struct(i);
                if(mac_stats_struct.mac == mac)
                    index = i;
                    break;
                end
            end
            % Now parse the values: MCS and TX gain.

            % new MCS
            ret = strfind(tline,'MCS');
            aux_str = tline(ret(2):end);
            end_of_desired_str = find(aux_str==',',1);
            mcs_string = aux_str(5:end_of_desired_str-1);
            mcs = str2num(['int8(' mcs_string ')']);
            mac_stats_struct.mac_action.new_mcs = [mac_stats_struct.mac_action.new_mcs, mcs];
            %fprintf(1,'\t New MCS: %d\n',mcs);

            % new TX Gain
            ret = strfind(tline,'TX gain:');
            aux_str = tline(ret:end);
            tx_gain_string = aux_str(10:end);
            tx_gain = str2num(['int8(' tx_gain_string ')']);
            mac_stats_struct.mac_action.new_tx_gain = [mac_stats_struct.mac_action.new_tx_gain, tx_gain];
            %fprintf(1,'\t New TX Gain: %d\n',tx_gain);

            % Timestamp diff.
            time_diff = mac_action_timestamp - first_timestamp;
            mac_stats_struct.mac_action.timestamp_diff = [mac_stats_struct.mac_action.timestamp_diff, time_diff];
            mac_stats_struct.total_time_diff = [mac_stats_struct.total_time_diff, time_diff];
        end

       acks_struct(index) = mac_stats_struct;
    end
    tline = fgetl(fid);
end
fclose(fid);

%% process file for ACK timeout.
string_to_look_for = '[trace] Reception timeout ';

string_to_look_for_mac = 'for MAC ';

% Open file.
full_path_to_file = strcat(folder,filename);
fid = fopen(full_path_to_file);

tline = fgetl(fid);
while ischar(tline)
    % Find line with ACK report.
    ret = strfind(tline,string_to_look_for);
    if(~isempty(ret))
        % Find MAC address inside the line.
        ret = strfind(tline,string_to_look_for_mac);
        if(~isempty(ret))
            mac_timeout_timestamp = str2double(tline(13:14))*3600 + str2double(tline(16:17))*60 + str2double(tline(19:20)) + str2double(tline(21:30));
            mac_string = tline(ret+8:end);
            end_of_desired_str = find(mac_string=='.',1);
            mac_string = mac_string(1:end_of_desired_str-1);
            mac = str2num(['int64(' mac_string ')']);
            %fprintf(1,'MAC: %d\n',mac);

            % Add mac to struct.
            number_of_acks = length(acks_struct);
            index = -1;
            for i=1:1:number_of_acks
                mac_stats_struct = acks_struct(i);
                if(mac_stats_struct.mac == mac)
                    index = i;
                    break;
                end
            end
            % Now parse the values: MCS and TX gain.

            % new TX Gain
            ret = strfind(tline,'Increase TX power to ');
            aux_str = tline(ret:end);
            end_of_desired_str = find(aux_str=='i',1);
            tx_gain_string = aux_str(22:end_of_desired_str-3);
            tx_gain = str2num(['int8(' tx_gain_string ')']);
            mac_stats_struct.mac_timeout.new_tx_gain = [mac_stats_struct.mac_timeout.new_tx_gain, tx_gain];
            %fprintf(1,'\t New TX Gain: %d\n',tx_gain);

            % new MCS
            ret = strfind(tline,'with mcs ');
            aux_str = tline(ret:end);
            mcs_string = aux_str(10:end);
            mcs = str2num(['int8(' mcs_string ')']);
            mac_stats_struct.mac_timeout.new_mcs = [mac_stats_struct.mac_timeout.new_mcs, mcs];
            %fprintf(1,'\t New MCS: %d\n',mcs);

            % Timestamp diff.
            time_diff = mac_timeout_timestamp - first_timestamp;
            mac_stats_struct.mac_timeout.timestamp_diff = [mac_stats_struct.mac_timeout.timestamp_diff, time_diff];
            mac_stats_struct.total_time_diff = [mac_stats_struct.total_time_diff, time_diff];
        end

       acks_struct(index) = mac_stats_struct;
    end
    tline = fgetl(fid);
end
fclose(fid);

%% Now, plot things for each MAC address.
LOWER_RSSI_BOUND = -37;
UPPER_RSSI_BOUND = -7;

number_of_acks = length(acks_struct);
for i=1:1:number_of_acks
    figure(i);
    mac_stats_struct = acks_struct(i);

    % Change the limits to analyse some specific part of the plot.
    minimum_time_diff = min(mac_stats_struct.total_time_diff);
    maximum_time_diff = max(mac_stats_struct.total_time_diff);

    plot(mac_stats_struct.mac_stats.timestamp_diff, mac_stats_struct.mac_stats.rssi,'b','linewidth',2);
    hold on

    plot(mac_stats_struct.mac_stats.timestamp_diff, mac_stats_struct.mac_stats.cqi,'r','linewidth',2);
    % Average CQI.
    avg_cqi = sum(mac_stats_struct.mac_stats.cqi)/length(mac_stats_struct.mac_stats.cqi);
    plot(mac_stats_struct.mac_stats.timestamp_diff, avg_cqi*ones(1,length(mac_stats_struct.mac_stats.cqi)),'--r','linewidth',2);
    avg_cqi_legend_str = sprintf('Rxed Avg. CQI: %1.2f',avg_cqi);

    plot(mac_stats_struct.mac_stats.timestamp_diff, mac_stats_struct.mac_stats.mcs,'k','linewidth',2);
    plot(mac_stats_struct.mac_action.timestamp_diff, mac_stats_struct.mac_action.new_mcs,'ko','linewidth',2);
    plot(mac_stats_struct.mac_timeout.timestamp_diff, mac_stats_struct.mac_timeout.new_mcs,'kx','linewidth',2);

    plot(mac_stats_struct.mac_stats.timestamp_diff, mac_stats_struct.mac_stats.tx_gain,'g','linewidth',2);
    plot(mac_stats_struct.mac_action.timestamp_diff, mac_stats_struct.mac_action.new_tx_gain,'go','linewidth',2);
    plot(mac_stats_struct.mac_timeout.timestamp_diff, mac_stats_struct.mac_timeout.new_tx_gain,'gx','linewidth',2);

    x_axis_bounds = minimum_time_diff:maximum_time_diff;
    plot(x_axis_bounds,UPPER_RSSI_BOUND*ones(1,length(x_axis_bounds)),'linewidth',2);
    plot(x_axis_bounds,LOWER_RSSI_BOUND*ones(1,length(x_axis_bounds)),'linewidth',2);

    xlim([minimum_time_diff maximum_time_diff]);
    title_str = sprintf('Rxed ACK - Match: %s - MAC: %d',folder(3:11),mac_stats_struct.mac);
    xlabel('Time [s]');
    title(title_str);
    legend('Rxed ACK RSSI','Rxed ACK CQI',avg_cqi_legend_str,'Rxed ACK MCS','Action New MCS','Timeout New MCS','Rxed ACK TX Gain','Action New Tx Gain','Timeout New TX Gain','UPPER RSSI BOUND','LOWER RSSI BOUND');
    grid on
    hold off

    scaleFactor = 1.6;
    set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
    saveas_file_name = sprintf('%s_mac_%d.png',folder(3:11),mac_stats_struct.mac);
    saveas(gcf,saveas_file_name)
end
