clear all;close all;clc

bw_vector = [6 15 25 50];

slot_vector = [1 10 25];

guard_interval = 2; % in number of subframes or milliseconds.

current_tput = zeros(length(bw_vector),length(slot_vector),17);

expected_tput = zeros(length(bw_vector),length(slot_vector),17);

mcs_found = zeros(length(bw_vector),17);
for bw_idx=1:1:length(bw_vector)
    
    folderName = sprintf('./dir_tput_prb_%d/',bw_vector(bw_idx));
    
    cnt = 1;
    for mcs_idx = 0:1:31
        
        for nof_slots_idx=1:1:length(slot_vector)
            
            fileName = sprintf('%stput_prb_%d_mcs_%d_slots_%d.txt',folderName,bw_vector(bw_idx),mcs_idx,slot_vector(nof_slots_idx));
            
            ret = exist(fileName);
            
            if(ret > 0)
                
                tput_values = load(fileName);
                
                if(size(tput_values,2) >= 3)
                    
                    min = tput_values(7,1);
                    max = tput_values(7,2);
                    avg = tput_values(7,3);
                    
                    tput = (sum(tput_values(1:6,3)) - (min+max))/4;
                    
                    [tb_size] = getTBSize(getIndexFromPrB(bw_vector(bw_idx)),mcs_idx);
                    
                    full_tput = 2*((tb_size*8)/0.001);
                    
                    fprintf(1,'%d\t%d\t%d\t%f\t%f\t%f\t%f\n',bw_vector(bw_idx),mcs_idx,slot_vector(nof_slots_idx),full_tput,tput,avg,max);
                    
                    current_tput(bw_idx,nof_slots_idx,cnt) = max;
                    
                    expected_tput(bw_idx,nof_slots_idx,cnt) = full_tput;
                    
                    if(mcs_idx == 22 && bw_vector(bw_idx) == 50)
                        current_tput(bw_idx,nof_slots_idx,cnt) = max;
                    end
                    
                    if(mcs_idx == 28 && bw_vector(bw_idx) == 50) 
                        current_tput(bw_idx,nof_slots_idx,cnt) = max;
                    end
                    
                end
                
            end
            
        end
        
        if((ret > 0) & (size(tput_values,2) >= 3))
            mcs_found(bw_idx,cnt) = mcs_idx;
            cnt = cnt + 1;
        end
        
    end
    
    fprintf(1,'-----------------------------------------\n');
    
end

expected_tput_6prb = zeros(length(slot_vector),17);
current_tput_6prb = zeros(length(slot_vector),17);
expected_tput_15prb = zeros(length(slot_vector),17);
current_tput_15prb = zeros(length(slot_vector),17);
expected_tput_25prb = zeros(length(slot_vector),17);
current_tput_25prb = zeros(length(slot_vector),17);
expected_tput_50prb = zeros(length(slot_vector),17);
current_tput_50prb = zeros(length(slot_vector),17);
dc_vector = zeros(1,length(slot_vector));
for slot_num_idx = 1:1:length(slot_vector)
    
    dc_vector(slot_num_idx) = slot_vector(slot_num_idx)/( slot_vector(slot_num_idx) + guard_interval );

    expected_tput_6prb(slot_num_idx,:) = reshape(expected_tput(1,slot_num_idx,1:length(mcs_found)),1,17);
    current_tput_6prb(slot_num_idx,:) = reshape(current_tput(1,slot_num_idx,1:length(mcs_found)),1,17);

    expected_tput_15prb(slot_num_idx,:) = reshape(expected_tput(2,slot_num_idx,1:length(mcs_found)),1,17);
    current_tput_15prb(slot_num_idx,:) = reshape(current_tput(2,slot_num_idx,1:length(mcs_found)),1,17);

    expected_tput_25prb(slot_num_idx,:) = reshape(expected_tput(3,slot_num_idx,1:length(mcs_found)),1,17);
    current_tput_25prb(slot_num_idx,:) = reshape(current_tput(3,slot_num_idx,1:length(mcs_found)),1,17);

    expected_tput_50prb(slot_num_idx,:) = reshape(expected_tput(4,slot_num_idx,1:length(mcs_found)),1,17);
    current_tput_50prb(slot_num_idx,:) = reshape(current_tput(4,slot_num_idx,1:length(mcs_found)),1,17);

end

