/**
 * @file llmediaimplgstreamervidplug.h
 * @brief Video-consuming static GStreamer plugin for gst-to-LLMediaImpl
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

#ifndef __GST_SLVIDEO_H__
#define __GST_SLVIDEO_H__

#if LL_GSTREAMER010_ENABLED

extern "C" {
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideosink.h>
}

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_SLVIDEO \
  (gst_slvideo_get_type())
#define GST_SLVIDEO(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SLVIDEO,GstSLVideo))
#define GST_SLVIDEO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SLVIDEO,GstSLVideoClass))
#define GST_IS_SLVIDEO(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SLVIDEO))
#define GST_IS_SLVIDEO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SLVIDEO))

typedef struct _GstSLVideo      GstSLVideo;
typedef struct _GstSLVideoClass GstSLVideoClass;

typedef enum {
    SLV_PF_UNKNOWN = 0,
    SLV_PF_RGBX    = 1,
    SLV_PF_BGRX    = 2,
    SLV__END       = 3
} SLVPixelFormat;
const int SLVPixelFormatBytes[SLV__END] = {1, 4, 4};

struct _GstSLVideo
{
    GstVideoSink video_sink;

    GstCaps *caps;

    int fps_n, fps_d;
    int par_n, par_d;
    int height, width;
    SLVPixelFormat format;

    // SHARED WITH APPLICATION:
    // Access to the following should be protected by GST_OBJECT_LOCK() on
    // the GstSLVideo object, and should be totally consistent upon UNLOCK
    // (i.e. all written at once to reflect the current retained frame info
    // when the retained frame is updated.)
    bool retained_frame_ready; // new frame ready since flag last reset. (*TODO: could get the writer to wait on a semaphore instead of having the reader poll, potentially making dropped frames somewhat cheaper.)
    unsigned char*  retained_frame_data;
    int retained_frame_allocbytes;
    int retained_frame_width, retained_frame_height;
    SLVPixelFormat retained_frame_format;
    // sticky resize info
    bool resize_forced_always;
    int resize_try_width;
    int resize_try_height;
};

struct _GstSLVideoClass 
{
    GstVideoSinkClass parent_class;
};

GType gst_slvideo_get_type (void);

void gst_slvideo_init_class (void);

G_END_DECLS

#endif // LL_GSTREAMER010_ENABLED

#endif /* __GST_SLVIDEO_H__ */
