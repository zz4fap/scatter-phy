function [bw_idx] = getIndexFromPrB(nprb)

switch(nprb)
    case 6
        bw_idx = 1;
    case 15
        bw_idx = 2;       
    case 25
        bw_idx = 3;              
    case 50
        bw_idx = 4;        
    case 75
        bw_idx = 5;        
    case 100
        bw_idx = 6;
    otherwise
        fprintf(1,'Error.....\n')
end