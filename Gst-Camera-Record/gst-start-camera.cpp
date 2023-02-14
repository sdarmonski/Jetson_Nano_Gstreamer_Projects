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

#include "UvxGstPipelineVR.h"
#include "UvxGstPipelineVRS.h"

int main(int argc, char *argv[])
{
    // Check the input arguments
    if (!argv[1])
    {
        g_printerr("Invalid recording location specified!\n");
        return -1;
    }

    UvxGstPipelineVRS pgst(&argc, &argv);

    // Set recording location
    pgst.set_recording_location(argv[1]);
    g_print("Recording location set to %s\n", pgst.get_recording_location());

    // Initialize pipeline
    if ( !pgst.init_pipeline() )
    {
        g_printerr("Error initializing new pipeline.\n");
        return -1;
    }

    // Start the pipeline
    if (!pgst.start_pipeline())
    {
        g_printerr("Error starting pipeline.\n");
        return -1;
    }

    g_print("Freeing resources...\n");
    pgst.unref_pipeline();
    g_print("Pipeline closed!\n");
    return 0;
}
