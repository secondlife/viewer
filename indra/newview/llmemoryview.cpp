/** 
 * @file llmemoryview.cpp
 * @brief LLMemoryView class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "indra_constants.h"
#include "llmemoryview.h"

#include "llrect.h"
#include "llerror.h"
#include "llgl.h"
#include "llmath.h"
#include "llfontgl.h"
#include "llmemtype.h"

#include "llcharacter.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llstat.h"

#include "llfasttimer.h"



LLMemoryView::LLMemoryView(const std::string& name, const LLRect& rect)
:	LLView(name, rect, TRUE),
mDelay(120)
{
	setVisible(FALSE);
	mDumpTimer.reset();

#ifdef MEM_DUMP_DATA
	// clear out file.
	LLFILE *dump = LLFile::fopen("memusagedump.txt", "w");
	fclose(dump);
#endif
}

LLMemoryView::~LLMemoryView()
{
}

BOOL LLMemoryView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (mask & MASK_SHIFT)
	{
	}
	else if (mask & MASK_CONTROL)
	{
	}
	else
	{
	}
	return TRUE;
}

BOOL LLMemoryView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	return TRUE;
}


BOOL LLMemoryView::handleHover(S32 x, S32 y, MASK mask)
{
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

struct mtv_display_info {
	S32 memtype;
	const char *desc;
	const LLColor4 *color;
};

static const LLColor4 red0(0.5f, 0.0f, 0.0f, 1.0f);

static const struct mtv_display_info mtv_display_table[] =
{
	{ LLMemType::MTYPE_INIT,			"Init", 			&LLColor4::white },
	{ LLMemType::MTYPE_STARTUP,			"Startup", 			&LLColor4::cyan1 },
	{ LLMemType::MTYPE_MAIN,			"Main", 			&LLColor4::cyan2 },
	{ LLMemType::MTYPE_IMAGEBASE,		"ImageBase",		&LLColor4::yellow1 },
	{ LLMemType::MTYPE_IMAGERAW,		"ImageRaw", 		&LLColor4::yellow2 },
	{ LLMemType::MTYPE_IMAGEFORMATTED,	"ImageFmtd",		&LLColor4::yellow3 },
	{ LLMemType::MTYPE_APPFMTIMAGE,		"ViewerImageFmt",	&LLColor4::orange1 },
	{ LLMemType::MTYPE_APPRAWIMAGE,		"ViewerImageRaw",	&LLColor4::orange2 },
	{ LLMemType::MTYPE_APPAUXRAWIMAGE,	"ViewerImageAux",	&LLColor4::orange3 },
	{ LLMemType::MTYPE_DRAWABLE,		"Drawable", 		&LLColor4::green1 },
	{ LLMemType::MTYPE_OBJECT,			"ViewerObject",		&LLColor4::green2 },
	{ LLMemType::MTYPE_PIPELINE,		"Pipeline",			&LLColor4::green3 },
	{ LLMemType::MTYPE_PARTICLES,		"Particles",		&LLColor4::green4 },
	{ LLMemType::MTYPE_SPACE_PARTITION,	"Space Partition",	&LLColor4::blue2 },
	{ LLMemType::MTYPE_VERTEX_DATA,		"Vertex Buffer",	&LLColor4::blue3 },
	{ LLMemType::MTYPE_AVATAR,			"Avatar",			&LLColor4::purple1 },
	{ LLMemType::MTYPE_AVATAR_MESH,		"Avatar Mesh",		&LLColor4::purple2 },
	{ LLMemType::MTYPE_ANIMATION,		"Animation",		&LLColor4::purple3 },
	{ LLMemType::MTYPE_REGIONS,			"Regions",			&LLColor4::blue1 },
	{ LLMemType::MTYPE_VOLUME,			"Volume",			&LLColor4::pink1 },
	{ LLMemType::MTYPE_PRIMITIVE,		"Profile",			&LLColor4::pink2 },
 	{ LLMemType::MTYPE_TEMP1,			"Temp1",			&LLColor4::red1 },
 	{ LLMemType::MTYPE_TEMP2,			"Temp2",			&LLColor4::magenta1 },
 	{ LLMemType::MTYPE_TEMP3,			"Temp3",			&LLColor4::red2 },
 	{ LLMemType::MTYPE_TEMP4,			"Temp4",			&LLColor4::magenta2 },
 	{ LLMemType::MTYPE_TEMP5,			"Temp5",			&LLColor4::red3 },
 	{ LLMemType::MTYPE_TEMP6,			"Temp6",			&LLColor4::magenta3 },
 	{ LLMemType::MTYPE_TEMP7,			"Temp7",			&LLColor4::red4 },
 	{ LLMemType::MTYPE_TEMP8,			"Temp8",			&LLColor4::magenta4 },

 	{ LLMemType::MTYPE_OTHER,			"Other",			&red0 },
};
static const int MTV_DISPLAY_NUM  = (sizeof(mtv_display_table)/sizeof(mtv_display_table[0]));

void LLMemoryView::draw()
{
	std::string tdesc;
	S32 width = getRect().getWidth();
	S32 height = getRect().getHeight();
	
	LLGLSUIDefault gls_ui;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gl_rect_2d(0, height, width, 0, LLColor4(0.f, 0.f, 0.f, 0.25f));
	
#if MEM_TRACK_TYPE

	S32 left, top, right, bottom;
	S32 x, y;

	S32 margin = 10;
	S32 texth = (S32)LLFontGL::sMonospace->getLineHeight();

	S32 xleft = margin;
	S32 ytop = height - margin;
	S32 labelwidth = 0;
	S32 maxmaxbytes = 1;

	// Make sure all timers are accounted for
	// Set 'MT_OTHER' to unaccounted ticks last frame
	{
		S32 display_memtypes[LLMemType::MTYPE_NUM_TYPES];
		for (S32 i=0; i < LLMemType::MTYPE_NUM_TYPES; i++)
		{
			display_memtypes[i] = 0;
		}
		for (S32 i=0; i < MTV_DISPLAY_NUM; i++)
		{
			S32 tidx = mtv_display_table[i].memtype;
			display_memtypes[tidx]++;
		}
		LLMemType::sMemCount[LLMemType::MTYPE_OTHER] = 0;
		LLMemType::sMaxMemCount[LLMemType::MTYPE_OTHER] = 0;
		for (S32 tidx = 0; tidx < LLMemType::MTYPE_NUM_TYPES; tidx++)
		{
			if (display_memtypes[tidx] == 0)
			{
				LLMemType::sMemCount[LLMemType::MTYPE_OTHER] += LLMemType::sMemCount[tidx];
				LLMemType::sMaxMemCount[LLMemType::MTYPE_OTHER] += LLMemType::sMaxMemCount[tidx];
			}
		}
	}
	
	// Labels
	{
		y = ytop;
		S32 peak = 0;
		for (S32 i=0; i<MTV_DISPLAY_NUM; i++)
		{
			x = xleft;

			int tidx = mtv_display_table[i].memtype;
			S32 bytes = LLMemType::sMemCount[tidx];
			S32 maxbytes = LLMemType::sMaxMemCount[tidx];
			maxmaxbytes = llmax(maxbytes, maxmaxbytes);
			peak += maxbytes;
			S32 mbytes = bytes >> 20;

			tdesc = llformat("%s [%4d MB] in %06d NEWS",mtv_display_table[i].desc,mbytes, LLMemType::sNewCount[tidx]);
			LLFontGL::sMonospace->renderUTF8(tdesc, 0, x, y, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);
			
			y -= (texth + 2);

			S32 textw = LLFontGL::sMonospace->getWidth(tdesc);
			if (textw > labelwidth)
				labelwidth = textw;
		}

		S32 num_avatars = 0;
		S32 num_motions = 0;
		S32 num_loading_motions = 0;
		S32 num_loaded_motions = 0;
		S32 num_active_motions = 0;
		S32 num_deprecated_motions = 0;
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
			 iter != LLCharacter::sInstances.end(); ++iter)
		{
			num_avatars++;
			(*iter)->getMotionController().incMotionCounts(num_motions, num_loading_motions, num_loaded_motions, num_active_motions, num_deprecated_motions);
		}
		
		x = xleft;
		tdesc = llformat("Total Bytes: %d MB Overhead: %d KB Avs %d Motions:%d Loading:%d Loaded:%d Active:%d Dep:%d",
						 LLMemType::sTotalMem >> 20, LLMemType::sOverheadMem >> 10,
						 num_avatars, num_motions, num_loading_motions, num_loaded_motions, num_active_motions, num_deprecated_motions);
		LLFontGL::sMonospace->renderUTF8(tdesc, 0, x, y, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);
	}

	// Bars
	y = ytop;
	labelwidth += 8;
	S32 barw = width - labelwidth - xleft - margin;
	for (S32 i=0; i<MTV_DISPLAY_NUM; i++)
	{
		x = xleft + labelwidth;

		int tidx = mtv_display_table[i].memtype;
		S32 bytes = LLMemType::sMemCount[tidx];
		F32 frac = (F32)bytes / (F32)maxmaxbytes;
		S32 w = (S32)(frac * (F32)barw);
		left = x; right = x + w;
		top = y; bottom = y - texth;		
		gl_rect_2d(left, top, right, bottom, *mtv_display_table[i].color);

		S32 maxbytes = LLMemType::sMaxMemCount[tidx];
		F32 frac2 = (F32)maxbytes / (F32)maxmaxbytes;
		S32 w2 = (S32)(frac2 * (F32)barw);
		left = x + w + 1; right = x + w2;
		top = y; bottom = y - texth;
		LLColor4 tcolor = *mtv_display_table[i].color;
		tcolor.setAlpha(.5f);
		gl_rect_2d(left, top, right, bottom, tcolor);
		
		y -= (texth + 2);
	}

	dumpData();

#endif
	
	LLView::draw();
}

void LLMemoryView::setDataDumpInterval(float delay)
{
	mDelay = delay;
}

void LLMemoryView::dumpData()
{
#if MEM_TRACK_TYPE && MEM_DUMP_DATA
	if (mDelay && (mDumpTimer.getElapsedTimeF32() > mDelay ))
	{
		// reset timer
		mDumpTimer.reset();
		// append dump info to text file
		LLFILE *dump = LLFile::fopen("memusagedump.txt", "a");

		if (dump)
		{
			// write out total memory usage
			fprintf (dump, "Total memory in use = %09d (%03d MB)\n", LLMemType::sTotalMem, LLMemType::sTotalMem>>20);
			fprintf (dump, "High Water Mark = %09d (%03d MB)\n\n", LLMemType::sMaxTotalMem, LLMemType::sMaxTotalMem>>20);
			// dump out usage of 'new' for each memory type
			for (S32 i=0; i<LLMemType::MTYPE_NUM_TYPES; i++)
			{
				if (LLMemType::sMemCount[i])
				{
					std::string outData = llformat("MEM: % 20s %09d %03d MB (%09d %03d MB) in %06d News", LLMemType::sTypeDesc[i], LLMemType::sMemCount[i], LLMemType::sMemCount[i]>>20, LLMemType::sMaxMemCount[i], LLMemType::sMaxMemCount[i]>>20, LLMemType::sNewCount[i]);
					fprintf (dump, "%s\n", outData.c_str());
				}
			}
			fprintf (dump, "\n\n");

			fclose(dump);
		}
	}
#endif
}
