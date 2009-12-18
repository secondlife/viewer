/** 
 * @file lltextureview.cpp
 * @brief LLTextureView class implementation
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

#include <set>

#include "lltextureview.h"

#include "llrect.h"
#include "llerror.h"
#include "lllfsthread.h"
#include "llui.h"
#include "llimageworker.h"
#include "llrender.h"

#include "lltooltip.h"
#include "llappviewer.h"
#include "llselectmgr.h"
#include "lltexlayer.h"
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llvovolume.h"
extern F32 texmem_lower_bound_scale;

LLTextureView *gTextureView = NULL;
LLTextureSizeView *gTextureSizeView = NULL;
LLTextureSizeView *gTextureCategoryView = NULL;

//static
std::set<LLViewerFetchedTexture*> LLTextureView::sDebugImages;

////////////////////////////////////////////////////////////////////////////

static std::string title_string1a("Tex UUID Area  DDis(Req)  DecodePri(Fetch)     [download] pk/max");
static std::string title_string1b("Tex UUID Area  DDis(Req)  Fetch(DecodePri)     [download] pk/max");
static std::string title_string2("State");
static std::string title_string3("Pkt Bnd");
static std::string title_string4("  W x H (Dis) Mem");

static S32 title_x1 = 0;
static S32 title_x2 = 460;
static S32 title_x3 = title_x2 + 40;
static S32 title_x4 = title_x3 + 50;
static S32 texture_bar_height = 8;

////////////////////////////////////////////////////////////////////////////

class LLTextureBar : public LLView
{
public:
	LLPointer<LLViewerFetchedTexture> mImagep;
	S32 mHilite;

public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Mandatory<LLTextureView*> texture_view;
		Params()
		:	texture_view("texture_view")
		{
			mouse_opaque(false);
		}
	};
	LLTextureBar(const Params& p)
	:	LLView(p),
		mHilite(0),
		mTextureView(p.texture_view)
	{}

	virtual void draw();
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual LLRect getRequiredRect();	// Return the height of this object, given the set options.

// Used for sorting
	struct sort
	{
		bool operator()(const LLView* i1, const LLView* i2)
		{
			LLTextureBar* bar1p = (LLTextureBar*)i1;
			LLTextureBar* bar2p = (LLTextureBar*)i2;
			LLViewerFetchedTexture *i1p = bar1p->mImagep;
			LLViewerFetchedTexture *i2p = bar2p->mImagep;
			F32 pri1 = i1p->getDecodePriority(); // i1p->mRequestedDownloadPriority
			F32 pri2 = i2p->getDecodePriority(); // i2p->mRequestedDownloadPriority
			if (pri1 > pri2)
				return true;
			else if (pri2 > pri1)
				return false;
			else
				return i1p->getID() < i2p->getID();
		}
	};

	struct sort_fetch
	{
		bool operator()(const LLView* i1, const LLView* i2)
		{
			LLTextureBar* bar1p = (LLTextureBar*)i1;
			LLTextureBar* bar2p = (LLTextureBar*)i2;
			LLViewerFetchedTexture *i1p = bar1p->mImagep;
			LLViewerFetchedTexture *i2p = bar2p->mImagep;
			U32 pri1 = i1p->getFetchPriority() ;
			U32 pri2 = i2p->getFetchPriority() ;
			if (pri1 > pri2)
				return true;
			else if (pri2 > pri1)
				return false;
			else
				return i1p->getID() < i2p->getID();
		}
	};	
private:
	LLTextureView* mTextureView;
};

void LLTextureBar::draw()
{
	if (!mImagep)
	{
		return;
	}

	LLColor4 color;
	if (mImagep->getID() == LLAppViewer::getTextureFetch()->mDebugID)
	{
		color = LLColor4::cyan2;
	}
	else if (mHilite)
	{
		S32 idx = llclamp(mHilite,1,3);
		if (idx==1) color = LLColor4::orange;
		else if (idx==2) color = LLColor4::yellow;
		else color = LLColor4::pink2;
	}
	else if (mImagep->mDontDiscard)
	{
		color = LLColor4::green4;
	}
	else if (mImagep->getBoostLevel() > LLViewerTexture::BOOST_NONE)
	{
		color = LLColor4::magenta;
	}
	else if (mImagep->getDecodePriority() <= 0.0f)
	{
		color = LLColor4::grey; color[VALPHA] = .7f;
	}
	else
	{
		color = LLColor4::white; color[VALPHA] = .7f;
	}

	// We need to draw:
	// The texture UUID or name
	// The progress bar for the texture, highlighted if it's being download
	// Various numerical stats.
	std::string tex_str;
	S32 left, right;
	S32 top = 0;
	S32 bottom = top + 6;
	LLColor4 clr;

	LLGLSUIDefault gls_ui;
	
	// Name, pixel_area, requested pixel area, decode priority
	std::string uuid_str;
	mImagep->mID.toString(uuid_str);
	uuid_str = uuid_str.substr(0,7);
	if (mTextureView->mOrderFetch)
	{
		tex_str = llformat("%s %7.0f %d(%d) 0x%08x(%8.0f)",
						   uuid_str.c_str(),
						   mImagep->mMaxVirtualSize,
						   mImagep->mDesiredDiscardLevel,
						   mImagep->mRequestedDiscardLevel,
						   mImagep->mFetchPriority,
						   mImagep->getDecodePriority());
	}
	else
	{
		tex_str = llformat("%s %7.0f %d(%d) %8.0f(0x%08x)",
						   uuid_str.c_str(),
						   mImagep->mMaxVirtualSize,
						   mImagep->mDesiredDiscardLevel,
						   mImagep->mRequestedDiscardLevel,
						   mImagep->getDecodePriority(),
						   mImagep->mFetchPriority);
	}

	LLFontGL::getFontMonospace()->renderUTF8(tex_str, 0, title_x1, getRect().getHeight(),
									 color, LLFontGL::LEFT, LLFontGL::TOP);

	// State
	// Hack: mirrored from lltexturefetch.cpp
	struct { const std::string desc; LLColor4 color; } fetch_state_desc[] = {
		{ "---", LLColor4::red },	// INVALID
		{ "INI", LLColor4::white },	// INIT
		{ "DSK", LLColor4::cyan },	// LOAD_FROM_TEXTURE_CACHE
		{ "DSK", LLColor4::blue },	// CACHE_POST
		{ "NET", LLColor4::green },	// LOAD_FROM_NETWORK
		{ "SIM", LLColor4::green },	// LOAD_FROM_SIMULATOR
		{ "REQ", LLColor4::yellow },// SEND_HTTP_REQ
		{ "HTP", LLColor4::green },	// WAIT_HTTP_REQ
		{ "DEC", LLColor4::yellow },// DECODE_IMAGE
		{ "DEC", LLColor4::green }, // DECODE_IMAGE_UPDATE
		{ "WRT", LLColor4::purple },// WRITE_TO_CACHE
		{ "WRT", LLColor4::orange },// WAIT_ON_WRITE
		{ "END", LLColor4::red },   // DONE
#define LAST_STATE 12
		{ "CRE", LLColor4::magenta }, // LAST_STATE+1
		{ "FUL", LLColor4::green }, // LAST_STATE+2
		{ "BAD", LLColor4::red }, // LAST_STATE+3
		{ "MIS", LLColor4::red }, // LAST_STATE+4
		{ "---", LLColor4::white }, // LAST_STATE+5
	};
	const S32 fetch_state_desc_size = (S32)LL_ARRAY_SIZE(fetch_state_desc);
	S32 state =
		mImagep->mNeedsCreateTexture ? LAST_STATE+1 :
		mImagep->mFullyLoaded ? LAST_STATE+2 :
		mImagep->mMinDiscardLevel > 0 ? LAST_STATE+3 :
		mImagep->mIsMissingAsset ? LAST_STATE+4 :
		!mImagep->mIsFetching ? LAST_STATE+5 :
		mImagep->mFetchState;
	state = llclamp(state,0,fetch_state_desc_size-1);

	LLFontGL::getFontMonospace()->renderUTF8(fetch_state_desc[state].desc, 0, title_x2, getRect().getHeight(),
									 fetch_state_desc[state].color,
									 LLFontGL::LEFT, LLFontGL::TOP);
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	// Draw the progress bar.
	S32 bar_width = 100;
	S32 bar_left = 260;
	left = bar_left;
	right = left + bar_width;

	gGL.color4f(0.f, 0.f, 0.f, 0.75f);
	gl_rect_2d(left, top, right, bottom);

	F32 data_progress = mImagep->mDownloadProgress;
	
	if (data_progress > 0.0f)
	{
		// Downloaded bytes
		right = left + llfloor(data_progress * (F32)bar_width);
		if (right > left)
		{
			gGL.color4f(0.f, 0.f, 1.f, 0.75f);
			gl_rect_2d(left, top, right, bottom);
		}
	}

	S32 pip_width = 6;
	S32 pip_space = 14;
	S32 pip_x = title_x3 + pip_space/2;
	
	// Draw the packet pip
	const F32 pip_max_time = 5.f;
	F32 last_event = mImagep->mLastPacketTimer.getElapsedTimeF32();
	if (last_event < pip_max_time)
	{
		clr = LLColor4::white; 
	}
	else
	{
		last_event = mImagep->mRequestDeltaTime;
		if (last_event < pip_max_time)
		{
			clr = LLColor4::green;
		}
		else
		{
			last_event = mImagep->mFetchDeltaTime;
			if (last_event < pip_max_time)
			{
				clr = LLColor4::yellow;
			}
		}
	}
	if (last_event < pip_max_time)
	{
		clr.setAlpha(1.f - last_event/pip_max_time);
		gGL.color4fv(clr.mV);
		gl_rect_2d(pip_x, top, pip_x + pip_width, bottom);
	}
	pip_x += pip_width + pip_space;

	// we don't want to show bind/resident pips for textures using the default texture
	if (mImagep->hasGLTexture())
	{
		// Draw the bound pip
		last_event = mImagep->getTimePassedSinceLastBound();
		if (last_event < 1.f)
		{
			clr = mImagep->getMissed() ? LLColor4::red : LLColor4::magenta1;
			clr.setAlpha(1.f - last_event);
			gGL.color4fv(clr.mV);
			gl_rect_2d(pip_x, top, pip_x + pip_width, bottom);
		}
	}
	pip_x += pip_width + pip_space;

	
	{
		LLGLSUIDefault gls_ui;
		// draw the packet data
// 		{
// 			std::string num_str = llformat("%3d/%3d", mImagep->mLastPacket+1, mImagep->mPackets);
// 			LLFontGL::getFontMonospace()->renderUTF8(num_str, 0, bar_left + 100, getRect().getHeight(), color,
// 											 LLFontGL::LEFT, LLFontGL::TOP);
// 		}
		
		// draw the image size at the end
		{
			std::string num_str = llformat("%3dx%3d (%d) %7d", mImagep->getWidth(), mImagep->getHeight(),
				mImagep->getDiscardLevel(), mImagep->hasGLTexture() ? mImagep->getTextureMemory() : 0);
			LLFontGL::getFontMonospace()->renderUTF8(num_str, 0, title_x4, getRect().getHeight(), color,
											LLFontGL::LEFT, LLFontGL::TOP);
		}
	}

}

BOOL LLTextureBar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if ((mask & (MASK_CONTROL|MASK_SHIFT|MASK_ALT)) == MASK_ALT)
	{
		LLAppViewer::getTextureFetch()->mDebugID = mImagep->getID();
		return TRUE;
	}
	return LLView::handleMouseDown(x,y,mask);
}

LLRect LLTextureBar::getRequiredRect()
{
	LLRect rect;

	rect.mTop = texture_bar_height;

	return rect;
}

////////////////////////////////////////////////////////////////////////////

class LLGLTexMemBar : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Mandatory<LLTextureView*>	texture_view;
		Params()
		:	texture_view("texture_view")
		{
			S32 line_height = (S32)(LLFontGL::getFontMonospace()->getLineHeight() + .5f);
			rect(LLRect(0,0,100,line_height * 4));
		}
	};

	LLGLTexMemBar(const Params& p)
	:	LLView(p),
		mTextureView(p.texture_view)
	{}

	virtual void draw();	
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual LLRect getRequiredRect();	// Return the height of this object, given the set options.

private:
	LLTextureView* mTextureView;
};

void LLGLTexMemBar::draw()
{
	S32 bound_mem = BYTES_TO_MEGA_BYTES(LLViewerTexture::sBoundTextureMemoryInBytes);
 	S32 max_bound_mem = LLViewerTexture::sMaxBoundTextureMemInMegaBytes;
	S32 total_mem = BYTES_TO_MEGA_BYTES(LLViewerTexture::sTotalTextureMemoryInBytes);
	S32 max_total_mem = LLViewerTexture::sMaxTotalTextureMemInMegaBytes;
	F32 discard_bias = LLViewerTexture::sDesiredDiscardBias;
	F32 cache_usage = (F32)BYTES_TO_MEGA_BYTES(LLAppViewer::getTextureCache()->getUsage()) ;
	F32 cache_max_usage = (F32)BYTES_TO_MEGA_BYTES(LLAppViewer::getTextureCache()->getMaxUsage()) ;
	S32 line_height = (S32)(LLFontGL::getFontMonospace()->getLineHeight() + .5f);
	S32 v_offset = (S32)((texture_bar_height + 2.5f) * mTextureView->mNumTextureBars + 2.5f);
	//----------------------------------------------------------------------------
	LLGLSUIDefault gls_ui;
	LLColor4 text_color(1.f, 1.f, 1.f, 0.75f);
	LLColor4 color;
	
	std::string text;
	text = llformat("GL Tot: %d/%d MB Bound: %d/%d MB Raw Tot: %d MB Bias: %.2f Cache: %.1f/%.1f MB",
					total_mem,
					max_total_mem,
					bound_mem,
					max_bound_mem,
					LLImageRaw::sGlobalRawMemory >> 20,					discard_bias,
					cache_usage, cache_max_usage);
	//, cache_entries, cache_max_entries

	LLFontGL::getFontMonospace()->renderUTF8(text, 0, 0, v_offset + line_height*3,
											 text_color, LLFontGL::LEFT, LLFontGL::TOP);

	//----------------------------------------------------------------------------
#if 0
	S32 bar_left = 400;
	S32 bar_width = 200;
	S32 top = line_height*3 - 2 + v_offset;
	S32 bottom = top - 6;
	S32 left = bar_left;
	S32 right = left + bar_width;
	F32 bar_scale;
	
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

	// GL Mem Bar
		
	left = bar_left;
	text = "GL";
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, left, line_height*3,
											 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	
	left = bar_left+20;
	right = left + bar_width;
	
	gGL.color4f(0.5f, 0.5f, 0.5f, 0.75f); // grey
	gl_rect_2d(left, top, right, bottom);

	bar_scale = (F32)bar_width / (max_total_mem * 1.5f);
	right = left + llfloor(total_mem * bar_scale);
	right = llclamp(right, bar_left, bar_left + bar_width);
	
	color = (total_mem < llfloor(max_total_mem * texmem_lower_bound_scale)) ? LLColor4::green :
		  	(total_mem < max_total_mem) ? LLColor4::yellow : LLColor4::red;
	color[VALPHA] = .75f;
	glColor4fv(color.mV);
	
	gl_rect_2d(left, top, right, bottom); // red/yellow/green

	//
	bar_left += bar_width + bar_space;
	//top = bottom - 2; bottom = top - 6;
	
	// Bound Mem Bar

	left = bar_left;
	text = "GL";
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, left, line_height*3,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	left = bar_left + 20;
	right = left + bar_width;
	
	gGL.color4f(0.5f, 0.5f, 0.5f, 0.75f);
	gl_rect_2d(left, top, right, bottom);

	color = (bound_mem < llfloor(max_bound_mem * texmem_lower_bound_scale)) ? LLColor4::green :
		  	(bound_mem < max_bound_mem) ? LLColor4::yellow : LLColor4::red;
	color[VALPHA] = .75f;
	glColor4fv(color.mV);

	gl_rect_2d(left, top, right, bottom);
#else
	S32 left = 0 ;
#endif
	//----------------------------------------------------------------------------

	text = llformat("Textures: %d Fetch: %d(%d) Pkts:%d(%d) Cache R/W: %d/%d LFS:%d RAW:%d HTP:%d DEC:%d CRE:%d",
					gTextureList.getNumImages(),
					LLAppViewer::getTextureFetch()->getNumRequests(), LLAppViewer::getTextureFetch()->getNumDeletes(),
					LLAppViewer::getTextureFetch()->mPacketCount, LLAppViewer::getTextureFetch()->mBadPacketCount, 
					LLAppViewer::getTextureCache()->getNumReads(), LLAppViewer::getTextureCache()->getNumWrites(),
					LLLFSThread::sLocal->getPending(),
					LLImageRaw::sRawImageCount,
					LLAppViewer::getTextureFetch()->getNumHTTPRequests(),
					LLAppViewer::getImageDecodeThread()->getPending(), 
					gTextureList.mCreateTextureList.size());

	LLFontGL::getFontMonospace()->renderUTF8(text, 0, 0, v_offset + line_height*2,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);


	left = 550;
	F32 bandwidth = LLAppViewer::getTextureFetch()->getTextureBandwidth();
	F32 max_bandwidth = gSavedSettings.getF32("ThrottleBandwidthKBPS");
	color = bandwidth > max_bandwidth ? LLColor4::red : bandwidth > max_bandwidth*.75f ? LLColor4::yellow : text_color;
	color[VALPHA] = text_color[VALPHA];
	text = llformat("BW:%.0f/%.0f",bandwidth, max_bandwidth);
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, left, v_offset + line_height*2,
											 color, LLFontGL::LEFT, LLFontGL::TOP);
	
	S32 dx1 = 0;
	if (LLAppViewer::getTextureFetch()->mDebugPause)
	{
		LLFontGL::getFontMonospace()->renderUTF8(std::string("!"), 0, title_x1, v_offset + line_height,
										 text_color, LLFontGL::LEFT, LLFontGL::TOP);
		dx1 += 8;
	}
	if (mTextureView->mFreezeView)
	{
		LLFontGL::getFontMonospace()->renderUTF8(std::string("*"), 0, title_x1, v_offset + line_height,
										 text_color, LLFontGL::LEFT, LLFontGL::TOP);
		dx1 += 8;
	}
	if (mTextureView->mOrderFetch)
	{
		LLFontGL::getFontMonospace()->renderUTF8(title_string1b, 0, title_x1+dx1, v_offset + line_height,
										 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	}
	else
	{	
		LLFontGL::getFontMonospace()->renderUTF8(title_string1a, 0, title_x1+dx1, v_offset + line_height,
										 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	}
	
	LLFontGL::getFontMonospace()->renderUTF8(title_string2, 0, title_x2, v_offset + line_height,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);

	LLFontGL::getFontMonospace()->renderUTF8(title_string3, 0, title_x3, v_offset + line_height,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);

	LLFontGL::getFontMonospace()->renderUTF8(title_string4, 0, title_x4, v_offset + line_height,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
}

BOOL LLGLTexMemBar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	return FALSE;
}

LLRect LLGLTexMemBar::getRequiredRect()
{
	LLRect rect;
	rect.mTop = 8;
	return rect;
}

////////////////////////////////////////////////////////////////////////////
class LLGLTexSizeBar
{
public:
	LLGLTexSizeBar(S32 index, S32 left, S32 bottom, S32 right, S32 line_height)
	{
		mIndex = index ;
		mLeft = left ;
		mBottom = bottom ;
		mRight = right ;
		mLineHeight = line_height ;
		mTopLoaded = 0 ;
		mTopBound = 0 ;
		mScale = 1.0f ;
	}

	void setTop(S32 loaded, S32 bound, F32 scale) {mTopLoaded = loaded ; mTopBound = bound; mScale = scale ;}

	void draw();	
	BOOL handleHover(S32 x, S32 y, MASK mask, BOOL set_pick_size) ;
	
private:
	S32 mIndex ;
	S32 mLeft ;
	S32 mBottom ;
	S32 mRight ;
	S32 mTopLoaded ;
	S32 mTopBound ;
	S32 mLineHeight ;
	F32 mScale ;
};

BOOL LLGLTexSizeBar::handleHover(S32 x, S32 y, MASK mask, BOOL set_pick_size) 
{
	if(y > mBottom && (y < mBottom + (S32)(mTopLoaded * mScale) || y < mBottom + (S32)(mTopBound * mScale)))
	{
		LLImageGL::setCurTexSizebar(mIndex, set_pick_size);
	}
	return TRUE ;
}
void LLGLTexSizeBar::draw()
{
	LLGLSUIDefault gls_ui;

	if(LLImageGL::sCurTexSizeBar == mIndex)
	{
		F32 text_color[] = {1.f, 1.f, 1.f, 0.75f};	
		std::string text;
	
		text = llformat("%d", mTopLoaded) ;
		LLFontGL::getFontMonospace()->renderUTF8(text, 0, mLeft, mBottom + (S32)(mTopLoaded * mScale) + mLineHeight,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);

		text = llformat("%d", mTopBound) ;
		LLFontGL::getFontMonospace()->renderUTF8(text, 0, (mLeft + mRight) / 2, mBottom + (S32)(mTopBound * mScale) + mLineHeight,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	}

	F32 loaded_color[] = {1.0f, 0.0f, 0.0f, 0.75f};
	F32 bound_color[] = {1.0f, 1.0f, 0.0f, 0.75f};
	gl_rect_2d(mLeft, mBottom + (S32)(mTopLoaded * mScale), (mLeft + mRight) / 2, mBottom, loaded_color) ;
	gl_rect_2d((mLeft + mRight) / 2, mBottom + (S32)(mTopBound * mScale), mRight, mBottom, bound_color) ;
}
////////////////////////////////////////////////////////////////////////////

LLTextureView::LLTextureView(const LLTextureView::Params& p)
	:	LLContainerView(p),
		mFreezeView(FALSE),
		mOrderFetch(FALSE),
		mPrintList(FALSE),
		mNumTextureBars(0)
{
	setVisible(FALSE);
	
	setDisplayChildren(TRUE);
	mGLTexMemBar = 0;
}

LLTextureView::~LLTextureView()
{
	// Children all cleaned up by default view destructor.
	delete mGLTexMemBar;
	mGLTexMemBar = 0;
}

typedef std::pair<F32,LLViewerFetchedTexture*> decode_pair_t;
struct compare_decode_pair
{
	bool operator()(const decode_pair_t& a, const decode_pair_t& b)
	{
		return a.first > b.first;
	}
};

void LLTextureView::draw()
{
	if (!mFreezeView)
	{
// 		LLViewerObject *objectp;
// 		S32 te;

		for_each(mTextureBars.begin(), mTextureBars.end(), DeletePointer());
		mTextureBars.clear();
	
		delete mGLTexMemBar;
		mGLTexMemBar = 0;
	
		typedef std::multiset<decode_pair_t, compare_decode_pair > display_list_t;
		display_list_t display_image_list;
	
		if (mPrintList)
		{
			llinfos << "ID\tMEM\tBOOST\tPRI\tWIDTH\tHEIGHT\tDISCARD" << llendl;
		}
	
		for (LLViewerTextureList::image_priority_list_t::iterator iter = gTextureList.mImageList.begin();
			 iter != gTextureList.mImageList.end(); )
		{
			LLPointer<LLViewerFetchedTexture> imagep = *iter++;

			S32 cur_discard = imagep->getDiscardLevel();
			S32 desired_discard = imagep->mDesiredDiscardLevel;
			
			if (mPrintList)
			{
				S32 tex_mem = imagep->hasGLTexture() ? imagep->getTextureMemory() : 0 ;
				llinfos << imagep->getID()
					<< "\t" << tex_mem
						<< "\t" << imagep->getBoostLevel()
						<< "\t" << imagep->getDecodePriority()
						<< "\t" << imagep->getWidth()
						<< "\t" << imagep->getHeight()
						<< "\t" << cur_discard
						<< llendl;
			}

			if (imagep->getID() == LLAppViewer::getTextureFetch()->mDebugID)
			{
				static S32 debug_count = 0;
				++debug_count; // for breakpoints
			}
			
#if 0
			if (imagep->getDontDiscard())
			{
				continue;
			}

			if (imagep->isMissingAsset())
			{
				continue;
			}
#endif

#define HIGH_PRIORITY 100000000.f
			F32 pri;
			if (mOrderFetch)
			{
				pri = ((F32)imagep->mFetchPriority)/256.f;
			}
			else
			{
				pri = imagep->getDecodePriority();
			}
			pri = llclamp(pri, 0.0f, HIGH_PRIORITY-1.f);
			
			if (sDebugImages.find(imagep) != sDebugImages.end())
			{
				pri += 4*HIGH_PRIORITY;
			}

			if (!mOrderFetch)
			{
#if 1
			if (pri < HIGH_PRIORITY && LLSelectMgr::getInstance())
			{
				struct f : public LLSelectedTEFunctor
				{
					LLViewerFetchedTexture* mImage;
					f(LLViewerFetchedTexture* image) : mImage(image) {}
					virtual bool apply(LLViewerObject* object, S32 te)
					{
						return (mImage == object->getTEImage(te));
					}
				} func(imagep);
				const bool firstonly = true;
				bool match = LLSelectMgr::getInstance()->getSelection()->applyToTEs(&func, firstonly);
				if (match)
				{
					pri += 3*HIGH_PRIORITY;
				}
			}
#endif
#if 1
			if (pri < HIGH_PRIORITY && (cur_discard< 0 || desired_discard < cur_discard))
			{
				LLSelectNode* hover_node = LLSelectMgr::instance().getHoverNode();
				if (hover_node)
				{
					LLViewerObject *objectp = hover_node->getObject();
					if (objectp)
					{
						S32 tex_count = objectp->getNumTEs();
						for (S32 i = 0; i < tex_count; i++)
						{
							if (imagep == objectp->getTEImage(i))
							{
								pri += 2*HIGH_PRIORITY;
								break;
							}
						}
					}
				}
			}
#endif
#if 1
			if (pri > 0.f && pri < HIGH_PRIORITY)
			{
				if (imagep->mLastPacketTimer.getElapsedTimeF32() < 1.f ||
					imagep->mFetchDeltaTime < 0.25f)
				{
					pri += 1*HIGH_PRIORITY;
				}
			}
#endif
			}
			
	 		if (pri > 0.0f)
			{
				display_image_list.insert(std::make_pair(pri, imagep));
			}
		}
		
		if (mPrintList)
		{
			mPrintList = FALSE;
		}
		
		static S32 max_count = 50;
		S32 count = 0;
		mNumTextureBars = 0 ;
		for (display_list_t::iterator iter = display_image_list.begin();
			 iter != display_image_list.end(); iter++)
		{
			LLViewerFetchedTexture* imagep = iter->second;
			S32 hilite = 0;
			F32 pri = iter->first;
			if (pri >= 1 * HIGH_PRIORITY)
			{
				hilite = (S32)((pri+1) / HIGH_PRIORITY) - 1;
			}
			if ((hilite || count < max_count-10) && (count < max_count))
			{
				if (addBar(imagep, hilite))
				{
					count++;
				}
			}
		}

		if (mOrderFetch)
			sortChildren(LLTextureBar::sort_fetch());
		else
			sortChildren(LLTextureBar::sort());

		LLGLTexMemBar::Params tmbp;
		tmbp.name("gl texmem bar");
		tmbp.texture_view(this);
		mGLTexMemBar = LLUICtrlFactory::create<LLGLTexMemBar>(tmbp);
		addChild(mGLTexMemBar);
	
		reshape(getRect().getWidth(), getRect().getHeight(), TRUE);

		/*
		  count = gTextureList.getNumImages();
		  std::string info_string;
		  info_string = llformat("Global Info:\nTexture Count: %d", count);
		  mInfoTextp->setText(info_string);
		*/


		for (child_list_const_iter_t child_iter = getChildList()->begin();
			 child_iter != getChildList()->end(); ++child_iter)
		{
			LLView *viewp = *child_iter;
			if (viewp->getRect().mBottom < 0)
			{
				viewp->setVisible(FALSE);
			}
		}
	}
	
	LLContainerView::draw();

}

