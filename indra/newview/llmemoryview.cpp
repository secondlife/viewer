/** 
 * @file llmemoryview.cpp
 * @brief LLMemoryView class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
 */

#include "llviewerprecompiledheaders.h"

#include "llmemoryview.h"

#include "llappviewer.h"
#include "llallocator_heap_profile.h"
#include "llgl.h"						// LLGLSUIDefault
#include "llviewerwindow.h"
#include "llviewercontrol.h"

#include <sstream>
#include <boost/algorithm/string/split.hpp>

#include "llmemory.h"

LLMemoryView::LLMemoryView(const LLMemoryView::Params& p)
:	LLView(p),
	mPaused(FALSE),
	//mDelay(120),
    mAlloc(NULL)
{
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
		mPaused = !mPaused;
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

void LLMemoryView::refreshProfile()
{
	/*
    LLAllocator & alloc = LLAppViewer::instance()->getAllocator();
    if(alloc.isProfiling()) {
        std::string profile_text = alloc.getRawProfile();

        boost::algorithm::split(mLines, profile_text, boost::bind(std::equal_to<llwchar>(), '\n', _1));
    } else {
        mLines.clear();
    }
	*/
    if (mAlloc == NULL) {
        mAlloc = &LLAppViewer::instance()->getAllocator();
    }

	mLines.clear();

 	if(mAlloc->isProfiling()) 
	{
		const LLAllocatorHeapProfile &prof = mAlloc->getProfile();
		for(size_t i = 0; i < prof.mLines.size(); ++i)
		{
			std::stringstream ss;
			ss << "Unfreed Mem: " << (prof.mLines[i].mLiveSize >> 20) << " M     Trace: ";
			for(size_t k = 0; k < prof.mLines[i].mTrace.size(); ++k)
			{
				ss << LLMemType::getNameFromID(prof.mLines[i].mTrace[k]) << "  ";
			}
			mLines.push_back(utf8string_to_wstring(ss.str()));
		}
	}
}

void LLMemoryView::draw()
{
	const S32 UPDATE_INTERVAL = 60;
	const S32 MARGIN_AMT = 10; 
	static S32 curUpdate = UPDATE_INTERVAL;
    static LLUIColor s_console_color = LLUIColorTable::instance().getColor("ConsoleBackground", LLColor4U::black);	

	// setup update interval
	if (curUpdate >= UPDATE_INTERVAL)
	{
		refreshProfile();
		curUpdate = 0;
	}
	curUpdate++;

	// setup window properly
	S32 height = (S32) (gViewerWindow->getWindowRectScaled().getHeight()*0.75f);
	S32 width = (S32) (gViewerWindow->getWindowRectScaled().getWidth() * 0.9f);
	setRect(LLRect().setLeftTopAndSize(getRect().mLeft, getRect().mTop, width, height));
	
	// setup window color
	F32 console_opacity = llclamp(gSavedSettings.getF32("ConsoleBackgroundOpacity"), 0.f, 1.f);
	LLColor4 color = s_console_color;
	color.mV[VALPHA] *= console_opacity;

	LLGLSUIDefault gls_ui;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gl_rect_2d(0, height, width, 0, color);
	
    LLFontGL * font = LLFontGL::getFontSansSerifSmall(); 

	// draw remaining lines
	F32 y_pos = 0.f;
    F32 y_off = 0.f;

	F32 line_height = font->getLineHeight();
    S32 target_width = width - 2 * MARGIN_AMT;

	// cut off lines on bottom
	U32 max_lines = U32((height - 2 * line_height) / line_height);
	y_pos = height - MARGIN_AMT - line_height;
    y_off = 0.f;

#if !MEM_TRACK_MEM
	std::vector<LLWString>::const_iterator end = mLines.end();
    if(mLines.size() > max_lines) {
        end = mLines.begin() + max_lines;
    }
    for (std::vector<LLWString>::const_iterator i = mLines.begin(); i != end; ++i)
	{
		font->render(*i, 0, MARGIN_AMT, y_pos -  y_off,
            LLColor4::white,
			LLFontGL::LEFT, 
			LLFontGL::BASELINE,
            LLFontGL::NORMAL,
			LLFontGL::DROP_SHADOW,
			S32_MAX,
			target_width
			);
		y_off += line_height;
	}

#else
	LLMemTracker::getInstance()->preDraw(mPaused) ;

	{
		F32 x_pos = MARGIN_AMT ;
		U32 lines = 0 ;
		const char* str = LLMemTracker::getInstance()->getNextLine() ;
		while(str != NULL)
		{
			lines++ ;
			font->renderUTF8(str, 0, x_pos, y_pos -  y_off,
				LLColor4::white,
				LLFontGL::LEFT, 
				LLFontGL::BASELINE,
				LLFontGL::NORMAL,
				LLFontGL::DROP_SHADOW,
				S32_MAX,
				target_width,
				NULL, FALSE);
		
			str = LLMemTracker::getInstance()->getNextLine() ;
			y_off += line_height;

			if(lines >= max_lines)
			{
				lines = 0 ;
				x_pos += 512.f ;
				if(x_pos + 512.f > target_width)
				{
					break ;
				}

				y_pos = height - MARGIN_AMT - line_height;
				y_off = 0.f;
			}
		}
	}

	LLMemTracker::getInstance()->postDraw() ;
#endif

#if MEM_TRACK_TYPE

	S32 left, top, right, bottom;
	S32 x, y;

	S32 margin = 10;
	S32 texth = LLFontGL::getFontMonospace()->getLineHeight();

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
			LLFontGL::getFontMonospace()->renderUTF8(tdesc, 0, x, y, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);
			
			y -= (texth + 2);

			S32 textw = LLFontGL::getFontMonospace()->getWidth(tdesc);
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
		LLFontGL::getFontMonospace()->renderUTF8(tdesc, 0, x, y, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);
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
