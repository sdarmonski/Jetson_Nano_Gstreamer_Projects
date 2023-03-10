/*
This program is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with this program. If not, see <https://www.gnu.org/licenses/>.

Copyright (c) 2023 Stanislav Darmonski s.darmonski@gmail.com
*/

#pragma once

#include "UvxGstPipeline_Class.h"

// Pipeline class for visualization and recording
class UvxGstPipelineVR : public UvxGstPipeline
{
public:
    enum CfgParamsKeys {
        CAPTURE_DEV,
        CAMERA_START_REC_HOTKEY,
        CAMERA_STOP_REC_HOTKEY,
        CAMERA_QUIT_HOTKEY,
        IN_PIX_FMT,
        OUT_PIX_FMT,
        STREAMING_ENABLED,
        STREAM_QP_RANGE,
        STREAM_MTU,
        STREAM_RES_W,
        STREAM_RES_H,
        HOST_IP,
        HOST_PORT,
        REC_RES_W,
        REC_RES_H,
        REC_QP_RANGE,
        REC_FILENAME_PREFIX,
        REC_FILE_SIZE_BYTES,
        FREE_SPACE_RSRV_BYTES,
        REC_TEXT_COLOR_ARGB,
        REC_TEXT_FONT,
        OVERLAY_RES_W,
        OVERLAY_RES_H,
        OVERLAY_POS_X,
        OVERLAY_POS_Y,
        FREESPACE_WDOG_TIMOUT_S
    };

    typedef enum {
        REC_STOPPED,
        REC_STARTED,
        REC_STOP_REQUESTED,
        REC_START_REQUESTED
    } UvxGstRecState;

    // Constructor
    using UvxGstPipeline::UvxGstPipeline;

    // Initialize pipeline
    gboolean init_pipeline() override;

    // Start pipeline
    gboolean start_pipeline() override;

    // Free resources
    void unref_pipeline() override;

    // Get the available recording space in bytes
    gulong get_available_space_bytes() const { return availRecSpaceBytes; }

    // Get recording location
    gchar* get_recording_location() { return recLocation; }

    // Set recording location
    void set_recording_location(gchar *recLoc) { recLocation = recLoc; }

    // Set configuration parameters file
    void set_configuration_file(gchar *paramsFile) { cfgParamsFile = paramsFile; }

protected:
    // Initialize the configuration parameters
    void init_parameters();

    // Update the available disk space at the recording location
    void update_available_space_bytes();

    // Update the recording enabled flag
    void update_rec_enabled();

    // Create the recording branch elements
    gboolean create_recording_branch();

    // Create the streaming branch elements
    virtual gboolean create_streaming_branch() { return TRUE; }

    // Free streaming branch resources
    virtual gboolean unref_streaming_branch() { return TRUE; }

    // Get a recording filename with a specified file number
    gchar* get_rec_filename(guint fileNum);

    // Get a recording filename as a regular expression
    gchar* get_rec_filename();

    // Initialize the recording file number
    void init_rec_filenumber();

    // Start recording command
    void start_recording();

    // Stop recording command
    void stop_recording();

    // Callback for handling EOS
    static GstPadProbeReturn eos_event_probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer pobject);

    // Monitor available space at regular intervals
    static gboolean freespace_watchdog(gpointer pobject);

    // Recording timer visualization callback function
    static GstPadProbeReturn buffer_pad_probe_cb_REC_timer(GstPad *pad, GstPadProbeInfo *info, gpointer pobject);

    // Callback for handling pad blocking
    static GstPadProbeReturn pad_probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer pobject);

    // Process input commands
    static gboolean handle_input_commands(GIOChannel *source, GIOCondition cond, gpointer pobject);

    // Bus callback function for handling bus messages
    static gboolean bus_watch_cb(GstBus *bus, GstMessage *msg, gpointer pobject);

    // Visualization branch elements
    GstElement *videosource, *tee, *queue0, *textoverlay0, *converter0, *sink0;

    // Recording branch elements
    GstElement *queue1, *converter1, *encoder1, *parse1, *muxer1, *sink1;

    // Pads for connecting the branches to the source
    GstPad *tee_out0, *queue0_in, *queue0_out;
    GstPad *tee_out1, *queue1_in;

    // Capabilities for the source and converter elements
    GstCaps *sourceCaps, *convrt0Caps, *convrt1Caps;

    // Pipeline configuration parameters
    map<int, string> cfgParams;

    // Initial timestamp used for recording timer display
    GstClockTime recInitPts;

    // State of the recording branch
    UvxGstRecState recState = REC_STOPPED;

    // Filesystem statistics and status
    struct statvfs fsStatus;

    // Recording location
    gchar *recLocation;

    // Configuration parameters file
    gchar *cfgParamsFile = NULL;

    // Livestream detected status flag
    gboolean isLivestream = FALSE;

    // Recording enabled/disabled status flag
    gboolean recEnabled;

    // EOS received flag upon stop recording command
    gboolean recStopEosReceived = FALSE;

    // Available space for recording in bytes
    gulong availRecSpaceBytes;

    // Reserved free space in bytes
    gulong freeSpaceRsrvBytes = 0;

    // Recording file number
    guint recFileNum = 1;

    // Recording file number format
    gchar *recFileNumFmt;

    // Freespace watchdog callback process ID
    guint freespaceWatchdogPid = 0;
};
