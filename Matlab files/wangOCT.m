% from Biomedical Optics by Wang and Wu
% Chapter 9

% Use SI units throughout 
lambda0 = 830E-9; % center wavelength of source 
dlambda = 20E-9; % FWHM wavelength bandwidth of source 
ns=1.38; % refractive index of sample 
ls1 = 90E-6; % location of backscatterer 1 
ls2 = 150E-6; % location of backscatterer 2 
rs1 = 0.001; % reflectivity of backscatterer 1 - was 0.5
rs2 = 0.0; %rs2 = 0.0; % reflectivity of backscatterer 2 was 0.25
k0=2*pi/lambda0; % center propagation constant 
delta_k=2*pi*dlambda/lambda0^2; % FWHM bandwidth of k 
sigma_k = delta_k/sqrt(2*log(2)); % standard deviation of k 

N=2^10; %N=100; % number of sampling points was 2^10 
nsigma = 5; % number of standard deviations to plot on each side of kO 

subplot(4,1,1); % Generate the interferogram 
k = k0 + sigma_k*linspace(-nsigma,nsigma, N); % array for k 
S_k = exp(-(1/2)*(k-k0).^2/sigma_k^2); % Gaussian source PSD 
E_s1 = rs1*exp(i*2*k*ns*ls1); % sample electric field from scatter 1 
E_s2 = rs2*exp(i*2*k*ns*ls2); % sample electric field from scatter 2 
I_k1 = S_k .* abs(1 + E_s1 + E_s2).^2; % interferogram (r_R = 1) 
plot(k/k0,I_k1/max(I_k1), 'k'); 
title('Interferogram'); 
xlabel('Propagation constant k/k_0'); 
ylabel('Normalized intensity'); 
axis([0.9 1.1 0 1]); 

subplot(4,1,2); % Inverse Fourier transform (IFT) of the interferogram 
spec1 =abs(fftshift(ifft(I_k1)))/sqrt(N); 
dls_prime = 1/(2*nsigma*sigma_k/(2*pi)); % bin = 1/sampling range 
ls_prime = dls_prime*(-N/2:N/2-1); % frequency array 
plot(ls_prime/(2*ns),spec1/max(spec1), 'k'); % scale the frequency 
title('IFT of the interferogram'); 
xlabel('Depth Is (m)'); 
ylabel('Relative reflectivity'); 
axis([-2*ls2 2*ls2 0 1]); 

subplot(4,1,3); % IFT of the deconvolved interferogram 
spec1_norm =abs(fftshift(ifft(I_k1./S_k) ) )/sqrt(N); 
dls_prime = 1/(2*nsigma*sigma_k/(2*pi)); % bin size = 1/sampling range 
ls_prime = dls_prime*(-N/2:N/2-1); % frequency array 
plot(ls_prime/(2*ns),spec1_norm/max(spec1_norm), 'k'); 
title('IFT of the deconvolved interferogram'); 
xlabel('Depth Is (m)'); 
ylabel('Relative reflectivity'); 
axis([-2*ls2 2*ls2 0 1]); 

subplot(4,1,4); % IFT of the deconvolved differential interferogram 
I_k2 = S_k .* abs(-1 + E_s1 + E_s2).^2; % interferogram 
delta_I_k = I_k1 - I_k2; 
spec2=abs(fftshift(ifft(delta_I_k./S_k)))/sqrt(N); 
plot(ls_prime/(2*ns),spec2/max(spec2), 'k'); 
title('IFT of the deconvolved differential interferogram'); 
xlabel('Depth ls (m)') ; 
ylabel('Relative reflectivity') ; 
axis([-2*ls2 2*ls2 0 1]) ; 
