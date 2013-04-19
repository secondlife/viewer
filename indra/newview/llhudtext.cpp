/** 
 * @file llhudtext.cpp
 * @brief Floating text above objects, set via script with llSetText()
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llhudtext.h"

#include "llrender.h"

#include "llagent.h"
#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "lldrawable.h"
#include "llfontgl.h"
#include "llglheaders.h"
#include "llhudrender.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerobject.h"
#include "llvovolume.h"
#include "llviewerwindow.h"
#include "llstatusbar.h"
#include "llmenugl.h"
#include "pipeline.h"
#include <boost/tokenizer.hpp>


const F32 SPRING_STRENGTH = 0.7f;
const F32 RESTORATION_SPRING_TIME_CONSTANT = 0.1f;
const F32 HORIZONTAL_PADDING = 15.f;
const F32 VERTICAL_PADDING = 12.f;
const F32 BUFFER_SIZE = 2.f;
const F32 MIN_EDGE_OVERLAP = 3.f;
const F32 HUD_TEXT_MAX_WIDTH = 190.f;
const F32 HUD_TEXT_MAX_WIDTH_NO_BUBBLE = 1000.f;
const F32 RESIZE_TIME = 0.f;
const S32 NUM_OVERLAP_ITERATIONS = 10;
const F32 NEIGHBOR_FORCE_FRACTION = 1.f;
const F32 POSITION_DAMPING_TC = 0.2f;
const F32 MAX_STABLE_CAMERA_VELOCITY = 0.1f;
//const F32 LOD_0_SCREEN_COVERAGE = 0.15f;
//const F32 LOD_1_SCREEN_COVERAGE = 0.30f;
//const F32 LOD_2_SCREEN_COVERAGE = 0.40f;

std::set<LLPointer<LLHUDText> > LLHUDText::sTextObjects;
std::vector<LLPointer<LLHUDText> > LLHUDText::sVisibleTextObjects;
std::vector<LLPointer<LLHUDText> > LLHUDText::sVisibleHUDTextObjects;
BOOL LLHUDText::sDisplayText = TRUE ;

bool lltextobject_further_away::operator()(const LLPointer<LLHUDText>& lhs, const LLPointer<LLHUDText>& rhs) const
{
	return lhs->getDistance() > rhs->getDistance();
}


LLHUDText::LLHUDText(const U8 type) :
			LLHUDObject(type),
			mOnHUDAttachment(FALSE),
//			mVisibleOffScreen(FALSE),
			mWidth(0.f),
			mHeight(0.f),
			mFontp(LLFontGL::getFontSansSerifSmall()),
			mBoldFontp(LLFontGL::getFontSansSerifBold()),
			mMass(1.f),
			mMaxLines(10),
			mOffsetY(0),
			mTextAlignment(ALIGN_TEXT_CENTER),
			mVertAlignment(ALIGN_VERT_CENTER),
//			mLOD(0),
			mHidden(FALSE)
{
	mColor = LLColor4(1.f, 1.f, 1.f, 1.f);
	mDoFade = TRUE;
	mFadeDistance = 8.f;
	mFadeRange = 4.f;
	mZCompare = TRUE;
	mOffscreen = FALSE;
	mRadius = 0.1f;
	LLPointer<LLHUDText> ptr(this);
	sTextObjects.insert(ptr);
}

LLHUDText::~LLHUDText()
{
}

void LLHUDText::render()
{
	if (!mOnHUDAttachment && sDisplayText)
	{
		LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
		renderText();
	}
}

void LLHUDText::renderText()
{
	if (!mVisible || mHidden)
	{
		return;
	}

	gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);

	LLGLState gls_blend(GL_BLEND, TRUE);
	LLGLState gls_alpha(GL_ALPHA_TEST, TRUE);
	
	LLColor4 shadow_color(0.f, 0.f, 0.f, 1.f);
	F32 alpha_factor = 1.f;
	LLColor4 text_color = mColor;
	if (mDoFade)
	{
		if (mLastDistance > mFadeDistance)
		{
			alpha_factor = llmax(0.f, 1.f - (mLastDistance - mFadeDistance)/mFadeRange);
			text_color.mV[3] = text_color.mV[3]*alpha_factor;
		}
	}
	if (text_color.mV[3] < 0.01f)
	{
		return;
	}
	shadow_color.mV[3] = text_color.mV[3];

	mOffsetY = lltrunc(mHeight * ((mVertAlignment == ALIGN_VERT_CENTER) ? 0.5f : 1.f));

	// *TODO: cache this image
	LLUIImagePtr imagep = LLUI::getUIImage("Rounded_Square");

	// *TODO: make this a per-text setting
	LLColor4 bg_color = LLUIColorTable::instance().getColor("ObjectBubbleColor");
	bg_color.setAlpha(gSavedSettings.getF32("ChatBubbleOpacity") * alpha_factor);

	const S32 border_height = 16;
	const S32 border_width = 16;

	// *TODO move this into helper function
	F32 border_scale = 1.f;

	if (border_height * 2 > mHeight)
	{
		border_scale = (F32)mHeight / ((F32)border_height * 2.f);
	}
	if (border_width * 2 > mWidth)
	{
		border_scale = llmin(border_scale, (F32)mWidth / ((F32)border_width * 2.f));
	}

	// scale screen size of borders down
	//RN: for now, text on hud objects is never occluded

	LLVector3 x_pixel_vec;
	LLVector3 y_pixel_vec;
	
	if (mOnHUDAttachment)
	{
		x_pixel_vec = LLVector3::y_axis / (F32)gViewerWindow->getWorldViewWidthRaw();
		y_pixel_vec = LLVector3::z_axis / (F32)gViewerWindow->getWorldViewHeightRaw();
	}
	else
	{
		LLViewerCamera::getInstance()->getPixelVectors(mPositionAgent, y_pixel_vec, x_pixel_vec);
	}

	LLVector3 width_vec = mWidth * x_pixel_vec;
	LLVector3 height_vec = mHeight * y_pixel_vec;

	mRadius = (width_vec + height_vec).magVec() * 0.5f;

	LLVector2 screen_offset;
	screen_offset = mPositionOffset;

	LLVector3 render_position = mPositionAgent  
			+ (x_pixel_vec * screen_offset.mV[VX])
			+ (y_pixel_vec * screen_offset.mV[VY]);

	F32 y_offset = (F32)mOffsetY;
		
	// Render label
	{
		gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	}

	// Render text
	{
		// -1 mMaxLines means unlimited lines.
		S32 start_segment;
		S32 max_lines = getMaxLines();

		if (max_lines < 0) 
		{
			start_segment = 0;
		}
		else 
		{
			start_segment = llmax((S32)0, (S32)mTextSegments.size() - max_lines);
		}

		for (std::vector<LLHUDTextSegment>::iterator segment_iter = mTextSegments.begin() + start_segment;
			 segment_iter != mTextSegments.end(); ++segment_iter )
		{
			const LLFontGL* fontp = segment_iter->mFont;
			y_offset -= fontp->getLineHeight() - 1; // correction factor to match legacy font metrics

			U8 style = segment_iter->mStyle;
			LLFontGL::ShadowType shadow = LLFontGL::DROP_SHADOW;
	
			F32 x_offset;
			if (mTextAlignment== ALIGN_TEXT_CENTER)
			{
				x_offset = -0.5f*segment_iter->getWidth(fontp);
			}
			else // ALIGN_LEFT
			{
				x_offset = -0.5f * mWidth + (HORIZONTAL_PADDING / 2.f);
			}

			text_color = segment_iter->mColor;
			text_color.mV[VALPHA] *= alpha_factor;

			hud_render_text(segment_iter->getText(), render_position, *fontp, style, shadow, x_offset, y_offset, text_color, mOnHUDAttachment);
		}
	}
	/// Reset the default color to white.  The renderer expects this to be the default. 
	gGL.color4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void LLHUDText::setString(const std::string &text_utf8)
{
	mTextSegments.clear();
	addLine(text_utf8, mColor);
}

void LLHUDText::clearString()
{
	mTextSegments.clear();
}


void LLHUDText::addLine(const std::string &text_utf8,
						const LLColor4& color,
						const LLFontGL::StyleFlags style,
						const LLFontGL* font)
{
	LLWString wline = utf8str_to_wstring(text_utf8);
	if (!wline.empty())
	{
		// use default font for segment if custom font not specified
		if (!font)
		{
			font = mFontp;
		}
		typedef boost::tokenizer<boost::char_separator<llwchar>, LLWString::const_iterator, LLWString > tokenizer;
		LLWString seps(utf8str_to_wstring("\r\n"));
		boost::char_separator<llwchar> sep(seps.c_str());

		tokenizer tokens(wline, sep);
		tokenizer::iterator iter = tokens.begin();

		while (iter != tokens.end())
		{
			U32 line_length = 0;
			do	
			{
				F32 max_pixels = HUD_TEXT_MAX_WIDTH_NO_BUBBLE;
				S32 segment_length = font->maxDrawableChars(iter->substr(line_length).c_str(), max_pixels, wline.length(), LLFontGL::WORD_BOUNDARY_IF_POSSIBLE);
				LLHUDTextSegment segment(iter->substr(line_length, segment_length), style, color, font);
				mTextSegments.push_back(segment);
				line_length += segment_length;
			}
			while (line_length != iter->size());
			++iter;
		}
	}
}

void LLHUDText::setZCompare(const BOOL zcompare)
{
	mZCompare = zcompare;
}

void LLHUDText::setFont(const LLFontGL* font)
{
	mFontp = font;
}


void LLHUDText::setColor(const LLColor4 &color)
{
	mColor = color;
	for (std::vector<LLHUDTextSegment>::iterator segment_iter = mTextSegments.begin();
		 segment_iter != mTextSegments.end(); ++segment_iter )
	{
		segment_iter->mColor = color;
	}
}

void LLHUDText::setAlpha(F32 alpha)
{
	mColor.mV[VALPHA] = alpha;
	for (std::vector<LLHUDTextSegment>::iterator segment_iter = mTextSegments.begin();
		 segment_iter != mTextSegments.end(); ++segment_iter )
	{
		segment_iter->mColor.mV[VALPHA] = alpha;
	}
}


void LLHUDText::setDoFade(const BOOL do_fade)
{
	mDoFade = do_fade;
}

void LLHUDText::updateVisibility()
{
	if (mSourceObject)
	{
		mSourceObject->updateText();
	}
	
	mPositionAgent = gAgent.getPosAgentFromGlobal(mPositionGlobal);

	if (!mSourceObject)
	{
		//llwarns << "LLHUDText::updateScreenPos -- mSourceObject is NULL!" << llendl;
		mVisible = TRUE;
		if (mOnHUDAttachment)
		{
			sVisibleHUDTextObjects.push_back(LLPointer<LLHUDText> (this));
		}
		else
		{
			sVisibleTextObjects.push_back(LLPointer<LLHUDText> (this));
		}
		return;
	}

	// Not visible if parent object is dead
	if (mSourceObject->isDead())
	{
		mVisible = FALSE;
		return;
	}

	// for now, all text on hud objects is visible
	if (mOnHUDAttachment)
	{
		mVisible = TRUE;
		sVisibleHUDTextObjects.push_back(LLPointer<LLHUDText> (this));
		mLastDistance = mPositionAgent.mV[VX];
		return;
	}

	// push text towards camera by radius of object, but not past camera
	LLVector3 vec_from_camera = mPositionAgent - LLViewerCamera::getInstance()->getOrigin();
	LLVector3 dir_from_camera = vec_from_camera;
	dir_from_camera.normVec();

	if (dir_from_camera * LLViewerCamera::getInstance()->getAtAxis() <= 0.f)
	{ //text is behind camera, don't render
		mVisible = FALSE;
		return;
	}
		
	if (vec_from_camera * LLViewerCamera::getInstance()->getAtAxis() <= LLViewerCamera::getInstance()->getNear() + 0.1f + mSourceObject->getVObjRadius())
	{
		mPositionAgent = LLViewerCamera::getInstance()->getOrigin() + vec_from_camera * ((LLViewerCamera::getInstance()->getNear() + 0.1f) / (vec_from_camera * LLViewerCamera::getInstance()->getAtAxis()));
	}
	else
	{
		mPositionAgent -= dir_from_camera * mSourceObject->getVObjRadius();
	}

	mLastDistance = (mPositionAgent - LLViewerCamera::getInstance()->getOrigin()).magVec();

	if (!mTextSegments.size() || (mDoFade && (mLastDistance > mFadeDistance + mFadeRange)))
	{
		mVisible = FALSE;
		return;
	}

	LLVector3 x_pixel_vec;
	LLVector3 y_pixel_vec;

	LLViewerCamera::getInstance()->getPixelVectors(mPositionAgent, y_pixel_vec, x_pixel_vec);

	LLVector3 render_position = mPositionAgent + 			
			(x_pixel_vec * mPositionOffset.mV[VX]) +
			(y_pixel_vec * mPositionOffset.mV[VY]);

	mOffscreen = FALSE;
	if (!LLViewerCamera::getInstance()->sphereInFrustum(render_position, mRadius))
	{
//		if (!mVisibleOffScreen)
//		{
			mVisible = FALSE;
			return;
//		}
//		else
//		{
//			mOffscreen = TRUE;
//		}
	}

	mVisible = TRUE;
	sVisibleTextObjects.push_back(LLPointer<LLHUDText> (this));
}

LLVector2 LLHUDText::updateScreenPos(LLVector2 &offset)
{
	LLCoordGL screen_pos;
	LLVector2 screen_pos_vec;
	LLVector3 x_pixel_vec;
	LLVector3 y_pixel_vec;
	LLViewerCamera::getInstance()->getPixelVectors(mPositionAgent, y_pixel_vec, x_pixel_vec);
//	LLVector3 world_pos = mPositionAgent + (offset.mV[VX] * x_pixel_vec) + (offset.mV[VY] * y_pixel_vec);
//	if (!LLViewerCamera::getInstance()->projectPosAgentToScreen(world_pos, screen_pos, FALSE) && mVisibleOffScreen)
//	{
//		// bubble off-screen, so find a spot for it along screen edge
//		LLViewerCamera::getInstance()->projectPosAgentToScreenEdge(world_pos, screen_pos);
//	}

	screen_pos_vec.setVec((F32)screen_pos.mX, (F32)screen_pos.mY);

	LLRect world_rect = gViewerWindow->getWorldViewRectScaled();
	S32 bottom = world_rect.mBottom + STATUS_BAR_HEIGHT;

	LLVector2 screen_center;
	screen_center.mV[VX] = llclamp((F32)screen_pos_vec.mV[VX], (F32)world_rect.mLeft + mWidth * 0.5f, (F32)world_rect.mRight - mWidth * 0.5f);

	if(mVertAlignment == ALIGN_VERT_TOP)
	{
		screen_center.mV[VY] = llclamp((F32)screen_pos_vec.mV[VY], 
			(F32)bottom, 
			(F32)world_rect.mTop - mHeight - (F32)MENU_BAR_HEIGHT);
		mSoftScreenRect.setLeftTopAndSize(screen_center.mV[VX] - (mWidth + BUFFER_SIZE) * 0.5f, 
			screen_center.mV[VY] + (mHeight + BUFFER_SIZE), mWidth + BUFFER_SIZE, mHeight + BUFFER_SIZE);
	}
	else
	{
		screen_center.mV[VY] = llclamp((F32)screen_pos_vec.mV[VY], 
			(F32)bottom + mHeight * 0.5f, 
			(F32)world_rect.mTop - mHeight * 0.5f - (F32)MENU_BAR_HEIGHT);
		mSoftScreenRect.setCenterAndSize(screen_center.mV[VX], screen_center.mV[VY], mWidth + BUFFER_SIZE, mHeight + BUFFER_SIZE);
	}

	return offset + (screen_center - LLVector2((F32)screen_pos.mX, (F32)screen_pos.mY));
}

void LLHUDText::updateSize()
{
	F32 height = 0.f;
	F32 width = 0.f;

	S32 max_lines = getMaxLines();

	S32 start_segment;
	if (max_lines < 0) start_segment = 0;
	else start_segment = llmax((S32)0, (S32)mTextSegments.size() - max_lines);

	std::vector<LLHUDTextSegment>::iterator iter = mTextSegments.begin() + start_segment;
	while (iter != mTextSegments.end())
	{
		const LLFontGL* fontp = iter->mFont;
		height += fontp->getLineHeight() - 1; // correction factor to match legacy font metrics
		width = llmax(width, llmin(iter->getWidth(fontp), HUD_TEXT_MAX_WIDTH));
		++iter;
	}

	if (width == 0.f)
	{
		return;
	}

	width += HORIZONTAL_PADDING;
	height += VERTICAL_PADDING;

	// *TODO: Could do some sort of timer-based resize logic here
	F32 u = 1.f;
	mWidth = llmax(width, lerp(mWidth, (F32)width, u));
	mHeight = llmax(height, lerp(mHeight, (F32)height, u));
}

void LLHUDText::updateAll()
{
	// iterate over all text objects, calculate their restoration forces,
	// and add them to the visible set if they are on screen and close enough
	sVisibleTextObjects.clear();
	sVisibleHUDTextObjects.clear();
	
	TextObjectIterator text_it;
	for (text_it = sTextObjects.begin(); text_it != sTextObjects.end(); ++text_it)
	{
		LLHUDText* textp = (*text_it);
		textp->mTargetPositionOffset.clearVec();
		textp->updateSize();
		textp->updateVisibility();
	}
	
	// sort back to front for rendering purposes
	std::sort(sVisibleTextObjects.begin(), sVisibleTextObjects.end(), lltextobject_further_away());
	std::sort(sVisibleHUDTextObjects.begin(), sVisibleHUDTextObjects.end(), lltextobject_further_away());
}

//void LLHUDText::setLOD(S32 lod)
//{
//	mLOD = lod;
//	//RN: uncomment this to visualize LOD levels
//	//std::string label = llformat("%d", lod);
//	//setLabel(label);
//}

S32 LLHUDText::getMaxLines()
{
	return mMaxLines;
	//switch(mLOD)
	//{
	//case 0:
	//	return mMaxLines;
	//case 1:
	//	return mMaxLines > 0 ? mMaxLines / 2 : 5;
	//case 2:
	//	return mMaxLines > 0 ? mMaxLines / 3 : 2;
	//default:
	//	// label only
	//	return 0;
	//}
}

void LLHUDText::markDead()
{
	sTextObjects.erase(LLPointer<LLHUDText>(this));
	LLHUDObject::markDead();
}

void LLHUDText::renderAllHUD()
{
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();

	{
		LLGLEnable color_mat(GL_COLOR_MATERIAL);
		LLGLDepthTest depth(GL_FALSE, GL_FALSE);
		
		VisibleTextObjectIterator text_it;

		for (text_it = sVisibleHUDTextObjects.begin(); text_it != sVisibleHUDTextObjects.end(); ++text_it)
		{
			(*text_it)->renderText();
		}
	}
	
	LLVertexBuffer::unbind();

    LLVertexBuffer::unbind();

	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();
}

void LLHUDText::shiftAll(const LLVector3& offset)
{
	TextObjectIterator text_it;
	for (text_it = sTextObjects.begin(); text_it != sTextObjects.end(); ++text_it)
	{
		LLHUDText *textp = text_it->get();
		textp->shift(offset);
	}
}

void LLHUDText::shift(const LLVector3& offset)
{
	mPositionAgent += offset;
}

//static
// called when UI scale changes, to flush font width caches
void LLHUDText::reshape()
{
	TextObjectIterator text_it;
	for (text_it = sTextObjects.begin(); text_it != sTextObjects.end(); ++text_it)
	{
		LLHUDText* textp = (*text_it);
		std::vector<LLHUDTextSegment>::iterator segment_iter; 
		for (segment_iter = textp->mTextSegments.begin();
			 segment_iter != textp->mTextSegments.end(); ++segment_iter )
		{
			segment_iter->clearFontWidthMap();
		}
	}
}

//============================================================================

F32 LLHUDText::LLHUDTextSegment::getWidth(const LLFontGL* font)
{
	std::map<const LLFontGL*, F32>::iterator iter = mFontWidthMap.find(font);
	if (iter != mFontWidthMap.end())
	{
		return iter->second;
	}
	else
	{
		F32 width = font->getWidthF32(mText.c_str());
		mFontWidthMap[font] = width;
		return width;
	}
}
