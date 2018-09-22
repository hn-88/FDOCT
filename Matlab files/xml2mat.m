% clears all variables,
% reads spectrometer xml files
% and saves pixeldata as mat files of same filename.

%
%    $ Hari Nandakumar
%
%    $Date: 8 December 2017

clear all;

listing=dir('*.xml');
for ii = 1:length(listing)
    if (~(strcmp(listing(ii).name, 'OOISignatures.xml')) ) % we don't want to attempt to read pixel data from this
        pval=readpixeldata(listing(ii).name);        
        save(listing(ii).name(1:end-4),'pval')
    end
end

        
