// Water Detection Library
// Copyright (c) 2014, Motim Technologies Ltd.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, 
// are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this 
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation and/or 
// other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors 
// may be used to endorse or promote products derived from this software without 
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "FFT.h"
#include <math.h>
#include <string.h>

#define M_PI 3.14159265358979323846


unsigned int _log2( unsigned int x )
{
  unsigned int ans = 0 ;
  while( x>>=1 ) ans++;
  return ans ;
}

/**
 * Setup a new FFT object.
 * Note - N must be a power of 2!
 */
FFT::FFT(int _n)
{
	// setup FFT
	n = _n;
	log2n = (int)_log2((unsigned int) n);
	validSpectrumLength = (n / 2) + 1;

	// precompute tables
	COS_LOOKUP_TABLE = new float[n/2];
	SIN_LOOKUP_TABLE = new float[n/2];

	for(int i = 0; i < n/2; i++)
	{
		COS_LOOKUP_TABLE[i] = cosf(i * (2 * M_PI / n));
		SIN_LOOKUP_TABLE[i] = -sinf(i * (2 * M_PI / n));
	}

	imag 	= new float[n];
	zeroPad = new float[n];	// used to zero this.imag without creating new array every time

	for(int i = 0; i < n; i++)
		zeroPad[i] = 0.0f;
}

FFT::~FFT()
{
	delete [] COS_LOOKUP_TABLE;
	delete [] SIN_LOOKUP_TABLE;
	delete [] imag;
	delete [] zeroPad;
}

/**
 * Do interlace decomposition on a real input signal array.
 */
void FFT::bitSwapInPlace(float* inputReal)
{
	// Pre-computations
	int bin = 0;
	int halfLength = n >> 1;

	// Temp variables
	int cursor;
	float temp;

	// Interlace decomposition
	for (int i=1; i < n-1; i++)
	{
		cursor = halfLength;

		while ( cursor <= bin)
		{
			bin 	-= cursor;
			cursor >>= 1;		// half
		}

		bin += cursor;

		if (i < bin)
		{
			// swapping i with bin
			temp 				= inputReal[i];
			inputReal[i] 		= inputReal[bin];
			inputReal[bin] 	= temp;
		}
	}

}

/**
 * Real-valued fft: pad out complex entries with 0, calculate magnitudes within limits, return as input array.
 * input - the real-valued signal data
 * lowBin - the lowest bin we'll be using
 * highBin - the highest bin we'll be using (window size)
 */
void FFT::fftr(float* input, int lowBin, int highBin)
{

	int 	n1,
			lengthFactor=1,
			x;

	float 	cosx,
			sinx,
			temp1,
			temp2;

	// complex number padding
	memcpy(imag, zeroPad, n*sizeof(float));

	bitSwapInPlace(input);

	// Do the FFT calculation:
	// Outer loop: Loop thru all the 1-point signals (log2n of them)
	for (int i = 0; i < log2n; i++)
	{
		n1 = lengthFactor;
		lengthFactor <<= 1; // double it
		x = 0;

		// Middle loop: Loop thru the individual frequency spectra for this stage
		for (int j = 0; j < n1; j++)
		{
			sinx = SIN_LOOKUP_TABLE[x];
			cosx = COS_LOOKUP_TABLE[x];
			x +=  1 << (log2n-i-1);

			// Inner Loop: Calculate points in each spectra using the FFT butterfly
			for (int k = j; k < n; k += lengthFactor)
			{
				// calculate half-way factors
				temp1 = cosx*input[k+n1] - sinx*imag[k+n1];
				temp2 = sinx*input[k+n1] + cosx*imag[k+n1];

				// modify arrays
				input[k+n1] = input[k] - temp1;
				imag[k+n1] 	= imag[k] - temp2;
				input[k] 	= input[k] + temp1;
				imag[k] 	= imag[k] + temp2;

			}
		}
	}


	// testing
	for(int i = 0; i < lowBin; i++)
	{
		input[i] = 0;
	}
	for(int i = highBin; i < n; i++)
	{
		input[i] = 0;
	}

	// done! Now find magnitudes...
	for(int i = lowBin; i < highBin; i++)
	{
		// pass magnitudes back out as input array
		input[i] = input[i]*input[i] + imag[i]*imag[i]; //hypotf(input[i], imag[i]);
	}

}




