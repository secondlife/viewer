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
#include "glm/glm.hpp"
#include <boost/tokenizer.hpp>

const F32 HORIZONTAL_PADDING = 15.f;
const F32 VERTICAL_PADDING = 12.f;
const F32 BUFFER_SIZE = 2.f;
const F32 HUD_TEXT_MAX_WIDTH = 190.f;
const F32 HUD_TEXT_MAX_WIDTH_NO_BUBBLE = 1000.f;
const F32 MAX_DRAW_DISTANCE = 300.f;

std::set<LLPointer<LLHUDText> > LLHUDText::sTextObjects;
std::vector<LLPointer<LLHUDText> > LLHUDText::sVisibleTextObjects;
std::vector<LLPointer<LLHUDText> > LLHUDText::sVisibleHUDTextObjects;
bool LLHUDText::sDisplayText = true ;

bool lltextobject_further_away::operator()(const LLPointer<LLHUDText>& lhs, const LLPointer<LLHUDText>& rhs) const
{
    return lhs->getDistance() > rhs->getDistance();
}


LLHUDText::LLHUDText(const U8 type) :
            LLHUDObject(type),
            mOnHUDAttachment(false),
//          mVisibleOffScreen(false),
            mWidth(0.f),
            mHeight(0.f),
            mFontp(LLFontGL::getFontSansSerifSmall()),
            mBoldFontp(LLFontGL::getFontSansSerifBold()),
            mMass(1.f),
            mMaxLines(10),
            mOffsetY(0),
            mTextAlignment(ALIGN_TEXT_CENTER),
            mVertAlignment(ALIGN_VERT_CENTER),
//          mLOD(0),
            mHidden(false)
{
    mColor = LLColor4(1.f, 1.f, 1.f, 1.f);
    mDoFade = true;
    mFadeDistance = 8.f;
    mFadeRange = 4.f;
    mZCompare = true;
    mOffscreen = false;
    mRadius = 0.1f;
    LLPointer<LLHUDText> ptr(this);
    sTextObjects.insert(ptr);
}

LLHUDText::~LLHUDText() = default;

void LLHUDText::render()
{
    if (!mOnHUDAttachment && sDisplayText)
    {
        LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
        //LLGLDisable gls_stencil(GL_STENCIL_TEST);
        renderText();
    }
}

