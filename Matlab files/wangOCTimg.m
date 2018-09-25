% create a test Interferogram image 
% using the technique in 
% Biomedical Optics by Wang and Wu
% Chapter 9
%
% Hari Nandakumar
%
% $Date: 25 Sep 2018
%

% Using SI units throughout 
lambda0 = 850E-9; % center wavelength of source 
dlambda = 20E-9; % FWHM wavelength bandwidth of source 
ns=1.38; % refractive index of sample 
ls1 = 90E-6; % location of backscatterer 1 
ls2 = 150E-6; % location of backscatterer 2 
rs1 = 0.5; % reflectivity of backscatterer 1 - was 0.5
rs2 = 0.25; %rs2 = 0.0; % reflectivity of backscatterer 2 was 0.25
k0=2*pi/lambda0; % center propagation constant 
delta_k=2*pi*dlambda/lambda0^2; % FWHM bandwidth of k 
sigma_k = delta_k/sqrt(2*log(2)); % standard deviation of k 
sigma_lambda = dlambda/sqrt(2*log(2)); % standard deviation of lambda

N=128;  %2^10; %N=100; % number of sampling points was 2^10 
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


imgi = zeros(96,128);
backg = imgi;

for ii=1:96
  ls1 = ii*1E-6; % location of backscatterer 1 
  ls2 = (ii+50)*1E-6; % location of backscatterer 2
  E_s1 = rs1*exp(i*2*2*pi*ns*ls1./lambdas); % sample electric field from scatter 1 
  E_s2 = rs2*exp(i*2*2*pi*ns*ls2./lambdas); % sample electric field from scatter 2 
  I_l = S_lam .* abs(1 + E_s1 + E_s2).^2; % interferogram (r_R = 1) 
  imgi(ii,:)  = I_l/max(I_l);
  backg(ii,:) = S_lam/max(S_lam);
end
  
imshow(imgi); 
figure;
imshow(backg); 

imwrite(imgi,'imgi.png')
imwrite(backg, 'backg.png') 

%plot(k/k0,I_k1/max(I_k1), 'k'); 
