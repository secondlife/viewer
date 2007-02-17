/** 
 * @file lltextureview.cpp
 * @brief LLTextureView class implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include <set>

#include "lltextureview.h"

#include "llrect.h"
#include "llerror.h"

#include "viewer.h"
#include "llui.h"

#include "llviewerimagelist.h"
#include "llselectmgr.h"
#include "llviewerobject.h"
#include "llviewerimage.h"
#include "llhoverview.h"

LLTextureView *gTextureView = NULL;

//static
std::set<LLViewerImage*> LLTextureView::sDebugImages;

// Used for sorting
struct SortTextureBars
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

LLTextureView::LLTextureView(const std::string& name, const LLRect& rect)
:	LLContainerView(name, rect)
{
	setVisible(FALSE);
	mFreezeView = FALSE;
	
	mNumTextureBars = 0;
	setDisplayChildren(TRUE);
	mGLTexMemBar = 0;
}

LLTextureView::~LLTextureView()
{
	// Children all cleaned up by default view destructor.
	delete mGLTexMemBar;
	mGLTexMemBar = 0;
}

EWidgetType LLTextureView::getWidgetType() const
{
	return WIDGET_TYPE_TEXTURE_VIEW;
}

LLString LLTextureView::getWidgetTag() const
{
	return LL_TEXTURE_VIEW_TAG;
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
	
		for (LLViewerImageList::image_list_t::iterator iter = gImageList.mImageList.begin();
			 iter != gImageList.mImageList.end(); )
		{
			LLPointer<LLViewerImage> imagep = *iter++;
#if 1
			if (imagep->getDontDiscard())
			{
				continue;
			}
#endif
			if (imagep->isMissingAsset())
			{
				continue;
			}
			
#define HIGH_PRIORITY 100000000.f
			F32 pri = imagep->getDecodePriority();

			if (sDebugImages.find(imagep) != sDebugImages.end())
			{
				pri += 3*HIGH_PRIORITY;
			}
			
#if 1
			if (pri < HIGH_PRIORITY && gSelectMgr)
			{
				S32 te;
				LLViewerObject *objectp;
				LLObjectSelectionHandle selection = gSelectMgr->getSelection();
				for (selection->getFirstTE(&objectp, &te); objectp; selection->getNextTE(&objectp, &te))
				{
					if (imagep == objectp->getTEImage(te))
					{
						pri += 2*HIGH_PRIORITY;
						break;
					}
				}
			}
#endif
#if 1
			if (pri < HIGH_PRIORITY)
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
#if 0
			if (pri < HIGH_PRIORITY)
			{
				if (imagep->mBoostPriority)
				{
					pri += 4*HIGH_PRIORITY;
				}
			}
#endif
#if 1
			if (pri > 0.f && pri < HIGH_PRIORITY)
			{
				if (imagep->mLastPacketTimer.getElapsedTimeF32() < 1.f ||
					imagep->mLastDecodeTime.getElapsedTimeF32() < 1.f)
				{
					pri += 1*HIGH_PRIORITY;
				}
			}
#endif
//	 		if (pri > 0.0f)
			{
				display_image_list.insert(std::make_pair(pri, imagep));
			}
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

		sortChildren(SortTextureBars());
	
		mGLTexMemBar = new LLGLTexMemBar("gl texmem bar");
		addChild(mGLTexMemBar);
	
		reshape(mRect.getWidth(), mRect.getHeight(), TRUE);

		/*
		  count = gImageList.getNumImages();
		  char info_string[512];
		  sprintf(info_string, "Global Info:\nTexture Count: %d", count);
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
	if (!imagep)
	{
		return FALSE;
	}

	LLTextureBar *barp;
	LLRect r;

	mNumTextureBars++;

	for (std::vector<LLTextureBar*>::iterator iter = mTextureBars.begin();
		 iter != mTextureBars.end(); iter++)
	{
		LLTextureBar* barp = *iter;
		if (barp->mImagep == imagep)
		{
			barp->mHilite = hilite;
			return FALSE;
		}
	}

	barp = new LLTextureBar("texture bar", r);
	barp->mImagep = imagep;	
	barp->mHilite = hilite;

	addChild(barp);
	mTextureBars.push_back(barp);

	// Rearrange all child bars.
	reshape(mRect.getWidth(), mRect.getHeight());
	return TRUE;
}

BOOL LLTextureView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (mask & MASK_SHIFT)
	{
		mFreezeView = !mFreezeView;
		return TRUE;
	}
	else if (mask & MASK_CONTROL)
	{
		return FALSE;
	}
	else
	{
		return FALSE;
	}
}

BOOL LLTextureView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	return FALSE;
}

BOOL LLTextureView::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	if (key == ' ')
	{
		mFreezeView = !mFreezeView;
		return TRUE;
	}
	return FALSE;
}

