
load('dark.mat');
darkval=pval;

load('ref.mat');
refarm=pval;
refarm=refarm-darkval;

load('sample.mat');
samplearm=pval-darkval;

lambdas=readlambdadata('ref.xml');
lambdas = lambdas*1e-9;
k = 2*pi./lambdas;
lineark = linspace(min(k),max(k),length(k)*3);


for i=1:30
    %disp('Now reading set number ');
    %disp(i)
    
    filenamestr=strcat('j6', num2str(i+60),'.mat');
    load(filenamestr);
    j0val=pval-darkval;
    
    filenamestr=strcat('pi6', num2str(i+60),'.mat');
    load(filenamestr);
    pishiftedval=pval-darkval;
    
    filenamestr=strcat('i6', num2str(i+60),'.mat');
    load(filenamestr);
    pval=pval-darkval;
    
    
%     filenamestr=strcat('ps_', num2str(109-3*i+1),'.xml');
%     refarm=readpixeldata(filenamestr);
%     refarm=refarm-darkval;
%     
%     filenamestr=strcat('ps_', num2str(109-3*i+2),'.xml');
%     samplearm=readpixeldata(filenamestr);
%     samplearm=samplearm-darkval;
    
    apodi=zeros(size(pval));
    apodiw=apodi;
    apodij=apodi;
    
    apodi(2501:2800)= (pval(2501:2800)-pishiftedval(2501:2800)) ./ (samplearm(2501:2800)+refarm(2501:2800) );
     
    % removing dc
    dcval = mean(apodi(2501:2800));
    apodi(2501:2800)   = apodi(2501:2800) - dcval;
     
    windo = gausswin(300);
    windo = windo';
    apodiw(2501:2800) = apodi(2501:2800) .* windo;
    
	plineark = interp1(k,apodiw,lineark,'linear');
	
	transf=abs(ifft(plineark));
    ascan(i,:)=transf(1:5472);
    %%%%%%%%%%%%%%%%%%
    apodij(2501:2800)= (pval(2501:2800)-j0val(2501:2800)) ./ (j0val(2501:2800) );
    % removing dc
    dcval = mean(apodij(2501:2800));
    apodij(2501:2800)   = apodij(2501:2800) - dcval;
     
    apodij(2501:2800) = apodij(2501:2800) .* windo;
    
	plinearkj = interp1(k,apodij,lineark,'linear');
	
	transf=abs(ifft(plinearkj));
    ascanj(i,:)=transf(1:5472);
end

figure;
imagesc(ascan');
title('pi shifted linear')
colorbar;

figure;
imagesc(ascanj');
title('j0 linear')
colorbar;

figure;
imagesc(log(ascan'));
title('pi shifted log')
colorbar;

figure;
imagesc(log(ascanj'));
title('j0 log')
colorbar;

% min(k) is 9.5071e-004 (1/nm)
% max(k) is 51e-004
% there are 1:3648*3 points
% so linearized deltak is (51-9.5) / 3648*3 = 
% 0.0114e-4 / 3
%
% Then deltax, after fft, is given by
%  (1/deltak).*(1/length(y));
% = 3*(8.77e5)/3648*3 = 240.5 nm

% With a factor of 2. 
% Deltax then should be 120 nm.



  