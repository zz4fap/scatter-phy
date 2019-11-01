clear all;close all;clc

max_nof_files = 2;

start_file = 0;

%% ------------------------------------------------------------------------
folder = '../closer_coexistence/results_single_node_v0';

mcs_vector = [0 10 12 14 16 18 20 22 24 26 28];

prr_single_node = zeros(1,length(mcs_vector));
prr_percentage_single_node = zeros(1,length(mcs_vector));
for idx=1:1:length(mcs_vector)
       
    fileName = sprintf('%s/single_node_mcs_%d.txt',folder,mcs_vector(idx));
    
    results = load(fileName);
    
    total_pkts =  results(length(results),10) - results(1,10) + 1; % Total of packets synchronized: stat.rx_stat.total_packets_synchronized
    
    correctly_decoded = results(length(results),7);
    
    prr_single_node(idx) = correctly_decoded / total_pkts;
    
    prr_percentage_single_node(idx) = prr_single_node(idx)*100;
    
    valid_result = find(results(:,1) == 100);
    
    mcs_idx = results(valid_result(length(valid_result)),2);
    
    fprintf('MCS: %d - Correctly decoded : %d - PRR: %f - PRR: %f %%\n', mcs_idx, correctly_decoded, prr_single_node(idx),prr_percentage_single_node(idx));

end

%% ------------------------------------------------------------------------
mcs_vector = [0 10 12 14 16 18 20 22 24 26 28];

prr_no_filtering = zeros(1,length(mcs_vector));
prr_percentage_no_filtering = zeros(1,length(mcs_vector));
total_pkts = zeros(1,length(mcs_vector));
correctly_decoded = zeros(1,length(mcs_vector));
for k=start_file:1:(max_nof_files-1)
    
    folder = sprintf('../closer_coexistence/results_no_filtering_v%d',k);
    
    for idx=1:1:length(mcs_vector)
        
        fileName = sprintf('%s/closer_coexistance_no_filtering_mcs_%d.txt',folder,mcs_vector(idx));
        
        results = load(fileName);
        
        total_pkts(idx) =  total_pkts(idx) + (results(length(results),10) - results(1,10) + 1);
        
        correctly_decoded(idx) = correctly_decoded(idx) + (results(length(results),7) - results(1,7) + 1);

    end

end

for idx=1:1:length(mcs_vector)
    
    prr_no_filtering(idx) = correctly_decoded(idx) / total_pkts(idx);
    
    prr_percentage_no_filtering(idx) = prr_no_filtering(idx)*100;
    
    fprintf('MCS: %d - Correctly decoded : %d - PRR: %f - PRR: %f %%\n', mcs_vector(idx), correctly_decoded(idx), prr_no_filtering(idx), prr_percentage_no_filtering(idx));
    
end

%% ------------------------------------------------------------------------
mcs_vector = [0 10 12 14 16 18 20 22 24 26 28];
  
prr_fir_64 = zeros(1,length(mcs_vector));
prr_percentage_fir_64 = zeros(1,length(mcs_vector));
total_pkts = zeros(1,length(mcs_vector));
correctly_decoded = zeros(1,length(mcs_vector));
for k=start_file:1:(max_nof_files-1)
    
    folder = sprintf('../closer_coexistence/results_fir_64_v%d',k);
    
    for idx=1:1:length(mcs_vector)
        
        fileName = sprintf('%s/closer_coexistance_64_filter_mcs_%d.txt',folder,mcs_vector(idx));
        
        results = load(fileName);
        
        total_pkts(idx) = total_pkts(idx)+ (results(length(results),10) - results(1,10));
        
        correctly_decoded(idx) = correctly_decoded(idx) + (results(length(results),7) - results(1,7) + 1);
        
    end
    
end

for idx=1:1:length(mcs_vector)
    
    prr_fir_64(idx) = correctly_decoded(idx) / total_pkts(idx);
    
    prr_percentage_fir_64(idx) = prr_fir_64(idx)*100;
    
    fprintf('MCS: %d - Correctly decoded : %d - PRR: %f - PRR: %f %%\n', mcs_vector(idx), correctly_decoded(idx), prr_fir_64(idx), prr_percentage_fir_64(idx));
   
end

%% ------------------------------------------------------------------------
mcs_vector = [0 10 12 14 16 18 20 22 24 26 28];

