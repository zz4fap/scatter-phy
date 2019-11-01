clear all;close all;clc

bw_vector = [6 15 25 50];

slot_vector = [20];

current_tput = zeros(length(bw_vector),length(slot_vector),15);

expected_tput = zeros(length(bw_vector),length(slot_vector),15);

mcs_found = 0:1:31;
for bw_idx=1:1:length(bw_vector)
    
    cnt = 1;
    for mcs_idx = 0:1:31
        
        for nof_slots_idx=1:1:length(slot_vector)
            
            [tb_size] = getTBSize(getIndexFromPrB(bw_vector(bw_idx)),mcs_idx);
            
            full_tput = 2*((tb_size*8)/0.001);
            
            partial_tput = (95.24/100)*full_tput;
            
            expected_tput(bw_idx,nof_slots_idx,cnt) = full_tput;
            
            current_tput(bw_idx,nof_slots_idx,cnt) = partial_tput;
            
            cnt = cnt + 1;
            
        end
        
    end
    
    fprintf(1,'-----------------------------------------\n');
    
end

expected_tput_6prb = reshape(expected_tput(1,1,1:length(mcs_found)),1,32);
current_tput_6prb = reshape(current_tput(1,1,1:length(mcs_found)),1,32);

expected_tput_15prb = reshape(expected_tput(2,1,1:length(mcs_found)),1,32);

expected_tput_25prb = reshape(expected_tput(3,1,1:length(mcs_found)),1,32);

expected_tput_50prb = reshape(expected_tput(4,1,1:length(mcs_found)),1,32);

fdee_figure = figure('rend','painters','pos',[10 10 800 700]);
subplot(4,1,1)
plot(mcs_found,expected_tput_6prb./1e6,'k');
hold on
plot(mcs_found,current_tput_6prb./1e6,'k');
hold off
set(gca,'FontSize',9)
grid on
hxlabel = xlabel('MCS');
set(hxlabel, 'FontSize', 10);
hylabel = ylabel('Throughput [Mbps]');
set(hylabel, 'FontSize', 10);
htitle = title('2 x PHY BW: 1.4 MHz - 20 TBs - 1 ms gap');
set(htitle, 'FontSize', 10);
lgd = legend('Streaming (DC: 100 %)','20 Slots (DC: 95.24 %)','Location','northwest');
lgd.FontSize = 9;
xlim([0 31])
xticks([0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 31])

subplot(4,1,2)
plot(mcs_found,expected_tput_15prb./1e6,'k');
set(gca,'FontSize',9)
grid on
hxlabel = xlabel('MCS');
set(hxlabel, 'FontSize', 10);
hylabel = ylabel('Throughput [Mbps]');
set(hylabel, 'FontSize', 10);
htitle = title('2 x PHY BW: 3 MHz - 20 TBs - 1 ms gap');
set(htitle, 'FontSize', 10);
lgd = legend('Streaming (DC: 100 %)','Location','northwest');
lgd.FontSize = 9;
xlim([0 31])
xticks([0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 31])

subplot(4,1,3)
plot(mcs_found,expected_tput_25prb./1e6,'k');
set(gca,'FontSize',9)
grid on
hxlabel = xlabel('MCS');
set(hxlabel, 'FontSize', 10);
hylabel = ylabel('Throughput [Mbps]');
set(hylabel, 'FontSize', 10);
htitle = title('2 x PHY BW: 5 MHz - 20 TBs - 1 ms gap');
set(htitle, 'FontSize', 10);
lgd = legend('Streaming (DC: 100 %)','Location','northwest');
lgd.FontSize = 9;
xlim([0 31])
xticks([0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 31])

subplot(4,1,4)
plot(mcs_found,expected_tput_50prb./1e6,'k');
set(gca,'FontSize',9)
grid on
hxlabel = xlabel('MCS');
set(hxlabel, 'FontSize', 10);
hylabel = ylabel('Throughput [Mbps]');
set(hylabel, 'FontSize', 10);
htitle = title('2 x PHY BW: 10 MHz - 20 TBs - 1 ms gap');
set(htitle, 'FontSize', 10);
lgd = legend('Streaming (DC: 100 %)','Location','northwest');
lgd.FontSize = 9;
xlim([0 31])
xticks([0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 31])
