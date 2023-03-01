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

/* Stop recording process handler
gboolean UvxGstPipelineVR::stop_recording_cb(gpointer pobject)
{
    UvxGstPipelineVR *pVR = (UvxGstPipelineVR*)pobject;

    gst_element_set_state(pVR->queue1, GST_STATE_NULL);
    gst_element_set_state(pVR->converter1, GST_STATE_NULL);
    gst_element_set_state(pVR->encoder1, GST_STATE_NULL);
    gst_element_set_state(pVR->parse1, GST_STATE_NULL);
    gst_element_set_state(pVR->muxer1, GST_STATE_NULL);
    gst_element_set_state(pVR->sink1, GST_STATE_NULL);
    gst_bin_remove_many(GST_BIN(pVR->pipeline), pVR->queue1, pVR->converter1, pVR->encoder1, pVR->parse1, pVR->muxer1, pVR->sink1, NULL);

    pVR->recState = REC_STOPPED;
    pVR->recFileNum++;
    g_source_remove(pVR->recStopPid);
    pVR->recStopPid = 0;
    g_source_remove(pVR->freespaceWatchdogPid);
    pVR->freespaceWatchdogPid = 0;
    g_print("Recording stopped!\n");
    g_object_set(pVR->textoverlay0, "silent", TRUE, NULL);
    return TRUE;
} */

// Callback for handling EOS
GstPadProbeReturn UvxGstPipelineVR::eos_event_probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer pobject)
{
    UvxGstPipelineVR *pVR = (UvxGstPipelineVR*)pobject;

    if ( GST_EVENT_TYPE(GST_PAD_PROBE_INFO_DATA(info)) != GST_EVENT_EOS )
        return GST_PAD_PROBE_OK;

    g_print("EOS received!\n");
    gst_pad_remove_probe(pad, GST_PAD_PROBE_INFO_ID(info));

    if (pVR->recState == REC_STOP_REQUESTED)
        pVR->recStopEosReceived = TRUE; // pVR->recStopPid = g_idle_add((GSourceFunc)stop_recording_cb, (gpointer)pVR);
        
    return GST_PAD_PROBE_DROP;
}

// Monitor available space at regular intervals
gboolean UvxGstPipelineVR::freespace_watchdog(gpointer pobject)
{
    UvxGstPipelineVR *pVR = (UvxGstPipelineVR*)pobject;

    if ( pVR->recState == REC_STARTED )
    {
        // Update the available space for recording
        pVR->update_available_space_bytes();
        if ( pVR->availRecSpaceBytes < pVR->freeSpaceRsrvBytes )
        {
            g_print("Low diskspace at location %s - stopping the recording!\n", pVR->recLocation);
            pVR->stop_recording();
        }
    }

    return TRUE;
}

