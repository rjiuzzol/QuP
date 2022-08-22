function dataout = alg_wrapper(datain, calcset) %<<<1
% Part of QWTB. Wrapper script for algorithm wlsfit.
%
% See also qwtb

% Format input data --------------------------- %<<<1
##
## Inputs:
##   x: Reference values (x-values)
##   y: Observed or measured values (y-values)
##   w: weigths or y-uncertainties (all different -> heteroscedasticity)
##      if not specified or is all are equal (homoscedasticity), then ordinary least square is used.
##   n: degree of the polinomial for the regression (n>0)
##
##
## Outputs:
##   p: coefficients
##   s: struct
##     .y_cova: fitted y values covariance matrix
##     .unc: uncertainties on the coefficients p
##     .p_cova: coefficients covariance matrix
##     .y_hat: fitted y-values
##     .res: Residuals on the adjustment
##     .df:  Degree of freddom
##     .chi2: goodness of the fitting (Chi-squared value)

% function [p, s] = wlsfit (x, y, w, n)
  
x = datain.x.v;
y = datain.y.v;

WLS = [];

if isfield(datain, 'w')
  w = datain.w.v;
  WLS = 1;
  if calcset.verbose
    disp('QWTB: wlsfit wrapper: Using WLS fitting')
  end
else
  WLS = 0;
  if calcset.verbose
    disp('QWTB: wlsfit wrapper: Using OLS fitting')
  end
endif  

if isfield(datain, 'n')
  n = datain.n.v;
endif

% Call algorithm ---------------------------  %<<<1
if (WLS)
  [p, s] = wlsfit (x, y, w, n);
else
  [p, s] = wlsfit (x, y, n);
endif


% Format output data:  --------------------------- %<<<1

dataout.p.v = p;
dataout.s = s;

end % function

% vim settings modeline: vim: foldmarker=%<<<,%>>> fdm=marker fen ft=octave textwidth=80 tabstop=4 shiftwidth=4