BOOL LLTextureView::addBar(LLViewerFetchedTexture *imagep, S32 hilite)
{
	llassert(imagep);
	
	LLTextureBar *barp;
	LLRect r;

	mNumTextureBars++;

	LLTextureBar::Params tbp;
	tbp.name("texture bar");
	tbp.rect(r);
	tbp.texture_view(this);
	barp = LLUICtrlFactory::create<LLTextureBar>(tbp);
	barp->mImagep = imagep;	
	barp->mHilite = hilite;

	addChild(barp);
	mTextureBars.push_back(barp);

	return TRUE;
}

BOOL LLTextureView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if ((mask & (MASK_CONTROL|MASK_SHIFT|MASK_ALT)) == (MASK_ALT|MASK_SHIFT))
	{
		mPrintList = TRUE;
		return TRUE;
	}
	if ((mask & (MASK_CONTROL|MASK_SHIFT|MASK_ALT)) == (MASK_CONTROL|MASK_SHIFT))
	{
		LLAppViewer::getTextureFetch()->mDebugPause = !LLAppViewer::getTextureFetch()->mDebugPause;
		return TRUE;
	}
	if (mask & MASK_SHIFT)
	{
		mFreezeView = !mFreezeView;
		return TRUE;
	}
	if (mask & MASK_CONTROL)
	{
		mOrderFetch = !mOrderFetch;
		return TRUE;
	}
	return LLView::handleMouseDown(x,y,mask);
}