// Recording timer visualization
GstPadProbeReturn UvxGstPipelineVR::buffer_pad_probe_cb_REC_timer(GstPad *pad, GstPadProbeInfo *info, gpointer pobject)
{
    UvxGstPipelineVR *pVR = (UvxGstPipelineVR*)pobject;
    GstBuffer *buffer;
    GstClockTime recTime;
    guint HH, MM, SS;

    if ( pVR->recStopEosReceived )
    {
        g_object_set(pVR->textoverlay0, "silent", TRUE, NULL);
        gst_element_set_state(pVR->queue1, GST_STATE_NULL);
        gst_element_set_state(pVR->converter1, GST_STATE_NULL);
        gst_element_set_state(pVR->encoder1, GST_STATE_NULL);
        gst_element_set_state(pVR->parse1, GST_STATE_NULL);
        gst_element_set_state(pVR->muxer1, GST_STATE_NULL);
        gst_element_set_state(pVR->sink1, GST_STATE_NULL);
        gst_bin_remove_many(GST_BIN(pVR->pipeline), pVR->queue1, pVR->converter1, pVR->encoder1, pVR->parse1, pVR->muxer1, pVR->sink1, NULL);

        pVR->recState = REC_STOPPED;
        
        if ( pVR->recFileNum == 999 )
        {
            delete[] pVR->recFileNumFmt;
            pVR->recFileNumFmt = new gchar[3];
            strcpy(pVR->recFileNumFmt, "%d");
            g_print("Recording file number format specifier changed to %s!\n", pVR->recFileNumFmt);
        }

        pVR->recFileNum++;
        pVR->recStopEosReceived = FALSE;
        g_source_remove(pVR->freespaceWatchdogPid);
        pVR->freespaceWatchdogPid = 0;
        g_print("Recording stopped!\n");
    }

    if ( pVR->recState == REC_STOPPED )
    {
        // Exit and remove probe if recording is stopped
        gst_pad_remove_probe(pad, GST_PAD_PROBE_INFO_ID(info));
        return GST_PAD_PROBE_OK;
    }
    
    buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    if ( pVR->recState == REC_START_REQUESTED )
    {
        g_print("Recording started!\n");
        g_object_set(pVR->textoverlay0, "silent", FALSE, NULL);
        pVR->freespaceWatchdogPid = g_timeout_add_seconds(atoi(pVR->cfgParams[FREESPACE_WDOG_TIMOUT_S].c_str()), (GSourceFunc)freespace_watchdog, (gpointer)pVR);
        pVR->recState = REC_STARTED;
        pVR->recInitPts = GST_BUFFER_PTS(buffer);
    }

    // Calculate and display the recording time
    recTime = GST_BUFFER_PTS(buffer) - pVR->recInitPts;
    HH = (guint)(recTime / ((GstClockTime)3600000000000));
    MM = ((guint)(recTime / ((GstClockTime)60000000000))) % ((guint)60);
    SS = ((guint)(recTime / ((GstClockTime)1000000000))) % ((guint)60);

    gchar* fnum = g_strdup_printf("%d", pVR->recFileNum);
    gchar* hours = g_strdup_printf("%02d", HH);
    gchar* mins = g_strdup_printf("%02d", MM);
    gchar* secs = g_strdup_printf("%02d", SS);
    gchar* recTim = g_strconcat("REC", fnum, " " , hours, ":", mins, ":", secs, NULL);
    g_object_set(pVR->textoverlay0, "text", recTim, NULL);

    g_free(fnum);
    g_free(hours);
    g_free(mins);
    g_free(secs);
    g_free(recTim);
    return GST_PAD_PROBE_PASS;
}

