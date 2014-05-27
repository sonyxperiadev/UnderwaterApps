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

package com.motim.waterdetection;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.util.Log;

/**
 * WaterDetector detects if a device is submerged in water based
 * on the input of the device microphones
 */
public class WaterDetector {
	AudioRecord audioRecord;
	AudioTrack audioTrack;
	short[] buffer = new short[1024];
	
	private int bufferSize;
	private boolean active = false;
	private int playbackBufferSize;
	private byte[] playbackTone;
	private Thread toneThread;
	
	private Thread detectionThread;
	
	private final int sampleRateInHz = 44100;
	private final int freqOfTone = sampleRateInHz/2;
	
	private int airconsecutive = 0;
	private int uwconsecutive = 0;
	
	private int ratioCounts = 0;
	private double ratio = 0;
	
	private int frames = 80;
	private float totMic1 = 0;
	private float totMic2 = 0;
	
	/**
	 * WaterEventListener is used to report changes in submersion state
	 */
	public interface WaterEventListener {
		/**
		 * Called when subersion state is changed
		 * @param isSubmerged True if the phone is probably submerged in water, False otherwise
		 */
		void onWaterEvent(boolean isSubmerged);
	}
	
	WaterEventListener listener;
	private Runnable toneGenRunnable;
	private Runnable detectionRunnable;
	
	public WaterDetector() {
		bufferSize = AudioRecord.getMinBufferSize (sampleRateInHz, AudioFormat.CHANNEL_IN_STEREO,  AudioFormat.ENCODING_PCM_16BIT);
		playbackBufferSize = AudioTrack.getMinBufferSize(sampleRateInHz, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);

		//Set up playback tone
		
		playbackTone = new byte[playbackBufferSize*10];
		double sample[] = new double[playbackTone.length / 2];
		int numSamples = sample.length;
		
		for (int i = 0; i < numSamples; i++){
			sample[i] = Math.sin(2 * Math.PI * i / (sampleRateInHz/freqOfTone));
		}
		
		int idx = 0;
		int i;
		int ramp = numSamples / 10;
		
		// Ramp amplitude up (to avoid clicks)
		for (i = 0; i< ramp; ++i) {                                    
            double dVal = sample[i];
            final short val = (short) ((dVal * 32767 * i/ramp));
                                                                        
            playbackTone[idx++] = (byte) (val & 0x00ff);
            playbackTone[idx++] = (byte) ((val & 0xff00) >>> 8);
        }

		 // Max amplitude for most of the samples
        for (; i< numSamples - ramp; ++i) {                       
            double dVal = sample[i];
            final short val = (short) ((dVal * 32767));

            playbackTone[idx++] = (byte) (val & 0x00ff);
            playbackTone[idx++] = (byte) ((val & 0xff00) >>> 8);
        }

        // Ramp down to zero
        for (; i< numSamples; ++i) {  
            double dVal = sample[i];                                                         		
            final short val = (short) ((dVal * 32767 * (numSamples-i)/ramp ));

            playbackTone[idx++] = (byte) (val & 0x00ff);
            playbackTone[idx++] = (byte) ((val & 0xff00) >>> 8);
        }

        //Create runnable for playing tone
        toneGenRunnable = new Runnable() {
			@Override
			public void run() {
				while (active) {
					audioTrack.write(playbackTone, 0, playbackTone.length);
				}
				Log.d("WaterDetector", "Tone thread ending");
			}
        };
        
        //Create runnable for processing recorded audio samples
        detectionRunnable = new Runnable()
        {
        	@Override
        	public void run() {
        		while (active) {
        			int shortsRead = audioRecord.read(buffer, 0, buffer.length);
        			float[] record1 = new float[buffer.length/2];
        			float[] record2 = new float[buffer.length/2];

        			int i,j;

        			for (i = 0, j = 0; i < shortsRead; i+=2, j++)
        			{
        				record1[j] = 1000.f * (float)buffer[i];
        				record2[j] = 1000.f * (float)buffer[i+1];

        				totMic1 += Math.abs((float)buffer[i]);
        				totMic2 += Math.abs((float)buffer[i+1]);
        			}

        			totMic1 /= (float)shortsRead;
        			totMic2 /= (float)shortsRead;

        			if(buildSpectrogram(record1, record2))
        			{
        				//Some hysteresis for submersion state.
        				//Several consecutive measurements of the same reading
        				//are required to shift state.
        				
        				if(isUnderWater(totMic1, totMic2)) {
							uwconsecutive++;
        					airconsecutive=0;
        				}
						else {
							airconsecutive++;
        					uwconsecutive=0;
						}

						if (uwconsecutive >= 3)
						{
							if (listener != null) {
								listener.onWaterEvent(true);
							}
						}
						else if (airconsecutive >= 2)
						{
							if (listener != null) {
								listener.onWaterEvent(false);
							}
						}

            			totMic1 = 0;
            			totMic2 = 0;
        			}
        		}
        	}
        };

        //Choose model for native detection profile
        
        final int model;
        
        if (android.os.Build.MODEL.equals("C6606")) { //Xperia Z, model = 0
        	model = 0;
        	Log.d("WaterDetector", "Found Xperia Z");
        }
        else if (android.os.Build.MODEL.equals("C6902")) { //Xperia Z1, model = 1
        	model = 1;
        	Log.d("WaterDetector", "Found Xperia Z1");
        }
        else if (android.os.Build.MODEL.equals("C6916")) { //Xperia Z1s, model = 2
        	model = 2;
        	Log.d("WaterDetector", "Found Xperia Z1s");
        }
        else {
        	model = 0;
        	Log.d("WaterDetector", "Found Unknown device");
        }
        
        createEngine(frames, model);
	}
	
