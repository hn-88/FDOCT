function outv=readlambdadata(filename)
% readlambdadata(filename) - reads the xml file created by
% Ocean Optics Spectra Suite as part of its
% ProcSpec file. (unzip ProcSpec file to get the xml file)
% returns lambdadata as a vector.
%    $Date: September 17, 2016 $

M=textread(filename, '%s', 'delimiter', '\n', 'whitespace', '');

% There are 3648 wavelength data elements
% The first one starts on line 3690
outputv=[1:3648];

for index=1:3648
    textstring=sscanf(M{index+3689}, '%s');
    token=strtok(textstring, '<double>');
    outputv(index)=str2double(token);
end

outv = outputv;