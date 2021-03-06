#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <mex.h>
#include "topiclib.cpp"

// Syntax
//   [ PHI , THETA ] = sTBP( WD , J , N , ALPHA , BETA , SEED , OUTPUT )

void sTBP(double ALPHA, double BETA, int W, int J, int D, int NN, int OUTPUT, double *sr, 
	mwIndex *ir, mwIndex *jc, double *phi, double *theta) 
{
	int wi, di, i, j, topic, iter;
	double mutot, perp = 0.0, WBETA = (double) (W*BETA), xi, xitot = 0.0, JALPHA = (double) (J*ALPHA);
	double *phitot, *thetad, *phi2, *theta2, *phitot2, *mu;

	phitot = dvec(J);
	thetad = dvec(D);
	phi2 = dvec(J*W);
	theta2 = dvec(J*D);
	phitot2 = dvec(J);
	mu = dvec(J);

	/* random initialization */
	for (di=0; di<D; di++) {
		for (i=jc[di]; i<jc[di + 1]; i++) {
			wi = (int) ir[i];
			xi = sr[i];
			xitot += xi;
			// pick a random topic 0..J-1
			topic = (int) (J*drand());
			mu[topic] = (double) 1; // assign this word token to this topic
			phi[wi*J + topic] += xi; // increment phi count matrix
			theta[di*J + topic] += xi; // increment theta count matrix
			phitot[topic] += xi; // increment phitot matrix
			thetad[di] += xi;
		}
	}

	for (iter=0; iter<NN; iter++) {

		if (OUTPUT >= 1) {			
			if (((iter % 10)==0) && (iter != 0)) {
				/* calculate perplexity */
				perp = (double) 0;
				for (di=0; di<D; di++) {
					for (i=jc[di]; i<jc[di + 1]; i++) {
						wi = (int) ir[i];
						xi = sr[i];
						mutot = (double) 0;
						for (j=0; j<J; j++) {
							mutot += ((double) phi[wi*J + j] + (double) BETA)/
								((double) phitot[j] + (double) WBETA)*
								((double) theta[di*J + j] + (double) ALPHA)/
								((double) thetad[di] + (double) JALPHA);
						}					
						perp -= (log(mutot)*xi);
					}
				}
				mexPrintf("\tIteration %d of %d:\t%f\n", iter, NN, exp(perp/xitot));
				if ((iter % 10)==0) mexEvalString("drawnow;");
			}
		}

		/* clear phi2, theta2, and phitot2 */
		for (i=0; i<J*D; i++) theta2[i] = 0.0;
		for (i=0; i<J*W; i++) phi2[i] = 0.0;
		for (j=0; j<J; j++) phitot2[j] = 0.0;

		/* passing message mu */
		for (di=0; di<D; di++) {
			for (i=jc[di]; i<jc[di + 1]; i++) {
				wi = (int) ir[i];
				xi = sr[i];
				mutot = (double) 0;
				for (j=0; j<J; j++) {
					mu[j] = ((double) phi[wi*J + j] + (double) BETA)/
						((double) phitot[j] + (double) WBETA)*
						((double) theta[di*J + j] + (double) ALPHA);
					mutot += mu[j];
				}
				for (j=0; j<J; j++) {
					mu[j] /= mutot;
					theta2[di*J + j] += mu[j]*xi;
					phi2[wi*J + j] += mu[j]*xi;
					phitot2[j] += mu[j]*xi;
				}			
			}
		}

		/* update theta, phi and phitot */
		for (i=0; i<J*D; i++) theta[i] = theta2[i];
		for (i=0; i<J*W; i++) phi[i] = phi2[i];
		for (j=0; j<J; j++) phitot[j] = phitot2[j];
	}
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{
	double *phi, *theta, *sr;
	double ALPHA, BETA;
	mwIndex *ir, *jc;
	int W, J, D, NN, SEED, OUTPUT;

	/* Check for proper number of arguments. */
	if (nrhs < 7) {
		mexErrMsgTxt("At least 7 input arguments required");
	} else if (nlhs < 2) {
		mexErrMsgTxt("At least 2 output arguments required");
	}

	/* dealing with sparse array WD */
	if (mxIsDouble(prhs[0]) != 1) mexErrMsgTxt("WD must be a double precision matrix");
	sr = mxGetPr(prhs[0]);
	ir = mxGetIr(prhs[0]);
	jc = mxGetJc(prhs[0]);
	W = (int) mxGetM(prhs[0]);
	D = (int) mxGetN(prhs[0]);

	J = (int) mxGetScalar(prhs[1]);
	if (J<=0) mexErrMsgTxt("Number of topics must be greater than zero");

	NN = (int) mxGetScalar(prhs[2]);
	if (NN<0) mexErrMsgTxt("Number of iterations must be positive");

	ALPHA = (double) mxGetScalar(prhs[3]);
	if (ALPHA<0) mexErrMsgTxt("ALPHA must be greater than zero");

	BETA = (double) mxGetScalar(prhs[4]);
	if (BETA<0) mexErrMsgTxt("BETA must be greater than zero");

	SEED = (int) mxGetScalar(prhs[5]);

	OUTPUT = (int) mxGetScalar(prhs[6]);

	/* seeding */
	seedMT(1 + SEED*2); // seeding only works on uneven numbers

	/* allocate memory */
	phi = dvec(J*W);
	theta = dvec(J*D);

	/* run the learning algorithm */
	sTBP(ALPHA, BETA, W, J, D, NN, OUTPUT, sr, ir, jc, phi, theta);

	/* output */
	plhs[0] = mxCreateDoubleMatrix(J, W, mxREAL);
	mxSetPr(plhs[0], phi);

	plhs[1] = mxCreateDoubleMatrix(J, D, mxREAL);
	mxSetPr(plhs[1], theta);
}
