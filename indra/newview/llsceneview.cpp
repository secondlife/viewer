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
#include "llmeshrepository.h"

LLSceneView* gSceneView = NULL;

//borrow this helper function from llfasttimerview.cpp
template <class VEC_TYPE>
void removeOutliers(std::vector<VEC_TYPE>& data, F32 k);


LLSceneView::LLSceneView(const LLRect& rect)
	:	LLFloater(LLSD())
{
	setRect(rect);
	setVisible(false);
	
	setCanMinimize(false);
	setCanClose(true);
}

void LLSceneView::onClose(bool)
{
	setVisible(false);
}

void LLSceneView::onClickCloseBtn(bool)
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

	//triangle counts
	std::vector<S32> triangles[2]; 
	std::vector<S32> visible_triangles[2];
	S32 total_visible_triangles[] = {0, 0};
	S32 total_triangles[] = {0, 0};
	
	S32 total_visible_bytes[] = {0, 0};
	S32 total_bytes[] = {0, 0};

	//streaming cost
	std::vector<F32> streaming_cost[2];
	F32 total_streaming[] = { 0.f, 0.f };

	//physics cost
	std::vector<F32> physics_cost[2];
	F32 total_physics[] = { 0.f, 0.f };
	
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

				F32 radius = object->getScale().magVec();
				size[idx].push_back(radius);

				S32 visible = volume->getNumTriangles();
				S32 high_triangles = object->getHighLODTriangleCount();

				total_visible_triangles[idx] += visible;
				total_triangles[idx] += high_triangles;

				visible_triangles[idx].push_back(visible);
				triangles[idx].push_back(high_triangles);

                F32 streaming = object->getStreamingCost();
                total_streaming[idx] += streaming;
                streaming_cost[idx].push_back(streaming);

				F32 physics = object->getPhysicsCost();
				total_physics[idx] += physics;
				physics_cost[idx].push_back(physics);

				S32 bytes = 0;
				S32 visible_bytes = 0;
                LLMeshCostData costs;
                if (object->getCostData(costs))
                {
                    bytes = costs.getSizeTotal();
                    visible_bytes = costs.getSizeByLOD(object->getLOD());
                }

				total_bytes[idx] += bytes;
				total_visible_bytes[idx] += visible_bytes;
			}
		}
	}

	const char* category[] =
	{
		"Region",
		"Attachment"
	};

	S32 graph_pos[4];

	for (U32 i = 0; i < 4; ++i)
	{
		graph_pos[i] = new_rect.getHeight()/4*(i+1);
	}

	for (U32 idx = 0; idx < 2; idx++)
	{
		if (!size[idx].empty())
		{ //display graph of object sizes
			std::sort(size[idx].begin(), size[idx].end());

			ll_remove_outliers(size[idx], 1.f);

			LLRect size_rect;
			
			if (idx == 0)
			{
				size_rect = LLRect(margin, graph_pos[0]-margin, new_rect.getWidth()/2-margin, margin*2);
			}
			else
			{
				size_rect = LLRect(margin+new_rect.getWidth()/2, graph_pos[0]-margin, new_rect.getWidth()-margin, margin*2);
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

			std::string label = llformat("%s Object Sizes (m) -- [%.1f, %.1f] Mean: %.1f  Median: %.1f -- %d samples",
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
				tri_rect = LLRect(margin, graph_pos[1]-margin, new_rect.getWidth()/2-margin, graph_pos[0]+margin);
			}
			else
			{
				tri_rect = LLRect(new_rect.getWidth()/2+margin, graph_pos[1]-margin, new_rect.getWidth()-margin, graph_pos[0]+margin);
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

			gGL.begin(LLRender::LINE_STRIP);
			//plot triangles
			for (U32 i = 0; i < count; ++i)
			{
				U32 tri_count = triangles[idx][i];
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

			count = visible_triangles[idx].size();

			std::string label = llformat("%s Object Triangle Counts (Ktris) -- Visible: %.2f/%.2f (%.2f KB Visible)",
				category[idx], total_visible_triangles[idx]/1024.f, total_triangles[idx]/1024.f, total_visible_bytes[idx]/1024.f);

			LLFontGL::getFontMonospace()->renderUTF8(label,
											0 , tri_rect.mLeft, tri_rect.mTop+margin, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);

		}
	}

	for (U32 idx = 0; idx < 2; ++idx)
	{
		if (!streaming_cost[idx].empty())
		{ //plot graph of streaming cost
			std::sort(streaming_cost[idx].begin(), streaming_cost[idx].end());
			
			ll_remove_outliers(streaming_cost[idx], 1.f);
			
			LLRect tri_rect;
			if (idx == 0)
			{
				tri_rect = LLRect(margin, graph_pos[2]-margin, new_rect.getWidth()/2-margin, graph_pos[1]+margin);
			}
			else
			{
				tri_rect = LLRect(new_rect.getWidth()/2+margin, graph_pos[2]-margin, new_rect.getWidth()-margin, graph_pos[1]+margin);
			}

			gl_rect_2d(tri_rect, LLColor4::white, false);

			F32 streaming_domain[] = { 65536, 0 };
						
			//get domain of triangle counts
			for (U32 i = 0; i < streaming_cost[idx].size(); ++i)
			{
				streaming_domain[0] = llmin(streaming_domain[0], streaming_cost[idx][i]);
				streaming_domain[1] = llmax(streaming_domain[1], streaming_cost[idx][i]);		
			}

			F32 cost_range = streaming_domain[1]-streaming_domain[0];

			U32 count = streaming_cost[idx].size();

			F32 total = 0;

			gGL.begin(LLRender::LINE_STRIP);
			//plot triangles
			for (U32 i = 0; i < count; ++i)
			{
				F32 sc = streaming_cost[idx][i];
				total += sc;	
				F32 y = (F32) (sc-streaming_domain[0])/cost_range*tri_rect.getHeight()+tri_rect.mBottom;
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

			std::string label = llformat("%s Object Streaming Cost -- [%.2f, %.2f] Mean: %.2f  Total: %.2f",
											category[idx], streaming_domain[0], streaming_domain[1], total/count, total_streaming[idx]);

			LLFontGL::getFontMonospace()->renderUTF8(label,
											0 , tri_rect.mLeft, tri_rect.mTop+margin, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);

		}
	}

	for (U32 idx = 0; idx < 2; ++idx)
	{
		if (!physics_cost[idx].empty())
		{ //plot graph of physics cost
			std::sort(physics_cost[idx].begin(), physics_cost[idx].end());
			
			ll_remove_outliers(physics_cost[idx], 1.f);
			
			LLRect tri_rect;
			if (idx == 0)
			{
				tri_rect = LLRect(margin, graph_pos[3]-margin, new_rect.getWidth()/2-margin, graph_pos[2]+margin);
			}
			else
			{
				tri_rect = LLRect(new_rect.getWidth()/2+margin, graph_pos[3]-margin, new_rect.getWidth()-margin, graph_pos[2]+margin);
			}

			gl_rect_2d(tri_rect, LLColor4::white, false);

			F32 physics_domain[] = { 65536, 0 };
						
			//get domain of triangle counts
			for (U32 i = 0; i < physics_cost[idx].size(); ++i)
			{
				physics_domain[0] = llmin(physics_domain[0], physics_cost[idx][i]);
				physics_domain[1] = llmax(physics_domain[1], physics_cost[idx][i]);		
			}

			F32 cost_range = physics_domain[1]-physics_domain[0];

			U32 count = physics_cost[idx].size();

			F32 total = 0;

			gGL.begin(LLRender::LINE_STRIP);
			//plot triangles
			for (U32 i = 0; i < count; ++i)
			{
				F32 pc = physics_cost[idx][i];
				total += pc;	
				F32 y = (F32) (pc-physics_domain[0])/cost_range*tri_rect.getHeight()+tri_rect.mBottom;
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

			std::string label = llformat("%s Object Physics Cost -- [%.2f, %.2f] Mean: %.2f  Total: %.2f",
											category[idx], physics_domain[0], physics_domain[1], total/count, total_physics[idx]);

			LLFontGL::getFontMonospace()->renderUTF8(label,
											0 , tri_rect.mLeft, tri_rect.mTop+margin, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);

		}
	}

	
	

	LLView::draw();
}


