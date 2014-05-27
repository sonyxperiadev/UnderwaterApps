// Water Detection Demo
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

package com.motim.waterdetectiondemo;

import com.motim.waterdetection.WaterDetector;
import com.motim.waterdetection.WaterDetector.WaterEventListener;

import android.app.Activity;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.TranslateAnimation;
import android.widget.TextView;

public class MainActivity extends Activity {

	private static int ANIMATION_DURATION = 1000;
	
    private WaterDetector waterDetector;
	private View bucket;
	private View phone;
	private boolean lastSubmergeState;

	private View wetBg;

	private boolean animating = false;

	@Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //Find views
        bucket = findViewById(R.id.bucket);
        phone = findViewById(R.id.phone);
        wetBg = findViewById(R.id.wet_bg);
        
        //Set up water detection
        waterDetector = new WaterDetector();
        waterDetector.setWaterEventListener(new WaterEventListener() {

			@Override
			public void onWaterEvent(final boolean isSubmerged) {
				phone.post(new Runnable() {
					@Override
					public void run() {
						setSubmerged(isSubmerged);
					}
				});
				
			}
        });
    }
	
	private void setSubmerged(boolean isSubmerged) {
		//If the water detection state has changed, animate to the new state
		if (isSubmerged != lastSubmergeState) {
			lastSubmergeState = isSubmerged;

			//Only animate a change if an animation is not in progress.
			//If a change happens during animation, the new animation will
			//be played at the end of the current one.
			if (!animating ) {
				animate(isSubmerged);
			}
			
		}
	}

	private void animate(final boolean submerging) {
		animating = true;
		//Calculate difference in pixels that the phone must move
		int yDiff = (bucket.getTop() + bucket.getHeight()/2)  - (phone.getTop() + phone.getHeight()/2);
		
		//Smoothly translate phone graphic
		TranslateAnimation anim = new TranslateAnimation(0, 0, submerging ? 0 : yDiff, submerging ? yDiff : 0);
		anim.setDuration(ANIMATION_DURATION);
		anim.setFillAfter(true);
		phone.startAnimation(anim);
		
		//Smoothly fade wet background
		AlphaAnimation alphaAnim = new AlphaAnimation(submerging ? 0.0f : 1.0f, submerging ? 1.0f : 0.0f);
		alphaAnim.setDuration(ANIMATION_DURATION);
		alphaAnim.setFillAfter(true);
		
		//Add a check to the end of the animation for the submersion state changing.
		alphaAnim.setAnimationListener(new AnimationListener() {
			@Override
			public void onAnimationStart(Animation animation) {
			}
			
			@Override
			public void onAnimationEnd(Animation animation) {
				if (lastSubmergeState != submerging) {
					animate(lastSubmergeState);
				}
				else {
					animating = false;
				}
			}

			@Override
			public void onAnimationRepeat(Animation animation) {
			}
		});
		
		wetBg.setVisibility(View.VISIBLE);
		wetBg.startAnimation(alphaAnim);
	}
	
    @Override
    protected void onResume() {
    	super.onResume();
    	//Resume the water detector, starting the tone generator and audio recorder
    	waterDetector.onResume(this);
    }
    
    @Override
    protected void onPause() {
    	super.onPause();
    	//Pause the water detector, freeing the tone generator and audio recorder
    	waterDetector.onPause();
    }
    
    @Override
    protected void onDestroy() {
    	super.onDestroy();
    	//Destroy the water detector, freeing native resources
    	waterDetector.onDestroy();
    }    
}
