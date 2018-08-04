clear
clc
files = dir('*.mat');
tic
f = {files.name};
sf = natsortfiles(f);
for i=1:length(sf)
    m = matfile(char(sf(i)),'Writable',true);
    receiverX = m.receiverX;
    receiverY = m.receiverY;
    
    receiverX( ~any(receiverX,2), : ) = [];  %rows
    receiverX( :, ~any(receiverX,1) ) = [];  %columns
    
    receiverY( ~any(receiverY,2), : ) = [];  %rows
    receiverY( :, ~any(receiverY,1) ) = [];  %columns
    
    m.receiverX = receiverX;
    m.receiverY = receiverY;
end

toc
clear
clc