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

#include <fstream>
#include <map>
#include <gst/gst.h>
#include <sys/statvfs.h>
#include </usr/include/stdio.h>

using namespace std;

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
