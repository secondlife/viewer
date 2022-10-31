/** 
 * @file llmediaimplgstreamer_syms.h
 * @brief dynamic GStreamer symbol-grabbing code
 *
 * @cond
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 * @endcond
 */

#include "linden_common.h"

#if LL_GSTREAMER010_ENABLED

extern "C" {
#include <gst/gst.h>
}

bool grab_gst_syms(std::string gst_dso_name,
           std::string gst_dso_name_vid);
void ungrab_gst_syms();

#define LL_GST_SYM(REQ, GSTSYM, RTN, ...) extern RTN (*ll##GSTSYM)(__VA_ARGS__)
#include "llmediaimplgstreamer_syms_raw.inc"
#include "llmediaimplgstreamer_syms_rawv.inc"
#undef LL_GST_SYM

// regrettable hacks to give us better runtime compatibility with older systems
#define llg_return_if_fail(COND) do{if (!(COND)) return;}while(0)
#define llg_return_val_if_fail(COND,V) do{if (!(COND)) return V;}while(0)

// regrettable hacks because GStreamer was not designed for runtime loading
#undef GST_TYPE_MESSAGE
#define GST_TYPE_MESSAGE (llgst_message_get_type())
#undef GST_TYPE_OBJECT
#define GST_TYPE_OBJECT (llgst_object_get_type())
#undef GST_TYPE_PIPELINE
#define GST_TYPE_PIPELINE (llgst_pipeline_get_type())
#undef GST_TYPE_ELEMENT
#define GST_TYPE_ELEMENT (llgst_element_get_type())
#undef GST_TYPE_VIDEO_SINK
#define GST_TYPE_VIDEO_SINK (llgst_video_sink_get_type())
// more regrettable hacks to stub-out these .h-exposed GStreamer internals
void ll_gst_debug_register_funcptr(GstDebugFuncPtr func, gchar* ptrname);
#undef _gst_debug_register_funcptr
#define _gst_debug_register_funcptr ll_gst_debug_register_funcptr
GstDebugCategory* ll_gst_debug_category_new(gchar *name, guint color, gchar *description);
#undef _gst_debug_category_new
#define _gst_debug_category_new ll_gst_debug_category_new
#undef __gst_debug_enabled
#define __gst_debug_enabled (0)

// more hacks
#define LLGST_MESSAGE_TYPE_NAME(M) (llgst_message_type_get_name(GST_MESSAGE_TYPE(M)))

#endif // LL_GSTREAMER010_ENABLED