BOOL LLTextureView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	return FALSE;
}

BOOL LLTextureView::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	return FALSE;
}

//-----------------------------------------------------------------
LLTextureSizeView::LLTextureSizeView(const LLTextureSizeView::Params& p) : LLContainerView(p)
{
	setVisible(FALSE) ;

	mTextureSizeBarWidth = 30 ;
}

LLTextureSizeView::~LLTextureSizeView()
{
	if(mTextureSizeBar.size())
	{
		for(U32 i = 0 ; i < mTextureSizeBar.size() ; i++)
		{
			delete mTextureSizeBar[i] ;
		}
		mTextureSizeBar.clear() ;
	}
}
void LLTextureSizeView::draw()
{
	if(mType == TEXTURE_MEM_OVER_SIZE)
	{
		drawTextureSizeGraph();
	}
	else
	{
		drawTextureCategoryGraph() ;
	}
	
	LLView::draw();
}

BOOL LLTextureSizeView::handleHover(S32 x, S32 y, MASK mask) 
{
	if(x > mTextureSizeBarRect.mLeft && x < mTextureSizeBarRect.mRight)
	{
		mTextureSizeBar[(x - mTextureSizeBarRect.mLeft) / mTextureSizeBarWidth]->handleHover(x, y, mask, (mType == TEXTURE_MEM_OVER_SIZE)) ;
	}

	return TRUE ;
}