void LLHUDText::renderText()
{
    if (!mVisible || mHidden)
    {
        return;
    }

    gGL.getTexUnit(0)->enable(LLTexUnit::eTextureType::TT_TEXTURE);

    LLGLState gls_blend(GL_BLEND, true);

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

    // *TODO: make this a per-text setting
    static LLCachedControl<F32> bubble_opacity(gSavedSettings, "ChatBubbleOpacity");
    static LLUIColor nametag_bg_color = LLUIColorTable::instance().getColor("ObjectBubbleColor");
    LLColor4 bg_color = nametag_bg_color;
    bg_color.setAlpha(bubble_opacity * alpha_factor);

    const S32 border_height = 16;
    const S32 border_width = 16;

    // *TODO move this into helper function
    F32 border_scale = 1.f;

    if (border_height * 2 > mHeight)
    {
        border_scale = static_cast<F32>(mHeight) / (static_cast<F32>(border_height) * 2.f);
    }
    if (border_width * 2 > mWidth)
    {
        border_scale = llmin(border_scale, static_cast<F32>(mWidth) / (static_cast<F32>(border_width) * 2.f));
    }

    // scale screen size of borders down
    //RN: for now, text on hud objects is never occluded

    LLVector3 x_pixel_vec;
    LLVector3 y_pixel_vec;

    if (mOnHUDAttachment)
    {
        x_pixel_vec = LLVector3::y_axis / static_cast<F32>(gViewerWindow->getWorldViewWidthRaw());
        y_pixel_vec = LLVector3::z_axis / static_cast<F32>(gViewerWindow->getWorldViewHeightRaw());
    }
    else
    {
        // Bridge: getPixelVectors now takes/returns glm::vec3.
        glm::vec3 x_pixel_glm, y_pixel_glm;
        LLViewerCamera::getInstance()->getPixelVectors(static_cast<glm::vec3>(mPositionAgent), y_pixel_glm, x_pixel_glm);
        x_pixel_vec = LLVector3(x_pixel_glm.x, x_pixel_glm.y, x_pixel_glm.z);
        y_pixel_vec = LLVector3(y_pixel_glm.x, y_pixel_glm.y, y_pixel_glm.z);
    }

    LLVector3 width_vec = mWidth * x_pixel_vec;
    LLVector3 height_vec = mHeight * y_pixel_vec;

    mRadius = (width_vec + height_vec).length() * 0.5f;

    glm::vec2 screen_offset;
    screen_offset = mPositionOffset;

    LLVector3 render_position = mPositionAgent
            + (x_pixel_vec * screen_offset.x)
            + (y_pixel_vec * screen_offset.y);

    F32 y_offset = static_cast<F32>(mOffsetY);

    // Render label

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
            start_segment = llmax(static_cast<S32>(0), static_cast<S32>(mTextSegments.size()) - max_lines);
        }

        for (std::vector<LLHUDTextSegment>::iterator segment_iter = mTextSegments.begin() + start_segment;
             segment_iter != mTextSegments.end(); ++segment_iter )
        {
            const LLFontGL* fontp = segment_iter->mFont;
            y_offset -= fontp->getLineHeight() - 1; // correction factor to match legacy font metrics

            U8 style = segment_iter->mStyle;
            LLFontGL::ShadowType shadow = LLFontGL::ShadowType::DROP_SHADOW;

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
        using tokenizer = boost::tokenizer<boost::char_separator<llwchar>, LLWString::const_iterator, LLWString >;
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
                S32 segment_length = font->maxDrawableChars(iter->substr(line_length).c_str(), max_pixels, static_cast<S32>(wline.length()), LLFontGL::EWordWrapStyle::WORD_BOUNDARY_IF_POSSIBLE);
                LLHUDTextSegment segment(iter->substr(line_length, segment_length), style, color, font);
                mTextSegments.push_back(segment);
                line_length += segment_length;
            }
            while (line_length != iter->size());
            ++iter;
        }
    }
}

void LLHUDText::setZCompare(const bool zcompare)
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


void LLHUDText::setDoFade(const bool do_fade)
{
    mDoFade = do_fade;
}

