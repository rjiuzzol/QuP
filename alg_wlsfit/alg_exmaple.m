%
% Example
%

load ws_lsfit_0308.mat

n=1; %polinomial order 

% for WLS
[pw,sw] = wlsfit(Ref, C, uC,n);
disp('Results for WLS fitting');


%% Display results
% Results is 
disp(['Polinimial order      : ' num2str(n) ])
disp(['offset          : ' num2str(pw(2),6) ' +- ' num2str(sw.unc(2),4)])
disp(['linear coeff.   : ' num2str(pw(1),6) ' +- ' num2str(sw.unc(1),4)])
disp('Covariance Matrix:')
sw.p_cova
disp(['Freedom degree   : ' num2str(sw.df)])
disp(['Goodnes          : ' num2str(sw.chi2,5)])

pause;

# for OLS
[p,s]= wlsfit(Ref, C, n);

disp('Results for OLS fitting');



%% Display results
% Results is 
disp(['Polinimial order      : ' num2str(n) ])
disp(['offset          : ' num2str(p(2),6) ' +- ' num2str(s.unc(2),4)])
disp(['linear coeff.   : ' num2str(p(1),6) ' +- ' num2str(s.unc(1),4)])
disp('Covariance Matrix:')
sw.p_cova
disp(['Freedom degree   : ' num2str(s.df)])
disp(['Goodnes          : ' num2str(s.chi2,5)])
