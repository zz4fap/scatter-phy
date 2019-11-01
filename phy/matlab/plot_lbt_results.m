clear all;close all;clc

folder = './LBT/';
figure_file_name = 'lbt-comparison-colosseum';
trial_number = 0;

figure(1)

%% -------
filename = sprintf('%s%s',folder,'only-rx-colosseum-0.txt.mat');
load(filename)

subplot(2,2,1);
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

var_string = sprintf('RSSI variance: %f\nCQI variance: %f',var(rssi),var(cqi));
text(0.2e4,-7.5,var_string);

title_str = sprintf('Test: %s',test_name_str);
title(title_str);
xlabel('Time [s]');
leg=legend({'RSSI',avg_rssi_legend_str,'CQI',avg_cqi_legend_str},'Location','best');

first_legend_position=get(leg,'position')

grid on
hold off

rssi_2 = rssi;
cqi_2 = cqi;
title_str_2 = test_name_str;
clear rssi cqi test_name_str

%% -------
filename = sprintf('%s%s',folder,'without-lbt-colosseum-1.txt.mat');
load(filename)

subplot(2,2,2);
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

var_string = sprintf('RSSI variance: %f\nCQI variance: %f',var(rssi),var(cqi));
text(0.2e4,-7.5,var_string);

title_str = sprintf('Test: %s',test_name_str);
title(title_str);
xlabel('Time [s]');
legend({'RSSI',avg_rssi_legend_str,'CQI',avg_cqi_legend_str},'Location','best');

grid on
hold off

rssi_0 = rssi;
cqi_0 = cqi;
title_str_0 = test_name_str;
clear rssi cqi test_name_str

%% -------
filename = sprintf('%s%s',folder,'with-lbt-colosseum-0.txt.mat');
load(filename)

subplot(2,2,3);
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

var_string = sprintf('RSSI variance: %f\nCQI variance: %f',var(rssi),var(cqi));
text(0.2e4,-7.5,var_string);

title_str = sprintf('Test: %s',test_name_str);
title(title_str);
xlabel('Time [s]');
legend({'RSSI',avg_rssi_legend_str,'CQI',avg_cqi_legend_str},'Location','best');

grid on
hold off

rssi_1 = rssi;
cqi_1 = cqi;
title_str_1 = test_name_str;
clear rssi cqi test_name_str

%% -------
filename = sprintf('%s%s',folder,'both-nodes-with-lbt-colosseum-0.txt.mat');
load(filename)

subplot(2,2,4);
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

var_string = sprintf('RSSI variance: %f\nCQI variance: %f',var(rssi),var(cqi));
text(0.2e4,-7.5,var_string);

title_str = sprintf('Test: %s',test_name_str);
title(title_str);
xlabel('Time [s]');
legend({'RSSI',avg_rssi_legend_str,'CQI',avg_cqi_legend_str},'Location','best');

grid on
hold off

rssi_3 = rssi;
cqi_3 = cqi;
title_str_3 = test_name_str;
clear rssi cqi test_name_str

%% -------
% Save figure.
scaleFactor = 3.0;
set(gcf, 'Position', [100, 100, ceil(scaleFactor*560), ceil(scaleFactor*420)])
saveas_file_name = sprintf('%s%s_%d.png',folder,figure_file_name,trial_number);
saveas(gcf,saveas_file_name)

