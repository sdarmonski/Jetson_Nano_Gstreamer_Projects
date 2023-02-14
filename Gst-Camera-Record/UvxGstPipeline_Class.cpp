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

#include "UvxGstPipeline_Class.h"

// Constructor
UvxGstPipeline::UvxGstPipeline(int *argc, char ***argv)
{
    // Initialize GStreamer
    g_print("Initializing gstreamer...\n");
    gst_init(argc, argv);
}
