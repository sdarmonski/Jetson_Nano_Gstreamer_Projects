
DESCRIPTION
Gstreamer pipeline created with C++ for recording from camera, connected to the Jetson Nano.

INSTRUCTIONS
Create the binary file:
g++ gst-start-camera.cpp UvxGstPipeline_Class.cpp UvxGstPipelineVR.cpp -o gst-start-camera `pkg-config --cflags --libs gstreamer-1.0`

Run the binary file, providing a recording destination:
./gst-start-camera /home/<username>/Videos/

Run the binary file, providing a recording destination and a configuration parameters file
./gst-start-camera /home/<username>/Videos/ <path-to-configuration-file>

Default pipeline parameters are hardcoded and will be used if no configuration parameters file is specified.

Keyboard controls (case doesn't matter):
r -> start recording a new file
s -> stop recording
q -> quit (if a recording is started the file will not be finalized and will not be playable. Stop the recording first!)

A command from the keyboard is accepted after typing the respective letter and pressing the Enter key!

The code has been tested with the e-CAM24-CUNX camera from e-con Systems!

Requirements:
 - Gstreamer has to be installed for Linux, according to the instructions on the official Gstreamer website.
 - The configuration parameters file must contain each parameter which has to be modified, with its respective value, on a single line,
   according to the format
   <property>="<value>"
   A list of the currently supported parameters and their default values is given next:

   CAPTURE_DEV="/dev/video0" 
   IN_PIX_FMT="UYVY" 
   OUT_PIX_FMT="I420" 
   STREAMING_ENABLED="FALSE" 
   STREAM_QP_RANGE="35,35:35,35:-1,-1" 
   STREAM_MTU="60000" 
   STREAM_RES_W="640" 
   STREAM_RES_H="480" 
   HOST_IP="192.168.88.91" 
   HOST_PORT="5000" 
   REC_RES_W="1920" 
   REC_RES_H="1080" 
   REC_QP_RANGE="20,20:20,20:-1,-1" 
   REC_FILENAME_PREFIX="UVX-" 
   REC_FILE_SIZE_BYTES="1073741824" 
   FREE_SPACE_RSRV_BYTES="1073741824" 
   REC_TEXT_COLOR_ARGB="4294901760" 
   REC_TEXT_FONT="Sans, 6" 
   OVERLAY_RES_W="1280" 
   OVERLAY_RES_H="720" 
   OVERLAY_POS_X="0" 
   OVERLAY_POS_Y="24" 
   FREESPACE_WDOG_TIMOUT_S="20" 

   Note that the functionality behind some of the parameters may not yet be implemented!