//draw real-time texture mem bar over size
void LLTextureSizeView::drawTextureSizeGraph()
{
	if(mTextureSizeBar.size() == 0)
	{
		S32 line_height = (S32)(LLFontGL::getFontMonospace()->getLineHeight() + .5f);
		mTextureSizeBar.resize(LLImageGL::sTextureLoadedCounter.size()) ;
		mTextureSizeBarRect.set(700, line_height * 2 + 400, 700 + mTextureSizeBar.size() * mTextureSizeBarWidth, line_height * 2) ;
		
		for(U32 i = 0 ; i < mTextureSizeBar.size() ; i++)
		{				
			mTextureSizeBar[i] = new LLGLTexSizeBar(i, mTextureSizeBarRect.mLeft + i * mTextureSizeBarWidth , 
				line_height * 2, mTextureSizeBarRect.mLeft + (i + 1) * mTextureSizeBarWidth, line_height) ;				
		}			
	}

	F32 size_bar_scale = drawTextureSizeDistributionGraph() ;		
	for(U32 i = 0 ; i < mTextureSizeBar.size() ; i++)
	{
		mTextureSizeBar[i]->setTop(LLImageGL::sTextureLoadedCounter[i], LLImageGL::sTextureBoundCounter[i], size_bar_scale) ;
		mTextureSizeBar[i]->draw() ;
	}		
	LLImageGL::resetCurTexSizebar();
}