prr_fir_128 = zeros(1,length(mcs_vector));
prr_percentage_fir_128 = zeros(1,length(mcs_vector));
total_pkts = zeros(1,length(mcs_vector));
correctly_decoded = zeros(1,length(mcs_vector));
for k=start_file:1:(max_nof_files-1)
    
    folder = sprintf('../closer_coexistence/results_fir_128_v%d',k);
    
    for idx=1:1:length(mcs_vector)
        
        fileName = sprintf('%s/closer_coexistance_128_filter_mcs_%d.txt',folder,mcs_vector(idx));
        
        results = load(fileName);
        
        total_pkts(idx) = total_pkts(idx) + (results(length(results),10) - results(1,10));
        
        correctly_decoded(idx) = correctly_decoded(idx) + (results(length(results),7) - results(1,7) + 1);
        
    end
    
end

for idx=1:1:length(mcs_vector)
    
    prr_fir_128(idx) = correctly_decoded(idx) / total_pkts(idx);
    
    prr_percentage_fir_128(idx) = prr_fir_128(idx)*100;
    
    fprintf('MCS: %d - Correctly decoded : %d - PRR: %f - PRR: %f %%\n', mcs_vector(idx), correctly_decoded(idx), prr_fir_128(idx), prr_percentage_fir_128(idx));

end

%% ------------------------------------------------------------------------
prr_percentage_single_node_longer = zeros(1,15);
prr_percentage_single_node_longer(1) = prr_percentage_single_node(1);
prr_percentage_single_node_longer(2:5) = prr_percentage_single_node(1)*ones(1,4);
for j=1:1:10
    prr_percentage_single_node_longer(5+j) = prr_percentage_single_node(1+j);
end

prr_percentage_fir_128_longer = zeros(1,15);
prr_percentage_fir_128_longer(1) = prr_percentage_fir_128(1);
prr_percentage_fir_128_longer(2:5) = prr_percentage_fir_128(1)*ones(1,4);
for j=1:1:10
   prr_percentage_fir_128_longer(5+j) = prr_percentage_fir_128(1+j); 
end

prr_percentage_fir_64_longer = zeros(1,15);
prr_percentage_fir_64_longer(1) = prr_percentage_fir_64(1);
prr_percentage_fir_64_longer(2:5) = prr_percentage_fir_64(1)*ones(1,4);
for j=1:1:10
   prr_percentage_fir_64_longer(5+j) = prr_percentage_fir_64(1+j);
end

prr_percentage_no_filtering_longer = zeros(1,15);
prr_percentage_no_filtering_longer(1) = prr_percentage_no_filtering(1);
prr_percentage_no_filtering_longer(2:5) = prr_percentage_no_filtering(1)*ones(1,4);
for j=1:1:10
    prr_percentage_no_filtering_longer(5+j) = prr_percentage_no_filtering(1+j); 
end

fdee_figure1 = figure('rend','painters','pos',[10 10 700 600]);

linewidth = 1.5;

subplot(2,1,1)
plot(0:2:28,prr_percentage_single_node_longer,'LineWidth',linewidth)
hold on
plot(0:2:28,prr_percentage_fir_128_longer,'LineWidth',linewidth)
plot(0:2:28,prr_percentage_fir_64_longer,'LineWidth',linewidth)
plot(0:2:28,prr_percentage_no_filtering_longer,'LineWidth',linewidth)
grid on
xticks([0 2 4 6 8 10 12 14 16 18 20 22 24 26 28])
yticks([0 20 40 60 80 100])
xlim([0 28])
ylim([0 100])
xlabel('MCS')
ylabel('PRR %')
hold off
lgd = legend('Simplex','128 order FIR','64 order FIR','No Filtering','Location','southwest');
lgd.FontSize = 10;
set(gca,'fontsize',11)

subplot(2,1,2);
plot(0:2:28,prr_percentage_single_node_longer,'LineWidth',linewidth)
hold on
plot(0:2:28,prr_percentage_fir_128_longer,'LineWidth',linewidth)
plot(0:2:28,prr_percentage_fir_64_longer,'LineWidth',linewidth)
plot(0:2:28,prr_percentage_no_filtering_longer,'LineWidth',linewidth)
grid on
xticks([0 2 4 6 8 10 12 14 16 18 20 22 24 26 28])
yticks([0 20 40 60 80 100])
xlim([22 28])
ylim([0 100])
xlabel('MCS')
ylabel('PRR %')
hold off
lgd = legend('Simplex','128 order FIR','64 order FIR','No Filtering','Location','southwest');
lgd.FontSize = 10;
set(gca,'fontsize',11)