// Callback for handling pad blocking
GstPadProbeReturn UvxGstPipelineVR::pad_probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer pobject)
{
    GstStateChangeReturn ret;
    UvxGstPipelineVR *pVR = (UvxGstPipelineVR*)pobject;

    //g_print("Tee src pad1 is now blocked!\n");
    gst_pad_remove_probe(pad, GST_PAD_PROBE_INFO_ID(info));
    pVR->queue1_in = gst_element_get_static_pad(pVR->queue1, "sink");

    if ( pVR->recState == REC_START_REQUESTED )
    {
        g_print("Linking recording branch to pipeline...\n");
        if ( gst_pad_link(pVR->tee_out1, pVR->queue1_in) != GST_PAD_LINK_OK )
        {
            g_printerr("Tee source pad1 could not be linked.\n");
            gst_object_unref(pVR->queue1_in);
            gst_element_release_request_pad(pVR->tee, pVR->tee_out1);
            gst_object_unref(pVR->tee_out1);
            gst_bin_remove_many(GST_BIN(pVR->pipeline), pVR->queue1, pVR->converter1, pVR->encoder1, pVR->parse1, pVR->muxer1, pVR->sink1, NULL);
            pVR->recState = REC_STOPPED;
            return GST_PAD_PROBE_OK;
        }

        ret = gst_element_set_state(pVR->pipeline, GST_STATE_PLAYING);
        if ( ret == GST_STATE_CHANGE_FAILURE )
        {
            // TODO: What to do here - is the next enough?
            g_print("Failed setting pipeline to PLAYING state while trying to start recording!\n");
            gst_pad_unlink(pVR->tee_out1, pVR->queue1_in);
            gst_object_unref(pVR->queue1_in);
            gst_element_release_request_pad(pVR->tee, pVR->tee_out1);
            gst_object_unref(pVR->tee_out1);
            gst_bin_remove_many(GST_BIN(pVR->pipeline), pVR->queue1, pVR->converter1, pVR->encoder1, pVR->parse1, pVR->muxer1, pVR->sink1, NULL);
            gst_element_set_state(pVR->pipeline, GST_STATE_PLAYING);
            pVR->recState = REC_STOPPED;
            return GST_PAD_PROBE_OK;
        }

        gst_pad_add_probe(pVR->queue0_out, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)buffer_pad_probe_cb_REC_timer, (gpointer)pVR, NULL);
    }
    else if (pVR->recState == REC_STOP_REQUESTED)
    {
        g_print("Unlinking recording branch from pipeline...\n");
        if ( !gst_pad_unlink(pVR->tee_out1, pVR->queue1_in) )
        {
            g_print("Failed to unlink tee src pad1!\n");
            gst_object_unref(pVR->queue1_in);
            return GST_PAD_PROBE_OK;
        }

        g_print("Sending EOS to finalize current file...\n");
        GstPad *filesink_input = gst_element_get_static_pad(pVR->sink1, "sink");
        gulong eos_pp_id = gst_pad_add_probe(filesink_input, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, (GstPadProbeCallback)eos_event_probe_cb, (gpointer)pVR, NULL);

        if ( !gst_pad_send_event(pVR->queue1_in, gst_event_new_eos()) )
        {
            // TODO: What to do here - is the next enough?
            g_print("EOS sending failed!\n");
            gst_pad_remove_probe(filesink_input, eos_pp_id);
            gst_pad_link(pVR->tee_out1, pVR->queue1_in);
        }
        else
        {
            g_print("EOS sent...\n");
            gst_element_release_request_pad(pVR->tee, pVR->tee_out1);
            gst_object_unref(pVR->tee_out1);
        }

        gst_object_unref(filesink_input);
    }

    gst_object_unref(pVR->queue1_in);
    return GST_PAD_PROBE_OK;
}

// Bus callback function for handling messages
gboolean UvxGstPipelineVR::bus_watch_cb(GstBus *bus, GstMessage *msg, gpointer pobject)
{
    UvxGstPipelineVR *pVR = (UvxGstPipelineVR*)pobject;
    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_ERROR:
        {
            GError *err = NULL;
            gchar *dbg;

            gst_message_parse_error(msg, &err, &dbg);
            gst_object_default_error(msg->src, err, dbg);
            g_clear_error(&err);
            g_free(dbg);
            g_main_loop_quit(pVR->loopMain);
            break;
        }
        default:
        break;
    }
    
    return TRUE;
}

// Start recording command
void UvxGstPipelineVR::start_recording()
{
    // Exit if already recording
    if ( recState != REC_STOPPED )
        return;
    
    update_rec_enabled();
    if ( recEnabled != TRUE )
    {
        g_print("Start recording requested but recording is disabled!\n");
        return;
    }

    g_print("Start recording requested...\n");
    recState = REC_START_REQUESTED;
    
    if ( create_recording_branch() )
    {
        gst_pad_add_probe(tee_out1, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, (GstPadProbeCallback)pad_probe_cb, this, NULL);
    }
    else
    {
        g_print("Failed creating recording branch!\n");
        recState = REC_STOPPED;
    }
}

// Stop recording command
void UvxGstPipelineVR::stop_recording()
{
    // Exit if not recording
    if ( recState != REC_STARTED )
        return;

    g_print("Stop recording requested...\n");
    recState = REC_STOP_REQUESTED;
    gst_pad_add_probe(tee_out1, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, (GstPadProbeCallback)pad_probe_cb, this, NULL);
}

