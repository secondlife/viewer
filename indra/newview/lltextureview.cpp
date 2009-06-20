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

#include "llhoverview.h"
#include "llselectmgr.h"
#include "lltexlayer.h"
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llviewerobject.h"
#include "llviewerimage.h"
#include "llviewerimagelist.h"
#include "llappviewer.h"

extern F32 texmem_lower_bound_scale;

LLTextureView *gTextureView = NULL;

//static
std::set<LLViewerImage*> LLTextureView::sDebugImages;

////////////////////////////////////////////////////////////////////////////

static std::string title_string1a("Tex UUID Area  DDis(Req)  DecodePri(Fetch)     [download]        pk/max");
static std::string title_string1b("Tex UUID Area  DDis(Req)  Fetch(DecodePri)     [download]        pk/max");
static std::string title_string2("State");
static std::string title_string3("Pkt Bnd");
static std::string title_string4("  W x H (Dis) Mem");

static S32 title_x1 = 0;
static S32 title_x2 = 440;
static S32 title_x3 = title_x2 + 40;
static S32 title_x4 = title_x3 + 50;
static S32 texture_bar_height = 8;

////////////////////////////////////////////////////////////////////////////

class LLTextureBar : public LLView
{
public:
	LLPointer<LLViewerImage> mImagep;
	S32 mHilite;

public:
	LLTextureBar(const std::string& name, const LLRect& r, LLTextureView* texview)
		: LLView(name, r, FALSE),
		  mHilite(0),
		  mTextureView(texview)
	{
	}

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
			LLViewerImage *i1p = bar1p->mImagep;
			LLViewerImage *i2p = bar2p->mImagep;
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
			LLViewerImage *i1p = bar1p->mImagep;
			LLViewerImage *i2p = bar2p->mImagep;
			U32 pri1 = i1p->mFetchPriority;
			U32 pri2 = i2p->mFetchPriority;
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
	else if (mImagep->getBoostLevel())
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
		{ "URL", LLColor4::green2 },// LOAD_FROM_HTTP_GET_URL
		{ "HTP", LLColor4::green },	// LOAD_FROM_HTTP_GET_DATA
		{ "DEC", LLColor4::yellow },// DECODE_IMAGE
		{ "DEC", LLColor4::yellow },// DECODE_IMAGE_UPDATE
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
	S32 bar_left = 280;
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
	F32 last_event = mImagep->mLastPacketTimer.getElapsedTimeF32();
	if (last_event < 1.f)
	{
		clr = LLColor4::white; 
	}
	else
	{
		last_event = mImagep->mRequestDeltaTime;
		if (last_event < 1.f)
		{
			clr = LLColor4::green;
		}
		else
		{
			last_event = mImagep->mFetchDeltaTime;
			if (last_event < 1.f)
			{
				clr = LLColor4::yellow;
			}
		}
	}
	if (last_event < 1.f)
	{
		clr.setAlpha(1.f - last_event);
		gGL.color4fv(clr.mV);
		gl_rect_2d(pip_x, top, pip_x + pip_width, bottom);
	}
	pip_x += pip_width + pip_space;

	// we don't want to show bind/resident pips for textures using the default texture
	if (mImagep->getHasGLTexture())
	{
		// Draw the bound pip
		last_event = mImagep->sLastFrameTime - mImagep->mLastBindTime;
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
										mImagep->getDiscardLevel(), mImagep->mTextureMemory);
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
	LLGLTexMemBar(const std::string& name, LLTextureView* texview)
		: LLView(name, FALSE),
		  mTextureView(texview)
	{
		S32 line_height = (S32)(LLFontGL::getFontMonospace()->getLineHeight() + .5f);
		setRect(LLRect(0,0,100,line_height * 4));
	}

	virtual void draw();	
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual LLRect getRequiredRect();	// Return the height of this object, given the set options.

private:
	LLTextureView* mTextureView;
};

