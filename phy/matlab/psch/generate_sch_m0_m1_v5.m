clear all;close all;clc

LEN = 31;

MAX_FIRST_VALUE = 31;

MAX_SECOND_VALUE = 25;

sch_table = -1*ones(LEN,LEN);

overwrite_cnt = 0;
m0_ = 0;
m1_ = 0;
cnt = 1;
for mcs = 0:1:MAX_FIRST_VALUE
    
    for nof_slots = 0:1:MAX_SECOND_VALUE
        
        value = mcs*(MAX_SECOND_VALUE+1) + nof_slots;
                
        m0 = floor(value/LEN);
        m1 = mod(value,LEN);
        
        
        rx_mcs = floor(value/(MAX_SECOND_VALUE+1));
        rx_nof_slots = mod(value,(MAX_SECOND_VALUE+1));
        
        
        fprintf('cnt: %d - value: %d - MCS: %d - # slots: %d - m0: %d - m1: %d - Rx MCS: %d - Rx # slots: %d\n', cnt, value, mcs, nof_slots, m0, m1,rx_mcs, rx_nof_slots);
        
        
        if(mcs ~= rx_mcs)
            fprintf(1,'Error in mcs decoding................\n');
        end
        
        if(nof_slots ~= rx_nof_slots)
            fprintf(1,'Error in nof_slots decoding................\n');
        end
        
        old_value = sch_table(m0+1,m1+1);
        if(old_value > -1)
            overwrite_cnt = overwrite_cnt + 1;
            %fprintf(1,'[Overwriting value (%d)] old value: %d - new value: %d - m0: %d - m1: %d\n', overwrite_cnt, old_value, value, m0, m1);
        end
        
        % Add value to the table.
        sch_table(m0+1,m1+1) = value;
        
        if(cnt == LEN)
            m0_ = m0_ + 1;
            m1_ = m0_;
            cnt = 0;
        end
        
        cnt = cnt + 1;
        
    end
    
end

% not_in_table_cnt = 0;
% for value = 0:1:1023
%     
%     [row, col] = find(sch_table == value);
%     
%     if(length(row) > 1 || length(col) > 1)
%         fprintf(1,'[Duplicate value] value: %d - %d - %d\n', value, length(row),  length(col));
%     end
%     
%     if(isempty(row) || isempty(col))
%         fprintf(1,'[Empty table] value: %d - %d - %d\n', value, length(row),  length(col));
%         not_in_table_cnt = not_in_table_cnt + 1;
%     end
%     
%     
% end