/**
 * @file llmanip.cpp
 * @brief LLManip class implementation
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

#include "llmanip.h"

#include "llmath.h"
#include "v3math.h"
#include "llgl.h"
#include "llrender.h"
#include "llprimitive.h"
#include "llview.h"
#include "llviewertexturelist.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llviewercontrol.h"
#include "lldrawable.h"
#include "llfontgl.h"
#include "llhudrender.h"
#include "llselectmgr.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerjoint.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llworld.h"        // for LLWorld::getInstance()
#include "llresmgr.h"
#include "pipeline.h"
#include "llglheaders.h"
#include "lluiimage.h"
// Local constants...
const S32 VERTICAL_OFFSET = 50;

F32     LLManip::sHelpTextVisibleTime = 2.f;
F32     LLManip::sHelpTextFadeTime = 2.f;
S32     LLManip::sNumTimesHelpTextShown = 0;
S32     LLManip::sMaxTimesShowHelpText = 5;
F32     LLManip::sGridMaxSubdivisionLevel = 32.f;
F32     LLManip::sGridMinSubdivisionLevel = 1.f / 32.f;
LLVector2 LLManip::sTickLabelSpacing(60.f, 25.f);


//static
void LLManip::rebuild(LLViewerObject* vobj)
{
    LLDrawable* drawablep = vobj->mDrawable;
    if (drawablep && drawablep->getVOVolume())
    {
        gPipeline.markRebuild(drawablep,LLDrawable::REBUILD_VOLUME);
        drawablep->setState(LLDrawable::MOVE_UNDAMPED); // force to UNDAMPED
        drawablep->updateMove();
        LLSpatialGroup* group = drawablep->getSpatialGroup();
        if (group)
        {
            group->dirtyGeom();
            gPipeline.markRebuild(group);
        }

        LLViewerObject::const_child_list_t& child_list = vobj->getChildren();
        for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin(), endIter = child_list.end();
             iter != endIter; ++iter)
        {
            LLViewerObject* child = *iter;
            rebuild(child);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// LLManip


LLManip::LLManip( const std::string& name, LLToolComposite* composite )
    :
    LLTool( name, composite ),
    mInSnapRegime(false),
    mHighlightedPart(LL_NO_PART),
    mManipPart(LL_NO_PART)
{
}

void LLManip::getManipNormal(LLViewerObject* object, EManipPart manip, LLVector3 &normal)
{
    LLVector3 grid_origin;
    LLVector3 grid_scale;
    LLQuaternion grid_rotation;

    LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);

    if (manip >= LL_X_ARROW && manip <= LL_Z_ARROW)
    {
        LLVector3 arrow_axis;
        getManipAxis(object, manip, arrow_axis);

        LLVector3 cross = arrow_axis % LLViewerCamera::getInstance()->getAtAxis();
        normal = cross % arrow_axis;
        normal.normVec();
    }
    else if (manip >= LL_YZ_PLANE && manip <= LL_XY_PLANE)
    {
        switch (manip)
        {
        case LL_YZ_PLANE:
            normal = LLVector3::x_axis;
            break;
        case LL_XZ_PLANE:
            normal = LLVector3::y_axis;
            break;
        case LL_XY_PLANE:
            normal = LLVector3::z_axis;
            break;
        default:
            break;
        }
        normal.rotVec(grid_rotation);
    }
    else
    {
        normal.clearVec();
    }
}


bool LLManip::getManipAxis(LLViewerObject* object, EManipPart manip, LLVector3 &axis)
{
    LLVector3 grid_origin;
    LLVector3 grid_scale;
    LLQuaternion grid_rotation;

    LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rotation, grid_scale);

    if (manip == LL_X_ARROW)
    {
        axis = LLVector3::x_axis;
    }
    else if (manip == LL_Y_ARROW)
    {
        axis = LLVector3::y_axis;
    }
    else if (manip == LL_Z_ARROW)
    {
        axis = LLVector3::z_axis;
    }
    else
    {
        return false;
    }

    axis.rotVec( grid_rotation );
    return true;
}

F32 LLManip::getSubdivisionLevel(const LLVector3 &reference_point, const LLVector3 &translate_axis, F32 grid_scale, S32 min_pixel_spacing, F32 min_subdivisions, F32 max_subdivisions)
{
    //update current snap subdivision level
    LLVector3 cam_to_reference;
    if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
    {
        cam_to_reference = LLVector3(1.f / gAgentCamera.mHUDCurZoom, 0.f, 0.f);
    }
    else
    {
        cam_to_reference = reference_point - LLViewerCamera::getInstance()->getOrigin();
    }
    F32 current_range = cam_to_reference.normVec();

    F32 projected_translation_axis_length = (translate_axis % cam_to_reference).magVec();
    F32 subdivisions = llmax(projected_translation_axis_length * grid_scale / (current_range / LLViewerCamera::getInstance()->getPixelMeterRatio() * min_pixel_spacing), 0.f);
    // figure out nearest power of 2 that subdivides grid_scale with result > min_pixel_spacing
    subdivisions = llclamp((F32)pow(2.f, llfloor(log(subdivisions) / log(2.f))), min_subdivisions, max_subdivisions);

    return subdivisions;
}

void LLManip::handleSelect()
{
    mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
}

void LLManip::handleDeselect()
{
    mHighlightedPart = LL_NO_PART;
    mManipPart = LL_NO_PART;
    mObjectSelection = NULL;
}

LLObjectSelectionHandle LLManip::getSelection()
{
    return mObjectSelection;
}

bool LLManip::handleHover(S32 x, S32 y, MASK mask)
{
    // We only handle the event if mousedown started with us
    if( hasMouseCapture() )
    {
        if( mObjectSelection->isEmpty() )
        {
            // Somehow the object got deselected while we were dragging it.
            // Release the mouse
            setMouseCapture( false );
        }

        LL_DEBUGS("UserInput") << "hover handled by LLManip (active)" << LL_ENDL;
    }
    else
    {
        LL_DEBUGS("UserInput") << "hover handled by LLManip (inactive)" << LL_ENDL;
    }
    gViewerWindow->setCursor(UI_CURSOR_ARROW);
    return true;
}


bool LLManip::handleMouseUp(S32 x, S32 y, MASK mask)
{
    bool    handled = false;
    if( hasMouseCapture() )
    {
        handled = true;
        setMouseCapture( false );
    }
    return handled;
}

void LLManip::updateGridSettings()
{
    sGridMaxSubdivisionLevel = gSavedSettings.getBOOL("GridSubUnit") ? (F32)gSavedSettings.getS32("GridSubdivision") : 1.f;
}

bool LLManip::getMousePointOnPlaneAgent(LLVector3& point, S32 x, S32 y, LLVector3 origin, LLVector3 normal)
{
    LLVector3d origin_double = gAgent.getPosGlobalFromAgent(origin);
    LLVector3d global_point;
    bool result = getMousePointOnPlaneGlobal(global_point, x, y, origin_double, normal);
    point = gAgent.getPosAgentFromGlobal(global_point);
    return result;
}

bool LLManip::getMousePointOnPlaneGlobal(LLVector3d& point, S32 x, S32 y, LLVector3d origin, LLVector3 normal) const
{
    if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
    {
        bool result = false;
        F32 mouse_x = ((F32)x / gViewerWindow->getWorldViewWidthScaled() - 0.5f) * LLViewerCamera::getInstance()->getAspect() / gAgentCamera.mHUDCurZoom;
        F32 mouse_y = ((F32)y / gViewerWindow->getWorldViewHeightScaled() - 0.5f) / gAgentCamera.mHUDCurZoom;

        LLVector3 origin_agent = gAgent.getPosAgentFromGlobal(origin);
        LLVector3 mouse_pos = LLVector3(0.f, -mouse_x, mouse_y);
        if (llabs(normal.mV[VX]) < 0.001f)
        {
            // use largish value that should be outside HUD manipulation range
            mouse_pos.mV[VX] = 10.f;
        }
        else
        {
            mouse_pos.mV[VX] = (normal * (origin_agent - mouse_pos))
                                / (normal.mV[VX]);
            result = true;
        }

        point = gAgent.getPosGlobalFromAgent(mouse_pos);
        return result;
    }
    else
    {
        return gViewerWindow->mousePointOnPlaneGlobal(
                                        point, x, y, origin, normal );
    }

    //return false;
}

// Given the line defined by mouse cursor (a1 + a_param*(a2-a1)) and the line defined by b1 + b_param*(b2-b1),
// returns a_param and b_param for the points where lines are closest to each other.
// Returns false if the two lines are parallel.
bool LLManip::nearestPointOnLineFromMouse( S32 x, S32 y, const LLVector3& b1, const LLVector3& b2, F32 &a_param, F32 &b_param )
{
    LLVector3 a1;
    LLVector3 a2;

    if (mObjectSelection->getSelectType() == SELECT_TYPE_HUD)
    {
        F32 mouse_x = (((F32)x / gViewerWindow->getWindowWidthScaled()) - 0.5f) * LLViewerCamera::getInstance()->getAspect() / gAgentCamera.mHUDCurZoom;
        F32 mouse_y = (((F32)y / gViewerWindow->getWindowHeightScaled()) - 0.5f) / gAgentCamera.mHUDCurZoom;
        a1 = LLVector3(llmin(b1.mV[VX] - 0.1f, b2.mV[VX] - 0.1f, 0.f), -mouse_x, mouse_y);
        a2 = a1 + LLVector3(1.f, 0.f, 0.f);
    }
    else
    {
        a1 = gAgentCamera.getCameraPositionAgent();
        a2 = gAgentCamera.getCameraPositionAgent() + LLVector3(gViewerWindow->mouseDirectionGlobal(x, y));
    }

    bool parallel = true;
    LLVector3 a = a2 - a1;
    LLVector3 b = b2 - b1;

    LLVector3 normal;
    F32 dist, denom;
    normal = (b % a) % b;   // normal to plane (P) through b and (shortest line between a and b)
    normal.normVec();
    dist = b1 * normal;         // distance from origin to P

    denom = normal * a;
    if( (denom < -F_APPROXIMATELY_ZERO) || (F_APPROXIMATELY_ZERO < denom) )
    {
        a_param = (dist - normal * a1) / denom;
        parallel = false;
    }

    normal = (a % b) % a;   // normal to plane (P) through a and (shortest line between a and b)
    normal.normVec();
    dist = a1 * normal;         // distance from origin to P
    denom = normal * b;
    if( (denom < -F_APPROXIMATELY_ZERO) || (F_APPROXIMATELY_ZERO < denom) )
    {
        b_param = (dist - normal * b1) / denom;
        parallel = false;
    }

    return parallel;
}

LLVector3 LLManip::getSavedPivotPoint() const
{
    return LLSelectMgr::getInstance()->getSavedBBoxOfSelection().getCenterAgent();
}

LLVector3 LLManip::getPivotPoint()
{
    LLViewerObject* object = mObjectSelection->getFirstObject();
    if (object && mObjectSelection->getObjectCount() == 1 && mObjectSelection->getSelectType() != SELECT_TYPE_HUD)
    {
        LLSelectNode* select_node = mObjectSelection->getFirstNode();
        if (select_node->mSelectedGLTFNode != -1)
        {
            return object->getGLTFNodePositionAgent(select_node->mSelectedGLTFNode);
        }
        return mObjectSelection->getFirstObject()->getPivotPositionAgent();
    }
    return LLSelectMgr::getInstance()->getBBoxOfSelection().getCenterAgent();
}


void LLManip::renderGuidelines(bool draw_x, bool draw_y, bool draw_z)
{
    LLVector3 grid_origin;
    LLQuaternion grid_rot;
    LLVector3 grid_scale;
    LLSelectMgr::getInstance()->getGrid(grid_origin, grid_rot, grid_scale);

    const bool children_ok = true;
    LLViewerObject* object = mObjectSelection->getFirstRootObject(children_ok);
    if (!object)
    {
        return;
    }

    //LLVector3  center_agent  = LLSelectMgr::getInstance()->getBBoxOfSelection().getCenterAgent();
    LLVector3  center_agent  = getPivotPoint();

    gGL.pushMatrix();
    {
        gGL.translatef(center_agent.mV[VX], center_agent.mV[VY], center_agent.mV[VZ]);

        F32 angle_radians, x, y, z;

        grid_rot.getAngleAxis(&angle_radians, &x, &y, &z);
        gGL.rotatef(angle_radians * RAD_TO_DEG, x, y, z);

        F32 region_size = LLWorld::getInstance()->getRegionWidthInMeters();

        const F32 LINE_ALPHA = 0.33f;

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        LLUI::setLineWidth(1.5f);

        if (draw_x)
        {
            gGL.color4f(1.f, 0.f, 0.f, LINE_ALPHA);
            gGL.begin(LLRender::LINES);
            gGL.vertex3f( -region_size, 0.f, 0.f );
            gGL.vertex3f(  region_size, 0.f, 0.f );
            gGL.end();
        }

        if (draw_y)
        {
            gGL.color4f(0.f, 1.f, 0.f, LINE_ALPHA);
            gGL.begin(LLRender::LINES);
            gGL.vertex3f( 0.f, -region_size, 0.f );
            gGL.vertex3f( 0.f,  region_size, 0.f );
            gGL.end();
        }

        if (draw_z)
        {
            gGL.color4f(0.f, 0.f, 1.f, LINE_ALPHA);
            gGL.begin(LLRender::LINES);
            gGL.vertex3f( 0.f, 0.f, -region_size );
            gGL.vertex3f( 0.f, 0.f,  region_size );
            gGL.end();
        }
        LLUI::setLineWidth(1.0f);
    }
    gGL.popMatrix();
}

void LLManip::renderXYZ(const LLVector3 &vec)
{
    const S32 PAD = 10;
    std::string feedback_string;
    S32 window_center_x = gViewerWindow->getWorldViewRectScaled().getWidth() / 2;
    S32 window_center_y = gViewerWindow->getWorldViewRectScaled().getHeight() / 2;
    S32 vertical_offset = window_center_y - VERTICAL_OFFSET;


    gGL.pushMatrix();
    {
        LLUIImagePtr imagep = LLUI::getUIImage("Rounded_Square");
        gViewerWindow->setup2DRender();
        const LLVector2& display_scale = gViewerWindow->getDisplayScale();
        gGL.color4f(0.f, 0.f, 0.f, 0.7f);

        imagep->draw(
            (S32)((window_center_x - 115) * display_scale.mV[VX]),
            (S32)((window_center_y + vertical_offset - PAD) * display_scale.mV[VY]),
            (S32)(235 * display_scale.mV[VX]),
            (S32)((PAD * 2 + 10) * display_scale.mV[VY]),
            LLColor4(0.f, 0.f, 0.f, 0.7f) );

        LLFontGL* font = LLFontGL::getFontSansSerif();
        LLLocale locale(LLLocale::USER_LOCALE);
        LLGLDepthTest gls_depth(GL_FALSE);

        // render drop shadowed text (manually because of bigger 'distance')
        F32 right_x;
        feedback_string = llformat("X: %.3f", vec.mV[VX]);
        font->render(utf8str_to_wstring(feedback_string), 0, window_center_x - 102.f + 1.f, (F32)(window_center_y + vertical_offset) - 2.f, LLColor4::black,
            LLFontGL::LEFT, LLFontGL::BASELINE,
            LLFontGL::NORMAL, LLFontGL::NO_SHADOW, S32_MAX, 1000, &right_x);

        feedback_string = llformat("Y: %.3f", vec.mV[VY]);
        font->render(utf8str_to_wstring(feedback_string), 0, window_center_x - 27.f + 1.f, (F32)(window_center_y + vertical_offset) - 2.f, LLColor4::black,
            LLFontGL::LEFT, LLFontGL::BASELINE,
            LLFontGL::NORMAL, LLFontGL::NO_SHADOW, S32_MAX, 1000, &right_x);

        feedback_string = llformat("Z: %.3f", vec.mV[VZ]);
        font->render(utf8str_to_wstring(feedback_string), 0, window_center_x + 48.f + 1.f, (F32)(window_center_y + vertical_offset) - 2.f, LLColor4::black,
            LLFontGL::LEFT, LLFontGL::BASELINE,
            LLFontGL::NORMAL, LLFontGL::NO_SHADOW, S32_MAX, 1000, &right_x);

        // render text on top
        feedback_string = llformat("X: %.3f", vec.mV[VX]);
        font->render(utf8str_to_wstring(feedback_string), 0, window_center_x - 102.f, (F32)(window_center_y + vertical_offset), LLColor4(1.f, 0.5f, 0.5f, 1.f),
            LLFontGL::LEFT, LLFontGL::BASELINE,
            LLFontGL::NORMAL, LLFontGL::NO_SHADOW, S32_MAX, 1000, &right_x);

        feedback_string = llformat("Y: %.3f", vec.mV[VY]);
        font->render(utf8str_to_wstring(feedback_string), 0, window_center_x - 27.f, (F32)(window_center_y + vertical_offset), LLColor4(0.5f, 1.f, 0.5f, 1.f),
            LLFontGL::LEFT, LLFontGL::BASELINE,
            LLFontGL::NORMAL, LLFontGL::NO_SHADOW, S32_MAX, 1000, &right_x);

        feedback_string = llformat("Z: %.3f", vec.mV[VZ]);
        font->render(utf8str_to_wstring(feedback_string), 0, window_center_x + 48.f, (F32)(window_center_y + vertical_offset), LLColor4(0.5f, 0.5f, 1.f, 1.f),
            LLFontGL::LEFT, LLFontGL::BASELINE,
            LLFontGL::NORMAL, LLFontGL::NO_SHADOW, S32_MAX, 1000, &right_x);
    }
    gGL.popMatrix();

    gViewerWindow->setup3DRender();
}

void LLManip::renderTickText(const LLVector3& pos, const std::string& text, const LLColor4 &color)
{
    const LLFontGL* big_fontp = LLFontGL::getFontSansSerif();

    bool hud_selection = mObjectSelection->getSelectType() == SELECT_TYPE_HUD;
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    LLVector3 render_pos = pos;
    if (hud_selection)
    {
        F32 zoom_amt = gAgentCamera.mHUDCurZoom;
        F32 inv_zoom_amt = 1.f / zoom_amt;
        // scale text back up to counter-act zoom level
        render_pos = pos * zoom_amt;
        gGL.scalef(inv_zoom_amt, inv_zoom_amt, inv_zoom_amt);
    }

    // render shadow first
    LLColor4 shadow_color = LLColor4::black;
    shadow_color.mV[VALPHA] = color.mV[VALPHA] * 0.5f;
    gViewerWindow->setup3DViewport(1, -1);
    hud_render_utf8text(text, render_pos, nullptr, *big_fontp, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,  -0.5f * big_fontp->getWidthF32(text), 3.f, shadow_color, mObjectSelection->getSelectType() == SELECT_TYPE_HUD);
    gViewerWindow->setup3DViewport();
    hud_render_utf8text(text, render_pos, nullptr, *big_fontp, LLFontGL::NORMAL, LLFontGL::NO_SHADOW, -0.5f * big_fontp->getWidthF32(text), 3.f, color, mObjectSelection->getSelectType() == SELECT_TYPE_HUD);

    gGL.popMatrix();
}

void LLManip::renderTickValue(const LLVector3& pos, F32 value, const std::string& suffix, const LLColor4 &color)
{
    LLLocale locale(LLLocale::USER_LOCALE);

    const LLFontGL* big_fontp = LLFontGL::getFontSansSerif();
    const LLFontGL* small_fontp = LLFontGL::getFontSansSerifSmall();

    std::string val_string;
    std::string fraction_string;
    F32 val_to_print = ll_round(value, 0.001f);
    S32 fractional_portion = ll_round(fmodf(llabs(val_to_print), 1.f) * 100.f);
    if (val_to_print < 0.f)
    {
        if (fractional_portion == 0)
        {
            val_string = llformat("-%d%s", lltrunc(llabs(val_to_print)), suffix.c_str());
        }
        else
        {
            val_string = llformat("-%d", lltrunc(llabs(val_to_print)));
        }
    }
    else
    {
        if (fractional_portion == 0)
        {
            val_string = llformat("%d%s", lltrunc(llabs(val_to_print)), suffix.c_str());
        }
        else
        {
            val_string = llformat("%d", lltrunc(val_to_print));
        }
    }

    bool hud_selection = mObjectSelection->getSelectType() == SELECT_TYPE_HUD;
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    {
        LLVector3 render_pos = pos;
        if (hud_selection)
        {
            F32 zoom_amt = gAgentCamera.mHUDCurZoom;
            F32 inv_zoom_amt = 1.f / zoom_amt;
            // scale text back up to counter-act zoom level
            render_pos = pos * zoom_amt;
            gGL.scalef(inv_zoom_amt, inv_zoom_amt, inv_zoom_amt);
        }

        if (fractional_portion != 0)
        {
            fraction_string = llformat("%c%02d%s", LLResMgr::getInstance()->getDecimalPoint(), fractional_portion, suffix.c_str());

            hud_render_utf8text(val_string, render_pos, nullptr, *big_fontp, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW, -1.f * big_fontp->getWidthF32(val_string), 3.f, color, hud_selection);
            hud_render_utf8text(fraction_string, render_pos, nullptr, *small_fontp, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW, 1.f, 3.f, color, hud_selection);
        }
        else
        {
            hud_render_utf8text(val_string, render_pos, nullptr, *big_fontp, LLFontGL::NORMAL, LLFontGL::DROP_SHADOW, -0.5f * big_fontp->getWidthF32(val_string), 3.f, color, hud_selection);
        }
    }
    gGL.popMatrix();
}

LLColor4 LLManip::setupSnapGuideRenderPass(S32 pass)
{
    static LLUIColor grid_color_fg = LLUIColorTable::instance().getColor("GridlineColor");
    static LLUIColor grid_color_bg = LLUIColorTable::instance().getColor("GridlineBGColor");
    static LLUIColor grid_color_shadow = LLUIColorTable::instance().getColor("GridlineShadowColor");

    LLColor4 line_color;
    F32 line_alpha = gSavedSettings.getF32("GridOpacity");

    switch(pass)
    {
    case 0:
        // shadow
        gViewerWindow->setup3DViewport(1, -1);
        line_color = grid_color_shadow;
        line_color.mV[VALPHA] *= line_alpha;
        LLUI::setLineWidth(2.f);
        break;
    case 1:
        // hidden lines
        gViewerWindow->setup3DViewport();
        line_color = grid_color_bg;
        line_color.mV[VALPHA] *= line_alpha;
        LLUI::setLineWidth(1.f);
        break;
    case 2:
        // visible lines
        line_color = grid_color_fg;
        line_color.mV[VALPHA] *= line_alpha;
        break;
    }

    return line_color;
}
