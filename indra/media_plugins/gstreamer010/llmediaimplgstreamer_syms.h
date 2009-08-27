/** 
 * @file llmediaimplgstreamer_syms.h
 * @brief dynamic GStreamer symbol-grabbing code
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
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