void LLHUDText::updateVisibility()
{
    if (mSourceObject)
    {
        mSourceObject->updateText();
    }

    {
        const glm::vec3 g = gAgent.getPosAgentFromGlobal(mPositionGlobal);
        mPositionAgent = LLVector3(g.x, g.y, g.z);
    }

    if (!mSourceObject)
    {
        // Beacons
        mVisible = true;
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
        mVisible = false;
        return;
    }

    // for now, all text on hud objects is visible
    if (mOnHUDAttachment)
    {
        mVisible = true;
        sVisibleHUDTextObjects.push_back(LLPointer<LLHUDText> (this));
        mLastDistance = mPositionAgent.mV[VX];
        return;
    }

    // push text towards camera by radius of object, but not past camera
    const LLVector3 cam_origin_ll(LLViewerCamera::getInstance()->getOrigin());
    const LLVector3 cam_at_ll(LLViewerCamera::getInstance()->getAtAxis());
    LLVector3 vec_from_camera = mPositionAgent - cam_origin_ll;
    LLVector3 dir_from_camera = vec_from_camera;
    dir_from_camera.normalize();

    if (dir_from_camera * cam_at_ll <= 0.f)
    { //text is behind camera, don't render
        mVisible = false;
        return;
    }

    if (vec_from_camera * cam_at_ll <= LLViewerCamera::getInstance()->getNear() + 0.1f + mSourceObject->getVObjRadius())
    {
        mPositionAgent = cam_origin_ll + vec_from_camera * ((LLViewerCamera::getInstance()->getNear() + 0.1f) / (vec_from_camera * cam_at_ll));
    }
    else
    {
        mPositionAgent -= dir_from_camera * mSourceObject->getVObjRadius();
    }

    mLastDistance = (mPositionAgent - cam_origin_ll).length();

    if (!mTextSegments.size() || (mDoFade && (mLastDistance > mFadeDistance + mFadeRange)))
    {
        mVisible = false;
        return;
    }

    const glm::vec3 pos_agent_center_g = gAgent.getPosAgentFromGlobal(mPositionGlobal);
    LLVector3 pos_agent_center = LLVector3(pos_agent_center_g.x, pos_agent_center_g.y, pos_agent_center_g.z) - dir_from_camera;
    F32 last_distance_center = (pos_agent_center - cam_origin_ll).length();
    static LLCachedControl<F32> prim_text_max_dist(gSavedSettings, "PrimTextMaxDrawDistance");
    F32 max_draw_distance = prim_text_max_dist;

    if(max_draw_distance < 0)
    {
        max_draw_distance = 0;
        gSavedSettings.setF32("PrimTextMaxDrawDistance", max_draw_distance);
    }
    else if(max_draw_distance > MAX_DRAW_DISTANCE)
    {
        max_draw_distance = MAX_DRAW_DISTANCE;
        gSavedSettings.setF32("PrimTextMaxDrawDistance", MAX_DRAW_DISTANCE);
    }

    if(last_distance_center > max_draw_distance)
    {
        mVisible = false;
        return;
    }


    // Bridge: getPixelVectors / sphereInFrustum now take/return glm::vec3.
    glm::vec3 x_pixel_glm, y_pixel_glm;
    LLViewerCamera::getInstance()->getPixelVectors(static_cast<glm::vec3>(mPositionAgent), y_pixel_glm, x_pixel_glm);
    LLVector3 x_pixel_vec(x_pixel_glm.x, x_pixel_glm.y, x_pixel_glm.z);
    LLVector3 y_pixel_vec(y_pixel_glm.x, y_pixel_glm.y, y_pixel_glm.z);

    LLVector3 render_position = mPositionAgent +
            (x_pixel_vec * mPositionOffset.x) +
            (y_pixel_vec * mPositionOffset.y);

    mOffscreen = false;
    if (!LLViewerCamera::getInstance()->sphereInFrustum(static_cast<glm::vec3>(render_position), mRadius))
    {
//      if (!mVisibleOffScreen)
//      {
            mVisible = false;
            return;
//      }
//      else
//      {
//          mOffscreen = true;
//      }
    }

    mVisible = true;
    sVisibleTextObjects.push_back(LLPointer<LLHUDText> (this));
}

