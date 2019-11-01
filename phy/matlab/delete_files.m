clear all;close all;clc

filename = '*.png';

cmdstr = sprintf('find /home/zz4fap/Downloads/link_level_check/LBT/ -name %s',filename);

[a b] = system(cmdstr);

files = regexp(b, '[\f\n\r]', 'split');

for i=1:1:length(files)
    
    filename = char(files(i));
    
    ret1 = strfind(filename,'.mat');
    if(~isempty(ret1))
        cmd = sprintf('rm %s',filename);
        [c d] = system(cmd);
    end
end