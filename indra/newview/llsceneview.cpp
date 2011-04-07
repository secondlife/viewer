/** 
 * @file llsceneview.cpp
 * @brief LLSceneView class implementation
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

#include "llsceneview.h"
#include "llviewerwindow.h"
#include "pipeline.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llagent.h"
#include "llvolumemgr.h"

LLSceneView* gSceneView = NULL;

//borrow this helper function from llfasttimerview.cpp
template <class VEC_TYPE>
void removeOutliers(std::vector<VEC_TYPE>& data, F32 k);


LLSceneView::LLSceneView(const LLRect& rect)
	:	LLFloater(LLSD())
{
	setRect(rect);
	setVisible(FALSE);
	
	setCanMinimize(false);
	setCanClose(true);
}

void LLSceneView::onClickCloseBtn()
{
	setVisible(false);
}


void LLSceneView::draw()
{
	S32 margin = 10;
	S32 height = (S32) (gViewerWindow->getWindowRectScaled().getHeight()*0.75f);
	S32 width = (S32) (gViewerWindow->getWindowRectScaled().getWidth() * 0.75f);
	
	LLRect new_rect;
	new_rect.setLeftTopAndSize(getRect().mLeft, getRect().mTop, width, height);
	setRect(new_rect);

	// Draw the window background
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, LLColor4(0.f, 0.f, 0.f, 0.25f));
	

	//aggregate some statistics

	//object sizes
	std::vector<F32> size[2];

	std::vector<S32> triangles[2]; 
	std::vector<S32> visible_triangles[2];

	S32 total_visible_triangles[] = {0, 0};
	S32 total_triangles[] = {0, 0};

	
	U32 object_count = 0;

	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		for (U32 i = 0; i < gObjectList.getNumObjects(); ++i)
		{
			LLViewerObject* object = gObjectList.getObject(i);
			
			if (object && 
				object->getVolume()&&
				object->getRegion() == region)
			{
				U32 idx = object->isAttachment() ? 1 : 0;

				LLVolume* volume = object->getVolume();
				object_count++;
				
				F32 radius = object->getScale().magVec();
				size[idx].push_back(radius);

				S32 visible = volume->getNumTriangles();
				S32 high_triangles = object->getHighLODTriangleCount();

				total_visible_triangles[idx] += visible;
				total_triangles[idx] += high_triangles;

				visible_triangles[idx].push_back(visible);
				triangles[idx].push_back(high_triangles);
			}
		}
	}

	const char* category[] =
	{
		"Region",
		"Attachment"
	};

	for (U32 idx = 0; idx < 2; idx++)
	{
		if (!size[idx].empty())
		{ //display graph of object sizes
			std::sort(size[idx].begin(), size[idx].end());

			ll_remove_outliers(size[idx], 1.f);

			LLRect size_rect;
			
			if (idx == 0)
			{
				size_rect = LLRect(margin, new_rect.getHeight()/2-margin*2, new_rect.getWidth()/2-margin, margin*2);
			}
			else
			{
				size_rect = LLRect(margin+new_rect.getWidth()/2, new_rect.getHeight()/2-margin*2, new_rect.getWidth()-margin, margin*2);
			}

			gl_rect_2d(size_rect, LLColor4::white, false);

			F32 size_domain[] = { 128.f, 0.f };
			
			//get domain of sizes
			for (U32 i = 0; i < size[idx].size(); ++i)
			{
				size_domain[0] = llmin(size_domain[0], size[idx][i]);
				size_domain[1] = llmax(size_domain[1], size[idx][i]);
			}

			F32 size_range = size_domain[1]-size_domain[0];
			
			U32 count = size[idx].size();

			F32 total = 0.f;

			gGL.begin(LLRender::LINE_STRIP);

			for (U32 i = 0; i < count; ++i)
			{
				F32 rad = size[idx][i];
				total += rad;	
				F32 y = (rad-size_domain[0])/size_range*size_rect.getHeight()+size_rect.mBottom;
				F32 x = (F32) i / count * size_rect.getWidth() + size_rect.mLeft;

				gGL.vertex2f(x,y);

				if (i%4096 == 0)
				{
					gGL.end();
					gGL.flush();
					gGL.begin(LLRender::LINE_STRIP);
				}
			}

			gGL.end();
			gGL.flush();

			std::string label = llformat("%s Object Sizes (m) -- Min: %.1f   Max: %.1f   Mean: %.1f   Median: %.1f -- %d samples",
											category[idx], size_domain[0], size_domain[1], total/count, size[idx][count/2], count);

			LLFontGL::getFontMonospace()->renderUTF8(label,
											0 , size_rect.mLeft, size_rect.mTop+margin, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);

		}
	}

	for (U32 idx = 0; idx < 2; ++idx)
	{
		if (!triangles[idx].empty())
		{ //plot graph of visible/total triangles
			std::sort(triangles[idx].begin(), triangles[idx].end());
			
			ll_remove_outliers(triangles[idx], 1.f);
			
			LLRect tri_rect;
			if (idx == 0)
			{
				tri_rect = LLRect(margin, new_rect.getHeight()-margin*2, new_rect.getWidth()/2-margin, new_rect.getHeight()/2 + margin);
			}
			else
			{
				tri_rect = LLRect(new_rect.getWidth()/2+margin, new_rect.getHeight()-margin*2, new_rect.getWidth()-margin, new_rect.getHeight()/2 + margin);
			}

			gl_rect_2d(tri_rect, LLColor4::white, false);

			S32 tri_domain[] = { 65536, 0 };
						
			//get domain of triangle counts
			for (U32 i = 0; i < triangles[idx].size(); ++i)
			{
				tri_domain[0] = llmin(tri_domain[0], triangles[idx][i]);
				tri_domain[1] = llmax(tri_domain[1], triangles[idx][i]);		
			}

			U32 triangle_range = tri_domain[1]-tri_domain[0];

			U32 count = triangles[idx].size();

			U32 total = 0;

			gGL.begin(LLRender::LINE_STRIP);
			//plot triangles
			for (U32 i = 0; i < count; ++i)
			{
				U32 tri_count = triangles[idx][i];
				total += tri_count;	
				F32 y = (F32) (tri_count-tri_domain[0])/triangle_range*tri_rect.getHeight()+tri_rect.mBottom;
				F32 x = (F32) i / count * tri_rect.getWidth() + tri_rect.mLeft;

				gGL.vertex2f(x,y);

				if (i%4096 == 0)
				{
					gGL.end();
					gGL.flush();
					gGL.begin(LLRender::LINE_STRIP);
				}
			}

			gGL.end();
			gGL.flush();

			U32 total_visible = 0;
			count = visible_triangles[idx].size();

			for (U32 i = 0; i < count; ++i)
			{
				U32 tri_count = visible_triangles[idx][i];
				total_visible += tri_count;	
			}

			std::string label = llformat("%s Object Triangle Counts (Ktris) -- Min: %.2f   Max: %.2f   Mean: %.2f   Median: %.2f   (%.2f/%.2f visible) -- %d samples",
											category[idx], tri_domain[0]/1024.f, tri_domain[1]/1024.f, (total/count)/1024.f, triangles[idx][count/2]/1024.f, total_visible_triangles[idx]/1024.f, total_triangles[idx]/1024.f, count);

			LLFontGL::getFontMonospace()->renderUTF8(label,
											0 , tri_rect.mLeft, tri_rect.mTop+margin, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);

		}
	}


	LLView::draw();
}