glm::vec2 LLHUDText::updateScreenPos(glm::vec2 &offset)
{
    LLCoordGL screen_pos;
    glm::vec2 screen_pos_vec;
    // Bridge: getPixelVectors now takes/returns glm::vec3.
    glm::vec3 x_pixel_glm, y_pixel_glm;
    LLViewerCamera::getInstance()->getPixelVectors(static_cast<glm::vec3>(mPositionAgent), y_pixel_glm, x_pixel_glm);
    LLVector3 x_pixel_vec(x_pixel_glm.x, x_pixel_glm.y, x_pixel_glm.z);
    LLVector3 y_pixel_vec(y_pixel_glm.x, y_pixel_glm.y, y_pixel_glm.z);
//  LLVector3 world_pos = mPositionAgent + (offset.x * x_pixel_vec) + (offset.y * y_pixel_vec);
//  if (!LLViewerCamera::getInstance()->projectPosAgentToScreen(world_pos, screen_pos, false) && mVisibleOffScreen)
//  {
//      // bubble off-screen, so find a spot for it along screen edge
//      LLViewerCamera::getInstance()->projectPosAgentToScreenEdge(world_pos, screen_pos);
//  }

    screen_pos_vec = glm::vec2(static_cast<F32>(screen_pos.mX), static_cast<F32>(screen_pos.mY));

    LLRect world_rect = gViewerWindow->getWorldViewRectScaled();
    S32 bottom = world_rect.mBottom + STATUS_BAR_HEIGHT;

    glm::vec2 screen_center;
    screen_center.x = llclamp(static_cast<F32>(screen_pos_vec.x), static_cast<F32>(world_rect.mLeft) + mWidth * 0.5f, static_cast<F32>(world_rect.mRight) - mWidth * 0.5f);

    if(mVertAlignment == ALIGN_VERT_TOP)
    {
        screen_center.y = llclamp(static_cast<F32>(screen_pos_vec.y),
            static_cast<F32>(bottom),
            static_cast<F32>(world_rect.mTop) - mHeight - static_cast<F32>(MENU_BAR_HEIGHT));
        mSoftScreenRect.setLeftTopAndSize(screen_center.x - (mWidth + BUFFER_SIZE) * 0.5f,
            screen_center.y + (mHeight + BUFFER_SIZE), mWidth + BUFFER_SIZE, mHeight + BUFFER_SIZE);
    }
    else
    {
        screen_center.y = llclamp(static_cast<F32>(screen_pos_vec.y),
            static_cast<F32>(bottom) + mHeight * 0.5f,
            static_cast<F32>(world_rect.mTop) - mHeight * 0.5f - static_cast<F32>(MENU_BAR_HEIGHT));
        mSoftScreenRect.setCenterAndSize(screen_center.x, screen_center.y, mWidth + BUFFER_SIZE, mHeight + BUFFER_SIZE);
    }

    return offset + (screen_center - glm::vec2(static_cast<F32>(screen_pos.mX), static_cast<F32>(screen_pos.mY)));
}

void LLHUDText::updateSize()
{
    F32 height = 0.f;
    F32 width = 0.f;

    S32 max_lines = getMaxLines();

    S32 start_segment;
    if (max_lines < 0) start_segment = 0;
    else start_segment = llmax(static_cast<S32>(0), static_cast<S32>(mTextSegments.size()) - max_lines);

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
    mWidth = llmax(width, lerp(mWidth, static_cast<F32>(width), u));
    mHeight = llmax(height, lerp(mHeight, static_cast<F32>(height), u));
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
        textp->mTargetPositionOffset = glm::vec2(0.0f);
        textp->updateSize();
        textp->updateVisibility();
    }

    // sort back to front for rendering purposes
    std::ranges::sort(sVisibleTextObjects, lltextobject_further_away());
    std::ranges::sort(sVisibleHUDTextObjects, lltextobject_further_away());
}

//void LLHUDText::setLOD(S32 lod)
//{
//  mLOD = lod;
//  //RN: uncomment this to visualize LOD levels
//  //std::string label = llformat("%d", lod);
//  //setLabel(label);
//}

S32 LLHUDText::getMaxLines()
{
    return mMaxLines;
    //switch(mLOD)
    //{
    //case 0:
    //  return mMaxLines;
    //case 1:
    //  return mMaxLines > 0 ? mMaxLines / 2 : 5;
    //case 2:
    //  return mMaxLines > 0 ? mMaxLines / 3 : 2;
    //default:
    //  // label only
    //  return 0;
    //}
}

void LLHUDText::markDead()
{
    // make sure we have at least one pointer
    // till the end of the function
    LLPointer<LLHUDText> ptr(this);
    sTextObjects.erase(ptr);
    LLHUDObject::markDead();
}

void LLHUDText::renderAllHUD()
{
    LLGLState::checkStates();

    {
        LLGLDepthTest depth(GL_FALSE, GL_FALSE);

        VisibleTextObjectIterator text_it;

        for (text_it = sVisibleHUDTextObjects.begin(); text_it != sVisibleHUDTextObjects.end(); ++text_it)
        {
            (*text_it)->renderText();
        }
    }

    LLVertexBuffer::unbind();

    LLGLState::checkStates();
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