// Process input commands
gboolean UvxGstPipelineVR::handle_input_commands(GIOChannel *source, GIOCondition cond, gpointer pobject)
{
    gchar *str = NULL;
    if ( g_io_channel_read_line(source, &str, NULL, NULL, NULL) != G_IO_STATUS_NORMAL )
        return TRUE;

    UvxGstPipelineVR *pVR = (UvxGstPipelineVR*)pobject;
    gchar cmd = g_ascii_tolower(str[0]);

    if ( cmd == pVR->cfgParams[CAMERA_START_REC_HOTKEY].at(0) )
        pVR->start_recording();
    else if ( cmd == pVR->cfgParams[CAMERA_STOP_REC_HOTKEY].at(0) )
        pVR->stop_recording();
    else if ( cmd == pVR->cfgParams[CAMERA_QUIT_HOTKEY].at(0) )
    {
        g_print("Exiting...\n");
        g_main_loop_quit(pVR->loopMain);
    }
    
    g_free(str);
    return TRUE;
}

// Create the recording branch elements
gboolean UvxGstPipelineVR::create_recording_branch()
{
    gchar *recFileName, *recLoc;
    g_print("Creating recording branch elements...\n");
    queue1 = gst_element_factory_make ("queue", "queue1");
    converter1 = gst_element_factory_make("nvvidconv", "converter1");
    encoder1 = gst_element_factory_make("nvv4l2h264enc", "encoder1");
    parse1 = gst_element_factory_make("h264parse", "parse1");
    muxer1 = gst_element_factory_make("mp4mux", "muxer1");
    sink1 = gst_element_factory_make("filesink", "sink1");
    if (!queue1 || !converter1 || !encoder1 || !parse1 || !muxer1 || !sink1)
    {
        g_printerr("Not all recording branch elements could be created.\n");
        return FALSE;
    }
    
    recFileName = get_rec_filename(recFileNum);
    g_object_set(encoder1, "qp-range", cfgParams[REC_QP_RANGE].c_str(), NULL);
    recLoc = g_strconcat(recLocation, recFileName, NULL);
    g_object_set(sink1, "location", recLoc, NULL);

    gst_bin_add_many(GST_BIN(pipeline), queue1, converter1, encoder1, parse1, muxer1, sink1, NULL);
    gst_element_set_state(queue1, GST_STATE_NULL);
    gst_element_set_state(converter1, GST_STATE_NULL);
    gst_element_set_state(encoder1, GST_STATE_NULL);
    gst_element_set_state(parse1, GST_STATE_NULL);
    gst_element_set_state(muxer1, GST_STATE_NULL);
    gst_element_set_state(sink1, GST_STATE_NULL);

    g_print("Linking recording branch elements...\n");
    if (gst_element_link(queue1, converter1) != TRUE)
    {
        g_printerr("Elements queue1 and converter1 could not be linked.\n");
        gst_bin_remove_many(GST_BIN(pipeline), queue1, converter1, encoder1, parse1, muxer1, sink1, NULL);
        return FALSE;
    }

    if (gst_element_link_filtered(converter1, encoder1, convrt1Caps) != TRUE)
    {
        g_printerr("Elements converter1 and encoder1 could not be linked.\n");
        gst_bin_remove_many(GST_BIN(pipeline), queue1, converter1, encoder1, parse1, muxer1, sink1, NULL);
        return FALSE;
    }

    if (gst_element_link_many(encoder1, parse1, muxer1, sink1, NULL) != TRUE)
    {
        g_printerr("Elements encoder1, parse1, muxer1 and sink1 could not be linked.\n");
        gst_bin_remove_many(GST_BIN(pipeline), queue1, converter1, encoder1, parse1, muxer1, sink1, NULL);
        return FALSE;
    }

    // Get "Request" pad from the tee element
    tee_out1 = gst_element_get_request_pad(tee, "src_%u");

    g_free(recFileName);
    g_free(recLoc);
    return TRUE;
}

