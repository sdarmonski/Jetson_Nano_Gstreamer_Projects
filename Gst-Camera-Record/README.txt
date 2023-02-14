
DESCRIPTION
Gstreamer pipeline created with C++ for recording from camera, connected to the Jetson Nano.

INSTRUCTIONS
Create the binary file:
g++ gst-start-camera.cpp UvxGstPipeline_Class.cpp UvxGstPipelineVR.cpp -o gst-start-camera `pkg-config --cflags --libs gstreamer-1.0`

Run the binary file, providing a recording destination:
./gst-start-camera /home/<username>/Videos/

Keyboard controls (case doesn't matter):
r -> start recording a new file
s -> stop recording
q -> quit (if a recording is started the file will not be finalized and will not be playable. Stop the recording first!)

A command from the keyboard is accepted after typing the respective letter and pressing the Enter key!

The code has been tested with the e-CAM24-CUNX camera from e-con Systems!

Requirements:
Gstreamer has to be installed for Linux, according to the instructions on the official Gstreamer website
The pipeline parameters are specified in the header file UvxGstPipeline_Class.h