fdee_figure = figure('rend','painters','pos',[10 10 800 700]);
subplot(4,1,1)
plot(mcs_found(1,:),expected_tput_6prb(1,:)./1e6,'k');
hold on
plot(mcs_found(1,:),current_tput_6prb(1,1:length(mcs_found))./1e6,'k*--');
plot(mcs_found(1,:),current_tput_6prb(2,1:length(mcs_found))./1e6,'ko--');
plot(mcs_found(1,:),current_tput_6prb(3,1:length(mcs_found))./1e6,'ks--');
set(gca,'FontSize',9)
grid on
hold off
hxlabel = xlabel('MCS');
set(hxlabel, 'FontSize', 10);
hylabel = ylabel('Throughput [Mbps]');
set(hylabel, 'FontSize', 10);
htitle = title('2 x PHY BW: 1.4 MHz - Guard interval: 2 ms');
set(htitle, 'FontSize', 10);
legStr1 = sprintf('%d Slot (DC: %1.2f %%)', slot_vector(1), dc_vector(1)*100);
legStr2 = sprintf('%d Slot (DC: %1.2f %%)', slot_vector(2), dc_vector(2)*100);
legStr3 = sprintf('%d Slot (DC: %1.2f %%)', slot_vector(3), dc_vector(3)*100);
lgd = legend('Streaming (DC: 100 %)', legStr1, legStr2, legStr3,'Location','northwest');
lgd.FontSize = 9;
xlim([0 31])
xticks([0 2 4 6 8 10 12 14 16 18 20 22 24 25 26 28 30 31])

subplot(4,1,2)
mcs_vector = mcs_found(2,1:end);
plot(mcs_vector,expected_tput_15prb(1,:)./1e6,'k');
hold on
plot(mcs_vector,current_tput_15prb(1,1:length(mcs_found))./1e6,'k*--');
plot(mcs_vector,current_tput_15prb(2,1:length(mcs_found))./1e6,'ko--');
plot(mcs_vector,current_tput_15prb(3,1:length(mcs_found))./1e6,'ks--');
set(gca,'FontSize',9)
grid on
hold off
hxlabel = xlabel('MCS');
set(hxlabel, 'FontSize', 10);
hylabel = ylabel('Throughput [Mbps]');
set(hylabel, 'FontSize', 10);
htitle = title('2 x PHY BW: 3 MHz - Guard interval: 2 ms');
set(htitle, 'FontSize', 10);
legStr1 = sprintf('%d Slot (DC: %1.2f %%)', slot_vector(1), dc_vector(1)*100);
legStr2 = sprintf('%d Slot (DC: %1.2f %%)', slot_vector(2), dc_vector(2)*100);
legStr3 = sprintf('%d Slot (DC: %1.2f %%)', slot_vector(3), dc_vector(3)*100);
lgd = legend('Streaming (DC: 100 %)', legStr1, legStr2, legStr3,'Location','northwest');
lgd.FontSize = 9;
xlim([0 31])
xticks([0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 31])

subplot(4,1,3)
mcs_vector = mcs_found(3,1:end);
plot(mcs_vector,expected_tput_25prb(1,:)./1e6,'k');
hold on;
plot(mcs_vector,current_tput_25prb(1,1:length(mcs_found))./1e6,'k*--');
plot(mcs_vector,current_tput_25prb(2,1:length(mcs_found))./1e6,'ko--');
plot(mcs_vector,current_tput_25prb(3,1:length(mcs_found))./1e6,'ks--');
set(gca,'FontSize',9)
grid on
hold off
hxlabel = xlabel('MCS');
set(hxlabel, 'FontSize', 10);
hylabel = ylabel('Throughput [Mbps]');
set(hylabel, 'FontSize', 10);
htitle = title('2 x PHY BW: 5 MHz - Guard interval: 2 ms');
set(htitle, 'FontSize', 10);
legStr1 = sprintf('%d Slot (DC: %1.2f %%)', slot_vector(1), dc_vector(1)*100);
legStr2 = sprintf('%d Slot (DC: %1.2f %%)', slot_vector(2), dc_vector(2)*100);
legStr3 = sprintf('%d Slot (DC: %1.2f %%)', slot_vector(3), dc_vector(3)*100);
lgd = legend('Streaming (DC: 100 %)', legStr1, legStr2, legStr3,'Location','northwest');
lgd.FontSize = 9;
xlim([0 31])
xticks([0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 31])

subplot(4,1,4)
mcs_vector = mcs_found(4,1:end);
plot(mcs_vector,expected_tput_50prb(1,:)./1e6,'k');
hold on;
plot(mcs_vector,current_tput_50prb(1,1:length(mcs_found))./1e6,'k*--');
plot(mcs_vector,current_tput_50prb(2,1:length(mcs_found))./1e6,'ko--');
plot(mcs_vector,current_tput_50prb(3,1:length(mcs_found))./1e6,'ks--');
set(gca,'FontSize',9)
grid on
hold off
hxlabel = xlabel('MCS');
set(hxlabel, 'FontSize', 10);
hylabel = ylabel('Throughput [Mbps]');
set(hylabel, 'FontSize', 10);
htitle = title('2 x PHY BW: 10 MHz - Guard interval: 2 ms');
set(htitle, 'FontSize', 10);
legStr1 = sprintf('%d Slot (DC: %1.2f %%)', slot_vector(1), dc_vector(1)*100);
legStr2 = sprintf('%d Slot (DC: %1.2f %%)', slot_vector(2), dc_vector(2)*100);
legStr3 = sprintf('%d Slot (DC: %1.2f %%)', slot_vector(3), dc_vector(3)*100);
lgd = legend('Streaming (DC: 100 %)', legStr1, legStr2, legStr3,'Location','northwest');
lgd.FontSize = 9;
xlim([0 31])
xticks([0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 31])
