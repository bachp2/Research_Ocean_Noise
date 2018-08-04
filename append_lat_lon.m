clear
clc
files = dir('*.mat');
tic
f = {files.name};
sf = natsortfiles(f);
tlat = [];
tlon = [];
l = 0;
for i=1:length(sf)
    m = matfile(char(sf(i)),'Writable',true);
    tlat = [tlat m.lat];
    tlon = [tlon m.lon];
    l = l + length(m.lon);
end
save airguns_lat_lon tlat tlon;
toc
clear
clc