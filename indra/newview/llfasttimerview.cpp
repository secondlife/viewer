/** 
 * @file llfasttimerview.cpp
 * @brief LLFastTimerView class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "llfasttimerview.h"
#include "llviewerwindow.h"
#include "llrect.h"
#include "llerror.h"
#include "llgl.h"
#include "llrender.h"
#include "llmath.h"
#include "llfontgl.h"

#include "llappviewer.h"
#include "llviewerimagelist.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llstat.h"

#include "llfasttimer.h"

//////////////////////////////////////////////////////////////////////////////

static const S32 MAX_VISIBLE_HISTORY = 10;
static const S32 LINE_GRAPH_HEIGHT = 240;

struct ft_display_info {
	int timer;
	const char *desc;
	const LLColor4 *color;
	S32 disabled; // initialized to 0
	int level; // calculated based on desc
	int parent; // calculated
};

static const LLColor4 red0(0.5f, 0.0f, 0.0f, 1.0f);
static const LLColor4 green0(0.0f, 0.5f, 0.0f, 1.0f);
static const LLColor4 blue0(0.0f, 0.0f, 0.5f, 1.0f);
static const LLColor4 blue7(0.0f, 0.0f, 0.5f, 1.0f);

static const LLColor4 green7(0.6f, 1.0f, 0.4f, 1.0f);
static const LLColor4 green8(0.4f, 1.0f, 0.6f, 1.0f);
static const LLColor4 green9(0.6f, 1.0f, 0.6f, 1.0f);

// green(6), blue, yellow, orange, pink(2), cyan
// red (5) magenta (4)
static struct ft_display_info ft_display_table[] =
{
	{ LLFastTimer::FTM_FRAME,				"Frame",				&LLColor4::white, 0 },
	{ LLFastTimer::FTM_MESSAGES,			" System Messages",		&LLColor4::grey1, 1 },
	{ LLFastTimer::FTM_MOUSEHANDLER,		"  Mouse",				&LLColor4::grey1, 0 },
	{ LLFastTimer::FTM_KEYHANDLER,			"  Keyboard",			&LLColor4::grey1, 0 },
	{ LLFastTimer::FTM_SLEEP,				" Sleep",				&LLColor4::grey2, 0 },
	{ LLFastTimer::FTM_IDLE,				" Idle",				&blue0, 0 },
	{ LLFastTimer::FTM_PUMP,				"  Pump",				&LLColor4::magenta2, 1 },
	{ LLFastTimer::FTM_CURL,				"   Curl",				&LLColor4::magenta3, 0 },
	{ LLFastTimer::FTM_INVENTORY,			"  Inventory Update",	&LLColor4::purple6, 1 },
	{ LLFastTimer::FTM_AUTO_SELECT,			"   Open and Select",	&LLColor4::red, 0 },
	{ LLFastTimer::FTM_FILTER,				"   Filter",			&LLColor4::red2, 0 },
	{ LLFastTimer::FTM_ARRANGE,				"   Arrange",			&LLColor4::red3, 0 },
	{ LLFastTimer::FTM_REFRESH,				"   Refresh",			&LLColor4::red4, 0 },
	{ LLFastTimer::FTM_SORT,				"   Sort",				&LLColor4::red5, 0 },
	{ LLFastTimer::FTM_RESET_DRAWORDER,		"  ResetDrawOrder",		&LLColor4::pink1, 0 },
	{ LLFastTimer::FTM_WORLD_UPDATE,		"  World Update",		&LLColor4::blue1, 1 },
	{ LLFastTimer::FTM_UPDATE_MOVE,			"   Move Objects",		&LLColor4::pink2, 0 },
	{ LLFastTimer::FTM_OCTREE_BALANCE,		"    Octree Balance", &LLColor4::red3, 0 },
	{ LLFastTimer::FTM_SIMULATE_PARTICLES,	"   Particle Sim",	&LLColor4::blue4, 0 },
	{ LLFastTimer::FTM_OBJECTLIST_UPDATE,	"  Object Update",	&LLColor4::purple1, 1 },
	{ LLFastTimer::FTM_AVATAR_UPDATE,		"   Avatars",		&LLColor4::purple2, 0 },
	{ LLFastTimer::FTM_JOINT_UPDATE,		"    Joints",		&LLColor4::purple3, 0 },
	{ LLFastTimer::FTM_ATTACHMENT_UPDATE,	"    Attachments",	&LLColor4::purple4, 0 },
	{ LLFastTimer::FTM_UPDATE_ANIMATION,	"     Animation",	&LLColor4::purple5, 0 },
	{ LLFastTimer::FTM_FLEXIBLE_UPDATE,		"   Flex Update",	&LLColor4::pink2, 0 },
	{ LLFastTimer::FTM_LOD_UPDATE,			"   LOD Update",	&LLColor4::magenta1, 0 },
	{ LLFastTimer::FTM_REGION_UPDATE,		"  Region Update",	&LLColor4::cyan2, 0 },
	{ LLFastTimer::FTM_NETWORK,				"  Network",		&LLColor4::orange1, 1 },
	{ LLFastTimer::FTM_IDLE_NETWORK,		"   Decode Msgs",	&LLColor4::orange2, 0 },
	{ LLFastTimer::FTM_PROCESS_MESSAGES,	"    Process Msgs", &LLColor4::orange3, 0 },
	{ LLFastTimer::FTM_PROCESS_OBJECTS,		"     Object Updates",&LLColor4::orange4, 0 },
	{ LLFastTimer::FTM_CREATE_OBJECT,		"      Create Obj",	 &LLColor4::orange5, 0 },
//	{ LLFastTimer::FTM_LOAD_AVATAR,			"       Load Avatar", &LLColor4::pink2, 0 },
	{ LLFastTimer::FTM_PROCESS_IMAGES,		"     Image Updates",&LLColor4::orange6, 0 },
	{ LLFastTimer::FTM_PIPELINE,			"     Pipeline",	&LLColor4::magenta4, 0 },
	{ LLFastTimer::FTM_CLEANUP,				"  Cleanup",		&LLColor4::cyan3, 0 },
	{ LLFastTimer::FTM_AUDIO_UPDATE,		"  Audio Update",	&LLColor4::yellow3, 0 },
	{ LLFastTimer::FTM_VFILE_WAIT,			"  VFile Wait",		&LLColor4::cyan6, 0 },
//	{ LLFastTimer::FTM_IDLE_CB,				"  Callbacks",		&LLColor4::pink1, 0 },
	{ LLFastTimer::FTM_RENDER,				" Render",			&green0, 1 },
	{ LLFastTimer::FTM_HUD_EFFECTS,			"  HUD Effects",	&LLColor4::orange1, 0 },
	{ LLFastTimer::FTM_HUD_UPDATE,			"  HUD Update",	&LLColor4::orange2, 0 },
	{ LLFastTimer::FTM_UPDATE_SKY,			"  Sky Update",		&LLColor4::cyan1, 0 },
	{ LLFastTimer::FTM_UPDATE_TEXTURES,		"  Textures",		&LLColor4::pink2, 0 },
	{ LLFastTimer::FTM_GEO_UPDATE,			"  Geo Update",	&LLColor4::blue3, 1 },
	{ LLFastTimer::FTM_UPDATE_PRIMITIVES,	"   Volumes",		&LLColor4::blue4, 0 },
	{ LLFastTimer::FTM_GEN_VOLUME,			"    Gen Volume",	&LLColor4::yellow3, 0 },
	{ LLFastTimer::FTM_GEN_FLEX,			"    Flexible",	&LLColor4::yellow4, 0 },
	{ LLFastTimer::FTM_GEN_TRIANGLES,		"    Triangles",	&LLColor4::yellow5, 0 },
	{ LLFastTimer::FTM_UPDATE_AVATAR,		"   Avatar",		&LLColor4::yellow1, 0 },
	{ LLFastTimer::FTM_UPDATE_TREE,			"   Tree",			&LLColor4::yellow2, 0 },
	{ LLFastTimer::FTM_UPDATE_TERRAIN,		"   Terrain",		&LLColor4::yellow6, 0 },
	{ LLFastTimer::FTM_UPDATE_CLOUDS,		"   Clouds",		&LLColor4::yellow7, 0 },
	{ LLFastTimer::FTM_UPDATE_GRASS,		"   Grass",		&LLColor4::yellow8, 0 },
	{ LLFastTimer::FTM_UPDATE_WATER,		"   Water",		&LLColor4::yellow9, 0 },
	{ LLFastTimer::FTM_GEO_LIGHT,			"   Lighting",		&LLColor4::yellow1, 0 },
	{ LLFastTimer::FTM_GEO_SHADOW,			"   Shadow",		&LLColor4::black, 0 },
	{ LLFastTimer::FTM_UPDATE_PARTICLES,	"   Particles",	&LLColor4::blue5, 0 },
	{ LLFastTimer::FTM_GEO_RESERVE,			"   Reserve",		&LLColor4::blue6, 0 },
	{ LLFastTimer::FTM_UPDATE_LIGHTS,		"   Lights",		&LLColor4::yellow2, 0 },
	{ LLFastTimer::FTM_GEO_SKY,				"   Sky",			&LLColor4::yellow3, 0 },
	{ LLFastTimer::FTM_UPDATE_WLPARAM,		"  Windlight Param",&LLColor4::magenta2, 0 },
	{ LLFastTimer::FTM_CULL,				"  Object Cull",	&LLColor4::blue2, 1 },
    { LLFastTimer::FTM_CULL_REBOUND,		"   Rebound",		&LLColor4::blue3, 0 },
	{ LLFastTimer::FTM_FRUSTUM_CULL,		"   Frustum Cull",	&LLColor4::blue4, 0 },
	{ LLFastTimer::FTM_OCCLUSION_READBACK,	"   Occlusion Read", &LLColor4::red2, 0 },
	{ LLFastTimer::FTM_IMAGE_UPDATE,		"  Image Update",	&LLColor4::yellow4, 1 },
	{ LLFastTimer::FTM_IMAGE_CREATE,		"   Image CreateGL",&LLColor4::yellow5, 0 },
	{ LLFastTimer::FTM_IMAGE_DECODE,		"   Image Decode",	&LLColor4::yellow6, 0 },
	{ LLFastTimer::FTM_IMAGE_MARK_DIRTY,	"   Dirty Textures",&LLColor4::red1, 0 },
	{ LLFastTimer::FTM_STATESORT,			"  State Sort",	&LLColor4::orange1, 1 },
	{ LLFastTimer::FTM_STATESORT_DRAWABLE,	"   Drawable",		&LLColor4::orange2, 0 },
	{ LLFastTimer::FTM_STATESORT_POSTSORT,	"   Post Sort",	&LLColor4::orange3, 0 },
	{ LLFastTimer::FTM_REBUILD_OCCLUSION_VB,"    Occlusion",		&LLColor4::cyan5, 0 },
	{ LLFastTimer::FTM_REBUILD_VBO,			"    VBO Rebuild",	&LLColor4::red4, 0 },
	{ LLFastTimer::FTM_REBUILD_VOLUME_VB,	"     Volume",		&LLColor4::blue1, 0 },
//	{ LLFastTimer::FTM_REBUILD_NONE_VB,		"      Unknown",	&LLColor4::cyan5, 0 },
//	{ LLFastTimer::FTM_REBUILD_BRIDGE_VB,	"     Bridge",		&LLColor4::blue2, 0 },
//	{ LLFastTimer::FTM_REBUILD_HUD_VB,		"     HUD",			&LLColor4::blue3, 0 },
	{ LLFastTimer::FTM_REBUILD_TERRAIN_VB,	"     Terrain",		&LLColor4::blue4, 0 },
//	{ LLFastTimer::FTM_REBUILD_WATER_VB,	"     Water",		&LLColor4::blue5, 0 },
//	{ LLFastTimer::FTM_REBUILD_TREE_VB,		"     Tree",		&LLColor4::cyan1, 0 },
	{ LLFastTimer::FTM_REBUILD_PARTICLE_VB,	"     Particle",	&LLColor4::cyan2, 0 },
//	{ LLFastTimer::FTM_REBUILD_CLOUD_VB,	"     Cloud",		&LLColor4::cyan3, 0 },
//	{ LLFastTimer::FTM_REBUILD_GRASS_VB,	"     Grass",		&LLColor4::cyan4, 0 },
 	{ LLFastTimer::FTM_RENDER_GEOMETRY,		"  Geometry",		&LLColor4::green2, 1 },
	{ LLFastTimer::FTM_POOLS,				"   Pools",			&LLColor4::green3, 1 },
	{ LLFastTimer::FTM_POOLRENDER,			"    RenderPool",	&LLColor4::green4, 1 },
	{ LLFastTimer::FTM_RENDER_TERRAIN,		"     Terrain",		&LLColor4::green6, 0 },
	{ LLFastTimer::FTM_RENDER_CHARACTERS,	"     Avatars",		&LLColor4::yellow1, 0 },
	{ LLFastTimer::FTM_RENDER_SIMPLE,		"     Simple",		&LLColor4::yellow2, 0 },
	{ LLFastTimer::FTM_RENDER_FULLBRIGHT,	"     Fullbright",	&LLColor4::yellow5, 0 },
	{ LLFastTimer::FTM_RENDER_GLOW,			"     Glow",		&LLColor4::orange1, 0 },
	{ LLFastTimer::FTM_RENDER_GRASS,		"     Grass",		&LLColor4::yellow6, 0 },
	{ LLFastTimer::FTM_RENDER_INVISIBLE,	"     Invisible",	&LLColor4::red2, 0 },
	{ LLFastTimer::FTM_RENDER_SHINY,		"     Shiny",		&LLColor4::yellow3, 0 },
	{ LLFastTimer::FTM_RENDER_BUMP,			"     Bump",		&LLColor4::yellow4, 0 },
	{ LLFastTimer::FTM_RENDER_TREES,		"     Trees",		&LLColor4::yellow8, 0 },
	{ LLFastTimer::FTM_RENDER_OCCLUSION,	"     Occlusion",	&LLColor4::red1, 0 },
	{ LLFastTimer::FTM_RENDER_CLOUDS,		"     Clouds",		&LLColor4::yellow5, 0 },
	{ LLFastTimer::FTM_RENDER_ALPHA,		"     Alpha",		&LLColor4::yellow6, 0 },
	{ LLFastTimer::FTM_RENDER_HUD,			"     HUD",			&LLColor4::yellow7, 0 },
	{ LLFastTimer::FTM_RENDER_WATER,		"     Water",		&LLColor4::yellow9, 0 },
	{ LLFastTimer::FTM_RENDER_WL_SKY,		"     WL Sky",		&LLColor4::blue3,	0 },
	{ LLFastTimer::FTM_RENDER_FAKE_VBO_UPDATE,"     Fake VBO update",		&LLColor4::red2,	0 },
	{ LLFastTimer::FTM_RENDER_BLOOM,		"   Bloom",			&LLColor4::blue4, 0 },
	{ LLFastTimer::FTM_RENDER_BLOOM_FBO,		"    First FBO",			&LLColor4::blue, 0 },
	{ LLFastTimer::FTM_RENDER_UI,			"  UI",				&LLColor4::cyan4, 1 },
	{ LLFastTimer::FTM_RENDER_TIMER,		"   Timers",		&LLColor4::cyan5, 1, 0 },
	{ LLFastTimer::FTM_RENDER_FONTS,		"   Fonts",			&LLColor4::pink1, 0 },
	{ LLFastTimer::FTM_SWAP,				"  Swap",			&LLColor4::pink2, 0 },
	{ LLFastTimer::FTM_CLIENT_COPY,			"  Client Copy",	&LLColor4::red1, 1},

#if 0 || !LL_RELEASE_FOR_DOWNLOAD
	{ LLFastTimer::FTM_TEMP1,				" Temp1",			&LLColor4::red1, 0 },
	{ LLFastTimer::FTM_TEMP2,				" Temp2",			&LLColor4::magenta1, 0 },
	{ LLFastTimer::FTM_TEMP3,				" Temp3",			&LLColor4::red2, 0 },
	{ LLFastTimer::FTM_TEMP4,				" Temp4",			&LLColor4::magenta2, 0 },
	{ LLFastTimer::FTM_TEMP5,				" Temp5",			&LLColor4::red3, 0 },
	{ LLFastTimer::FTM_TEMP6,				" Temp6",			&LLColor4::magenta3, 0 },
	{ LLFastTimer::FTM_TEMP7,				" Temp7",			&LLColor4::red4, 0 },
	{ LLFastTimer::FTM_TEMP8,				" Temp8",			&LLColor4::magenta4, 0 },
#endif
	
	{ LLFastTimer::FTM_OTHER,				" Other",			&red0 }
};
static int ft_display_didcalc = 0;
static const int FTV_DISPLAY_NUM  = (sizeof(ft_display_table)/sizeof(ft_display_table[0]));

S32 ft_display_idx[FTV_DISPLAY_NUM]; // line of table entry for display purposes (for collapse)

LLFastTimerView::LLFastTimerView(const std::string& name, const LLRect& rect)
	:	LLFloater(name, rect, std::string("Fast Timers"))
{
	setVisible(FALSE);
	mDisplayMode = 0;
	mAvgCountTotal = 0;
	mMaxCountTotal = 0;
	mDisplayCenter = 1;
	mDisplayCalls = 0;
	mDisplayHz = 0;
	mScrollIndex = 0;
	mHoverIndex = -1;
	mHoverBarIndex = -1;
	mBarStart = new S32[(MAX_VISIBLE_HISTORY + 1) * FTV_DISPLAY_NUM];
	memset(mBarStart, 0, (MAX_VISIBLE_HISTORY + 1) * FTV_DISPLAY_NUM * sizeof(S32));
	mBarEnd = new S32[(MAX_VISIBLE_HISTORY + 1) * FTV_DISPLAY_NUM];
	memset(mBarEnd, 0, (MAX_VISIBLE_HISTORY + 1) * FTV_DISPLAY_NUM * sizeof(S32));
	mSubtractHidden = 0;
	mPrintStats = -1;	

	// One-time setup
	if (!ft_display_didcalc)
	{
		int pidx[FTV_DISPLAY_NUM];
		int pdisabled[FTV_DISPLAY_NUM];
		for (S32 i=0; i < FTV_DISPLAY_NUM; i++)
		{
			int level = 0;
			const char *text = ft_display_table[i].desc;
			while(text[0] == ' ')
			{
				text++;
				level++;
			}
			llassert(level < FTV_DISPLAY_NUM);
			ft_display_table[i].desc = text;
			ft_display_table[i].level = level;
			if (level > 0)
			{
				ft_display_table[i].parent = pidx[level-1];
				if (pdisabled[level-1])
				{
					ft_display_table[i].disabled = 3;
				}
			}
			else
			{
				ft_display_table[i].parent = -1;
			}
			ft_display_idx[i] = i;
			pidx[level] = i;
			pdisabled[level] = ft_display_table[i].disabled;
		}
		ft_display_didcalc = 1;
	}
}

LLFastTimerView::~LLFastTimerView()
{
	delete[] mBarStart;
	delete[] mBarEnd;
}

BOOL LLFastTimerView::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (mBarRect.pointInRect(x, y))
	{
		S32 bar_idx = MAX_VISIBLE_HISTORY - ((y - mBarRect.mBottom) * (MAX_VISIBLE_HISTORY + 2) / mBarRect.getHeight());
		bar_idx = llclamp(bar_idx, 0, MAX_VISIBLE_HISTORY);
		mPrintStats = bar_idx;
// 		return TRUE; // for now, pass all mouse events through
	}
	return FALSE;
}

S32 LLFastTimerView::getLegendIndex(S32 y)
{
	S32 idx = (getRect().getHeight() - y) / ((S32) LLFontGL::sMonospace->getLineHeight()+2) - 5;
	if (idx >= 0 && idx < FTV_DISPLAY_NUM)
	{
		return ft_display_idx[idx];
	}
	
	return -1;
}

BOOL LLFastTimerView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (x < mBarRect.mLeft) 
	{
		S32 legend_index = getLegendIndex(y);
		if (legend_index >= 0 && legend_index < FTV_DISPLAY_NUM)
		{
			S32 disabled = ft_display_table[legend_index].disabled;
			if (++disabled > 2)
				disabled = 0;
			ft_display_table[legend_index].disabled = disabled;
			S32 level = ft_display_table[legend_index].level;

			// propagate enable/disable to all children
			legend_index++;
			while (legend_index < FTV_DISPLAY_NUM && ft_display_table[legend_index].level > level)
			{
				ft_display_table[legend_index].disabled = disabled ? 3 : 0;
				legend_index++;
			}
		}
	}
	else if (mask & MASK_ALT)
	{
		if (mask & MASK_SHIFT)
		{
			mSubtractHidden = !mSubtractHidden;
		}
		else if (mask & MASK_CONTROL)
		{
			mDisplayHz = !mDisplayHz;	
		}
		else
		{
			mDisplayCalls = !mDisplayCalls;
		}
	}
	else if (mask & MASK_SHIFT)
	{
		if (++mDisplayMode > 3)
			mDisplayMode = 0;
	}
	else if (mask & MASK_CONTROL)
	{
		if (++mDisplayCenter > 2)
			mDisplayCenter = 0;
	}
	else
	{
		// pause/unpause
		LLFastTimer::sPauseHistory = !LLFastTimer::sPauseHistory;
		// reset scroll to bottom when unpausing
		if (!LLFastTimer::sPauseHistory)
		{
			mScrollIndex = 0;
		}
	}
	// SJB: Don't pass mouse clicks through the display
	return TRUE;
}

BOOL LLFastTimerView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	return FALSE;
}


BOOL LLFastTimerView::handleHover(S32 x, S32 y, MASK mask)
{
	if(LLFastTimer::sPauseHistory && mBarRect.pointInRect(x, y))
	{
		mHoverIndex = -1;
		mHoverBarIndex = MAX_VISIBLE_HISTORY - ((y - mBarRect.mBottom) * (MAX_VISIBLE_HISTORY + 2) / mBarRect.getHeight());
		if (mHoverBarIndex == 0)
		{
			return TRUE;
		}
		else if (mHoverBarIndex == -1)
		{
			mHoverBarIndex = 0;
		}
		for (S32 i = 0; i < FTV_DISPLAY_NUM; i++)
		{
			if (x > mBarStart[mHoverBarIndex * FTV_DISPLAY_NUM + i] &&
				x < mBarEnd[mHoverBarIndex * FTV_DISPLAY_NUM + i] &&
				ft_display_table[i].disabled <= 1)
			{
				mHoverIndex = i;
			}
		}
	}
	else if (x < mBarRect.mLeft) 
	{
		S32 legend_index = getLegendIndex(y);
		if (legend_index >= 0 && legend_index < FTV_DISPLAY_NUM)
		{
			mHoverIndex = legend_index;
		}
	}
	
	return FALSE;
}

BOOL LLFastTimerView::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	LLFastTimer::sPauseHistory = TRUE;
	mScrollIndex = llclamp(mScrollIndex - clicks, 
							0, llmin(LLFastTimer::sLastFrameIndex, (S32)LLFastTimer::FTM_HISTORY_NUM-MAX_VISIBLE_HISTORY));
	return TRUE;
}

void LLFastTimerView::draw()
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_TIMER);
	
	std::string tdesc;

	F64 clock_freq = (F64)LLFastTimer::countsPerSecond();
	F64 iclock_freq = 1000.0 / clock_freq;
	
	S32 margin = 10;
	S32 height = (S32) (gViewerWindow->getVirtualWindowRect().getHeight()*0.75f);
	S32 width = (S32) (gViewerWindow->getVirtualWindowRect().getWidth() * 0.75f);
	
	// HACK: casting away const. Should use setRect or some helper function instead.
		const_cast<LLRect&>(getRect()).setLeftTopAndSize(getRect().mLeft, getRect().mTop, width, height);

	S32 left, top, right, bottom;
	S32 x, y, barw, barh, dx, dy;
	S32 texth, textw;
	LLPointer<LLUIImage> box_imagep = LLUI::getUIImage("rounded_square.tga");

	// Make sure all timers are accounted for
	// Set 'FTM_OTHER' to unaccounted ticks last frame
	{
		S32 display_timer[LLFastTimer::FTM_NUM_TYPES];
		S32 hidx = LLFastTimer::sLastFrameIndex % LLFastTimer::FTM_HISTORY_NUM;
		for (S32 i=0; i < LLFastTimer::FTM_NUM_TYPES; i++)
		{
			display_timer[i] = 0;
		}
		for (S32 i=0; i < FTV_DISPLAY_NUM; i++)
		{
			S32 tidx = ft_display_table[i].timer;
			display_timer[tidx] = 1;
		}
		LLFastTimer::sCountHistory[hidx][LLFastTimer::FTM_OTHER] = 0;
		LLFastTimer::sCallHistory[hidx][LLFastTimer::FTM_OTHER] = 0;
		for (S32 tidx = 0; tidx < LLFastTimer::FTM_NUM_TYPES; tidx++)
		{
			U64 counts = LLFastTimer::sCountHistory[hidx][tidx];
			if (counts > 0 && display_timer[tidx] == 0)
			{
				LLFastTimer::sCountHistory[hidx][LLFastTimer::FTM_OTHER] += counts;
				LLFastTimer::sCallHistory[hidx][LLFastTimer::FTM_OTHER] += 1;
			}
		}
		LLFastTimer::sCountAverage[LLFastTimer::FTM_OTHER] = 0;
		LLFastTimer::sCallAverage[LLFastTimer::FTM_OTHER] = 0;
		for (S32 h = 0; h < LLFastTimer::FTM_HISTORY_NUM; h++)
		{
			LLFastTimer::sCountAverage[LLFastTimer::FTM_OTHER] += LLFastTimer::sCountHistory[h][LLFastTimer::FTM_OTHER];
			LLFastTimer::sCallAverage[LLFastTimer::FTM_OTHER] += LLFastTimer::sCallHistory[h][LLFastTimer::FTM_OTHER];
		}
		LLFastTimer::sCountAverage[LLFastTimer::FTM_OTHER] /= LLFastTimer::FTM_HISTORY_NUM;
		LLFastTimer::sCallAverage[LLFastTimer::FTM_OTHER] /= LLFastTimer::FTM_HISTORY_NUM;
	}
	
	// Draw the window background
	{
		LLGLSNoTexture gls_ui_no_texture;
		gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, LLColor4(0.f, 0.f, 0.f, 0.25f));
	}
	
	S32 xleft = margin;
	S32 ytop = margin;
	
	// Draw some help
	{
		
		x = xleft;
		y = height - ytop;
		texth = (S32)LLFontGL::sMonospace->getLineHeight();

		char modedesc[][32] = {
			"2 x Average ",
			"Max         ",
			"Recent Max  ",
			"100 ms      "
		};
		char centerdesc[][32] = {
			"Left      ",
			"Centered  ",
			"Ordered   "
		};

		tdesc = llformat("Full bar = %s [Click to pause/reset] [SHIFT-Click to toggle]",modedesc[mDisplayMode]);
		LLFontGL::sMonospace->renderUTF8(tdesc, 0, x, y, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);

		textw = LLFontGL::sMonospace->getWidth(tdesc);

		x = xleft, y -= (texth + 2);
		tdesc = llformat("Justification = %s [CTRL-Click to toggle]",centerdesc[mDisplayCenter]);
		LLFontGL::sMonospace->renderUTF8(tdesc, 0, x, y, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);
		y -= (texth + 2);

		LLFontGL::sMonospace->renderUTF8(std::string("[Right-Click log selected] [ALT-Click toggle counts] [ALT-SHIFT-Click sub hidden]"),
										 0, x, y, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);
		y -= (texth + 2);
	}

	// Calc the total ticks
	S32 histmax = llmin(LLFastTimer::sLastFrameIndex+1, MAX_VISIBLE_HISTORY);
	U64 ticks_sum[LLFastTimer::FTM_HISTORY_NUM+1][FTV_DISPLAY_NUM];
	for (S32 j=-1; j<LLFastTimer::FTM_HISTORY_NUM; j++)
	{
		S32 hidx;
		if (j >= 0)
			hidx = (LLFastTimer::sLastFrameIndex+j) % LLFastTimer::FTM_HISTORY_NUM;
		else
			hidx = -1;

		// calculate tick info by adding child ticks to parents
		for (S32 i=0; i < FTV_DISPLAY_NUM; i++)
		{
			if (mSubtractHidden && ft_display_table[i].disabled > 1)
			{
				continue;
			}
			// Get ticks
			S32 tidx = ft_display_table[i].timer;
			if (hidx >= 0)
				 ticks_sum[j+1][i] = LLFastTimer::sCountHistory[hidx][tidx];
			else
				 ticks_sum[j+1][i] = LLFastTimer::sCountAverage[tidx];
			S32 pidx = ft_display_table[i].parent;
			// Add ticks to parents
			while (pidx >= 0)
			{
				ticks_sum[j+1][pidx] += ticks_sum[j+1][i];
				pidx = ft_display_table[pidx].parent;
			}
		}
	}
		
	// Draw the legend

	S32 legendwidth = 0;
	xleft = margin;
	ytop = y;

	y -= (texth + 2);

	S32 cur_line = 0;
	S32 display_line[FTV_DISPLAY_NUM];
	for (S32 i=0; i<FTV_DISPLAY_NUM; i++)
	{
		S32 disabled = ft_display_table[i].disabled;
		if (disabled == 3)
		{
			continue; // skip row
		}
		display_line[i] = cur_line;
		ft_display_idx[cur_line] = i;
		cur_line++;
		S32 level = ft_display_table[i].level;
		S32 parent = ft_display_table[i].parent;
		
		x = xleft;

		left = x; right = x + texth;
		top = y; bottom = y - texth;
		S32 scale_offset = 0;
		if (i == mHoverIndex)
		{
			scale_offset = llfloor(sinf(mHighlightTimer.getElapsedTimeF32() * 6.f) * 2.f);
		}
		gl_rect_2d(left - scale_offset, top + scale_offset, right + scale_offset, bottom - scale_offset, *ft_display_table[i].color);

		int tidx = ft_display_table[i].timer;
		F32 ms = 0;
		S32 calls = 0;
		if (mHoverBarIndex > 0 && mHoverIndex >= 0)
		{
			S32 hidx = (LLFastTimer::sLastFrameIndex + (mHoverBarIndex - 1) - mScrollIndex) % LLFastTimer::FTM_HISTORY_NUM;
			S32 bidx = LLFastTimer::FTM_HISTORY_NUM - mScrollIndex - mHoverBarIndex;
			U64 ticks = ticks_sum[bidx+1][i]; // : LLFastTimer::sCountHistory[hidx][tidx];
			ms = (F32)((F64)ticks * iclock_freq);
			calls = (S32)LLFastTimer::sCallHistory[hidx][tidx];
		}
		else
		{
			U64 ticks = ticks_sum[0][i];
			ms = (F32)((F64)ticks * iclock_freq);
			calls = (S32)LLFastTimer::sCallAverage[tidx];
		}
		if (mDisplayCalls)
		{
			tdesc = llformat("%s (%d)",ft_display_table[i].desc,calls);
		}
		else
		{
			tdesc = llformat("%s [%.1f]",ft_display_table[i].desc,ms);
		}
		dx = (texth+4) + level*8;

		LLColor4 color = disabled > 1 ? LLColor4::grey : LLColor4::white;
		if (level > 0)
		{
			S32 line_start_y = (top + bottom) / 2;
			S32 line_end_y = line_start_y + ((texth + 2) * (display_line[i] - display_line[parent])) - (texth / 2);
			gl_line_2d(x + dx - 8, line_start_y, x + dx, line_start_y, color);
			S32 line_x = x + (texth + 4) + ((level - 1) * 8);
			gl_line_2d(line_x, line_start_y, line_x, line_end_y, color);
			if (disabled == 1)
			{
				gl_line_2d(line_x+4, line_start_y-3, line_x+4, line_start_y+4, color);
			}
		}

		x += dx;
		BOOL is_child_of_hover_item = (i == mHoverIndex);
		S32 next_parent = ft_display_table[i].parent;
		while(!is_child_of_hover_item && next_parent >= 0)
		{
			is_child_of_hover_item = (mHoverIndex == next_parent);
			next_parent = ft_display_table[next_parent].parent;
		}

		if (is_child_of_hover_item)
		{
			LLFontGL::sMonospace->renderUTF8(tdesc, 0, x, y, color, LLFontGL::LEFT, LLFontGL::TOP, LLFontGL::BOLD);
		}
		else
		{
			LLFontGL::sMonospace->renderUTF8(tdesc, 0, x, y, color, LLFontGL::LEFT, LLFontGL::TOP);
		}
		y -= (texth + 2);

		textw = dx + LLFontGL::sMonospace->getWidth(std::string(ft_display_table[i].desc)) + 40;
		if (textw > legendwidth)
			legendwidth = textw;
	}
	for (S32 i=cur_line; i<FTV_DISPLAY_NUM; i++)
	{
		ft_display_idx[i] = -1;
	}
	xleft += legendwidth + 8;
	// ytop = ytop;

	// update rectangle that includes timer bars
	mBarRect.mLeft = xleft;
	mBarRect.mRight = getRect().mRight - xleft;
	mBarRect.mTop = ytop - ((S32)LLFontGL::sMonospace->getLineHeight() + 4);
	mBarRect.mBottom = margin + LINE_GRAPH_HEIGHT;

	y = ytop;
	barh = (ytop - margin - LINE_GRAPH_HEIGHT) / (MAX_VISIBLE_HISTORY + 2);
	dy = barh>>2; // spacing between bars
	if (dy < 1) dy = 1;
	barh -= dy;
	barw = width - xleft - margin;

	// Draw the history bars
	if (LLFastTimer::sLastFrameIndex >= 0)
	{	
		U64 totalticks;
		if (!LLFastTimer::sPauseHistory)
		{
			U64 ticks = 0;
			int hidx = (LLFastTimer::sLastFrameIndex - mScrollIndex) % LLFastTimer::FTM_HISTORY_NUM;
			for (S32 i=0; i<FTV_DISPLAY_NUM; i++)
			{
				if (mSubtractHidden && ft_display_table[i].disabled > 1)
				{
					continue;
				}
				int tidx = ft_display_table[i].timer;
				ticks += LLFastTimer::sCountHistory[hidx][tidx];
			}
			if (LLFastTimer::sCurFrameIndex >= 10)
			{
				U64 framec = LLFastTimer::sCurFrameIndex;
				U64 avg = (U64)mAvgCountTotal;
				mAvgCountTotal = (avg*framec + ticks) / (framec + 1);
				if (ticks > mMaxCountTotal)
				{
					mMaxCountTotal = ticks;
				}
			}
#if 1
			if (ticks < mAvgCountTotal/100 || ticks > mAvgCountTotal*100)
				LLFastTimer::sResetHistory = 1;
#endif
			if (LLFastTimer::sCurFrameIndex < 10 || LLFastTimer::sResetHistory)
			{
				mAvgCountTotal = ticks;
				mMaxCountTotal = ticks;
			}
		}

		if (mDisplayMode == 0)
		{
			totalticks = mAvgCountTotal*2;
		}
		else if (mDisplayMode == 1)
		{
			totalticks = mMaxCountTotal;
		}
		else if (mDisplayMode == 2)
		{
			// Calculate the max total ticks for the current history
			totalticks = 0;
			for (S32 j=0; j<histmax; j++)
			{
				U64 ticks = 0;
				for (S32 i=0; i<FTV_DISPLAY_NUM; i++)
				{
					if (mSubtractHidden && ft_display_table[i].disabled > 1)
					{
						continue;
					}
					int tidx = ft_display_table[i].timer;
					ticks += LLFastTimer::sCountHistory[j][tidx];
				}
				if (ticks > totalticks)
					totalticks = ticks;
			}
		}
		else
		{
			totalticks = (U64)(clock_freq * .1); // 100 ms
		}
		
		// Draw MS ticks
		{
			U32 ms = (U32)((F64)totalticks * iclock_freq) ;

			tdesc = llformat("%.1f ms |", (F32)ms*.25f);
			x = xleft + barw/4 - LLFontGL::sMonospace->getWidth(tdesc);
			LLFontGL::sMonospace->renderUTF8(tdesc, 0, x, y, LLColor4::white,
										 LLFontGL::LEFT, LLFontGL::TOP);
			
			tdesc = llformat("%.1f ms |", (F32)ms*.50f);
			x = xleft + barw/2 - LLFontGL::sMonospace->getWidth(tdesc);
			LLFontGL::sMonospace->renderUTF8(tdesc, 0, x, y, LLColor4::white,
										 LLFontGL::LEFT, LLFontGL::TOP);
			
			tdesc = llformat("%.1f ms |", (F32)ms*.75f);
			x = xleft + (barw*3)/4 - LLFontGL::sMonospace->getWidth(tdesc);
			LLFontGL::sMonospace->renderUTF8(tdesc, 0, x, y, LLColor4::white,
										 LLFontGL::LEFT, LLFontGL::TOP);
			
			tdesc = llformat( "%d ms |", ms);
			x = xleft + barw - LLFontGL::sMonospace->getWidth(tdesc);
			LLFontGL::sMonospace->renderUTF8(tdesc, 0, x, y, LLColor4::white,
										 LLFontGL::LEFT, LLFontGL::TOP);
		}

		LLRect graph_rect;
		// Draw borders
		{
			LLGLSNoTexture gls_ui_no_texture;
			gGL.color4f(0.5f,0.5f,0.5f,0.5f);

			S32 by = y + 2;
			
			y -= ((S32)LLFontGL::sMonospace->getLineHeight() + 4);

			//heading
			gl_rect_2d(xleft-5, by, getRect().getWidth()-5, y+5, FALSE);

			//tree view
			gl_rect_2d(5, by, xleft-10, 5, FALSE);

			by = y + 5;
			//average bar
			gl_rect_2d(xleft-5, by, getRect().getWidth()-5, by-barh-dy-5, FALSE);
			
			by -= barh*2+dy;
			
			//current frame bar
			gl_rect_2d(xleft-5, by, getRect().getWidth()-5, by-barh-dy-2, FALSE);
			
			by -= barh+dy+1;
			
			//history bars
			gl_rect_2d(xleft-5, by, getRect().getWidth()-5, LINE_GRAPH_HEIGHT-barh-dy-2, FALSE);			
			
			by = LINE_GRAPH_HEIGHT-barh-dy-7;
			
			//line graph
			graph_rect = LLRect(xleft-5, by, getRect().getWidth()-5, 5);
			
			gl_rect_2d(graph_rect, FALSE);
		}
		
		// Draw bars for each history entry
		// Special: -1 = show running average
		LLViewerImage::bindTexture(box_imagep->getImage());
		for (S32 j=-1; j<histmax && y > LINE_GRAPH_HEIGHT; j++)
		{
			int sublevel_dx[FTV_DISPLAY_NUM+1];
			int sublevel_left[FTV_DISPLAY_NUM+1];
			int sublevel_right[FTV_DISPLAY_NUM+1];
			S32 tidx;
			if (j >= 0)
			{
				tidx = LLFastTimer::FTM_HISTORY_NUM - j - 1 - mScrollIndex;
			}
			else
			{
				tidx = -1;
			}
			
			x = xleft;
			
			// draw the bars for each stat
			int xpos[FTV_DISPLAY_NUM+1];
			int deltax[FTV_DISPLAY_NUM+1];
			xpos[0] = xleft;

			for (S32 i = 0; i < FTV_DISPLAY_NUM; i++)
			{
				if (ft_display_table[i].disabled > 1)
				{
					continue;
				}

				F32 frac = (F32)ticks_sum[tidx+1][i] / (F32)totalticks;
		
				dx = llround(frac * (F32)barw);
				deltax[i] = dx;
				
				int level = ft_display_table[i].level;
				int parent = ft_display_table[i].parent;
				llassert(level < FTV_DISPLAY_NUM);
				llassert(parent < FTV_DISPLAY_NUM);
				
				left = xpos[level];
				
				S32 prev_idx = i - 1;
				while (prev_idx > 0)
				{
					if (ft_display_table[prev_idx].disabled <= 1)
					{
						break;
					}
					prev_idx--;
				}
				S32 next_idx = i + 1;
				while (next_idx < FTV_DISPLAY_NUM)
				{
					if (ft_display_table[next_idx].disabled <= 1)
					{
						break;
					}
					next_idx++;
				}

				if (level == 0)
						{
					sublevel_left[level] = xleft;
					sublevel_dx[level] = dx;
					sublevel_right[level] = sublevel_left[level] + sublevel_dx[level];
					}
				else if (i==0 || ft_display_table[prev_idx].level < level)
				{
					// If we are the first entry at a new sublevel block, calc the
					//   total width of this sublevel and modify left to align block.
						U64 sublevelticks = ticks_sum[tidx+1][i];
						for (S32 k=i+1; k<FTV_DISPLAY_NUM; k++)
						{
							if (ft_display_table[k].level < level)
								break;
							if (ft_display_table[k].disabled <= 1 && ft_display_table[k].level == level)
								sublevelticks += ticks_sum[tidx+1][k];
						}
						F32 subfrac = (F32)sublevelticks / (F32)totalticks;
					sublevel_dx[level] = (int)(subfrac * (F32)barw + .5f);

					if (mDisplayCenter == 1) // center aligned
					{
						left += (deltax[parent] - sublevel_dx[level])/2;
					}
					else if (mDisplayCenter == 2) // right aligned
					{
						left += (deltax[parent] - sublevel_dx[level]);
				}

					sublevel_left[level] = left;
					sublevel_right[level] = sublevel_left[level] + sublevel_dx[level];
				}				

				right = left + dx;
				xpos[level] = right;
				xpos[level+1] = left;
				
				mBarStart[(j + 1) * FTV_DISPLAY_NUM + i] = left;
				mBarEnd[(j + 1) * FTV_DISPLAY_NUM + i] = right;

				top = y;
				bottom = y - barh;

				if (right > left)
				{
					//U32 rounded_edges = 0;
					LLColor4 color = *ft_display_table[i].color;
					S32 scale_offset = 0;

					BOOL is_child_of_hover_item = (i == mHoverIndex);
					S32 next_parent = ft_display_table[i].parent;
					while(!is_child_of_hover_item && next_parent >= 0)
					{
						is_child_of_hover_item = (mHoverIndex == next_parent);
						next_parent = ft_display_table[next_parent].parent;
					}

					if (i == mHoverIndex)
					{
						scale_offset = llfloor(sinf(mHighlightTimer.getElapsedTimeF32() * 6.f) * 3.f);
						//color = lerp(color, LLColor4::black, -0.4f);
					}
					else if (mHoverIndex >= 0 && !is_child_of_hover_item)
					{
						color = lerp(color, LLColor4::grey, 0.8f);
					}

					gGL.color4fv(color.mV);
					F32 start_fragment = llclamp((F32)(left - sublevel_left[level]) / (F32)sublevel_dx[level], 0.f, 1.f);
					F32 end_fragment = llclamp((F32)(right - sublevel_left[level]) / (F32)sublevel_dx[level], 0.f, 1.f);
					gl_segmented_rect_2d_fragment_tex(sublevel_left[level], top - level + scale_offset, sublevel_right[level], bottom + level - scale_offset, box_imagep->getTextureWidth(), box_imagep->getTextureHeight(), 16, start_fragment, end_fragment);

				}
					
			}
			y -= (barh + dy);
			if (j < 0)
				y -= barh;
		}
		
		//draw line graph history
		{
			LLGLSNoTexture no_texture;
			LLLocalClipRect clip(graph_rect);
			
			//normalize based on last frame's maximum
			static U64 last_max = 0;
			static F32 alpha_interp = 0.f;
			U64 max_ticks = llmax(last_max, (U64) 1);			
			F32 ms = (F32)((F64)max_ticks * iclock_freq);
			
			//display y-axis range
			std::string tdesc;
			 if (mDisplayCalls)
				tdesc = llformat("%d calls", (int)max_ticks);
			else if (mDisplayHz)
				tdesc = llformat("%d Hz", (int)max_ticks);
			else
				tdesc = llformat("%4.2f ms", ms);
							
			x = graph_rect.mRight - LLFontGL::sMonospace->getWidth(tdesc)-5;
			y = graph_rect.mTop - ((S32)LLFontGL::sMonospace->getLineHeight());
 
			LLFontGL::sMonospace->renderUTF8(tdesc, 0, x, y, LLColor4::white,
										 LLFontGL::LEFT, LLFontGL::TOP);

			//highlight visible range
			{
				S32 first_frame = LLFastTimer::FTM_HISTORY_NUM - mScrollIndex;
				S32 last_frame = first_frame - MAX_VISIBLE_HISTORY;
				
				F32 frame_delta = ((F32) (graph_rect.getWidth()))/(LLFastTimer::FTM_HISTORY_NUM-1);
				
				F32 right = (F32) graph_rect.mLeft + frame_delta*first_frame;
				F32 left = (F32) graph_rect.mLeft + frame_delta*last_frame;
				
				gGL.color4f(0.5f,0.5f,0.5f,0.3f);
				gl_rect_2d((S32) left, graph_rect.mTop, (S32) right, graph_rect.mBottom);
				
				if (mHoverBarIndex >= 0)
				{
					S32 bar_frame = first_frame - mHoverBarIndex;
					F32 bar = (F32) graph_rect.mLeft + frame_delta*bar_frame;

					gGL.color4f(0.5f,0.5f,0.5f,1);
				
					gGL.begin(LLVertexBuffer::LINES);
					gGL.vertex2i((S32)bar, graph_rect.mBottom);
					gGL.vertex2i((S32)bar, graph_rect.mTop);
					gGL.end();
				}
			}
			
			U64 cur_max = 0;
			for (S32 idx = 0; idx < FTV_DISPLAY_NUM; ++idx)
			{
				if (ft_display_table[idx].disabled > 1)
				{	//skip disabled timers
					continue;
				}
				
				//fatten highlighted timer
				if (mHoverIndex == idx)
				{
					gGL.flush();
					glLineWidth(3);
				}
			
				const F32 * col = ft_display_table[idx].color->mV;
				
				F32 alpha = 1.f;
				
				if (mHoverIndex >= 0 &&
					idx != mHoverIndex)
				{	//fade out non-hihglighted timers
					if (ft_display_table[idx].parent != mHoverIndex)
					{
						alpha = alpha_interp;
					}
				}

				gGL.color4f(col[0], col[1], col[2], alpha);				
				gGL.begin(LLVertexBuffer::LINE_STRIP);
				for (U32 j = 0; j < LLFastTimer::FTM_HISTORY_NUM; j++)
				{
					U64 ticks = ticks_sum[j+1][idx];

					if (mDisplayHz)
					{
						F64 tc = (F64) (ticks+1) * iclock_freq;
						tc = 1000.f/tc;
						ticks = llmin((U64) tc, (U64) 1024);
					}
					else if (mDisplayCalls)
					{
						S32 tidx = ft_display_table[idx].timer;
						S32 hidx = (LLFastTimer::sLastFrameIndex + j) % LLFastTimer::FTM_HISTORY_NUM;
						ticks = (S32)LLFastTimer::sCallHistory[hidx][tidx];
					}
										
					if (alpha == 1.f)
					{ //normalize to highlighted timer
						cur_max = llmax(cur_max, ticks);
					}
					F32 x = graph_rect.mLeft + ((F32) (graph_rect.getWidth()))/(LLFastTimer::FTM_HISTORY_NUM-1)*j;
					F32 y = graph_rect.mBottom + (F32) graph_rect.getHeight()/max_ticks*ticks;
					gGL.vertex2f(x,y);
				}
				gGL.end();
				
				if (mHoverIndex == idx)
				{
					gGL.flush();
					glLineWidth(1);
				}
			}
			
			//interpolate towards new maximum
			F32 dt = gFrameIntervalSeconds*3.f;
			last_max = (U64) ((F32) last_max + ((F32) cur_max- (F32) last_max) * dt);
			F32 alpha_target = last_max > cur_max ?
								llmin((F32) last_max/ (F32) cur_max - 1.f,1.f) :
								llmin((F32) cur_max/ (F32) last_max - 1.f,1.f);
			
			alpha_interp = alpha_interp + (alpha_target-alpha_interp) * dt;

			if (mHoverIndex >= 0)
			{
				x = (graph_rect.mRight + graph_rect.mLeft)/2;
				y = graph_rect.mBottom + 8;

				LLFontGL::sMonospace->renderUTF8(std::string(ft_display_table[mHoverIndex].desc), 0, x, y, LLColor4::white,
					LLFontGL::LEFT, LLFontGL::BOTTOM);
			}					
		}
	}

	// Output stats for clicked bar to log
	if (mPrintStats >= 0)
	{
		std::string legend_stat;
		S32 stat_num;
		S32 first = 1;
		for (stat_num = 0; stat_num < FTV_DISPLAY_NUM; stat_num++)
		{
			if (ft_display_table[stat_num].disabled > 1)
				continue;
			if (!first)
				legend_stat += ", ";
			first=0;
			legend_stat += ft_display_table[stat_num].desc;
		}
		llinfos << legend_stat << llendl;

		std::string timer_stat;
		first = 1;
		for (stat_num = 0; stat_num < FTV_DISPLAY_NUM; stat_num++)
		{
			S32 disabled = ft_display_table[stat_num].disabled;
			if (disabled > 1)
				continue;
			if (!first)
				timer_stat += ", ";
			first=0;
			U64 ticks;
			S32 tidx = ft_display_table[stat_num].timer;
			if (mPrintStats > 0)
			{
				S32 hidx = (LLFastTimer::sLastFrameIndex+(mPrintStats-1)-mScrollIndex) % LLFastTimer::FTM_HISTORY_NUM;
				ticks = disabled >= 1 ? ticks_sum[mPrintStats][stat_num] : LLFastTimer::sCountHistory[hidx][tidx];
			}
			else
			{
				ticks = disabled >= 1 ? ticks_sum[0][stat_num] : LLFastTimer::sCountAverage[tidx];
			}
			F32 ms = (F32)((F64)ticks * iclock_freq);

			timer_stat += llformat("%.1f",ms);
		}
		llinfos << timer_stat << llendl;
		mPrintStats = -1;
	}
		
	mHoverIndex = -1;
	mHoverBarIndex = -1;

	LLView::draw();
}

F64 LLFastTimerView::getTime(LLFastTimer::EFastTimerType tidx)
{
	// Find table index
	S32 i;
	for (i=0; i<FTV_DISPLAY_NUM; i++)
	{
		if (tidx == ft_display_table[i].timer)
		{
			break;
		}
	}

	if (i == FTV_DISPLAY_NUM)
	{
		// walked off the end of ft_display_table without finding
		// the desired timer type
		llwarns << "Timer type " << tidx << " not known." << llendl;
		return F64(0.0);
	}

	S32 table_idx = i;
	
	// Add child ticks to parent
	U64 ticks = LLFastTimer::sCountAverage[tidx];
	S32 level = ft_display_table[table_idx].level;
	for (i=table_idx+1; i<FTV_DISPLAY_NUM; i++)
	{
		if (ft_display_table[i].level <= level)
		{
			break;
		}
		ticks += LLFastTimer::sCountAverage[ft_display_table[i].timer];
	}

	return (F64)ticks / (F64)LLFastTimer::countsPerSecond();
}