	/**
	 * Set the event listener for water submersion events
	 * @param listener The WaterEventListener to act on submersion events
	 */
	public void setWaterEventListener(WaterEventListener listener) {
		this.listener = listener;
	}
	
	/**
	 * Resume recording of input audio for processing and resume test tone generation
	 * @param context The Android context of the application using the water detector
	 */
	public void onResume(Context context) {
		AudioManager mAudioManager = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
		mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC), 0);
		
		audioRecord = new AudioRecord(MediaRecorder.AudioSource.CAMCORDER, sampleRateInHz, AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_16BIT, bufferSize);
		audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT, playbackTone.length, AudioTrack.MODE_STREAM);
		
		if (audioRecord.getState() == AudioRecord.STATE_INITIALIZED 
				&& audioTrack.getState() == AudioTrack.STATE_INITIALIZED) {
			audioRecord.startRecording();
			active = true;
		
			audioTrack.play();
			
			toneThread = new Thread(toneGenRunnable);
			toneThread.start();
			
			detectionThread = new Thread(detectionRunnable);
			detectionThread.start();
		}
		else {
			audioRecord.release();
			audioTrack.release();
		}
	}
	
	/**
	 * Stop recording of input audio for processing and stop test tone generation
	 */
	public void onPause() {
		active = false;
		try {
			if (toneThread != null) {
				toneThread.join();
			}
			
			if (detectionThread != null) {
				detectionThread.join();
			}
			
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		
		if (audioRecord != null) {
			audioRecord.stop();
			audioRecord.release();
			audioRecord = null;
		}
		
		if (audioTrack != null) {
			audioTrack.stop();
			audioTrack.release();
			audioTrack = null;
		}
	}
	
	/**
	 * Free native resources
	 */
	public void onDestroy() {
		shutdown();
	}
	
	/**
	 * Native method for allocating resources
	 * @param frames Number of spectrogram frames to use in the detector
	 * @param model Phone model configuration to use. 0: Xperia Z, 1: Xperia Z1, 2: Xperia Z1s
	 */
    private static native void createEngine(int frames, int model);
    
    /**
     * Native method for freeing resources
     */
    private static native void shutdown();

    /**
     * Native method for building the internal spectrogram from input data
     * @param arr1 Samples recorded from first microphone
     * @param arr2 Samples recorded from second microphone
     * @return True if the detector can be polled for underwater state, False otherwise
     */
    private static native boolean buildSpectrogram(float[] arr1, float[] arr2);
    
    /**
     * Test for the device being underwater.
     * @param amp1 Average of the absolute values of samples recorded from first microphone
     * @param amp2 Average of the absolute values of samples recorded from second microphone
     * @return True if the device is probably underwater, False otherwise
     */
    private static native boolean isUnderWater(float amp1, float amp2);
	
    /** Load jni .so on initialization */
    static {
         System.loadLibrary("WaterDetection");
    }
}
