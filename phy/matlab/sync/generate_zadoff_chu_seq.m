function pss = generate_zadoff_chu_seq(u,Nzc)

n = 0:1:Nzc-1;

pss = exp((-1i.*pi.*u.*n.*(n+1))./Nzc);

