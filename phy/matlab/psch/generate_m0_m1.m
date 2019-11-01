clear all;close all;clc

LEN = 31;

sch_table = -1*ones(LEN,LEN);

overwrite_cnt = 0;
for value = 0:1:1024
    
    
    NID1 = (value);
    NID2 = mod((value*3), 3);
    
    q_prime = floor(NID1/30);
    
    q = floor(((NID1+q_prime*(q_prime+1)/2))/30);
    
    m_prime = NID1 + q *(q+1)/2;
    
    m0 = mod(m_prime, LEN);
    
    m1 = mod(m0 + floor(m_prime/LEN)+1,LEN);
    
    %fprintf('value: %d - m0: %d - m1: %d\n', value, m0, m1);
    
    old_value = sch_table(m0+1,m1+1);
    if(old_value > -1)
        fprintf(1,'[Overwriting value] old value: %d - new value: %d - m0: %d - m1: %d\n', old_value, value, m0, m1);
        overwrite_cnt = overwrite_cnt + 1;
    end
    
    sch_table(m0+1,m1+1) = value;
    
end

not_in_table_cnt = 0;
for value = 0:1:1023
    
    [row, col] = find(sch_table == value);
    
    if(length(row) > 1 || length(col) > 1)
        fprintf(1,'[Duplicate value] value: %d - %d - %d\n', value, length(row),  length(col));
    end
    
    if(isempty(row) || isempty(col))
        fprintf(1,'[Empty table] value: %d - %d - %d\n', value, length(row),  length(col));
        not_in_table_cnt = not_in_table_cnt + 1;
    end
    
    
end