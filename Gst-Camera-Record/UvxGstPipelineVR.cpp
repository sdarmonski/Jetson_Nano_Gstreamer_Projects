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
        if ( pVR->availRecSpaceBytes < FREE_SPACE_RSRV_BYTES )
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
        pVR->freespaceWatchdogPid = g_timeout_add_seconds(FREESPACE_WDOG_TIMOUT_S, (GSourceFunc)freespace_watchdog, (gpointer)pVR);
        pVR->recState = REC_STARTED;
        pVR->recInitPts = GST_BUFFER_PTS(buffer);
    }

    // Calculate and display the recording time
    recTime = GST_BUFFER_PTS(buffer) - pVR->recInitPts;
    HH = (guint)(recTime / ((GstClockTime)3600000000000));
    MM = ((guint)(recTime / ((GstClockTime)60000000000))) % ((guint)3600);
    SS = ((guint)(recTime / ((GstClockTime)1000000000))) % ((guint)60);

    g_object_set(pVR->textoverlay0, "text", g_strconcat("REC", g_strdup_printf("%d", pVR->recFileNum), " " , g_strdup_printf("%02d", HH), ":", g_strdup_printf("%02d", MM), ":", g_strdup_printf("%02d", SS), NULL), NULL);

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
            g_print("EOS sent.\n");
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

    switch (g_ascii_tolower(str[0]))
    {
        case 'r':
            pVR->start_recording();
            break;
        case 's':
            pVR->stop_recording();
            break;
        case 'q':
            g_print("Exiting...\n");
            g_main_loop_quit(pVR->loopMain);
            break;
        default:
            break;
    }
    
    g_free(str);
    return TRUE;
}

// Create the recording branch elements
gboolean UvxGstPipelineVR::create_recording_branch()
{
    gchar *recFileName;
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
    
    recFileName = g_strconcat(REC_FILENAME_PREFIX, g_strdup_printf("%03d", recFileNum), ".mp4", NULL);
    g_object_set(encoder1, "qp-range", REC_QP_RANGE, NULL);
    g_object_set(sink1, "location", g_strconcat(recLocation, recFileName, NULL), NULL);

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
    return TRUE;
}

// Initialize pipeline
gboolean UvxGstPipelineVR::init_pipeline()
{
    g_print("Initializing new pipeline...\n");

    // Check the available space for recording
    update_available_space_bytes();
    if ( availRecSpaceBytes < FREE_SPACE_RSRV_BYTES )
    {
        g_print("Not enough space left for recording at location %s - minimum required available space is %lu bytes. Recording disabled!\n",
                recLocation, FREE_SPACE_RSRV_BYTES);
        recEnabled = FALSE;
    }
    else
    {
        g_print("Statistics for recording location %s\n", recLocation);
        g_print("\tFilesystem block size = %lu\n", fsStatus.f_bsize);
        g_print("\tFree blocks = %ld\n", fsStatus.f_bfree);
        g_print("\tFree blocks for unprivileged users = %ld\n", fsStatus.f_bavail);
        g_print("Available recording space at %s is %lu bytes. Recording is enabled!\n", recLocation, availRecSpaceBytes);
        recEnabled = TRUE;
    }

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
    g_object_set(videosource, "device", CAPTURE_DEV, NULL);
    g_object_set(tee, "allow-not-linked", TRUE, NULL);
    g_object_set(textoverlay0, "text", "", "silent", TRUE, "color", REC_TEXT_COLOR_ARGB, "draw-shadow", FALSE, "valignment", 2, "halignment", 0, "font-desc", REC_TEXT_FONT, NULL);
    g_object_set(sink0, "overlay-x", OVERLAY_POS_X, "overlay-y", OVERLAY_POS_Y, "overlay-w", OVERLAY_RES_W, "overlay-h", OVERLAY_RES_H, "sync", FALSE, NULL);
    sourceCaps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, IN_PIX_FMT, "width", G_TYPE_INT, REC_RES_W, "height", G_TYPE_INT, REC_RES_H, NULL);
    convrt0Caps = gst_caps_from_string(g_strconcat("video/x-raw(memory:NVMM), width=(int)", g_strdup_printf("%d", OVERLAY_RES_W),
                                                    ", height=(int)", g_strdup_printf("%d", OVERLAY_RES_H),
                                                    ", format=(string)", OUT_PIX_FMT, NULL));    
    convrt1Caps = gst_caps_from_string(g_strconcat("video/x-raw(memory:NVMM), width=(int)", g_strdup_printf("%d", REC_RES_W),
                                                    ", height=(int)", g_strdup_printf("%d", REC_RES_H),
                                                    ", format=(string)", OUT_PIX_FMT, NULL));

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
    if ( availRecSpaceBytes < FREE_SPACE_RSRV_BYTES )
        recEnabled = FALSE;
    else
        recEnabled = TRUE;
}
