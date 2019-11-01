clear all;close all;clc

folder = './LBT/';

filename = 'both-nodes-with-lbt-colosseum-0.txt';
%filename = 'without-lbt-colosseum-1.txt';
%filename = 'with-lbt-colosseum-0.txt';
%filename = 'only-rx-colosseum-0.txt';


%% Process file for received ACK message.
string_to_look_for = 'RSSI:';

% Open file.
full_path_to_file = strcat(folder,filename);
fid = fopen(full_path_to_file);

% RX ACK Message structure.
rssi = [];
cqi = [];

tline = fgetl(fid);
while ischar(tline)
    
    % *** Retrieve RSSI from log. ***
    end_of_desired_str = find(tline==':',1);
    aux_str = tline(end_of_desired_str+2:end);
    end_of_desired_str = find(aux_str==':',1);
    rssi_string = aux_str(1:end_of_desired_str-5);
    rssi = [rssi, str2num(rssi_string)];
    %fprintf(1,'RSSI: %f\n',rssi);
 
    % *** Retrieve CQI from log. ***
    end_of_desired_str = find(tline==':',2);
    cqi_string = tline(end_of_desired_str(2)+2:end);
    cqi = [cqi, str2num(cqi_string)];
    %fprintf(1,'CQI: %d\n',cqi);
    
    tline = fgetl(fid);
end
fclose(fid);

%% Now, plot things.

% Plot RSSI and CQI.
start_index = 1;
end_index = 19025; %length(rssi);
rssi = rssi(start_index:end_index);
cqi = cqi(start_index:end_index);
for i=1:1:1
    figure(i);
 
    % Plot RSSI.
    plot(rssi,'b','linewidth',2);
    hold on
    
    % Calculate Average RSSI.
    avg_rssi = sum(rssi)/length(rssi);
    plot(avg_rssi*ones(1,length(rssi)),'--b','linewidth',2);
    avg_rssi_legend_str = sprintf('Avg. RSSI: %1.2f',avg_rssi);
    
    % Plot CQI.
    plot(cqi,'r','linewidth',2);
    
    % Calculate Average CQI.
    avg_cqi = sum(cqi)/length(cqi);
    plot(avg_cqi*ones(1,length(cqi)),'--r','linewidth',2);
    avg_cqi_legend_str = sprintf('Avg. CQI: %1.2f',avg_cqi);

    title_str = sprintf('Test: %s',filename(1:end-4));
    title(title_str);
    xlabel('Time [s]');
    legend('RSSI',avg_rssi_legend_str,'CQI',avg_cqi_legend_str);

    grid on
    hold off
    
    % Save figure.
    scaleFactor = 1.6;
    set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
    saveas_fig_name = sprintf('%s%s.fig',folder,filename);
    saveas(gcf,saveas_fig_name)
    saveas_mat_name = sprintf('%s%s.mat',folder,filename);
    test_name_str = filename(1:end-4);
    save(saveas_mat_name,'cqi','rssi','test_name_str')
end