// Initialize pipeline
gboolean UvxGstPipelineVR::init_pipeline()
{
    g_print("Initializing new pipeline...\n");
    init_parameters();
    g_print("Recording location set to %s\n", get_recording_location());

    // Check the available space for recording
    freeSpaceRsrvBytes = (gulong)atol(cfgParams[FREE_SPACE_RSRV_BYTES].c_str());
    update_available_space_bytes();
    if ( availRecSpaceBytes < freeSpaceRsrvBytes )
    {
        g_print("Not enough space left for recording - minimum required available space is %lu bytes\n", freeSpaceRsrvBytes);
        recEnabled = FALSE;
    }
    else
    {
        g_print("Statistics for recording location:\n");
        g_print("\tFilesystem block size = %lu\n", fsStatus.f_bsize);
        g_print("\tFree blocks = %ld\n", fsStatus.f_bfree);
        g_print("\tFree blocks for unprivileged users = %ld\n", fsStatus.f_bavail);
        g_print("Available recording space is %lu bytes\n", availRecSpaceBytes);
        recEnabled = TRUE;
    }

    // Initialize the recording file number
    init_rec_filenumber();

    // Create the elements
    g_print("Creating visualization elements...\n");
    io_stdin = g_io_channel_unix_new(fileno(stdin));
    pipeline = gst_pipeline_new(NULL);
    videosource = gst_element_factory_make("v4l2src", "videosource");
    tee = gst_element_factory_make("tee","tee");
    queue0 = gst_element_factory_make ("queue", "queue0");
    textoverlay0 = gst_element_factory_make("textoverlay", "textoverlay0");
    converter0 = gst_element_factory_make("nvvidconv", "converter0");
    sink0 = gst_element_factory_make("nvoverlaysink", "sink0");
  
    if (!pipeline || !videosource || !tee || !queue0 || !textoverlay0 || !converter0 || !sink0)
    {
        g_printerr("Not all elements could be created.\n");
        return FALSE;
    }

    // Configure elements
    gchar *caps_convrt0 = g_strconcat("video/x-raw(memory:NVMM), width=(int)", cfgParams[OVERLAY_RES_W].c_str(),
                                      ", height=(int)", cfgParams[OVERLAY_RES_H].c_str(),
                                      ", format=(string)", cfgParams[OUT_PIX_FMT].c_str(), NULL);

    gchar *caps_convrt1 = g_strconcat("video/x-raw(memory:NVMM), width=(int)", cfgParams[REC_RES_W].c_str(),
                                      ", height=(int)", cfgParams[REC_RES_H].c_str(),
                                      ", format=(string)", cfgParams[OUT_PIX_FMT].c_str(), NULL);

    g_object_set(videosource, "device", cfgParams[CAPTURE_DEV].c_str(), NULL);
    g_object_set(tee, "allow-not-linked", TRUE, NULL);
    g_object_set(textoverlay0, "text", "", "silent", TRUE, "color", (guint)atoi(cfgParams[REC_TEXT_COLOR_ARGB].c_str()),
                               "draw-shadow", FALSE, "valignment", 2, "halignment", 0,
                               "font-desc", cfgParams[REC_TEXT_FONT].c_str(), NULL);
    
    g_object_set(sink0, "overlay-x", atoi(cfgParams[OVERLAY_POS_X].c_str()), "overlay-y", atoi(cfgParams[OVERLAY_POS_Y].c_str()),
                        "overlay-w", (guint)atoi(cfgParams[OVERLAY_RES_W].c_str()), "overlay-h", (guint)atoi(cfgParams[OVERLAY_RES_H].c_str()),
                        "sync", FALSE, NULL);
    
    sourceCaps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, cfgParams[IN_PIX_FMT].c_str(),
                                     "width", G_TYPE_INT, atoi(cfgParams[REC_RES_W].c_str()),
                                     "height", G_TYPE_INT, atoi(cfgParams[REC_RES_H].c_str()), NULL);
    
    convrt0Caps = gst_caps_from_string(caps_convrt0);
    convrt1Caps = gst_caps_from_string(caps_convrt1);

    // Create the visualization branch
    gst_bin_add_many(GST_BIN(pipeline), videosource, tee, queue0, textoverlay0, converter0, sink0, NULL);

    // Link the elements in the visualization branch
    g_print("Linking visualization elements...\n");
    if (gst_element_link_filtered(videosource, tee, sourceCaps) != TRUE)
    {
        g_printerr("Elements videosource and tee could not be linked.\n");
        gst_object_unref(pipeline);
        return FALSE;
    }

    if (gst_element_link_many(queue0, textoverlay0, converter0, NULL) != TRUE)
    {
        g_printerr("Elements queue0, textoverlay0 and converter0 could not be linked.\n");
        gst_object_unref(pipeline);
        return FALSE;
    }

    if (gst_element_link_filtered(converter0, sink0, convrt0Caps) != TRUE)
    {
        g_printerr("Elements converter0 and sink0 could not be linked.\n");
        gst_object_unref(pipeline);
        return FALSE;
    }

    // Get "Request" pad from the tee element
    tee_out0 = gst_element_get_request_pad(tee, "src_%u");

    // Get sink pad from the queue element
    queue0_in = gst_element_get_static_pad(queue0, "sink");

    // Link the first source pad of the tee element with its respective queue pad - this completes the visualization pipeline branch
    if (gst_pad_link(tee_out0, queue0_in) != GST_PAD_LINK_OK)
    {
        g_printerr("Tee source pad0 could not be linked.\n");
        gst_object_unref(pipeline);
        return FALSE;
    }

    // Get pad for PTS offset while recording
    queue0_out = gst_element_get_static_pad(queue0, "src");

    // Free temporary arrays
    g_free(caps_convrt0);
    g_free(caps_convrt1);

    // Create streaming branch and exit
    return create_streaming_branch();
}