void LLGLTexMemBar::draw()
{
	S32 bound_mem = BYTES_TO_MEGA_BYTES(LLViewerImage::sBoundTextureMemoryInBytes);
 	S32 max_bound_mem = LLViewerImage::sMaxBoundTextureMemInMegaBytes;
	S32 total_mem = BYTES_TO_MEGA_BYTES(LLViewerImage::sTotalTextureMemoryInBytes);
	S32 max_total_mem = LLViewerImage::sMaxTotalTextureMemInMegaBytes;
	F32 discard_bias = LLViewerImage::sDesiredDiscardBias;
	S32 line_height = (S32)(LLFontGL::getFontMonospace()->getLineHeight() + .5f);
	
	//----------------------------------------------------------------------------
	LLGLSUIDefault gls_ui;
	F32 text_color[] = {1.f, 1.f, 1.f, 0.75f};
	
	std::string text;
	text = llformat("GL Tot: %d/%d MB Bound: %d/%d MB Discard Bias: %.2f",
					total_mem,
					max_total_mem,
					bound_mem,
					max_bound_mem,
					discard_bias);

	LLFontGL::getFontMonospace()->renderUTF8(text, 0, 0, line_height*3,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);

	//----------------------------------------------------------------------------
	S32 bar_left = 380;
	S32 bar_width = 200;
	S32 top = line_height*3 - 2;
	S32 bottom = top - 6;
	S32 left = bar_left;
	S32 right = left + bar_width;

	F32 bar_scale = (F32)bar_width / (max_bound_mem * 1.5f);
	
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	
	gGL.color4f(0.5f, 0.5f, 0.5f, 0.75f);
	gl_rect_2d(left, top, right, bottom);

	
	left = bar_left;
	right = left + llfloor(bound_mem * bar_scale);
	if (bound_mem < llfloor(max_bound_mem * texmem_lower_bound_scale))
	{
		gGL.color4f(0.f, 1.f, 0.f, 0.75f);
	}
	else if (bound_mem < max_bound_mem)
	{
		gGL.color4f(1.f, 1.f, 0.f, 0.75f);
	}
	else
	{
		gGL.color4f(1.f, 0.f, 0.f, 0.75f);
	}
	gl_rect_2d(left, top, right, bottom);

	bar_scale = (F32)bar_width / (max_total_mem * 1.5f);
	
	top = bottom - 2;
	bottom = top - 6;
	left = bar_left;
	right = left + llfloor(total_mem * bar_scale);
	if (total_mem < llfloor(max_total_mem * texmem_lower_bound_scale))
	{
		gGL.color4f(0.f, 1.f, 0.f, 0.75f);
	}
	else if (total_mem < max_total_mem)
	{
		gGL.color4f(1.f, 1.f, 0.f, 0.75f);
	}
	else
	{
		gGL.color4f(1.f, 0.f, 0.f, 0.75f);
	}
	gl_rect_2d(left, top, right, bottom);

	//----------------------------------------------------------------------------

	text = llformat("Textures: Count: %d Fetch: %d(%d) Pkts:%d(%d) Cache R/W: %d/%d LFS:%d IW:%d(%d) RAW:%d mRaw:%d mAux:%d CB:%d",
					gImageList.getNumImages(),
					LLAppViewer::getTextureFetch()->getNumRequests(), LLAppViewer::getTextureFetch()->getNumDeletes(),
					LLAppViewer::getTextureFetch()->mPacketCount, LLAppViewer::getTextureFetch()->mBadPacketCount, 
					LLAppViewer::getTextureCache()->getNumReads(), LLAppViewer::getTextureCache()->getNumWrites(),
					LLLFSThread::sLocal->getPending(),
					LLImageWorker::sCount, LLImageWorker::getWorkerThread()->getNumDeletes(),
					LLImageRaw::sRawImageCount, LLViewerImage::sRawCount, LLViewerImage::sAuxCount,
					gImageList.mCallbackList.size());

	LLFontGL::getFontMonospace()->renderUTF8(text, 0, 0, line_height*2,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	
	S32 dx1 = 0;
	if (LLAppViewer::getTextureFetch()->mDebugPause)
	{
		LLFontGL::getFontMonospace()->renderUTF8(std::string("!"), 0, title_x1, line_height,
										 text_color, LLFontGL::LEFT, LLFontGL::TOP);
		dx1 += 8;
	}
	if (mTextureView->mFreezeView)
	{
		LLFontGL::getFontMonospace()->renderUTF8(std::string("*"), 0, title_x1, line_height,
										 text_color, LLFontGL::LEFT, LLFontGL::TOP);
		dx1 += 8;
	}
	if (mTextureView->mOrderFetch)
	{
		LLFontGL::getFontMonospace()->renderUTF8(title_string1b, 0, title_x1+dx1, line_height,
										 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	}
	else
	{	
		LLFontGL::getFontMonospace()->renderUTF8(title_string1a, 0, title_x1+dx1, line_height,
										 text_color, LLFontGL::LEFT, LLFontGL::TOP);
	}
	
	LLFontGL::getFontMonospace()->renderUTF8(title_string2, 0, title_x2, line_height,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);

	LLFontGL::getFontMonospace()->renderUTF8(title_string3, 0, title_x3, line_height,
									 text_color, LLFontGL::LEFT, LLFontGL::TOP);

	LLFontGL::getFontMonospace()->renderUTF8(title_string4, 0, title_x4, line_height,
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

LLTextureView::LLTextureView(const std::string& name, const LLRect& rect)
	:	LLContainerView(name, rect),
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

typedef std::pair<F32,LLViewerImage*> decode_pair_t;
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
	
		for (LLViewerImageList::image_priority_list_t::iterator iter = gImageList.mImageList.begin();
			 iter != gImageList.mImageList.end(); )
		{
			LLPointer<LLViewerImage> imagep = *iter++;

			S32 cur_discard = imagep->getDiscardLevel();
			S32 desired_discard = imagep->mDesiredDiscardLevel;
			
			if (mPrintList)
			{
				llinfos << imagep->getID()
						<< "\t" <<  imagep->mTextureMemory
						<< "\t" << imagep->getBoostLevel()
						<< "\t" << imagep->getDecodePriority()
						<< "\t" << imagep->getWidth()
						<< "\t" << imagep->getHeight()
						<< "\t" << cur_discard
						<< llendl;
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
					LLViewerImage* mImage;
					f(LLViewerImage* image) : mImage(image) {}
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
				LLViewerObject *objectp = gHoverView->getLastHoverObject();
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
		for (display_list_t::iterator iter = display_image_list.begin();
			 iter != display_image_list.end(); iter++)
		{
			LLViewerImage* imagep = iter->second;
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

		mGLTexMemBar = new LLGLTexMemBar("gl texmem bar", this);
		addChild(mGLTexMemBar);
	
		reshape(getRect().getWidth(), getRect().getHeight(), TRUE);

		/*
		  count = gImageList.getNumImages();
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

BOOL LLTextureView::addBar(LLViewerImage *imagep, S32 hilite)
{
	llassert(imagep);
	
	LLTextureBar *barp;
	LLRect r;

	mNumTextureBars++;

	barp = new LLTextureBar("texture bar", r, this);
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


