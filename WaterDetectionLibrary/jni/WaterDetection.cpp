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

#include <jni.h>
#include "FFT.h"
#include <string.h>
#include <math.h>

//#include <stdio.h>
//FILE * pFile;

static int state = -1;
static float * spectrogram1;
static float * spectrogram2;
static int numSpectra = 0;

static FFT * fft;
static int n = 512;

static int w = 3;
static int numbins = n/2 + 1;
static int frames = 100;

//features
static int numfeatures = 8;
static float * first;
static int firstacc = 2;

static float mean1,mean2, max1,max2, var1,var2;
static int model = 0;

// Z, Z1, Z1S respectively. Data from desktop training.
static float k[] = { //air value bias scale-factor. Below one for less false negatives and above one for less false positives.
		1.0f,
		1.0f,
		1.0f
};

static float bias1[] = {
		35.5462,
		20.5982,
		3.6766
};

static float bias2[] = {
		-35.9042,
		-20.1277,
		-3.4051
};

static float weights1[][9] = {
		{ 	-7.5540,  -0.7146,   2.5971, -18.4074,  -8.8711,   0.5688,   0.2565,   0.0755,  -0.1047},
		{  -26.0055,  11.1361,   3.5664,  -3.3576,  -3.5706,  -0.6180,   0.5575,   0.1595,   0.4632},
		{ 	 0.6594,   2.0125,   1.8729,  -5.2284,  -2.7234,   0.9259,  -0.0331,   0.0046,  -0.7622}
};

static float weights2[][9] = {
		{ 	 7.8489,   1.0318,  -2.3280,  18.8388,   9.3777,   0.1654,   0.4106,   0.2758,   0.2114},
		{ 	26.1086, -10.7750,  -3.4174,   3.5933,   3.6595,   0.5035,   0.5272,   0.1125,  -0.0820},
		{	-0.7273,  -1.6547,  -1.9048,   5.3467,   3.1399,  -1.1836,   0.3017,   0.0063,   0.5362}
};

extern "C"
{
	void Java_com_motim_waterdetection_WaterDetector_createEngine(JNIEnv* env, jclass clazz, jint _frames, jint _model);
	void Java_com_motim_waterdetection_WaterDetector_shutdown(JNIEnv* env, jclass clazz);

	jboolean Java_com_motim_waterdetection_WaterDetector_buildSpectrogram(JNIEnv* env, jclass clazz, jfloatArray arr1, jfloatArray arr2);
	jboolean Java_com_motim_waterdetection_WaterDetector_isUnderWater(JNIEnv* env, jclass clazz, jfloat amp1, jfloat amp2);
}

void Java_com_motim_waterdetection_WaterDetector_createEngine(JNIEnv* env, jclass clazz, jint _frames, jint _model)
{
	frames = _frames;
	model = _model;

	fft = new FFT(n);
	spectrogram1 = new float[numbins*frames];
	spectrogram2 = new float[numbins*frames];

	first = new float [numfeatures]; for(int i=0; i<numfeatures; i++) first[i] = 0.f;
}

void Java_com_motim_waterdetection_WaterDetector_shutdown(JNIEnv* env, jclass clazz)
{
	delete fft;
	delete [] spectrogram1;
	delete [] spectrogram2;

	delete [] first;
}

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
static void getSpectrogramValues(int spectrum)
{
	float * spectrogram = NULL;

	spectrum == 1 ? spectrogram = spectrogram1 : spectrogram = spectrogram2;

	//normalize
	for(int i=0; i<numbins*frames; i++)
		spectrogram[i] = logf(spectrogram[i] + 0.01f) * 10.f;

	float mean = 0;
	float max = 0;

	int total=0;
	for(int i=0; i<frames; i++)
		for(int j=0; j<w; j++)
		{
			float val = spectrogram[i * numbins + j];
			mean += val;
			max = MAX(val, max);
			total++;
		}

	mean /= (float)total;

	float var = 0;
	total=0;
	for(int i=0; i<frames; i++)
		for(int j=0; j< w; j++)
		{
			float val = spectrogram[i * numbins + j];
			var += (val - mean)*(val - mean);
			total++;
		}

	var /= (float)total;

	if(spectrum == 1)
	{
		mean1 = mean;
		max1 = max;
		var1 = var;
	}
	else
	{
		mean2 = mean;
		max2 = max;
		var2 = var;
	}
}

jboolean Java_com_motim_waterdetection_WaterDetector_isUnderWater(JNIEnv* env, jclass clazz, jfloat amp1, jfloat amp2)
{


	if(!state) return false;
	else if(state <= firstacc)
	{
		first[0] += mean1;
		first[1] += max1;
		first[2] += var1;

		first[3] += mean2;
		first[4] += max2;
		first[5] += var2;

		first[6] += amp1;
		first[7] += amp2;

		if(state == firstacc)
			for(int i=0; i<numfeatures; i++)
			{
				first[i] /= (float)firstacc;
				if(first[i] == 0.f) first[i] = 0.01f;
			}

		return false;
	}
	else
	{
		float input[numfeatures];
		input[0] = mean1;
		input[1] = max1;
		input[2] = var1;

		input[3] = mean2;
		input[4] = max2;
		input[5] = var2;

		input[6] = amp1;
		input[7] = amp2;

		float air_value = bias1[model] * k[model];
		float uw_value = bias2[model];

		for(int i=0; i< numfeatures; i++)
		{
			air_value += weights1[model][i] * (input[i]/first[i]);
			uw_value  += weights2[model][i] * (input[i]/first[i]);
		}

		air_value += weights1[model][numfeatures] * ((amp2 - amp1)/((amp2 + amp1)/2.f));
		uw_value  += weights2[model][numfeatures] * ((amp2 - amp1)/((amp2 + amp1)/2.f));

		return uw_value  > air_value;
	}
}

jboolean Java_com_motim_waterdetection_WaterDetector_buildSpectrogram(JNIEnv* env, jclass clazz, jfloatArray arr1, jfloatArray arr2)
{
	jfloat *data1 = env->GetFloatArrayElements(arr1, 0);
	jfloat *data2 = env->GetFloatArrayElements(arr2, 0);

	fft->fftr(data1, 0, n);
	fft->fftr(data2, 0, n);

	memcpy(&spectrogram1[numSpectra * numbins], data1, numbins * sizeof(float));
	memcpy(&spectrogram2[numSpectra * numbins], data2, numbins * sizeof(float));

	env->ReleaseFloatArrayElements(arr1, data1, 0);
	env->ReleaseFloatArrayElements(arr2, data2, 0);

	if(++numSpectra == frames)
	{
		getSpectrogramValues(1);
		getSpectrogramValues(2);

		state++;
		numSpectra = 0;
		return true;
	}
	else
		return false;
}
