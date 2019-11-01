function[a] = scatter_pss_zc(cell_id, SCATTER_PSS_LEN)
% Function returns 1 out of 3 possible Zadoff-Chu sequences used in Scatter system.

if(SCATTER_PSS_LEN <= 67)
    Nzc = 67;
elseif(SCATTER_PSS_LEN <= 71)
    Nzc = 71;
else
    Nzc = 73;
end

seq_length = SCATTER_PSS_LEN;

u = 0;
if cell_id == 0
    u = 59;
end
if cell_id == 1
    u = 47;
end
if cell_id == 2
    u = 23;
end

a = zeros(seq_length, 1);
for n = 0:1:(seq_length/2) - 1
    a(n+1) = exp(complex(0,-1)*pi*u*n*(n+1)/Nzc);
end    
for n = (seq_length/2):1:seq_length-1
    a(n+1) = exp(complex(0,-1)*pi*u*(n+1)*(n+2)/Nzc);
end    

end
