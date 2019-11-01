function [data, position]=read_complex(filename,size,precision,offset)
fid=fopen(filename);
fseek(fid,offset,'bof');
position = ftell(fid);
data=fread(fid,size*2,precision);
data=data(1:2:end-1)+1i.*data(2:2:end);
fclose(fid);