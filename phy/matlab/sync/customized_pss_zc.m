function[seq] = customized_pss_zc(u)
% Function returns a Zadoff-Chu sequence with any root index.

Nzc = 62;

seq = zeros(Nzc,1);
for n = 0:1:30
    seq(n+1) = exp(complex(0,-1)*pi*u*n*(n+1)/63);
end    
for n = 31:1:61
    seq(n+1) = exp(complex(0,-1)*pi*u*(n+1)*(n+2)/63);
end    

end
