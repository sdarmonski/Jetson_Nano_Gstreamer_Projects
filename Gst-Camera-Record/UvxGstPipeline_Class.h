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

#include <gst/gst.h>
#include <sys/statvfs.h>
#include </usr/include/stdio.h>

using namespace std;

// Pipeline configuration parameters
#define CAPTURE_DEV                 "/dev/video0"
#define IN_PIX_FMT                  "UYVY"
#define OUT_PIX_FMT                 "I420"
#define STREAMING_ENABLED           FALSE
#define STREAM_QP_RANGE             "35,35:35,35:-1,-1"
#define STREAM_MTU                  60000
#define STREAM_RES_W                640
#define STREAM_RES_H                480
#define HOST_IP                     "192.168.88.91"
#define HOST_PORT                   "5000"
#define REC_RES_W                   1920
#define REC_RES_H                   1080
#define REC_QP_RANGE                "20,20:20,20:-1,-1"
#define REC_FILENAME_PREFIX         "UVX-"
#define REC_FILE_SIZE_BYTES         1073741824
#define FREE_SPACE_RSRV_BYTES       (gulong)1073741824
#define REC_TEXT_COLOR_ARGB         4294901760
#define REC_TEXT_FONT               "Sans, 6"
#define OVERLAY_RES_W               1280
#define OVERLAY_RES_H               720
#define OVERLAY_POS_X               0
#define OVERLAY_POS_Y               24
#define FREESPACE_WDOG_TIMOUT_S     20
// TODO: the pipeline configuration parameters should be read from the file /etc/default/gstreamer-setup

// Base pipeline class, common for all specific pipelines
class UvxGstPipeline
{
public:
    // Constructor
    UvxGstPipeline(int *argc, char ***argv);

    // Initialize pipeline
    virtual gboolean init_pipeline() = 0;

    // Start pipeline
    virtual gboolean start_pipeline() = 0;

    // Free resources
    virtual void unref_pipeline() = 0;

protected:
    GIOChannel *io_stdin;
    GMainLoop *loopMain;
    GstElement *pipeline;
};