// Start pipeline
gboolean UvxGstPipelineVR::start_pipeline()
{
    g_print("Starting pipeline...\n");

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return FALSE;
    }
    else if (ret == GST_STATE_CHANGE_NO_PREROLL)
    {
        isLivestream = TRUE;
        g_print("Livestream detected!\n");
    }

    loopMain = g_main_loop_new(NULL, FALSE);

    // Add a bus watch
    gst_bus_add_watch(GST_ELEMENT_BUS(pipeline), (GstBusFunc)(bus_watch_cb), this);

    // Add a watch for handling input commands comming through stdin
    g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_input_commands, this);

    // Start main loop
    g_main_loop_run(loopMain);

    return TRUE;
}

// Free resources
void UvxGstPipelineVR::unref_pipeline()
{
    gst_caps_unref(sourceCaps);
    gst_caps_unref(convrt0Caps);
    gst_caps_unref(convrt1Caps);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_bus_remove_watch(GST_ELEMENT_BUS(pipeline));

    gst_element_release_request_pad(tee, tee_out0);
    gst_object_unref(queue0_in);
    gst_object_unref(queue0_out);
    gst_object_unref(tee_out0);

    unref_streaming_branch();
    
    gst_object_unref(pipeline);
    g_main_loop_unref(loopMain);
    g_io_channel_unref(io_stdin);
    delete[] recFileNumFmt;
}

// Update the available disk space at the recording location
void UvxGstPipelineVR::update_available_space_bytes()
{
    if (statvfs(recLocation, &(fsStatus)) != 0)
    {
        g_print("Failed getting available space at %s\n", recLocation);
        availRecSpaceBytes = 0;
        return;
    }

    // The available space is f_bsize * f_bavail in bytes
    availRecSpaceBytes = fsStatus.f_bsize * fsStatus.f_bavail;
}

// Update the recording enabled flag
void UvxGstPipelineVR::update_rec_enabled()
{
    update_available_space_bytes();
    if ( availRecSpaceBytes < freeSpaceRsrvBytes )
        recEnabled = FALSE;
    else
        recEnabled = TRUE;
}

// Get a recording filename with a specified file number
gchar* UvxGstPipelineVR::get_rec_filename(guint fileNum)
{
    gchar* temp = g_strdup_printf(recFileNumFmt, fileNum);
    gchar* ret = g_strconcat(cfgParams[REC_FILENAME_PREFIX].c_str(), temp, ".mp4", NULL);
    g_free(temp);
    return ret;
}

// Get a recording filename as a regular expression
gchar* UvxGstPipelineVR::get_rec_filename()
{
    return g_strconcat(cfgParams[REC_FILENAME_PREFIX].c_str(), "([0-9]+)\\.mp4", NULL);
}