//draw background of texture size bar graph
F32 LLTextureSizeView::drawTextureSizeDistributionGraph()
{	
	//scale
	F32 scale = 1.0f ;
	
	LLGLSUIDefault gls_ui;

	{
		S32 count = 0 ;
		for(U32 i = 0 ; i < LLImageGL::sTextureLoadedCounter.size() ; i++)
		{
			if(LLImageGL::sTextureLoadedCounter[i] > count)
			{
				count = LLImageGL::sTextureLoadedCounter[i] ;
			}
		}
		if(count > mTextureSizeBarRect.getHeight())
		{
			scale = (F32)mTextureSizeBarRect.getHeight() / count ;
		}
	}

	S32 line_height = (S32)(LLFontGL::getFontMonospace()->getLineHeight() + .5f);
	S32 left = mTextureSizeBarRect.mLeft ;
	S32 bottom = mTextureSizeBarRect.mBottom ;
	S32 right = mTextureSizeBarRect.mRight ;
	S32 top = mTextureSizeBarRect.mTop ;

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	
	//background rect
	gl_rect_2d(left - 25, top + 30, right + 100, bottom - 25, LLColor4(0.0f, 0.0f, 0.0f, 0.25f)) ;

	//--------------------------------------------------
	gGL.color4f(1.0f, 0.5f, 0.5f, 0.75f);
	gl_line_2d(left, bottom, right, bottom) ; //x axis
	gl_line_2d(left, bottom, left, top) ; //y axis

	//ruler
	//--------------------------------------------------
	gGL.color4f(1.0f, 0.5f, 0.5f, 0.5f);
	for(S32 i = bottom + 50 ; i <= top ; i += 50)
	{
		gl_line_2d(left, i, right, i) ;
	}

	//texts
	//--------------------------------------------------
	F32 text_color[] = {1.f, 1.f, 1.f, 0.75f};	
	std::string text;
	
	//-------
	//x axis: size label
	text = llformat("%d", 0) ;
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, left + 12, bottom - line_height / 2,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	for(U32 i = 1 ; i < mTextureSizeBar.size() ; i++)
	{
		text = llformat("%d", (1 << (i / 2)) + ((i & 1) ? ((1 << (i / 2)) >> 1) : 0)) ;
		LLFontGL::getFontMonospace()->renderUTF8(text, 0, left + i * mTextureSizeBarWidth + 12, bottom - line_height / 2,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	}
	text = llformat("(w + h)/2") ;
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, right + 10, bottom - line_height / 2,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	//-------

	//y axis: number label
	for(S32 i = bottom + 50 ; i <= top ; i += 50)
	{
		text = llformat("%d", (S32)((i - bottom) / scale)) ;
		LLFontGL::getFontMonospace()->renderUTF8(text, 0, left - 20, i + line_height / 2 ,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
		LLFontGL::getFontMonospace()->renderUTF8(text, 0, right + 5, i + line_height / 2 ,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	}

	//--------------------------------------------------
	F32 loaded_color[] = {1.0f, 0.0f, 0.0f, 0.75f};
	gl_rect_2d(left + 70, top + line_height * 2, left + 90, top + line_height, loaded_color) ;
	text = llformat("Loaded") ;
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, left + 100, top + line_height * 2,
									 loaded_color, LLFontGL::LEFT, LLFontGL::TOP);

	F32 bound_color[] = {1.0f, 1.0f, 0.0f, 0.75f};
	gl_rect_2d(left + 170, top + line_height * 2, left + 190, top + line_height, bound_color) ;
	text = llformat("Bound") ;
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, left + 200, top + line_height * 2,
									 bound_color, LLFontGL::LEFT, LLFontGL::TOP);

	//--------------------------------------------------

	//title
	text = llformat("Texture Size Distribution") ;
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, left + 250, top + line_height * 3,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	return scale ;
}

//draw real-time texture mem bar over category
void LLTextureSizeView::drawTextureCategoryGraph()
{
	if(mTextureSizeBar.size() == 0)
	{
		S32 line_height = (S32)(LLFontGL::getFontMonospace()->getLineHeight() + .5f);
		mTextureSizeBar.resize(LLViewerTexture::getTotalNumOfCategories()) ;
		mTextureSizeBarRect.set(700, line_height * 2 + 400, 700 + mTextureSizeBar.size() * mTextureSizeBarWidth, line_height * 2) ;
		
		for(U32 i = 0 ; i < mTextureSizeBar.size() ; i++)
		{				
			mTextureSizeBar[i] = new LLGLTexSizeBar(i, mTextureSizeBarRect.mLeft + i * mTextureSizeBarWidth , 
				line_height * 2, mTextureSizeBarRect.mLeft + (i + 1) * mTextureSizeBarWidth, line_height) ;				
		}			
	}

	F32 size_bar_scale = drawTextureCategoryDistributionGraph() ;		
	for(U32 i = 0 ; i < mTextureSizeBar.size() ; i++)
	{
		U32 k = LLViewerTexture::getIndexFromCategory(i) ;
		mTextureSizeBar[i]->setTop(LLImageGL::sTextureMemByCategory[k] >> 20, LLImageGL::sTextureMemByCategoryBound[k] >> 20, size_bar_scale) ;
		mTextureSizeBar[i]->draw() ;
	}		
	LLImageGL::resetCurTexSizebar();
}

//draw background for TEXTURE_MEM_OVER_CATEGORY
F32 LLTextureSizeView::drawTextureCategoryDistributionGraph() 
{
	//scale
	F32 scale = 4.0f ;
	
	LLGLSUIDefault gls_ui;

	{
		S32 count = 0 ;
		for(U32 i = 0 ; i < LLImageGL::sTextureMemByCategory.size() ; i++)
		{
			S32 tmp = LLImageGL::sTextureMemByCategory[i] >> 20 ;
			if(tmp > count)
			{
				count = tmp ;
			}
		}
		if(count > mTextureSizeBarRect.getHeight() * 0.25f)
		{
			scale = (F32)mTextureSizeBarRect.getHeight() * 0.25f / count ;
		}
	}

	S32 line_height = (S32)(LLFontGL::getFontMonospace()->getLineHeight() + .5f);
	S32 left = mTextureSizeBarRect.mLeft ;
	S32 bottom = mTextureSizeBarRect.mBottom ;
	S32 right = mTextureSizeBarRect.mRight ;
	S32 top = mTextureSizeBarRect.mTop ;

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	
	//background rect
	gl_rect_2d(left - 25, top + 30, right + 100, bottom - 25, LLColor4(0.0f, 0.0f, 0.0f, 0.25f)) ;

	//--------------------------------------------------
	gGL.color4f(1.0f, 0.5f, 0.5f, 0.75f);
	gl_line_2d(left, bottom, right, bottom) ; //x axis
	gl_line_2d(left, bottom, left, top) ; //y axis

	//ruler
	//--------------------------------------------------
	gGL.color4f(1.0f, 0.5f, 0.5f, 0.5f);
	for(S32 i = bottom + 50 ; i <= top ; i += 50)
	{
		gl_line_2d(left, i, right, i) ;
	}

	//texts
	//--------------------------------------------------
	F32 text_color[] = {1.f, 1.f, 1.f, 0.75f};	
	std::string text;
	
	//-------
	//x axis: size label			
	static char category[LLViewerTexture::MAX_GL_IMAGE_CATEGORY][4] = 
	{"Non", "Bak", "Av", "Cld", "Scp", "Hi", "Trn", "Slt", "Hud", "Bsf", "UI", "Pvw", "Map", "Mvs", "Slf", "Loc", "Scr", "Dyn", "Mdi", "ALT", "Oth" } ;

	text = llformat("%s", category[0]) ;
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, left + 12, bottom - line_height / 2,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	for(U32 i = 1 ; i < mTextureSizeBar.size() ; i++)
	{
		text = llformat("%s", category[i]) ;
		LLFontGL::getFontMonospace()->renderUTF8(text, 0, left + i * mTextureSizeBarWidth + 12, bottom - line_height / 2,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	}
	//-------

	//y axis: number label
	for(S32 i = bottom + 50 ; i <= top ; i += 50)
	{
		text = llformat("%d", (S32)((i - bottom) / scale)) ;
		LLFontGL::getFontMonospace()->renderUTF8(text, 0, left - 20, i + line_height / 2 ,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
		LLFontGL::getFontMonospace()->renderUTF8(text, 0, right + 5, i + line_height / 2 ,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	}

	text = llformat("MB") ;
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, left - 20, top + line_height * 2 ,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	//--------------------------------------------------
	F32 loaded_color[] = {1.0f, 0.0f, 0.0f, 0.75f};
	gl_rect_2d(left + 70, top + line_height * 2, left + 90, top + line_height, loaded_color) ;
	text = llformat("Loaded") ;
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, left + 100, top + line_height * 2,
									 loaded_color, 
									 LLFontGL::LEFT, LLFontGL::TOP);

	F32 bound_color[] = {1.0f, 1.0f, 0.0f, 0.75f};
	gl_rect_2d(left + 170, top + line_height * 2, left + 190, top + line_height, bound_color) ;
	text = llformat("Bound") ;
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, left + 200, top + line_height * 2,
									 bound_color, LLFontGL::LEFT, LLFontGL::TOP);

	//--------------------------------------------------

	//title
	text = llformat("Texture Category Distribution") ;
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, left + 250, top + line_height * 3,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);

	return scale ;
}
