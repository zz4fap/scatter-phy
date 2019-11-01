clear all;close all;clc

NOF_SLOTS = 2;

NOF_SYMBOLS = 7;

NOF_PRB = 25;

FFT_LEN = 384;

NOF_RE = 12;

resource_grid_size = NOF_SLOTS*NOF_SYMBOLS*NOF_RE*NOF_PRB;

filename = '/home/zz4fap/work/dcf_tdma_turbo_decoder_higher_mcs/scatter/build/received_resource_grid.dat';
fileID = fopen(filename);
resource_grid = complex(zeros(1,resource_grid_size),zeros(1,resource_grid_size));
idx = 1;
value = fread(fileID, 2, 'float');
while ~feof(fileID)
    resource_grid(idx) = complex(value(1),value(2));
    value = fread(fileID, 2, 'float');
    idx = idx + 1;
end
fclose(fileID);

symb_grid_len = NOF_RE*NOF_PRB;
tolal_symbs = NOF_SLOTS*NOF_SYMBOLS;
symbols = zeros(tolal_symbs,symb_grid_len);
for i=1:1:tolal_symbs
    figure;
    symbols(i,:) = resource_grid(1+(i-1)*symb_grid_len:symb_grid_len*i);
    plot(0:1:symb_grid_len-1, abs(resource_grid(1+(i-1)*symb_grid_len:symb_grid_len*i)))
    ylim([0 1.1])
    titleStr = sprintf('Symbol #%d',i-1);
    title(titleStr)
end

symbol0 = symbols(1,:).';
symbol1 = symbols(2,:).';
symbol2 = symbols(3,:).';
symbol3 = symbols(4,:).';
symbol4 = symbols(5,:).';
symbol5 = symbols(6,:).';