// Initialize the recording file number
void UvxGstPipelineVR::init_rec_filenumber()
{
    GDir *recDir;
    gchar *temp;
    const gchar *filename;
    guint fnum_max = 0;

    g_print("Initializing recording file number...\n");
    recDir = g_dir_open(get_recording_location(), 0, NULL);

    if (!recDir)
    {
        g_print("Error opening recording location %s\n", get_recording_location());
    }
    else
    {
        GRegex *regex_match;
        gchar *reg_pattern = get_rec_filename();
        regex_match = g_regex_new(reg_pattern, (GRegexCompileFlags)0, (GRegexMatchFlags)0, NULL);

        while ( ( filename = g_dir_read_name(recDir) ) )
        {
            // Go through each file in the recording location and check it for regex matching
            if ( g_regex_match(regex_match, filename, (GRegexMatchFlags)0, NULL) )
            {
                // Match found - get the file number
                gchar **part = g_regex_split(regex_match, filename, (GRegexMatchFlags)0);
                guint fnum = atoi(part[1]);

                if ( fnum > fnum_max )
                    fnum_max = fnum;

                g_strfreev(part);
            }
        }
        
        g_regex_unref(regex_match);
        g_free(reg_pattern);
        g_dir_close(recDir);
    }

    recFileNum = fnum_max + 1;

    // Set the file number display format
    if ( recFileNum > 999 )
    {
        recFileNumFmt = new gchar[3];
        strcpy(recFileNumFmt, "%d");
    }
    else
    {
        recFileNumFmt = new gchar[5];
        strcpy(recFileNumFmt, "%03d");
    }

    temp = g_strdup_printf(recFileNumFmt, recFileNum);
    g_print("Format specifier set to %s\n", recFileNumFmt);
    g_print("Recording file number initialized to %s\n", temp);
    g_free(temp);
}

