clear all;close all;clc


[a b] = system('find /home/zz4fap/Downloads/link_level_check/LBT/ -name *.newmat');

files = regexp(b, '[\f\n\r]', 'split');

for i=1:1:length(files)
    
    filename = char(files(i));
    
    newname = sprintf('%s.mat',filename(1:end-7));
    
    cmd = sprintf('mv %s %s',filename,newname);
    [c d] = system(cmd);
    
    a=1;
    
end