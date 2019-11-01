clear all;close all;clc

LEN = 31;

sch_table = -1*ones(LEN,LEN);

overwrite_cnt = 0;
m0_ = 0;
m1_ = 0;
cnt = 1;
for srn_id = 0:1:255
    
    for interface_id = 0:1:1
        
        value = srn_id*2 + interface_id;
                
        m0 = floor(value/LEN);
        m1 = mod(value,LEN);
        
        
        rx_srn_id = floor(value/2);
        rx_interface_id = mod(value,2);
        
        
        fprintf('cnt: %d - value: %d - SRN ID: %d - Interface ID: %d - m0: %d - m1: %d - Rx SNR ID: %d - Rx Interface ID: %d\n', cnt, value, srn_id, interface_id, m0, m1,rx_srn_id, rx_interface_id);
        
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