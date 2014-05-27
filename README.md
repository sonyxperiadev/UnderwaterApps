# Water Detection Library

Copyright 2014, Motim Technologies Ltd, All rights reserved.
Licensed under the BSD 3-Clause License

Motim Technologies Ltd accepts no responsibility for damage caused to devices which are
unsupported or not in a waterproof state.

ENSURE ALL WATERPROOF FLAPS ARE SECURELY CLOSED BEFORE SUBMERGING PHONE

## Introduction

This library was developed to allow waterproof Sony Xperia phones to detect when they are
submerged in water to allow the creation of a range of novel apps. 

## Theory of Operation

The detector generates a test tone with a frequency beyond that of normal human hearing and
records sound through the two microphones of the Xperia phone for processing. Spectrograms
for both channels are generated and the minimum, maximum, mean and variance values for both
spectrograms around the frequency of the test tone are calculated. These combined eight
features (four for each channel), as well as an additional feature computed from the amplitude
difference between the channels, are used as the inputs for a neural network with two outputs,
the in air and in water states. The state with highest output value is considered to be the
current one.

See "waterdetection.html" for more information.

## Usage

To use the library for your own applications, import the WaterDetection project into an
Eclipse workspace. In the project settings for your own application, add the WaterDetection project
as a required project on the build path (under Java Build Path > Projects). Then make sure
WaterDetection is ticked to be exported (Java Build Path > Order and Export). Finally, reference
WaterDetection as an Android library on the Android page of the project settings.

The native components of the WaterDetection project need to be built before the detector will work.
If you haven't already, install the Android NDK. Run ndk-build in the WaterDetection library folder,
if the native compilation succeeds, there should be a range of folders for various architectures in
the "lib" folder, each containing a file called libWaterDetection.so.

Finally, a few permissions need to be granted to your app in order for the water detector to set up 
audio playback and recording properly. In the AndroidManifest.xml file of your project, include 
the following lines:

    <!-- Required for audio input to water detector -->
    <uses-permission android:name="android.permission.RECORD_AUDIO"/>
    <!-- Required for setting volume for measurement tone generator -->
    <uses-permission android:name="android.permission.MODIFY_AUDIO_SETTINGS"/>
    
## Demo Project

A Demo app demonstrating the Water Detection Library can be found in this repository called "WaterDetectionDemo".
This demo app shows a phone and a bucket of water. When the physical phone is submerged the app shows the phone image
as submerged in the bucket. 

To use the demo project, import the WaterDetectionDemo project into the same workspace as the WaterDetection project
and build/run it as an Android Application.
