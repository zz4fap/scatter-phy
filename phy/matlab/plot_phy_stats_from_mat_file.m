clear all;close all;clc

%BATCH_ID=[19039 19040 19041 19042 19043 19044 19045 19086 19087];

%BATCH_ID=[19160 19163 19164 19166 19167 19168 19169];

%BATCH_ID = [19171 19172 19173 19175 19176 19177 19179 19180 19181 19182 19183 19184];

BATCH_ID = [20155];

for id_cnt=1:1:length(BATCH_ID)
    
    folder = sprintf('/home/zz4fap/Downloads/link_level_check/LBT/OLD_8/RESERVATION-%d-all-logs/',BATCH_ID(id_cnt));
    
    delete_str=sprintf('rm %s*.png',folder);
    system(delete_str);
    
    files = dir(folder);
    
    figure_counter = 0;
    for k=3:1:length(files)
        
        file = files(k).name;
        
        ret1 = strfind(file,'.mat');
        
        ret2 = strfind(file,'-mac-stats-structs.mat');
        
        ret3 = strfind(file,'-ai-stats-structs.mat');
        
        if(files(k).isdir == 0 && ~isempty(ret1) && isempty(ret2) && isempty(ret3))
            
            % Create the .mat file name.
            dot_mat_file_name = sprintf('%s%s',folder,file);
            
            % Clear and load .mat.
            clear tx_stats_struct rx_stats_struct wrong_rx_stats_struct
            load(dot_mat_file_name);
            
            %% Now, plot TX Stats for each channel.
            if(1)
                number_ot_tx_stats = length(tx_stats_struct);
                for i=1:1:number_ot_tx_stats
                    
                    tx_stats = tx_stats_struct(i);
                    
                    if(tx_stats.channel == 0)
                        
                        if(0)
                            last_timestamp = 0;
                            for kk=1:1:length(tx_stats.timestamp_diff)
                                if(tx_stats.mcs(kk) >= 15)
                                    diff = tx_stats.timestamp_diff(kk) - last_timestamp;
                                    last_timestamp = tx_stats.timestamp_diff(kk);
                                    fprintf(1,'Diff: %f\n',diff);
                                end
                            end
                        end
                        
                        if(length(tx_stats.timestamp_diff) > 1)
                            figure_counter = figure_counter + 1;
                            figure(figure_counter);
                            
                            timediff_indexes = find(tx_stats.timestamp_diff > 400 & tx_stats.timestamp_diff < 405);
                            
                            % Change the limits to analyse some specific part of the plot.
                            minimum_time_diff = min(tx_stats.timestamp_diff(timediff_indexes));
                            maximum_time_diff = max(tx_stats.timestamp_diff(timediff_indexes));
                            % Figure out maximum and minimum time axis values.
                            x_axis_bounds = minimum_time_diff:maximum_time_diff;
                            
                            subplot(3,1,1)
                            % Plot Number o TX slots.
                            plot(tx_stats.timestamp_diff(timediff_indexes), tx_stats.number_of_slots(timediff_indexes),'b','linewidth',2);
                            hold on
                            
                            % Plot MCS.
                            plot(tx_stats.timestamp_diff(timediff_indexes), tx_stats.mcs(timediff_indexes),'.r','linewidth',2);
                            % Plot Average MCS.
                            avg_mcs = sum(tx_stats.mcs)/length(tx_stats.mcs);
                            plot(x_axis_bounds, avg_mcs*ones(1,length(x_axis_bounds)),'--r','linewidth',2);
                            avg_mcs_legend_str = sprintf('TXed Avg. MCS: %1.2f',avg_mcs);
                            
                            % Plot coding time.
                            %plot(tx_stats.timestamp_diff, tx_stats.coding_time,'k','linewidth',2);
                            % Plot Average coding time.
                            %avg_coding_time = sum(tx_stats.coding_time)/length(tx_stats.coding_time);
                            %plot(x_axis_bounds, avg_coding_time*ones(1,length(x_axis_bounds)),'--k','linewidth',2);
                            %avg_coding_time_legend_str = sprintf('Avg. Coding Time: %1.2f [ms]',avg_coding_time);
                            
                            xlim([minimum_time_diff maximum_time_diff]);
                            title_str = sprintf('TX Stats - Match: %s - Channel: %d',file(1:end-18),tx_stats.channel);
                            xlabel('Time [s]');
                            title(title_str);
                            %legend('# TX slots','TX Gain [dB]','MCS',avg_mcs_legend_str,'Coding Time [ms]', avg_coding_time_legend_str);
                            legend('# TX slots','MCS',avg_mcs_legend_str);
                            grid on
                            hold off
                            
                            subplot(3,1,2)
                            % Plot TX Gain
                            plot(tx_stats.timestamp_diff(timediff_indexes), tx_stats.tx_gain(timediff_indexes),'g','linewidth',2);
                            
                            xlim([minimum_time_diff maximum_time_diff]);
                            xlabel('Time [s]');
                            %legend('# TX slots','TX Gain [dB]','MCS',avg_mcs_legend_str,'Coding Time [ms]', avg_coding_time_legend_str);
                            legend('TX Gain [dB]');
                            grid on
                            hold off
                            
                            subplot(3,1,3)
                            % Plot Coding time
                            plot(tx_stats.timestamp_diff(timediff_indexes), tx_stats.coding_time(timediff_indexes),'k.','linewidth',2);
                            avg_coding_time = sum(tx_stats.coding_time)/length(tx_stats.coding_time);
                            
                            xlim([minimum_time_diff maximum_time_diff]);
                            xlabel('Time [s]');
                            legend_str = sprintf('Coding time - Avg: %1.4f [ms]',avg_coding_time);
                            legend(legend_str);
                            grid on
                            hold off
                            
                            % Save figure.
                            scaleFactor = 1.6;
                            set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
                            saveas_file_name = sprintf('%s%s-tx-stats-channel_%d.png',folder,file(1:end-18),tx_stats.channel);
                            saveas(gcf,saveas_file_name);
                        end
                    end
                end
            end
            
            %% Now, plot RX Stats for each channel.
            if(0)
                number_ot_rx_stats = length(rx_stats_struct);
                for i=1:1:number_ot_rx_stats
                    
                    rx_stats = rx_stats_struct(i);
                    
                    if(length(rx_stats.timestamp_diff) > 1)
                        figure_counter = figure_counter + 1;
                        fig=figure(figure_counter);
                        fig.Name = 'RX Statistics';
                        
                        timediff_indexes = find(rx_stats.timestamp_diff > 0 & rx_stats.timestamp_diff < 10000);
                        
                        % Change the limits to analyse some specific part of the plot.
                        minimum_time_diff = min(rx_stats.timestamp_diff(timediff_indexes));
                        maximum_time_diff = max(rx_stats.timestamp_diff(timediff_indexes));
                        % Figure out maximum and minimum time axis values.
                        x_axis_bounds = minimum_time_diff:maximum_time_diff;
                        
                        subplot(3,1,1)
                        
                        % Plot MCS.
                        plot(rx_stats.timestamp_diff(timediff_indexes), rx_stats.mcs(timediff_indexes),'r.','linewidth',2);
                        hold on
                        % Calculate average MCS.
                        avg_mcs = sum(rx_stats.mcs(timediff_indexes))/length(rx_stats.mcs(timediff_indexes));
                        avg_mcs_legend_str = sprintf('MCS - Avg.: %1.2f',avg_mcs);
                        % Plot decoding time.
                        plot(rx_stats.timestamp_diff(timediff_indexes), rx_stats.decoding_time(timediff_indexes),'k','linewidth',2);
                        % Plot Average decoding time.
                        avg_decoding_time = sum(rx_stats.decoding_time(timediff_indexes))/length(rx_stats.decoding_time(timediff_indexes));
                        avg_decoding_time_legend_str = sprintf('Decoding Time - Avg. %1.2f [ms]',avg_decoding_time);
                        
                        % Plot NOI.
                        plot(rx_stats.timestamp_diff(timediff_indexes), rx_stats.noi(timediff_indexes),'g','linewidth',2);
                        
                        xlim([minimum_time_diff maximum_time_diff]);
                        title_str = sprintf('RX Stats - Match: %s - Channel: %d',file(1:end-18),rx_stats.channel);
                        xlabel('Time [s]');
                        title(title_str);
                        legend(avg_mcs_legend_str,avg_decoding_time_legend_str,'NOI');
                        grid on
                        hold off
                        
                        %% ------------------------------------------------------------
                        subplot(3,1,2)
                        % PLot RSSI
                        plot(rx_stats.timestamp_diff(timediff_indexes), rx_stats.rssi(timediff_indexes),'linewidth',2);
                        hold on
                        % Plot CQI
                        plot(rx_stats.timestamp_diff(timediff_indexes), rx_stats.cqi(timediff_indexes),'linewidth',2);
                        % Plot Peak
                        plot(rx_stats.timestamp_diff(timediff_indexes), rx_stats.peak(timediff_indexes),'linewidth',2);
                        
                        xlim([minimum_time_diff maximum_time_diff]);
                        rx_slots_str = sprintf('Correctly received slots: %d - Errors: %d',rx_stats.total_number_of_slots(length(rx_stats.total_number_of_slots)),rx_stats.error(length(rx_stats.error)));
                        title_str = sprintf(rx_slots_str);
                        xlabel('Time [s]');
                        title(title_str);
                        legend('RSSI [dBm]','CQI','Peak');
                        grid on
                        hold off
                        
                        subplot(3,1,3)
                        % Plot noise
                        plot(rx_stats.timestamp_diff(timediff_indexes), rx_stats.noise(timediff_indexes),'linewidth',2);
                        
                        xlim([minimum_time_diff maximum_time_diff]);
                        rx_slots_str = sprintf('Correctly received slots: %d - Errors: %d',rx_stats.total_number_of_slots(length(rx_stats.total_number_of_slots)),rx_stats.error(length(rx_stats.error)));
                        title_str = sprintf(rx_slots_str);
                        xlabel('Time [s]');
                        title(title_str);
                        legend('Noise');
                        grid on
                        hold off
                        
                        % Save figure.
                        scaleFactor = 2;
                        set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
                        saveas_file_name = sprintf('%s%s-rx-stats-channel_%d.png',folder,file(1:end-18),rx_stats.channel);
                        saveas(gcf,saveas_file_name);
                    end
                end
                
            end
            
            %% Now, plot Wrong RX Stats for each channel.
            if(0)
                number_of_wrong_rx_stats = length(wrong_rx_stats_struct);
                for i=1:1:number_of_wrong_rx_stats
                    
                    wrong_rx_stats = wrong_rx_stats_struct(i);
                    
                    if(length(wrong_rx_stats.timestamp_diff) > 1)
                        figure_counter = figure_counter + 1;
                        fig=figure(figure_counter);
                        fig.Name = 'Wrong RX Slots Statistics';
                        
                        % Change the limits to analyse some specific part of the plot.
                        minimum_time_diff = min(wrong_rx_stats.timestamp_diff);
                        maximum_time_diff = max(wrong_rx_stats.timestamp_diff);
                        % Figure out maximum and minimum time axis values.
                        x_axis_bounds = minimum_time_diff:maximum_time_diff;
                        
                        subplot(2,1,1)
                        % Plot Peak.
                        plot(wrong_rx_stats.timestamp_diff, wrong_rx_stats.peak,'linewidth',2);
                        hold on
                        % Plot RSSI.
                        plot(wrong_rx_stats.timestamp_diff, wrong_rx_stats.rssi,'linewidth',2);
                        % Plot CFI.
                        plot(wrong_rx_stats.timestamp_diff, wrong_rx_stats.cfi,'linewidth',2);
                        % Plot DCI.
                        plot(wrong_rx_stats.timestamp_diff, wrong_rx_stats.dci,'linewidth',2);
                        % Plot NOI.
                        plot(wrong_rx_stats.timestamp_diff, wrong_rx_stats.noi,'linewidth',2);
                        % Plot CFO.
                        plot(wrong_rx_stats.timestamp_diff, wrong_rx_stats.cfo,'linewidth',2);
                        
                        xlim([minimum_time_diff maximum_time_diff]);
                        title_str = sprintf('Wrong RX Stats - Match: %s - Channel: %d',file(1:end-18),wrong_rx_stats.channel);
                        xlabel('Time [s]');
                        title(title_str);
                        legend('Peak','RSSI [dBm]','CFI','DCI','NOI','CFO [KHz]');
                        grid on
                        hold off
                        
                        subplot(2,1,2)
                        % Plot Noise.
                        plot(wrong_rx_stats.timestamp_diff, wrong_rx_stats.noise,'linewidth',2);
                        xlim([minimum_time_diff maximum_time_diff]);
                        
                        wrong_rx_slots_str = sprintf('Total number of wrong decoded slots: %d',wrong_rx_stats.number_of_slots(length(wrong_rx_stats.number_of_slots)));
                        title_str = sprintf(wrong_rx_slots_str);
                        xlabel('Time [s]');
                        title(title_str);
                        legend('Noise');
                        grid on
                        
                        % Save figure.
                        scaleFactor = 1.6;
                        set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
                        saveas_file_name = sprintf('%s%s-wrong-rx-stats-channel_%d.png',folder,file(1:end-18),wrong_rx_stats.channel);
                        saveas(gcf,saveas_file_name);
                    end
                end
            end
            
            %% Now, plot Wrong RX Stats for each channel.
            if(0)
                number_of_wrong_rx_stats = length(wrong_rx_stats_struct);
                for i=1:1:number_of_wrong_rx_stats
                    
                    wrong_rx_stats = wrong_rx_stats_struct(i);
                    
                    if(length(wrong_rx_stats.timestamp_diff) > 1)
                        
                        decoding_error = [];
                        decoding_error.channel = wrong_rx_stats.channel;
                        decoding_error.timestamp_diff = [];
                        decoding_error.peak = [];
                        decoding_error.rssi = [];
                        decoding_error.noi = [];
                        decoding_error.cfo = [];
                        decoding_error.noise = [];
                        decoding_error.number_of_slots = [];
                        
                        detection_error = [];
                        detection_error.channel = wrong_rx_stats.channel;
                        detection_error.timestamp_diff = [];
                        detection_error.peak = [];
                        detection_error.rssi = [];
                        detection_error.noi = [];
                        detection_error.cfo = [];
                        detection_error.noise = [];
                        detection_error.number_of_slots = [];
                        detection_error_cnt = 0;
                        decoding_error_cnt = 0;
                        for kk=1:1:length(wrong_rx_stats.timestamp_diff)
                            if(wrong_rx_stats.cfi(kk) == 1 && wrong_rx_stats.dci(kk) == 1)
                                decoding_error.timestamp_diff = [decoding_error.timestamp_diff wrong_rx_stats.timestamp_diff(kk)];
                                decoding_error.peak = [decoding_error.peak wrong_rx_stats.peak(kk)];
                                decoding_error.rssi = [decoding_error.rssi wrong_rx_stats.rssi(kk)];
                                decoding_error.noi = [decoding_error.noi wrong_rx_stats.noi(kk)];
                                decoding_error.cfo = [decoding_error.cfo wrong_rx_stats.cfo(kk)];
                                decoding_error.noise = [decoding_error.noise wrong_rx_stats.noise(kk)];
                                decoding_error.number_of_slots = [decoding_error.number_of_slots  wrong_rx_stats.number_of_slots(kk)];
                                decoding_error_cnt = decoding_error_cnt + 1;
                            else
                                detection_error.timestamp_diff = [detection_error.timestamp_diff wrong_rx_stats.timestamp_diff(kk)];
                                detection_error.peak = [detection_error.peak wrong_rx_stats.peak(kk)];
                                detection_error.rssi = [detection_error.rssi wrong_rx_stats.rssi(kk)];
                                detection_error.noi = [detection_error.noi wrong_rx_stats.noi(kk)];
                                detection_error.cfo = [detection_error.cfo wrong_rx_stats.cfo(kk)];
                                detection_error.noise = [detection_error.noise wrong_rx_stats.noise(kk)];
                                detection_error.number_of_slots = [detection_error.number_of_slots  wrong_rx_stats.number_of_slots(kk)];
                                detection_error_cnt = detection_error_cnt + 1;
                            end
                        end
                        
                        figure_counter = figure_counter + 1;
                        fig=figure(figure_counter);
                        fig.Name = 'Decoding Error';
                        
                        % Change the limits to analyse some specific part of the plot.
                        minimum_time_diff = min(decoding_error.timestamp_diff);
                        maximum_time_diff = max(decoding_error.timestamp_diff);
                        % Figure out maximum and minimum time axis values.
                        x_axis_bounds = minimum_time_diff:maximum_time_diff;
                        
                        subplot(2,1,1)
                        % Plot Peak.
                        plot(decoding_error.timestamp_diff, decoding_error.peak,'linewidth',2);
                        hold on
                        % Plot RSSI.
                        plot(decoding_error.timestamp_diff, decoding_error.rssi,'linewidth',2);
                        % Plot NOI.
                        plot(decoding_error.timestamp_diff, decoding_error.noi,'linewidth',2);
                        % Plot CFO.
                        plot(decoding_error.timestamp_diff, decoding_error.cfo,'linewidth',2);
                        
                        xlim([minimum_time_diff maximum_time_diff]);
                        title_str = sprintf('Decoding error - Match: %s - Channel: %d',file(1:end-18),decoding_error.channel);
                        xlabel('Time [s]');
                        title(title_str);
                        legend('Peak','RSSI [dBm]','NOI','CFO [KHz]');
                        grid on
                        hold off
                        
                        subplot(2,1,2)
                        % Plot Noise.
                        plot(decoding_error.timestamp_diff, decoding_error.noise,'linewidth',2);
                        xlim([minimum_time_diff maximum_time_diff]);
                        
                        wrong_rx_slots_str = sprintf('Total Errors: %d - Decoding errors: %d',wrong_rx_stats.number_of_slots(length(wrong_rx_stats.number_of_slots)),decoding_error_cnt);
                        title_str = sprintf(wrong_rx_slots_str);
                        xlabel('Time [s]');
                        title(title_str);
                        legend('Noise');
                        grid on
                        
                        % Save figure.
                        scaleFactor = 1.6;
                        set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
                        saveas_file_name = sprintf('%s%s-decoding-error-stats-channel_%d.png',folder,file(1:end-18),decoding_error.channel);
                        saveas(gcf,saveas_file_name);
                        
                        %% ******************** Detection Error *************************
                        figure_counter = figure_counter + 1;
                        fig=figure(figure_counter);
                        fig.Name = 'Detection Error';
                        
                        % Change the limits to analyse some specific part of the plot.
                        minimum_time_diff = min(detection_error.timestamp_diff);
                        maximum_time_diff = max(detection_error.timestamp_diff);
                        % Figure out maximum and minimum time axis values.
                        x_axis_bounds = minimum_time_diff:maximum_time_diff;
                        
                        subplot(2,1,1)
                        % Plot Peak.
                        plot(detection_error.timestamp_diff, detection_error.peak,'linewidth',2);
                        hold on
                        % Plot RSSI.
                        plot(detection_error.timestamp_diff, detection_error.rssi,'linewidth',2);
                        % Plot NOI.
                        plot(detection_error.timestamp_diff, detection_error.noi,'linewidth',2);
                        % Plot CFO.
                        plot(detection_error.timestamp_diff, detection_error.cfo,'linewidth',2);
                        
                        xlim([minimum_time_diff maximum_time_diff]);
                        title_str = sprintf('Detection error - Match: %s - Channel: %d',file(1:end-18),detection_error.channel);
                        xlabel('Time [s]');
                        title(title_str);
                        legend('Peak','RSSI [dBm]','NOI','CFO [KHz]');
                        grid on
                        hold off
                        
                        subplot(2,1,2)
                        % Plot Noise.
                        plot(detection_error.timestamp_diff, detection_error.noise,'linewidth',2);
                        xlim([minimum_time_diff maximum_time_diff]);
                        
                        wrong_rx_slots_str = sprintf('Total errors: %d - Detection errors: %d',wrong_rx_stats.number_of_slots(length(wrong_rx_stats.number_of_slots)),detection_error_cnt);
                        title_str = sprintf(wrong_rx_slots_str);
                        xlabel('Time [s]');
                        title(title_str);
                        legend('Noise');
                        grid on
                        
                        % Save figure.
                        scaleFactor = 1.6;
                        set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
                        saveas_file_name = sprintf('%s%s-detection-error-stats-channel_%d.png',folder,file(1:end-18),detection_error.channel);
                        saveas(gcf,saveas_file_name);
                    end
                end
            end
            
            
        end
    end
end
