% reconstruct FDOCT result from a test Interferogram image 
% to check the reconstruction done in C
% in BscanFFTsim.bin
%
% using the framework in 
% Biomedical Optics by Wang and Wu
% Chapter 9
%
% Hari Nandakumar
%
% $Date: 1 Oct 2018
%

% Using SI units throughout 

pkg load image;
pkg load signal;

lambda0 = 850E-9; % center wavelength of source 
dlambda = 20E-9; % FWHM wavelength bandwidth of source 
ns=1.0; % refractive index of sample 
ls1 = 90E-6; % location of backscatterer 1 
ls2 = 150E-6; % location of backscatterer 2 
rs1 = 0.5; % reflectivity of backscatterer 1 - was 0.5
rs2 = 0.005; %rs2 = 0.0; % reflectivity of backscatterer 2 was 0.25
k0=2*pi/lambda0; % center propagation constant 
delta_k=2*pi*dlambda/lambda0^2; % FWHM bandwidth of k 
sigma_k = delta_k/sqrt(2*log(2)); % standard deviation of k 
sigma_lambda = dlambda/sqrt(2*log(2)); % standard deviation of lambda

N=1280;  %2^10; %N=100; % number of sampling points was 2^10 
nsigma = 2; % number of standard deviations to plot on each side of kO 

%subplot(4,1,1); % Generate the interferogram 
k = k0 + sigma_k*linspace(-nsigma,nsigma, N); % array for k 
lambdas = lambda0 + sigma_lambda*linspace(-nsigma,nsigma, N); % array for lambda 
%S_k = exp(-(1/2)*(k-k0).^2/sigma_k^2); % Gaussian source PSD 
S_lam = exp(-(1/2)*(lambdas-lambda0).^2/sigma_lambda^2); % Gaussian source PSD 
%E_s1 = rs1*exp(i*2*k*ns*ls1); % sample electric field from scatter 1 
%E_s2 = rs2*exp(i*2*k*ns*ls2); % sample electric field from scatter 2 
%I_k1 = S_k .* abs(1 + E_s1 + E_s2).^2; % interferogram (r_R = 1) 
%plot(lambdas*1e9,S_lam)


imgi = zeros(960,1280);
backg = imgi;

for ii=1:60
  ls1 = ii*10*1E-6; % location of backscatterer 1 
  ls2 = (ii*10+50)*1E-6; % location of backscatterer 2
  E_s1 = rs1*exp(i*2*2*pi*ns*ls1./lambdas); % sample electric field from scatter 1 
  E_s2 = rs2*exp(i*2*2*pi*ns*ls2./lambdas); % sample electric field from scatter 2 
  I_l = S_lam .* abs(1 + E_s1 + E_s2).^2; % interferogram (r_R = 1) 
  I_pi = S_lam .* abs(-1 + E_s1 + E_s2).^2;
  imgi(ii*10-9:ii*10,:)  = ones(10,1)*I_l./max(I_l);
  piimgi(ii*10-9:ii*10,:)  = ones(10,1)*I_pi./max(I_pi);
  backg(ii*10-9:ii*10,:) = ones(10,1)*S_lam./max(S_lam);
end

for ii=61:96
  ls1 = ii*1E-6; % location of backscatterer 1 
  ls2 = 60*1E-6; % location of backscatterer 2
  E_s1 = rs1*exp(i*2*2*pi*ns*ls1./lambdas); % sample electric field from scatter 1 
  E_s2 = rs2*exp(i*2*2*pi*ns*ls2./lambdas); % sample electric field from scatter 2 
  I_l = S_lam .* abs(1 + E_s1 + E_s2).^2; % interferogram (r_R = 1) 
  I_pi = S_lam .* abs(-1 + E_s1 + E_s2).^2;
  imgi(ii*10-9:ii*10,:)  = ones(10,1)*I_l./max(I_l);
  piimgi(ii*10-9:ii*10,:)  = ones(10,1)*I_pi./max(I_pi);
  backg(ii*10-9:ii*10,:) = ones(10,1)*S_lam./max(S_lam);
end

 
%figure; 
%imshow(imgi); 
%figure;
%imshow(backg); 

%imwrite(imgi,'imgi.png')
%imwrite(backg, 'backg.png') 


%plot(k/k0,I_k1/max(I_k1), 'k'); 

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% reconstruction
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% check out 8 bit imgi
imgi = floor(imgi*255);
piimgi = floor(piimgi*255);
backg = floor(backg*255);

lambdamin = 816e-9;
lambdamax = 884e-9;
deltalambda = (lambdamax - lambdamin ) / columns(imgi);
numfftpoints = 320;
kmax=2*pi/lambdamin;
kmin=2*pi/(lambdamax-deltalambda);
deltak = (kmax - kmin) / numfftpoints;
resizedim=imresize(imgi,0.25); % similar to 4x binning
resizedbk=imresize(backg,0.25); 
resizedpi=imresize(piimgi,0.25);
plinear = zeros(rows(resizedim),numfftpoints);
bscantransposed = plinear;

