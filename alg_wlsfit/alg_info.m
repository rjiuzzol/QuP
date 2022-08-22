function alginfo = alg_info() %<<<1
% Part of QWTB. Info script for algorithm 'wlsfit'.
%
% See also qwtb

alginfo.id = 'wlsfit';
alginfo.name = 'Weighted Least Square Fitting Algortihm';
alginfo.desc = 'Least Square fitting algortihm using ordinary or weighted fitting on a n-positive polinomial order.';
alginfo.citation = '';
alginfo.remarks = '';
alginfo.license = 'MIT License';

alginfo.inputs(1).name = 'x';
alginfo.inputs(1).desc = 'Reference values (x-values)';
alginfo.inputs(1).alternative = 1;
alginfo.inputs(1).optional = 0;
alginfo.inputs(1).parameter = 0;

alginfo.inputs(2).name = 'y';
alginfo.inputs(2).desc = 'Observed or measured values (y-values)';
alginfo.inputs(2).alternative = 1;
alginfo.inputs(2).optional = 0;
alginfo.inputs(2).parameter = 0;

alginfo.inputs(3).name = 'w';
alginfo.inputs(3).desc = ' weigths or y-uncertainties (all different -> heteroscedasticity). If not specified or is all are equal (homoscedasticity), then ordinary least square is used.';
alginfo.inputs(3).alternative = 1;
alginfo.inputs(3).optional = 1;
alginfo.inputs(3).parameter = 0;

alginfo.inputs(4).name = 'n';
alginfo.inputs(4).desc = 'degree of the polinomial for the regression (n>0)';
alginfo.inputs(4).alternative = 0;
alginfo.inputs(4).optional = 0;
alginfo.inputs(4).parameter = 0;


## Outputs:
##   p: coefficients
##   s: struct
##     .y_cova: 
##     .unc: uncertainties on the coefficients p
##     .p_cova: coefficients covariance matrix
##     .y_hat: fitted y-values
##     .res: Residuals on the adjustment
##     .df:  Degree of freddom
##     .chi2: goodness of the fitting (Chi-squared value)

alginfo.outputs(1).name = 'p';
alginfo.outputs(1).desc = 'coefficients';

alginfo.outputs(2).name = 's';
alginfo.outputs(2).desc = 'struct';


alginfo.providesGUF = 1;
alginfo.providesMCM = 0;


% vim settings modeline: vim: foldmarker=%<<<,%>>> fdm=marker fen ft=octave textwidth=80 tabstop=4 shiftwidth=4
