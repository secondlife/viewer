/** 
 * @file llmediaimplgstreamer.h
 * @author Tofu Linden
 * @brief implementation that supports media playback via GStreamer.
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

// header guard
#ifndef llmediaimplgstreamer_h
#define llmediaimplgstreamer_h

#if LL_GSTREAMER010_ENABLED

extern "C" {
#include <stdio.h>
#include <gst/gst.h>

#include "apr_pools.h"
#include "apr_dso.h"
}


extern "C" {
gboolean llmediaimplgstreamer_bus_callback (GstBus     *bus,
					    GstMessage *message,
					    gpointer    data);
}

#endif // LL_GSTREAMER010_ENABLED

#endif // llmediaimplgstreamer_h