lambdas = linspace(lambdamin, lambdamax-deltalambda, columns(resizedim));  % Mat::zeros(cv::Size(1, data_y.cols), CV_64F);		//Size(cols,rows)
k = 2*pi ./ lambdas;
klinear = linspace(kmin, kmax, numfftpoints); %Mat::zeros(cv::Size(1, numfftpoints), CV_64F);
diffk = diff(k); %Mat::zeros(cv::Size(1, data_y.cols), CV_64F);

%fractionalk = zeros(1, numfftpoints);
%slopes = zeros(rows(imgi), columns(imgi));
%nearestkindex = zeros( 1, numfftpoints );


windowt = (tukeywin(columns(resizedim), 1.0) )';
windowg = (gausswin(columns(resizedim)))';
windowb = (blackmanharris(columns(resizedim)))';
windowbth = (barthannwin(columns(resizedim)))';
 
apodi   = (resizedim-resizedpi)./resizedbk;


for indexi=1:rows(resizedim) 
  % DC removal
  apodi(indexi,:) = apodi(indexi,:) - mean(apodi(indexi,:));
  % windowing
  apodig(indexi,:)=apodi(indexi,:).*windowg;
  apodit(indexi,:)=apodi(indexi,:).*windowt;
  apodib(indexi,:)=apodi(indexi,:).*windowb;
  apodibth(indexi,:)=apodi(indexi,:).*windowbth;
 end
 

for indexi=1:rows(resizedim) 
  plinear(indexi,:)=interp1(k,apodi(indexi,:),klinear,'linear');
  plinearg(indexi,:)=interp1(k,apodig(indexi,:),klinear,'linear');
  plineart(indexi,:)=interp1(k,apodit(indexi,:),klinear,'linear');
  plinearb(indexi,:)=interp1(k,apodib(indexi,:),klinear,'linear');
  plinearbth(indexi,:)=interp1(k,apodibth(indexi,:),klinear,'linear');
 
 end
  
% figure;
% imagesc(plinear); title('Linearized'); colorbar;
% figure;
% imagesc(plinearg); title('Linearized with Gaussian Window'); colorbar;
% figure;
% imagesc(plineart); title('Linearized with Tukey Window'); colorbar;
% figure;
% imagesc(plinearb); title('Linearized with BH Window'); colorbar;
% 
 for indexi=1:rows(resizedim) 
  bscantransposed(indexi,:)=abs(ifft(plinear(indexi,:)));
  bscantransposedg(indexi,:)=abs(ifft(plinearg(indexi,:)));
  bscantransposedt(indexi,:)=abs(ifft(plineart(indexi,:)));
  bscantransposedb(indexi,:)=abs(ifft(plinearb(indexi,:)));
  bscantransposedbth(indexi,:)=abs(ifft(plinearbth(indexi,:)));
 
 end
 
 %figure;
 %imagesc(bscantransposed); title('bscantransposed');
 bscan=bscantransposed';
 bscang=bscantransposedg';
 bscant=bscantransposedt';
 bscanb=bscantransposedb';
 bscanbth=bscantransposedbth';
% bscan(1,:)=zeros(1,rows(resizedim));
% bscant(1,:)=zeros(1,rows(resizedim));
% bscang(1,:)=zeros(1,rows(resizedim));
 figure;
 imagesc(10*log10(bscan(1:160,:)./max(bscan(1:160,:)))); title('Rect Window');colorbar;
 figure;
 imagesc(10*log10(bscang(1:160,:)./max(bscang(1:160,:)))); title('Gaussian Window');colorbar;
 figure;
 imagesc(10*log10(bscant(1:160,:)./max(bscant(1:160,:)))); title('Tukey Window');colorbar;
 figure;
 imagesc(10*log10(bscanb(1:160,:)./max(bscanb(1:160,:)))); title('BH Window');colorbar;
 figure;
 imagesc(10*log10(bscanbth(1:160,:)./max(bscanbth(1:160,:)))); title('BTH Window');colorbar;
 
 figure;
 plot(bscang(1:160,230)); title('Linear plot, Gaussian window');
  
  deltaklinear = kmax-kmin;
 deltax=pi/deltaklinear;
 xaxis=[0:159]*deltax*1000000;
 figure;
 plot(xaxis,10*log10(bscang(1:160,230)./max(bscang(1:160,230))),'b');title('Log plot');hold on;
 plot(xaxis,10*log10(bscan(1:160,230)./max(bscan(1:160,230))),'r'); xlabel('microns')
 plot(xaxis,10*log10(bscant(1:160,230)./max(bscant(1:160,230))),'k');
 
 