function[a] = scatter_pss_zcv1(cell_id, SCATTER_PSS_LEN)
% Function returns 1 out of 3 possible Zadoff-Chu sequences used in Scatter system.
if(SCATTER_PSS_LEN == 62)
    Nzc = 63;
elseif(SCATTER_PSS_LEN == 64)
    Nzc = 65;
elseif(SCATTER_PSS_LEN == 66)
    Nzc = 67;
elseif(SCATTER_PSS_LEN == 68)
    Nzc = 69;
elseif(SCATTER_PSS_LEN == 70)
    Nzc = 71;
elseif(SCATTER_PSS_LEN == 72)
    Nzc = 73;
end

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

a = zeros(SCATTER_PSS_LEN, 1);
for n = 0:1:(SCATTER_PSS_LEN/2) - 1
    a(n+1) = exp(complex(0,-1)*pi*u*n*(n+1)/Nzc);
end    
for n = (SCATTER_PSS_LEN/2):1:SCATTER_PSS_LEN-1
    a(n+1) = exp(complex(0,-1)*pi*u*(n+1)*(n+2)/Nzc);
end    

end