// Initialize the configuration parameters
void UvxGstPipelineVR::init_parameters()
{
    fstream gstSetupFile;
    string line;
    string parameter;
    string value;
    size_t pequals, pquoute;
    map<string,int> M;

    // Create the default parameters map
    cfgParams[CAPTURE_DEV] = "/dev/video0";
    cfgParams[CAMERA_START_REC_HOTKEY] = "r";
    cfgParams[CAMERA_STOP_REC_HOTKEY] = "s";
    cfgParams[CAMERA_QUIT_HOTKEY] = "q";
    cfgParams[IN_PIX_FMT] = "UYVY";
    cfgParams[OUT_PIX_FMT] = "I420";
    cfgParams[STREAMING_ENABLED] = "FALSE";
    cfgParams[STREAM_QP_RANGE] = "35,35:35,35:-1,-1";
    cfgParams[STREAM_MTU] = "60000";
    cfgParams[STREAM_RES_W] = "640";
    cfgParams[STREAM_RES_H] = "480";
    cfgParams[HOST_IP] = "192.168.88.91";
    cfgParams[HOST_PORT] = "5000";
    cfgParams[REC_RES_W] = "1920";
    cfgParams[REC_RES_H] = "1080";
    cfgParams[REC_QP_RANGE] = "20,20:20,20:-1,-1";
    cfgParams[REC_FILENAME_PREFIX] = "UVX-";
    cfgParams[REC_FILE_SIZE_BYTES] = "1073741824";
    cfgParams[FREE_SPACE_RSRV_BYTES] = "1073741824";
    cfgParams[REC_TEXT_COLOR_ARGB] = "4294901760";
    cfgParams[REC_TEXT_FONT] = "Sans, 6";
    cfgParams[OVERLAY_RES_W] = "1280";
    cfgParams[OVERLAY_RES_H] = "720";
    cfgParams[OVERLAY_POS_X] = "0";
    cfgParams[OVERLAY_POS_Y] = "0";
    cfgParams[FREESPACE_WDOG_TIMOUT_S] = "20";

    // Create the map from the parameter names to their internal enumeration representation
    M["CAPTURE_DEV"] = CAPTURE_DEV;
    M["CAMERA_START_REC_HOTKEY"] = CAMERA_START_REC_HOTKEY;
    M["CAMERA_STOP_REC_HOTKEY"] = CAMERA_STOP_REC_HOTKEY;
    M["CAMERA_QUIT_SERVICE_HOTKEY"] = CAMERA_QUIT_HOTKEY;
    M["IN_PIX_FMT"] = IN_PIX_FMT;
    M["OUT_PIX_FMT"] = OUT_PIX_FMT;
    M["STREAMING_ENABLED"] = STREAMING_ENABLED;
    M["STREAM_QP_RANGE"] = STREAM_QP_RANGE;
    M["STREAM_MTU"] = STREAM_MTU;
    M["STREAM_RES_W"] = STREAM_RES_W;
    M["STREAM_RES_H"] = STREAM_RES_H;
    M["HOST_IP"] = HOST_IP;
    M["HOST_PORT"] = HOST_PORT;
    M["REC_RES_W"] = REC_RES_W;
    M["REC_RES_H"] = REC_RES_H;
    M["REC_QP_RANGE"] = REC_QP_RANGE;
    M["REC_FILENAME_PREFIX"] = REC_FILENAME_PREFIX;
    M["REC_FILE_SIZE_BYTES"] = REC_FILE_SIZE_BYTES;
    M["FREE_SPACE_RSRV_BYTES"] = FREE_SPACE_RSRV_BYTES;
    M["REC_TEXT_COLOR_ARGB"] = REC_TEXT_COLOR_ARGB;
    M["REC_TEXT_FONT"] = REC_TEXT_FONT;
    M["OVERLAY_RES_W"] = OVERLAY_RES_W;
    M["OVERLAY_RES_H"] = OVERLAY_RES_H;
    M["OVERLAY_POS_X"] = OVERLAY_POS_X;
    M["OVERLAY_POS_Y"] = OVERLAY_POS_Y;
    M["FREESPACE_WDOG_TIMOUT_S"] = FREESPACE_WDOG_TIMOUT_S;

    if (!cfgParamsFile)
    {
        g_print("No configuration parameters file specified. Default parameter set will be used!\n");
        goto exit;
    }

    // Process the configuration parameters file
    g_print("Configuration parameters file set to %s. Processing...\n", cfgParamsFile);
    gstSetupFile.open(cfgParamsFile, ios::in);

    if ( !gstSetupFile.is_open() )
    {
        g_printerr("Failed opening configuration file %s. Default parameter set will be used!\n", cfgParamsFile);
        goto exit;
    }

    while ( getline(gstSetupFile, line) )
    {
        try {
            if ( line.at(0) == '#')
                continue; // Skip comments

            pequals = line.find('=');
            if ( (pequals + 3) > line.length() )
                continue; // Skip invalid lines

            if ( line.at(pequals+1) != '"' )
                continue; // Skip invalid lines - the equals sign must be followed by a quoute

            // Find the position of the second quoute
            pquoute = line.find('"',pequals+2);
            if ( ((pquoute + 1) > line.length()) || (pquoute < (pequals + 2)) )
                continue; // Skip invalid lines

            // Get the parameter
            parameter = line.substr(0, pequals);

            // Search for the parameter in the map
            if ( M.find(parameter) == M.end() )
                continue; // Parameter not found in map
            
            // Get the number of characters for the value
            size_t numch = pquoute - (pequals + 2);
            if ( numch > 0 )
            {
                // Set the value of the parameter
                value = line.substr(pequals+2, numch);
                cfgParams[M[parameter]] = value;
                // g_print("Parameter %s set to %s\n", parameter.c_str(), cfgParams[M[parameter]].c_str());
            }
            // else -> the parameter value is empty
        } catch ( std::out_of_range ) {
            // Skip the eronous line
            continue;
        }
    }
    
    gstSetupFile.close();

exit:
    // Print the resulting configuration parameters
    auto it = M.begin();
    while (it != M.end())
    {
        g_print("\tParameter %s set to %s\n", it->first.c_str(), cfgParams[it->second].c_str());
        it++;
    }
}
