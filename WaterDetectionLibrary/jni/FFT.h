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

#ifndef FFT_H_
#define FFT_H_

// An FFT implementation for real value input

class FFT
{
	// Functions
public:

	~FFT();

	/**
	 * Constructs with a given N. Precalculates lookup tables.
	 */
	FFT(int _n);

	/**
	 * Take a real valued fft within the given limiting bins.
	 * Reuses the input array, overwriting it with the magnitudes of the FFT spectrum.
	 */
	void fftr(float* input, int lowBin, int highBin);

private:

	/**
	 * Does bit swapping - the first stage of the FFT cycle.
	 */
	void bitSwapInPlace(float* inputReal);

	// Data
public:

	int n;							// length of the FFT
	int validSpectrumLength;		// The Nyquist bin

private:
	int log2n;						// partway calculation - log2(n)

	// Lookup tables for sin & cos, precalculated.
	float* COS_LOOKUP_TABLE;
	float* SIN_LOOKUP_TABLE;

	float* imag;	// imaginary value padding
	float* zeroPad;

};

#endif /* FFT_H_ */
