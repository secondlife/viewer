/**
 * @file llmediaimplgstreamervidplug.h
 * @brief Video-consuming static GStreamer plugin for gst-to-LLMediaImpl
 *
 * @cond
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
