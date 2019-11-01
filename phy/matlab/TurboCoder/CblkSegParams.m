function  [C, Kplus, validK, bByC, b] = CblkSegParams(tbLen)
%#codegen
%% Code block segmentation
blkSize = tbLen + 24;
maxCBlkLen = 6144;
if (blkSize <= maxCBlkLen)
    C = 1;          % number of code blocks
    b = blkSize;    % total bits
else
    L = 24;
    C = ceil(blkSize/(maxCBlkLen-L));
    b = blkSize + C*L; 
end

% Values of K from table 5.1.3-3
validK = [40:8:512 528:16:1024 1056:32:2048 2112:64:6144].';

% First segment size
temp = find(validK >= b/C);
Kplus = validK(temp(1), 1);     % minimum K

bByC = b/C;

% if(b/C ~= Kplus)
%     fprintf('b/C: %f different from Kplus: %d\n', b/C, Kplus);
% end