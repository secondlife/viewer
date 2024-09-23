/**
 * @file lltoolpie.cpp
 * @brief LLToolPie class implementation
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

#include "lltoolpie.h"

#include "indra_constants.h"
#include "llclickaction.h"
#include "llparcel.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llavatarnamecache.h"
#include "llfocusmgr.h"
#include "llfirstuse.h"
#include "llfloaterland.h"
#include "llfloaterreg.h"
#include "llfloaterscriptdebug.h"
#include "lltooltip.h"
#include "llhudeffecttrail.h"
#include "llhudicon.h"
#include "llhudmanager.h"
#include "llkeyboard.h"
#include "llmediaentry.h"
#include "llmenugl.h"
#include "llmutelist.h"
#include "llresmgr.h"  // getMonetaryString
#include "llselectmgr.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lltoolselect.h"
#include "lltrans.h"
#include "llviewercamera.h"
#include "llviewerparcelmedia.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llviewerinput.h"
#include "llviewermedia.h"
#include "llvoavatarself.h"
#include "llviewermediafocus.h"
#include "llworld.h"
#include "llui.h"
#include "llweb.h"
#include "pipeline.h"   // setHighlightObject
#include "lluiusage.h"

extern bool gDebugClicks;

static void handle_click_action_play();
static void handle_click_action_open_media(LLPointer<LLViewerObject> objectp);
static ECursorType cursor_from_parcel_media(U8 click_action);

LLToolPie::LLToolPie()
:   LLTool(std::string("Pie")),
    mMouseButtonDown( false ),
    mMouseOutsideSlop( false ),
    mMouseSteerX(-1),
    mMouseSteerY(-1),
    mClickAction(0),
    mClickActionBuyEnabled( true ),
    mClickActionPayEnabled( true ),
    mDoubleClickTimer()
{
}

bool LLToolPie::handleAnyMouseClick(S32 x, S32 y, MASK mask, EMouseClickType clicktype, bool down)
{
    bool result = LLMouseHandler::handleAnyMouseClick(x, y, mask, clicktype, down);

    // This override DISABLES the keyboard focus reset that LLTool::handleAnyMouseClick adds.
    // LLToolPie will do the right thing in its pick callback.

    return result;
}

bool LLToolPie::handleMouseDown(S32 x, S32 y, MASK mask)
{
    if (mDoubleClickTimer.getStarted())
    {
        mDoubleClickTimer.stop();
    }

    mMouseOutsideSlop = false;
    mMouseDownX = x;
    mMouseDownY = y;
    LLTimer pick_timer;
    bool pick_rigged = false; //gSavedSettings.getBOOL("AnimatedObjectsAllowLeftClick");
    LLPickInfo transparent_pick = gViewerWindow->pickImmediate(x, y, true /*includes transparent*/, pick_rigged, false, true, false);
    LLPickInfo visible_pick = gViewerWindow->pickImmediate(x, y, false, pick_rigged);
    LLViewerObject *transp_object = transparent_pick.getObject();
    LLViewerObject *visible_object = visible_pick.getObject();

    // Current set of priorities
    // 1. Transparent attachment pick
    // 2. Transparent actionable pick
    // 3. Visible attachment pick (e.x we click on attachment under invisible floor)
    // 4. Visible actionable pick
    // 5. Transparent pick (e.x. movement on transparent object/floor, our default pick)
    // left mouse down always picks transparent (but see handleMouseUp).
    // Also see LLToolPie::handleHover() - priorities are a bit different there.
    // Todo: we need a more consistent set of rules to work with
    if (transp_object == visible_object || !visible_object ||
        !transp_object) // avoid potential for null dereference below, don't make assumptions about behavior of pickImmediate
    {
        mPick = transparent_pick;
    }
    else
    {
        // Select between two non-null picks
        LLViewerObject *transp_parent = transp_object->getRootEdit();
        LLViewerObject *visible_parent = visible_object->getRootEdit();
        if (transp_object->isAttachment())
        {
            // 1. Transparent attachment
            mPick = transparent_pick;
        }
        else if (transp_object->getClickAction() != CLICK_ACTION_DISABLED
                 && (useClickAction(mask, transp_object, transp_parent) || transp_object->flagHandleTouch() || (transp_parent && transp_parent->flagHandleTouch())))
        {
            // 2. Transparent actionable pick
            mPick = transparent_pick;
        }
        else if (visible_object->isAttachment())
        {
            // 3. Visible attachment pick
            mPick = visible_pick;
        }
        else if (visible_object->getClickAction() != CLICK_ACTION_DISABLED
                 && (useClickAction(mask, visible_object, visible_parent) || visible_object->flagHandleTouch() || (visible_parent && visible_parent->flagHandleTouch())))
        {
            // 4. Visible actionable pick
            mPick = visible_pick;
        }
        else
        {
            // 5. Default: transparent
            mPick = transparent_pick;
        }
    }
    LL_INFOS() << "pick_rigged is " << (S32) pick_rigged << " pick time elapsed " << pick_timer.getElapsedTimeF32() << LL_ENDL;

    mPick.mKeyMask = mask;

    mMouseButtonDown = true;

    // If nothing clickable is picked, needs to return
    // false for click-to-walk or click-to-teleport to work.
    return handleLeftClickPick();
}

// Spawn context menus on right mouse down so you can drag over and select
// an item.
bool LLToolPie::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    bool pick_reflection_probe = gSavedSettings.getBOOL("SelectReflectionProbes");

    // don't pick transparent so users can't "pay" transparent objects
    mPick = gViewerWindow->pickImmediate(x, y,
                                         /*bool pick_transparent*/ false,
                                         /*bool pick_rigged*/ true,
                                         /*bool pick_particle*/ true,
                                         /*bool pick_unselectable*/ true,
                                         pick_reflection_probe);
    mPick.mKeyMask = mask;

    // claim not handled so UI focus stays same
    if(gAgentCamera.getCameraMode() != CAMERA_MODE_MOUSELOOK)
    {
        handleRightClickPick();
    }
    return false;
}

bool LLToolPie::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
    LLToolMgr::getInstance()->clearTransientTool();
    return LLTool::handleRightMouseUp(x, y, mask);
}

bool LLToolPie::handleScrollWheelAny(S32 x, S32 y, S32 clicks_x, S32 clicks_y)
{
    bool res = false;
    // mHoverPick should have updated on its own and we should have a face
    // in LLViewerMediaFocus in case of media, so just reuse mHoverPick
    if (mHoverPick.mUVCoords.mV[VX] >= 0.f && mHoverPick.mUVCoords.mV[VY] >= 0.f)
    {
        res = LLViewerMediaFocus::getInstance()->handleScrollWheel(mHoverPick.mUVCoords, clicks_x, clicks_y);
    }
    else
    {
        // this won't provide correct coordinates in case of object selection
        res = LLViewerMediaFocus::getInstance()->handleScrollWheel(x, y, clicks_x, clicks_y);
    }
    return res;
}

bool LLToolPie::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
    return handleScrollWheelAny(x, y, 0, clicks);
}

bool LLToolPie::handleScrollHWheel(S32 x, S32 y, S32 clicks)
{
    return handleScrollWheelAny(x, y, clicks, 0);
}

// True if you selected an object.
bool LLToolPie::handleLeftClickPick()
{
    S32 x = mPick.mMousePt.mX;
    S32 y = mPick.mMousePt.mY;
    MASK mask = mPick.mKeyMask;
    if (mPick.mPickType == LLPickInfo::PICK_PARCEL_WALL)
    {
        LLParcel* parcel = LLViewerParcelMgr::getInstance()->getCollisionParcel();
        if (parcel)
        {
            LLViewerParcelMgr::getInstance()->selectCollisionParcel();
            if (parcel->getParcelFlag(PF_USE_PASS_LIST)
                && !LLViewerParcelMgr::getInstance()->isCollisionBanned())
            {
                // if selling passes, just buy one
                void* deselect_when_done = (void*)true;
                LLPanelLandGeneral::onClickBuyPass(deselect_when_done);
            }
            else
            {
                // not selling passes, get info
                LLFloaterReg::showInstance("about_land");
            }
        }

        gFocusMgr.setKeyboardFocus(NULL);
        return LLTool::handleMouseDown(x, y, mask);
    }

    // didn't click in any UI object, so must have clicked in the world
    LLViewerObject *object = mPick.getObject();
    LLViewerObject *parent = NULL;

    if (mPick.mPickType != LLPickInfo::PICK_LAND)
    {
        LLViewerParcelMgr::getInstance()->deselectLand();
    }

    if (object)
    {
        parent = object->getRootEdit();
    }

    if (handleMediaClick(mPick))
    {
        return true;
    }

    // If it's a left-click, and we have a special action, do it.
    if (useClickAction(mask, object, parent))
    {
        mClickAction = 0;
        if (object && object->getClickAction())
        {
            mClickAction = object->getClickAction();
        }
        else if (parent && parent->getClickAction())
        {
            mClickAction = parent->getClickAction();
        }

        switch(mClickAction)
        {
        case CLICK_ACTION_TOUCH:
            // touch behavior down below...
            break;
        case CLICK_ACTION_SIT:
            {
                if (isAgentAvatarValid() && !gAgentAvatarp->isSitting()) // agent not already sitting
                {
                    handle_object_sit_or_stand();
                    // put focus in world when sitting on an object
                    gFocusMgr.setKeyboardFocus(NULL);
                    return true;
                } // else nothing (fall through to touch)
            }
        case CLICK_ACTION_PAY:
            if ( mClickActionPayEnabled )
            {
                if ((object && object->flagTakesMoney())
                    || (parent && parent->flagTakesMoney()))
                {
                    // pay event goes to object actually clicked on
                    mClickActionObject = object;
                    mLeftClickSelection = LLToolSelect::handleObjectSelection(mPick, false, true);
                    if (LLSelectMgr::getInstance()->selectGetAllValid())
                    {
                        // call this right away, since we have all the info we need to continue the action
                        selectionPropertiesReceived();
                    }
                    return true;
                }
            }
            break;
        case CLICK_ACTION_BUY:
            if ( mClickActionBuyEnabled )
            {
                mClickActionObject = parent;
                mLeftClickSelection = LLToolSelect::handleObjectSelection(mPick, false, true, true);
                if (LLSelectMgr::getInstance()->selectGetAllValid())
                {
                    // call this right away, since we have all the info we need to continue the action
                    selectionPropertiesReceived();
                }
                return true;
            }
            break;
        case CLICK_ACTION_OPEN:
            if (parent && parent->allowOpen())
            {
                mClickActionObject = parent;
                mLeftClickSelection = LLToolSelect::handleObjectSelection(mPick, false, true, true);
                if (LLSelectMgr::getInstance()->selectGetAllValid())
                {
                    // call this right away, since we have all the info we need to continue the action
                    selectionPropertiesReceived();
                }
            }
            return true;
        case CLICK_ACTION_PLAY:
            handle_click_action_play();
            return true;
        case CLICK_ACTION_OPEN_MEDIA:
            // mClickActionObject = object;
            handle_click_action_open_media(object);
            return true;
        case CLICK_ACTION_ZOOM:
            {
                const F32 PADDING_FACTOR = 2.f;
                LLViewerObject* object = gObjectList.findObject(mPick.mObjectID);

                if (object)
                {
                    gAgentCamera.setFocusOnAvatar(false, ANIMATE);

                    LLBBox bbox = object->getBoundingBoxAgent() ;
                    F32 angle_of_view = llmax(0.1f, LLViewerCamera::getInstance()->getAspect() > 1.f ? LLViewerCamera::getInstance()->getView() * LLViewerCamera::getInstance()->getAspect() : LLViewerCamera::getInstance()->getView());
                    F32 distance = bbox.getExtentLocal().magVec() * PADDING_FACTOR / atan(angle_of_view);

                    LLVector3 obj_to_cam = LLViewerCamera::getInstance()->getOrigin() - bbox.getCenterAgent();
                    obj_to_cam.normVec();

                    LLVector3d object_center_global = gAgent.getPosGlobalFromAgent(bbox.getCenterAgent());
                    gAgentCamera.setCameraPosAndFocusGlobal(object_center_global + LLVector3d(obj_to_cam * distance),
                                                      object_center_global,
                                                      mPick.mObjectID );
                }
            }
            return true;
        case CLICK_ACTION_DISABLED:
            return true;
        default:
            // nothing
            break;
        }
    }

    // put focus back "in world"
    if (gFocusMgr.getKeyboardFocus())
    {
        gFocusMgr.setKeyboardFocus(NULL);
    }

    bool touchable = object
                     && (object->getClickAction() != CLICK_ACTION_DISABLED)
                     && (object->flagHandleTouch() || (parent && parent->flagHandleTouch()));

    // Switch to grab tool if physical or triggerable
    if (object &&
        !object->isAvatar() &&
        ((object->flagUsePhysics() || (parent && !parent->isAvatar() && parent->flagUsePhysics())) || touchable)
        )
    {
        gGrabTransientTool = this;
        mMouseButtonDown = false;
        LLToolGrab::getInstance()->setClickedInMouselook(gAgentCamera.cameraMouselook());
        LLToolMgr::getInstance()->getCurrentToolset()->selectTool( LLToolGrab::getInstance() );
        return LLToolGrab::getInstance()->handleObjectHit( mPick );
    }

    LLHUDIcon* last_hit_hud_icon = mPick.mHUDIcon;
    if (!object && last_hit_hud_icon && last_hit_hud_icon->getSourceObject())
    {
        LLFloaterScriptDebug::show(last_hit_hud_icon->getSourceObject()->getID());
    }

    // If left-click never selects or spawns a menu
    // Eat the event.

    // mouse already released
    if (!mMouseButtonDown)
    {
        return true;
    }

    while (object && object->isAttachment() && !object->flagHandleTouch())
    {
        // don't pick avatar through hud attachment
        if (object->isHUDAttachment())
        {
            break;
        }
        object = (LLViewerObject*)object->getParent();
    }
    if (object && object == gAgentAvatarp)
    {
        // we left clicked on avatar, switch to focus mode
        mMouseButtonDown = false;
        LLToolMgr::getInstance()->setTransientTool(LLToolCamera::getInstance());
        gViewerWindow->hideCursor();
        LLToolCamera::getInstance()->setMouseCapture(true);
        LLToolCamera::getInstance()->setClickPickPending();
        LLToolCamera::getInstance()->pickCallback(mPick);
        gAgentCamera.setFocusOnAvatar(true, true);

        return true;
    }
    //////////
    //  // Could be first left-click on nothing
    //  LLFirstUse::useLeftClickNoHit();
    /////////

    return LLTool::handleMouseDown(x, y, mask);
}

bool LLToolPie::useClickAction(MASK mask,
                               LLViewerObject* object,
                               LLViewerObject* parent)
{
    return  mask == MASK_NONE
            && object
            && !object->isAttachment()
            && LLPrimitive::isPrimitive(object->getPCode())
                // useClickAction does not handle Touch (0) or Disabled action
            && ((object->getClickAction() && object->getClickAction() != CLICK_ACTION_DISABLED)
                || (parent && parent->getClickAction() && parent->getClickAction() != CLICK_ACTION_DISABLED));

}

U8 final_click_action(LLViewerObject* obj)
{
    if (!obj) return CLICK_ACTION_NONE;
    if (obj->isAttachment()) return CLICK_ACTION_NONE;

    U8 click_action = CLICK_ACTION_TOUCH;
    LLViewerObject* parent = obj->getRootEdit();
    U8 object_action = obj->getClickAction();
    U8 parent_action = parent ? parent->getClickAction() : CLICK_ACTION_TOUCH;
    if (parent_action == CLICK_ACTION_DISABLED || object_action)
    {
        // CLICK_ACTION_DISABLED ("None" in UI) is intended for child action to
        // override parent's action when assigned to parent or to child
        click_action = object_action;
    }
    else if (parent_action)
    {
        click_action = parent_action;
    }
    return click_action;
}

ECursorType LLToolPie::cursorFromObject(LLViewerObject* object)
{
    LLViewerObject* parent = NULL;
    if (object)
    {
        parent = object->getRootEdit();
    }
    U8 click_action = final_click_action(object);
    ECursorType cursor = UI_CURSOR_ARROW;
    switch(click_action)
    {
    case CLICK_ACTION_SIT:
        {
            if (isAgentAvatarValid() && !gAgentAvatarp->isSitting()) // not already sitting?
            {
                cursor = UI_CURSOR_TOOLSIT;
            }
        }
        break;
    case CLICK_ACTION_BUY:
        if ( mClickActionBuyEnabled )
        {
            LLSelectNode* node = LLSelectMgr::getInstance()->getHoverNode();
            if (!node || node->mSaleInfo.isForSale())
            {
                cursor = UI_CURSOR_TOOLBUY;
            }
        }
        break;
    case CLICK_ACTION_OPEN:
        // Open always opens the parent.
        if (parent && parent->allowOpen())
        {
            cursor = UI_CURSOR_TOOLOPEN;
        }
        break;
    case CLICK_ACTION_PAY:
        if ( mClickActionPayEnabled )
        {
            if ((object && object->flagTakesMoney())
                || (parent && parent->flagTakesMoney()))
            {
                cursor = UI_CURSOR_TOOLBUY;
            }
        }
        break;
    case CLICK_ACTION_ZOOM:
            cursor = UI_CURSOR_TOOLZOOMIN;
            break;
    case CLICK_ACTION_PLAY:
    case CLICK_ACTION_OPEN_MEDIA:
        cursor = cursor_from_parcel_media(click_action);
        break;
    case CLICK_ACTION_DISABLED:
        break;
    default:
        break;
    }
    return cursor;
}

void LLToolPie::resetSelection()
{
    mLeftClickSelection = NULL;
    mClickActionObject = NULL;
    mClickAction = 0;
}

bool LLToolPie::walkToClickedLocation()
{
    if (gAgent.getFlying()                          // don't auto-navigate while flying until that works
        || !gAgentAvatarp
        || gAgentAvatarp->isSitting())
    {
        return false;
    }

    LLUIUsage::instance().logCommand("Agent.WalkToClickedLocation");

    LLPickInfo saved_pick = mPick;
    if (gAgentCamera.getCameraMode() != CAMERA_MODE_MOUSELOOK)
    {
        mPick = gViewerWindow->pickImmediate(mHoverPick.mMousePt.mX, mHoverPick.mMousePt.mY,
            false /* ignore transparent */,
            false /* ignore rigged */,
            false /* ignore particles */);
    }
    else
    {
        // We do not handle hover in mouselook as we do in other modes, so
        // use croshair's position to do a pick
        mPick = gViewerWindow->pickImmediate(gViewerWindow->getWorldViewRectScaled().getWidth() / 2,
            gViewerWindow->getWorldViewRectScaled().getHeight() / 2,
            false /* ignore transparent */,
            false /* ignore rigged */,
            false /* ignore particles */);
    }

    if (mPick.mPickType == LLPickInfo::PICK_OBJECT)
    {
        if (mPick.getObject() && mPick.getObject()->isHUDAttachment())
        {
            mPick = saved_pick;
            return false;
        }
    }

    LLViewerObject* avatar_object = mPick.getObject();

    // get pointer to avatar
    while (avatar_object && !avatar_object->isAvatar())
    {
        avatar_object = (LLViewerObject*)avatar_object->getParent();
    }

    if (avatar_object && ((LLVOAvatar*)avatar_object)->isSelf())
    {
        const F64 SELF_CLICK_WALK_DISTANCE = 3.0;
        // pretend we picked some point a bit in front of avatar
        mPick.mPosGlobal = gAgent.getPositionGlobal() + LLVector3d(LLViewerCamera::instance().getAtAxis()) * SELF_CLICK_WALK_DISTANCE;
    }

    if ((mPick.mPickType == LLPickInfo::PICK_LAND && !mPick.mPosGlobal.isExactlyZero()) ||
        (mPick.mObjectID.notNull() && !mPick.mPosGlobal.isExactlyZero()))
    {
        gAgentCamera.setFocusOnAvatar(true, true);

        if (mAutoPilotDestination) { mAutoPilotDestination->markDead(); }
        mAutoPilotDestination = (LLHUDEffectBlob *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BLOB, false);
        mAutoPilotDestination->setPositionGlobal(mPick.mPosGlobal);
        mAutoPilotDestination->setPixelSize(5);
        mAutoPilotDestination->setColor(LLColor4U(170, 210, 190));
        mAutoPilotDestination->setDuration(3.f);

        LLVector3d pos = LLToolPie::getInstance()->getPick().mPosGlobal;
        gAgent.startAutoPilotGlobal(pos, std::string(), NULL, NULL, NULL, 0.f, 0.03f, false);
        LLFirstUse::notMoving(false);
        showVisualContextMenuEffect();
        return true;
    }
    else
    {
        LL_DEBUGS() << "walk target was "
            << (mPick.mPosGlobal.isExactlyZero() ? "zero" : "not zero")
            << ", pick type was " << (mPick.mPickType == LLPickInfo::PICK_LAND ? "land" : "not land")
            << ", pick object was " << mPick.mObjectID
            << LL_ENDL;
        mPick = saved_pick;
        return false;
    }
}

bool LLToolPie::teleportToClickedLocation()
{
    if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK)
    {
        // We do not handle hover in mouselook as we do in other modes, so
        // use croshair's position to do a pick
        bool pick_rigged = false;
        mHoverPick = gViewerWindow->pickImmediate(gViewerWindow->getWorldViewRectScaled().getWidth() / 2,
                                                  gViewerWindow->getWorldViewRectScaled().getHeight() / 2,
                                                  false,
                                                  pick_rigged);
    }
    LLViewerObject* objp = mHoverPick.getObject();
    LLViewerObject* parentp = objp ? objp->getRootEdit() : NULL;

    if (objp && (objp->getAvatar() == gAgentAvatarp || objp == gAgentAvatarp)) // ex: nametag
    {
        // Don't teleport to self, teleporting to other avatars is fine
        return false;
    }

    bool is_in_world = mHoverPick.mObjectID.notNull() && objp && !objp->isHUDAttachment();
    bool is_land = mHoverPick.mPickType == LLPickInfo::PICK_LAND;
    bool pos_non_zero = !mHoverPick.mPosGlobal.isExactlyZero();
    bool has_touch_handler = (objp && objp->flagHandleTouch()) || (parentp && parentp->flagHandleTouch());
    U8 click_action = final_click_action(objp); // default action: 0 - touch
    bool has_click_action = (click_action || has_touch_handler) && click_action != CLICK_ACTION_DISABLED;

    if (pos_non_zero && (is_land || (is_in_world && !has_click_action)))
    {
        LLVector3d pos = mHoverPick.mPosGlobal;
        pos.mdV[VZ] += gAgentAvatarp->getPelvisToFoot();
        gAgent.teleportViaLocationLookAt(pos);
        mPick = mHoverPick;
        showVisualContextMenuEffect();
        return true;
    }
    return false;
}

// When we get object properties after left-clicking on an object
// with left-click = buy, if it's the same object, do the buy.

// static
void LLToolPie::selectionPropertiesReceived()
{
    // Make sure all data has been received.
    // This function will be called repeatedly as the data comes in.
    if (!LLSelectMgr::getInstance()->selectGetAllValid())
    {
        return;
    }

    LLObjectSelection* selection = LLToolPie::getInstance()->getLeftClickSelection();
    if (selection)
    {
        LLViewerObject* selected_object = selection->getPrimaryObject();
        // since we don't currently have a way to lock a selection, it could have changed
        // after we initially clicked on the object
        if (selected_object == LLToolPie::getInstance()->getClickActionObject())
        {
            U8 click_action = LLToolPie::getInstance()->getClickAction();
            switch (click_action)
            {
            case CLICK_ACTION_BUY:
                if ( LLToolPie::getInstance()->mClickActionBuyEnabled )
                {
                    handle_buy();
                }
                break;
            case CLICK_ACTION_PAY:
                if ( LLToolPie::getInstance()->mClickActionPayEnabled )
                {
                    handle_give_money_dialog();
                }
                break;
            case CLICK_ACTION_OPEN:
                LLFloaterReg::showInstance("openobject");
                break;
            case CLICK_ACTION_DISABLED:
                break;
            default:
                break;
            }
        }
    }
    LLToolPie::getInstance()->resetSelection();
}

bool LLToolPie::handleHover(S32 x, S32 y, MASK mask)
{
    bool pick_rigged = false; //gSavedSettings.getBOOL("AnimatedObjectsAllowLeftClick");
    mHoverPick = gViewerWindow->pickImmediate(x, y, false, pick_rigged);
    LLViewerObject *parent = NULL;
    LLViewerObject *object = mHoverPick.getObject();
    LLSelectMgr::getInstance()->setHoverObject(object, mHoverPick.mObjectFace);
    if (object)
    {
        parent = object->getRootEdit();
    }

    if (!handleMediaHover(mHoverPick)
        && !mMouseOutsideSlop
        && mMouseButtonDown
        // disable camera steering if click on land is not used for moving
        && gViewerInput.isMouseBindUsed(CLICK_LEFT, MASK_NONE, MODE_THIRD_PERSON))
    {
        S32 delta_x = x - mMouseDownX;
        S32 delta_y = y - mMouseDownY;
        if (delta_x * delta_x + delta_y * delta_y > DRAG_N_DROP_DISTANCE_THRESHOLD * DRAG_N_DROP_DISTANCE_THRESHOLD)
        {
            startCameraSteering();
            steerCameraWithMouse(x, y);
            gViewerWindow->setCursor(UI_CURSOR_TOOLGRAB);
        }
        else
        {
            gViewerWindow->setCursor(UI_CURSOR_ARROW);
        }
    }
    else if (inCameraSteerMode())
    {
        steerCameraWithMouse(x, y);
        gViewerWindow->setCursor(UI_CURSOR_TOOLGRAB);
    }
    else
    {
        // perform a separate pick that detects transparent objects since they respond to 1-click actions
        LLPickInfo click_action_pick = gViewerWindow->pickImmediate(x, y, false, pick_rigged);

        LLViewerObject* click_action_object = click_action_pick.getObject();

        if (click_action_object && useClickAction(mask, click_action_object, click_action_object->getRootEdit()))
        {
            ECursorType cursor = cursorFromObject(click_action_object);
            gViewerWindow->setCursor(cursor);
            LL_DEBUGS("UserInput") << "hover handled by LLToolPie (inactive)" << LL_ENDL;
        }

        else if ((object && !object->isAvatar() && object->flagUsePhysics())
                 || (parent && !parent->isAvatar() && parent->flagUsePhysics()))
        {
            gViewerWindow->setCursor(UI_CURSOR_TOOLGRAB);
            LL_DEBUGS("UserInput") << "hover handled by LLToolPie (inactive)" << LL_ENDL;
        }
        else if ((!object || object->getClickAction() != CLICK_ACTION_DISABLED)
                 && ((object && object->flagHandleTouch()) || (parent && parent->flagHandleTouch()))
                 && (!object || !object->isAvatar()))
        {
            gViewerWindow->setCursor(UI_CURSOR_HAND);
            LL_DEBUGS("UserInput") << "hover handled by LLToolPie (inactive)" << LL_ENDL;
        }
        else
        {
            gViewerWindow->setCursor(UI_CURSOR_ARROW);
            LL_DEBUGS("UserInput") << "hover handled by LLToolPie (inactive)" << LL_ENDL;
        }
    }

    if(!object)
    {
        LLViewerMediaFocus::getInstance()->clearHover();
    }

    return true;
}

bool LLToolPie::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if (!mDoubleClickTimer.getStarted())
    {
        mDoubleClickTimer.start();
    }
    else
    {
        mDoubleClickTimer.reset();
    }
    LLViewerObject* obj = mPick.getObject();

    stopCameraSteering();
    mMouseButtonDown = false;

    gViewerWindow->setCursor(UI_CURSOR_ARROW);
    if (hasMouseCapture())
    {
        setMouseCapture(false);
    }

    LLToolMgr::getInstance()->clearTransientTool();
    gAgentCamera.setLookAt(LOOKAT_TARGET_CONVERSATION, obj); // maybe look at object/person clicked on

    return LLTool::handleMouseUp(x, y, mask);
}

void LLToolPie::stopClickToWalk()
{
    mPick.mPosGlobal = gAgent.getPositionGlobal();
    handle_go_to();
    if(mAutoPilotDestination)
    {
        mAutoPilotDestination->markDead();
    }
}

bool LLToolPie::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    if (gDebugClicks)
    {
        LL_INFOS() << "LLToolPie handleDoubleClick (becoming mouseDown)" << LL_ENDL;
    }

    if (handleMediaDblClick(mPick))
    {
        return true;
    }

    if (!mDoubleClickTimer.getStarted() || (mDoubleClickTimer.getElapsedTimeF32() > 0.3f))
    {
        mDoubleClickTimer.stop();
        return false;
    }
    mDoubleClickTimer.stop();

    return false;
}

static bool needs_tooltip(LLSelectNode* nodep)
{
    if (!nodep || !nodep->mValid)
        return false;
    return true;
}


bool LLToolPie::handleTooltipLand(std::string line, std::string tooltip_msg)
{
    //  Do not show hover for land unless prefs are set to allow it.
    if (!gSavedSettings.getBOOL("ShowLandHoverTip")) return true;

    LLViewerParcelMgr::getInstance()->setHoverParcel( mHoverPick.mPosGlobal );

    // Didn't hit an object, but since we have a land point we
    // must be hovering over land.

    LLParcel* hover_parcel = LLViewerParcelMgr::getInstance()->getHoverParcel();
    LLUUID owner;

    if ( hover_parcel )
    {
        owner = hover_parcel->getOwnerID();
    }

    // Line: "Land"
    line.clear();
    line.append(LLTrans::getString("TooltipLand"));
    if (hover_parcel)
    {
        line.append(hover_parcel->getName());
    }
    tooltip_msg.append(line);
    tooltip_msg.push_back('\n');

    // Line: "Owner: James Linden"
    line.clear();
    line.append(LLTrans::getString("TooltipOwner") + " ");

    if ( hover_parcel )
    {
        std::string name;
        if (LLUUID::null == owner)
        {
            line.append(LLTrans::getString("TooltipPublic"));
        }
        else if (hover_parcel->getIsGroupOwned())
        {
            if (gCacheName->getGroupName(owner, name))
            {
                line.append(name);
                line.append(LLTrans::getString("TooltipIsGroup"));
            }
            else
            {
                line.append(LLTrans::getString("RetrievingData"));
            }
        }
        else
        {
            LLAvatarName av_name;
            if (LLAvatarNameCache::get(owner, &av_name))
            {
                name = av_name.getUserName();
                line.append(name);
            }
            else
            {
                line.append(LLTrans::getString("RetrievingData"));
            }
        }
    }
    else
    {
        line.append(LLTrans::getString("RetrievingData"));
    }
    tooltip_msg.append(line);
    tooltip_msg.push_back('\n');

    // Line: "no fly, not safe, no build"

    // Don't display properties for your land.  This is just
    // confusing, because you can do anything on your own land.
    if ( hover_parcel && owner != gAgent.getID() )
    {
        S32 words = 0;

        line.clear();
        // JC - Keep this in the same order as the checkboxes
        // on the land info panel
        if ( !hover_parcel->getAllowModify() )
        {
            if ( hover_parcel->getAllowGroupModify() )
            {
                line.append(LLTrans::getString("TooltipFlagGroupBuild"));
            }
            else
            {
                line.append(LLTrans::getString("TooltipFlagNoBuild"));
            }
            words++;
        }

        if ( !hover_parcel->getAllowTerraform() )
        {
            if (words) line.append(", ");
            line.append(LLTrans::getString("TooltipFlagNoEdit"));
            words++;
        }

        if ( hover_parcel->getAllowDamage() )
        {
            if (words) line.append(", ");
            line.append(LLTrans::getString("TooltipFlagNotSafe"));
            words++;
        }

        // Maybe we should reflect the estate's block fly bit here as well?  DK 12/1/04
        if ( !hover_parcel->getAllowFly() )
        {
            if (words) line.append(", ");
            line.append(LLTrans::getString("TooltipFlagNoFly"));
            words++;
        }

        if ( !hover_parcel->getAllowOtherScripts() )
        {
            if (words) line.append(", ");
            if ( hover_parcel->getAllowGroupScripts() )
            {
                line.append(LLTrans::getString("TooltipFlagGroupScripts"));
            }
            else
            {
                line.append(LLTrans::getString("TooltipFlagNoScripts"));
            }

            words++;
        }

        if (words)
        {
            tooltip_msg.append(line);
            tooltip_msg.push_back('\n');
        }
    }

    if (hover_parcel && hover_parcel->getParcelFlag(PF_FOR_SALE))
    {
        LLStringUtil::format_map_t args;
        S32 price = hover_parcel->getSalePrice();
        args["[AMOUNT]"] = LLResMgr::getInstance()->getMonetaryString(price);
        line = LLTrans::getString("TooltipForSaleL$", args);
        tooltip_msg.append(line);
        tooltip_msg.push_back('\n');
    }

    // trim last newlines
    if (!tooltip_msg.empty())
    {
        tooltip_msg.erase(tooltip_msg.size() - 1);
        LLToolTipMgr::instance().show(tooltip_msg);
    }

    return true;
}

bool LLToolPie::handleTooltipObject( LLViewerObject* hover_object, std::string line, std::string tooltip_msg)
{
    if ( hover_object->isHUDAttachment() )
    {
        // no hover tips for HUD elements, since they can obscure
        // what the HUD is displaying
        return true;
    }

    if ( hover_object->isAttachment() )
    {
        // get root of attachment then parent, which is avatar
        LLViewerObject* root_edit = hover_object->getRootEdit();
        if (!root_edit)
        {
            // Strange parenting issue, don't show any text
            return true;
        }
        hover_object = (LLViewerObject*)root_edit->getParent();
        if (!hover_object)
        {
            // another strange parenting issue, bail out
            return true;
        }
    }

    line.clear();
    if (hover_object->isAvatar())
    {
        // only show tooltip if same inspector not already open
        LLFloater* existing_inspector = LLFloaterReg::findInstance("inspect_avatar");
        if (!existing_inspector
            || !existing_inspector->getVisible()
            || existing_inspector->getKey()["avatar_id"].asUUID() != hover_object->getID())
        {
            // Try to get display name + username
            std::string final_name;
            LLAvatarName av_name;
            if (LLAvatarNameCache::get(hover_object->getID(), &av_name))
            {
                final_name = av_name.getCompleteName();
            }
            else
            {
                final_name = LLTrans::getString("TooltipPerson");;
            }

            const F32 INSPECTOR_TOOLTIP_DELAY = 0.35f;

            LLInspector::Params p;
            p.fillFrom(LLUICtrlFactory::instance().getDefaultParams<LLInspector>());
            p.message(final_name);
            p.image.name("Inspector_I");
            p.click_callback(boost::bind(showAvatarInspector, hover_object->getID()));
            p.visible_time_near(6.f);
            p.visible_time_far(3.f);
            p.delay_time(INSPECTOR_TOOLTIP_DELAY);
            p.wrap(false);

            LLToolTipMgr::instance().show(p);
        }
    }
    else
    {
        //
        //  We have hit a regular object (not an avatar or attachment)
        //

        //
        //  Default prefs will suppress display unless the object is interactive
        //
        bool show_all_object_tips =
        (bool)gSavedSettings.getBOOL("ShowAllObjectHoverTip");
        LLSelectNode *nodep = LLSelectMgr::getInstance()->getHoverNode();

        // only show tooltip if same inspector not already open
        LLFloater* existing_inspector = LLFloaterReg::findInstance("inspect_object");
        if (nodep &&
            (!existing_inspector
             || !existing_inspector->getVisible()
             || existing_inspector->getKey()["object_id"].asUUID() != hover_object->getID()))
        {

            // Add price to tooltip for items on sale
            bool for_sale = for_sale_selection(nodep);
            if(for_sale)
            {
                LLStringUtil::format_map_t args;
                S32 price = nodep->mSaleInfo.getSalePrice();
                args["[AMOUNT]"] = LLResMgr::getInstance()->getMonetaryString(price);
                tooltip_msg.append(LLTrans::getString("TooltipPrice", args) );
            }

            if (nodep->mName.empty())
            {
                tooltip_msg.append(LLTrans::getString("TooltipNoName"));
            }
            else
            {
                tooltip_msg.append( nodep->mName );
            }

            bool has_media = false;
            bool is_time_based_media = false;
            bool is_web_based_media = false;
            bool is_media_playing = false;
            bool is_media_displaying = false;

            // Does this face have media?
            const LLTextureEntry* tep = hover_object->getTE(mHoverPick.mObjectFace);

            if(tep)
            {
                has_media = tep->hasMedia();
                const LLMediaEntry* mep = has_media ? tep->getMediaData() : NULL;
                if (mep)
                {
                    viewer_media_t media_impl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mep->getMediaID());
                    LLPluginClassMedia* media_plugin = NULL;

                    if (media_impl.notNull() && (media_impl->hasMedia()))
                    {
                        is_media_displaying = true;
                        //LLStringUtil::format_map_t args;

                        media_plugin = media_impl->getMediaPlugin();
                        if(media_plugin)
                        {
                            if(media_plugin->pluginSupportsMediaTime())
                            {
                                is_time_based_media = true;
                                is_web_based_media = false;
                                //args["[CurrentURL]"] =  media_impl->getMediaURL();
                                is_media_playing = media_impl->isMediaPlaying();
                            }
                            else
                            {
                                is_time_based_media = false;
                                is_web_based_media = true;
                                //args["[CurrentURL]"] =  media_plugin->getLocation();
                            }
                            //tooltip_msg.append(LLTrans::getString("CurrentURL", args));
                        }
                    }
                }
            }


            // Avoid showing tip over media that's displaying unless it's for sale
            // also check the primary node since sometimes it can have an action even though
            // the root node doesn't

            bool needs_tip = (!is_media_displaying ||
                              for_sale) &&
                (has_media ||
                 needs_tooltip(nodep) ||
                 needs_tooltip(LLSelectMgr::getInstance()->getPrimaryHoverNode()));

            if (show_all_object_tips || needs_tip)
            {
                LLInspector::Params p;
                p.fillFrom(LLUICtrlFactory::instance().getDefaultParams<LLInspector>());
                p.message(tooltip_msg);
                p.image.name("Inspector_I");
                p.click_callback(boost::bind(showObjectInspector, hover_object->getID(), mHoverPick.mObjectFace));
                p.time_based_media(is_time_based_media);
                p.web_based_media(is_web_based_media);
                p.media_playing(is_media_playing);
                p.click_playmedia_callback(boost::bind(playCurrentMedia, mHoverPick));
                p.click_homepage_callback(boost::bind(VisitHomePage, mHoverPick));
                p.visible_time_near(6.f);
                p.visible_time_far(3.f);
                p.delay_time(gSavedSettings.getF32("ObjectInspectorTooltipDelay"));
                p.wrap(false);

                LLToolTipMgr::instance().show(p);
            }
        }
    }

    return true;
}

bool LLToolPie::handleToolTip(S32 local_x, S32 local_y, MASK mask)
{
    static LLCachedControl<bool> show_hover_tips(*LLUI::getInstance()->mSettingGroups["config"], "ShowHoverTips", true);
    if (!show_hover_tips) return true;
    if (!mHoverPick.isValid()) return true;

    LLViewerObject* hover_object = mHoverPick.getObject();

    // update hover object and hover parcel
    LLSelectMgr::getInstance()->setHoverObject(hover_object, mHoverPick.mObjectFace);


    std::string tooltip_msg;
    std::string line;

    if ( hover_object )
    {
        handleTooltipObject(hover_object, line, tooltip_msg  );
    }
    else if (mHoverPick.mPickType == LLPickInfo::PICK_LAND)
    {
        handleTooltipLand(line, tooltip_msg);
    }

    return true;
}

static void show_inspector(const char* inspector, const char* param, const LLUUID& source_id)
{
    LLSD params;
    params[param] = source_id;
    if (LLToolTipMgr::instance().toolTipVisible())
    {
        LLRect rect = LLToolTipMgr::instance().getToolTipRect();
        params["pos"]["x"] = rect.mLeft;
        params["pos"]["y"] = rect.mTop;
    }

    LLFloaterReg::showInstance(inspector, params);
}


static void show_inspector(const char* inspector,  LLSD& params)
{
    if (LLToolTipMgr::instance().toolTipVisible())
    {
        LLRect rect = LLToolTipMgr::instance().getToolTipRect();
        params["pos"]["x"] = rect.mLeft;
        params["pos"]["y"] = rect.mTop;
    }

    LLFloaterReg::showInstance(inspector, params);
}


// static
void LLToolPie::showAvatarInspector(const LLUUID& avatar_id)
{
    show_inspector("inspect_avatar", "avatar_id", avatar_id);
}

// static
void LLToolPie::showObjectInspector(const LLUUID& object_id)
{
    show_inspector("inspect_object", "object_id", object_id);
}


// static
void LLToolPie::showObjectInspector(const LLUUID& object_id, const S32& object_face)
{
    LLSD params;
    params["object_id"] = object_id;
    params["object_face"] = object_face;
    show_inspector("inspect_object", params);
}

// static
void LLToolPie::playCurrentMedia(const LLPickInfo& info)
{
    //FIXME: how do we handle object in different parcel than us?
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (!parcel) return;

    LLPointer<LLViewerObject> objectp = info.getObject();

    // Early out cases.  Must clear media hover.
    // did not hit an object or did not hit a valid face
    if ( objectp.isNull() ||
        info.mObjectFace < 0 ||
        info.mObjectFace >= objectp->getNumTEs() )
    {
        return;
    }

    // Does this face have media?
    const LLTextureEntry* tep = objectp->getTE(info.mObjectFace);
    if (!tep)
        return;

    const LLMediaEntry* mep = tep->hasMedia() ? tep->getMediaData() : NULL;
    if(!mep)
        return;

    //TODO: Can you Use it?

    LLPluginClassMedia* media_plugin = NULL;

    viewer_media_t media_impl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mep->getMediaID());

    if(media_impl.notNull() && media_impl->hasMedia())
    {
        media_plugin = media_impl->getMediaPlugin();
        if (media_plugin && media_plugin->pluginSupportsMediaTime())
        {
            if(media_impl->isMediaPlaying())
            {
                media_impl->pause();
            }
            else
            {
                media_impl->play();
            }
        }
    }


}

// static
void LLToolPie::VisitHomePage(const LLPickInfo& info)
{
    //FIXME: how do we handle object in different parcel than us?
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (!parcel) return;

    LLPointer<LLViewerObject> objectp = info.getObject();

    // Early out cases.  Must clear media hover.
    // did not hit an object or did not hit a valid face
    if ( objectp.isNull() ||
        info.mObjectFace < 0 ||
        info.mObjectFace >= objectp->getNumTEs() )
    {
        return;
    }

    // Does this face have media?
    const LLTextureEntry* tep = objectp->getTE(info.mObjectFace);
    if (!tep)
        return;

    const LLMediaEntry* mep = tep->hasMedia() ? tep->getMediaData() : NULL;
    if(!mep)
        return;

    //TODO: Can you Use it?

    LLPluginClassMedia* media_plugin = NULL;

    viewer_media_t media_impl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mep->getMediaID());

    if(media_impl.notNull() && media_impl->hasMedia())
    {
        media_plugin = media_impl->getMediaPlugin();

        if (media_plugin && !(media_plugin->pluginSupportsMediaTime()))
        {
            media_impl->navigateHome();
        }
    }
}

void LLToolPie::handleSelect()
{
    // tool is reselected when app gets focus, etc.
}

void LLToolPie::handleDeselect()
{
    if( hasMouseCapture() )
    {
        setMouseCapture( false );  // Calls onMouseCaptureLost() indirectly
    }
    // remove temporary selection for pie menu
    LLSelectMgr::getInstance()->setHoverObject(NULL);

    // Menu may be still up during transfer to different tool.
    // toolfocus and toolgrab should retain menu, they will clear it if needed
    MASK override_mask = gKeyboard ? gKeyboard->currentMask(true) : 0;
    if (gMenuHolder && (!gMenuHolder->getVisible() || (override_mask & (MASK_ALT | MASK_CONTROL)) == 0))
    {
        // in most cases menu is useless without correct selection, so either keep both or discard both
        gMenuHolder->hideMenus();
        LLSelectMgr::getInstance()->validateSelection();
    }
}

LLTool* LLToolPie::getOverrideTool(MASK mask)
{
    static LLCachedControl<bool> enable_grab(gSavedSettings, "EnableGrab");
    if (enable_grab)
    {
        if (mask == DEFAULT_GRAB_MASK)
        {
            return LLToolGrab::getInstance();
        }
        else if (mask == (MASK_CONTROL | MASK_SHIFT))
        {
            return LLToolGrab::getInstance();
        }
    }
    return LLTool::getOverrideTool(mask);
}

void LLToolPie::stopEditing()
{
    if( hasMouseCapture() )
    {
        setMouseCapture( false );  // Calls onMouseCaptureLost() indirectly
    }
}

void LLToolPie::onMouseCaptureLost()
{
    stopCameraSteering();
    mMouseButtonDown = false;
    handleMediaMouseUp();
}

void LLToolPie::stopCameraSteering()
{
    mMouseOutsideSlop = false;
}

bool LLToolPie::inCameraSteerMode()
{
    return mMouseButtonDown && mMouseOutsideSlop;
}

// true if x,y outside small box around start_x,start_y
bool LLToolPie::outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y)
{
    S32 dx = x - start_x;
    S32 dy = y - start_y;

    return (dx <= -2 || 2 <= dx || dy <= -2 || 2 <= dy);
}


void LLToolPie::render()
{
    return;
}

static void handle_click_action_play()
{
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (!parcel) return;

    LLViewerMediaImpl::EMediaStatus status = LLViewerParcelMedia::getInstance()->getStatus();
    switch(status)
    {
        case LLViewerMediaImpl::MEDIA_PLAYING:
            LLViewerParcelMedia::getInstance()->pause();
            break;

        case LLViewerMediaImpl::MEDIA_PAUSED:
            LLViewerParcelMedia::getInstance()->start();
            break;

        default:
            LLViewerParcelMedia::getInstance()->play(parcel);
            break;
    }
}

bool LLToolPie::handleMediaClick(const LLPickInfo& pick)
{
    //FIXME: how do we handle object in different parcel than us?
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    LLPointer<LLViewerObject> objectp = pick.getObject();


    if (!parcel ||
        objectp.isNull() ||
        pick.mObjectFace < 0 ||
        pick.mObjectFace >= objectp->getNumTEs())
    {
        LLViewerMediaFocus::getInstance()->clearFocus();

        return false;
    }

    // Does this face have media?
    const LLTextureEntry* tep = objectp->getTE(pick.mObjectFace);
    if (!tep)
        return false;

    LLMediaEntry* mep = (tep->hasMedia()) ? tep->getMediaData() : NULL;
    if (!mep)
        return false;

    viewer_media_t media_impl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mep->getMediaID());

    if (gSavedSettings.getBOOL("MediaOnAPrimUI"))
    {
        if (!LLViewerMediaFocus::getInstance()->isFocusedOnFace(pick.getObject(), pick.mObjectFace) || media_impl.isNull())
        {
            // It's okay to give this a null impl
            LLViewerMediaFocus::getInstance()->setFocusFace(pick.getObject(), pick.mObjectFace, media_impl, pick.mNormal);
        }
        else
        {
            // Make sure keyboard focus is set to the media focus object.
            gFocusMgr.setKeyboardFocus(LLViewerMediaFocus::getInstance());
            LLEditMenuHandler::gEditMenuHandler = LLViewerMediaFocus::instance().getFocusedMediaImpl();

            media_impl->mouseDown(pick.mUVCoords, gKeyboard->currentMask(true));
            mMediaMouseCaptureID = mep->getMediaID();
            setMouseCapture(true);  // This object will send a mouse-up to the media when it loses capture.
        }

        return true;
    }

    LLViewerMediaFocus::getInstance()->clearFocus();

    return false;
}

bool LLToolPie::handleMediaDblClick(const LLPickInfo& pick)
{
    //FIXME: how do we handle object in different parcel than us?
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    LLPointer<LLViewerObject> objectp = pick.getObject();


    if (!parcel ||
        objectp.isNull() ||
        pick.mObjectFace < 0 ||
        pick.mObjectFace >= objectp->getNumTEs())
    {
        LLViewerMediaFocus::getInstance()->clearFocus();

        return false;
    }

    // Does this face have media?
    const LLTextureEntry* tep = objectp->getTE(pick.mObjectFace);
    if (!tep)
        return false;

    LLMediaEntry* mep = (tep->hasMedia()) ? tep->getMediaData() : NULL;
    if (!mep)
        return false;

    viewer_media_t media_impl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mep->getMediaID());

    if (gSavedSettings.getBOOL("MediaOnAPrimUI"))
    {
        if (!LLViewerMediaFocus::getInstance()->isFocusedOnFace(pick.getObject(), pick.mObjectFace) || media_impl.isNull())
        {
            // It's okay to give this a null impl
            LLViewerMediaFocus::getInstance()->setFocusFace(pick.getObject(), pick.mObjectFace, media_impl, pick.mNormal);
        }
        else
        {
            // Make sure keyboard focus is set to the media focus object.
            gFocusMgr.setKeyboardFocus(LLViewerMediaFocus::getInstance());
            LLEditMenuHandler::gEditMenuHandler = LLViewerMediaFocus::instance().getFocusedMediaImpl();

            media_impl->mouseDoubleClick(pick.mUVCoords, gKeyboard->currentMask(true));
            mMediaMouseCaptureID = mep->getMediaID();
            setMouseCapture(true);  // This object will send a mouse-up to the media when it loses capture.
        }

        return true;
    }

    LLViewerMediaFocus::getInstance()->clearFocus();

    return false;
}

bool LLToolPie::handleMediaHover(const LLPickInfo& pick)
{
    //FIXME: how do we handle object in different parcel than us?
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (!parcel) return false;

    LLPointer<LLViewerObject> objectp = pick.getObject();

    // Early out cases.  Must clear media hover.
    // did not hit an object or did not hit a valid face
    if ( objectp.isNull() ||
        pick.mObjectFace < 0 ||
        pick.mObjectFace >= objectp->getNumTEs() )
    {
        LLViewerMediaFocus::getInstance()->clearHover();
        return false;
    }

    // Does this face have media?
    const LLTextureEntry* tep = objectp->getTE(pick.mObjectFace);
    if(!tep)
        return false;

    const LLMediaEntry* mep = tep->hasMedia() ? tep->getMediaData() : NULL;
    if (mep
        && gSavedSettings.getBOOL("MediaOnAPrimUI"))
    {
        viewer_media_t media_impl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mep->getMediaID());

        if(media_impl.notNull())
        {
            // Update media hover object
            if (!LLViewerMediaFocus::getInstance()->isHoveringOverFace(objectp, pick.mObjectFace))
            {
                LLViewerMediaFocus::getInstance()->setHoverFace(objectp, pick.mObjectFace, media_impl, pick.mNormal);
            }

            // If this is the focused media face, send mouse move events.
            if (LLViewerMediaFocus::getInstance()->isFocusedOnFace(objectp, pick.mObjectFace))
            {
                media_impl->mouseMove(pick.mUVCoords, gKeyboard->currentMask(true));
                gViewerWindow->setCursor(media_impl->getLastSetCursor());
            }
            else
            {
                // This is not the focused face -- set the default cursor.
                gViewerWindow->setCursor(UI_CURSOR_ARROW);
            }

            return true;
        }
    }

    // In all other cases, clear media hover.
    LLViewerMediaFocus::getInstance()->clearHover();

    return false;
}

bool LLToolPie::handleMediaMouseUp()
{
    bool result = false;
    if(mMediaMouseCaptureID.notNull())
    {
        // Face media needs to know the mouse went up.
        viewer_media_t media_impl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mMediaMouseCaptureID);
        if(media_impl)
        {
            // This will send a mouseUp event to the plugin using the last known mouse coordinate (from a mouseDown or mouseMove), which is what we want.
            media_impl->onMouseCaptureLost();
        }

        mMediaMouseCaptureID.setNull();

        result = true;
    }

    return result;
}

static void handle_click_action_open_media(LLPointer<LLViewerObject> objectp)
{
    //FIXME: how do we handle object in different parcel than us?
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (!parcel) return;

    // did we hit an object?
    if (objectp.isNull()) return;

    // did we hit a valid face on the object?
    S32 face = LLToolPie::getInstance()->getPick().mObjectFace;
    if( face < 0 || face >= objectp->getNumTEs() ) return;

    // is media playing on this face?
    if (LLViewerMedia::getInstance()->getMediaImplFromTextureID(objectp->getTE(face)->getID()) != NULL)
    {
        handle_click_action_play();
        return;
    }

    std::string media_url = std::string ( parcel->getMediaURL () );
    std::string media_type = std::string ( parcel->getMediaType() );
    LLStringUtil::trim(media_url);

    LLWeb::loadURL(media_url);
}

static ECursorType cursor_from_parcel_media(U8 click_action)
{
    // HACK: This is directly referencing an impl name.  BAD!
    // This can be removed when we have a truly generic media browser that only
    // builds an impl based on the type of url it is passed.

    //FIXME: how do we handle object in different parcel than us?
    ECursorType open_cursor = UI_CURSOR_ARROW;
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (!parcel) return open_cursor;

    std::string media_url = std::string ( parcel->getMediaURL () );
    std::string media_type = std::string ( parcel->getMediaType() );
    LLStringUtil::trim(media_url);

    open_cursor = UI_CURSOR_TOOLMEDIAOPEN;

    LLViewerMediaImpl::EMediaStatus status = LLViewerParcelMedia::getInstance()->getStatus();
    switch(status)
    {
        case LLViewerMediaImpl::MEDIA_PLAYING:
            return click_action == CLICK_ACTION_PLAY ? UI_CURSOR_TOOLPAUSE : open_cursor;
        default:
            return UI_CURSOR_TOOLPLAY;
    }
}


// True if we handled the event.
bool LLToolPie::handleRightClickPick()
{
    S32 x = mPick.mMousePt.mX;
    S32 y = mPick.mMousePt.mY;
    MASK mask = mPick.mKeyMask;

    if (mPick.mPickType != LLPickInfo::PICK_LAND)
    {
        LLViewerParcelMgr::getInstance()->deselectLand();
    }

    // didn't click in any UI object, so must have clicked in the world
    LLViewerObject *object = mPick.getObject();

    // Can't ignore children here.
    LLToolSelect::handleObjectSelection(mPick, false, true);

    // Spawn pie menu
    if (mPick.mPickType == LLPickInfo::PICK_LAND)
    {
        LLParcelSelectionHandle selection = LLViewerParcelMgr::getInstance()->selectParcelAt( mPick.mPosGlobal );
        gMenuHolder->setParcelSelection(selection);
        gMenuLand->show(x, y);

        showVisualContextMenuEffect();

    }
    else if (mPick.mObjectID == gAgent.getID() )
    {
        if(!gMenuAvatarSelf)
        {
            //either at very early startup stage or at late quitting stage,
            //this event is ignored.
            return true ;
        }

        gMenuAvatarSelf->show(x, y);
    }
    else if (object)
    {
        gMenuHolder->setObjectSelection(LLSelectMgr::getInstance()->getSelection());

        bool is_other_attachment = (object->isAttachment() && !object->isHUDAttachment() && !object->permYouOwner());
        if (object->isAvatar() || is_other_attachment)
        {
            // Find the attachment's avatar
            while( object && object->isAttachment())
            {
                object = (LLViewerObject*)object->getParent();
                llassert(object);
            }

            if (!object)
            {
                return true; // unexpected, but escape
            }

            // Object is an avatar, so check for mute by id.
            LLVOAvatar* avatar = (LLVOAvatar*)object;
            std::string name = avatar->getFullname();
            std::string mute_msg;
            if (LLMuteList::getInstance()->isMuted(avatar->getID(), avatar->getFullname()))
            {
                mute_msg = LLTrans::getString("UnmuteAvatar");
            }
            else
            {
                mute_msg = LLTrans::getString("MuteAvatar");
            }

            if (is_other_attachment)
            {
                gMenuAttachmentOther->getChild<LLUICtrl>("Avatar Mute")->setValue(mute_msg);
                gMenuAttachmentOther->show(x, y);
            }
            else
            {
                gMenuAvatarOther->getChild<LLUICtrl>("Avatar Mute")->setValue(mute_msg);
                gMenuAvatarOther->show(x, y);
            }
        }
        else if (object->isAttachment())
        {
            gMenuAttachmentSelf->show(x, y);
        }
        else
        {
            // BUG: What about chatting child objects?
            std::string name;
            LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
            if (node)
            {
                name = node->mName;
            }

            gMenuObject->show(x, y);

            showVisualContextMenuEffect();
        }
    }
    else if (mPick.mParticleOwnerID.notNull())
    {
        if (gMenuMuteParticle && mPick.mParticleOwnerID != gAgent.getID())
        {
            gMenuMuteParticle->show(x,y);
        }
    }

    // non UI object - put focus back "in world"
    if (gFocusMgr.getKeyboardFocus())
    {
        gFocusMgr.setKeyboardFocus(NULL);
    }

    LLTool::handleRightMouseDown(x, y, mask);
    // We handled the event.
    return true;
}

void LLToolPie::showVisualContextMenuEffect()
{
    // VEFFECT: ShowPie
    LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_SPHERE, true);
    effectp->setPositionGlobal(mPick.mPosGlobal);
    effectp->setColor(LLColor4U(gAgent.getEffectColor()));
    effectp->setDuration(0.25f);
}

typedef enum e_near_far
{
    NEAR_INTERSECTION,
    FAR_INTERSECTION
} ENearFar;

bool intersect_ray_with_sphere( const LLVector3& ray_pt, const LLVector3& ray_dir, const LLVector3& sphere_center, F32 sphere_radius, e_near_far near_far, LLVector3& intersection_pt)
{
    // do ray/sphere intersection by solving quadratic equation
    LLVector3 sphere_to_ray_start_vec = ray_pt - sphere_center;
    F32 B = 2.f * ray_dir * sphere_to_ray_start_vec;
    F32 C = sphere_to_ray_start_vec.lengthSquared() - (sphere_radius * sphere_radius);

    F32 discriminant = B*B - 4.f*C;
    if (discriminant >= 0.f)
    {   // intersection detected, now find closest one
        F32 t0 = (-B - sqrtf(discriminant)) / 2.f;

        if (t0 > 0.f && near_far == NEAR_INTERSECTION)
        {
            intersection_pt = ray_pt + ray_dir * t0;
        }
        else
        {
            F32 t1 = (-B + sqrtf(discriminant)) / 2.f;
            intersection_pt = ray_pt + ray_dir * t1;
        }
        return true;
    }
    else
    {   // no intersection
        return false;
    }
}

void LLToolPie::startCameraSteering()
{
    LLFirstUse::notMoving(false);
    mMouseOutsideSlop = true;

    if (gAgentCamera.getFocusOnAvatar())
    {
        mSteerPick = mPick;

        // handle special cases of steering picks
        LLViewerObject* avatar_object = mSteerPick.getObject();

        // get pointer to avatar
        while (avatar_object && !avatar_object->isAvatar())
        {
            avatar_object = (LLViewerObject*)avatar_object->getParent();
        }

        // if clicking on own avatar...
        if (avatar_object && ((LLVOAvatar*)avatar_object)->isSelf())
        {
            // ...project pick point a few meters in front of avatar
            mSteerPick.mPosGlobal = gAgent.getPositionGlobal() + LLVector3d(LLViewerCamera::instance().getAtAxis()) * 3.0;
        }

        if (!mSteerPick.isValid())
        {
            mSteerPick.mPosGlobal = gAgent.getPosGlobalFromAgent(
                LLViewerCamera::instance().getOrigin() + gViewerWindow->mouseDirectionGlobal(mSteerPick.mMousePt.mX, mSteerPick.mMousePt.mY) * 100.f);
        }

        setMouseCapture(true);

        mMouseSteerX = mMouseDownX;
        mMouseSteerY = mMouseDownY;
        const LLVector3 camera_to_rotation_center   = gAgent.getFrameAgent().getOrigin() - LLViewerCamera::instance().getOrigin();
        const LLVector3 rotation_center_to_pick     = gAgent.getPosAgentFromGlobal(mSteerPick.mPosGlobal) - gAgent.getFrameAgent().getOrigin();

        mClockwise = camera_to_rotation_center * rotation_center_to_pick < 0.f;
        if (mMouseSteerGrabPoint) { mMouseSteerGrabPoint->markDead(); }
        mMouseSteerGrabPoint = (LLHUDEffectBlob *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BLOB, false);
        mMouseSteerGrabPoint->setPositionGlobal(mSteerPick.mPosGlobal);
        mMouseSteerGrabPoint->setColor(LLColor4U(170, 210, 190));
        mMouseSteerGrabPoint->setPixelSize(5);
        mMouseSteerGrabPoint->setDuration(2.f);
    }
}

void LLToolPie::steerCameraWithMouse(S32 x, S32 y)
{
    const LLViewerCamera& camera = LLViewerCamera::instance();
    const LLCoordFrame& rotation_frame = gAgent.getFrameAgent();
    const LLVector3 pick_pos = gAgent.getPosAgentFromGlobal(mSteerPick.mPosGlobal);
    const LLVector3 pick_rotation_center = rotation_frame.getOrigin() + parallel_component(pick_pos - rotation_frame.getOrigin(), rotation_frame.getUpAxis());
    const F32 MIN_ROTATION_RADIUS_FRACTION = 0.2f;
    const F32 min_rotation_radius = MIN_ROTATION_RADIUS_FRACTION * dist_vec(pick_rotation_center, camera.getOrigin());;
    const F32 pick_distance_from_rotation_center = llclamp(dist_vec(pick_pos, pick_rotation_center), min_rotation_radius, F32_MAX);
    const LLVector3 camera_to_rotation_center = pick_rotation_center - camera.getOrigin();
    const LLVector3 adjusted_camera_pos = LLViewerCamera::instance().getOrigin() + projected_vec(camera_to_rotation_center, rotation_frame.getUpAxis());
    const F32 camera_distance_from_rotation_center = dist_vec(adjusted_camera_pos, pick_rotation_center);

    LLVector3 mouse_ray = orthogonal_component(gViewerWindow->mouseDirectionGlobal(x, y), rotation_frame.getUpAxis());
    mouse_ray.normalize();

    LLVector3 old_mouse_ray = orthogonal_component(gViewerWindow->mouseDirectionGlobal(mMouseSteerX, mMouseSteerY), rotation_frame.getUpAxis());
    old_mouse_ray.normalize();

    F32 yaw_angle;
    F32 old_yaw_angle;
    LLVector3 mouse_on_sphere;
    LLVector3 old_mouse_on_sphere;

    if (intersect_ray_with_sphere(
            adjusted_camera_pos,
            mouse_ray,
            pick_rotation_center,
            pick_distance_from_rotation_center,
            FAR_INTERSECTION,
            mouse_on_sphere))
    {
        LLVector3 mouse_sphere_offset = mouse_on_sphere - pick_rotation_center;
        yaw_angle = atan2f(mouse_sphere_offset * rotation_frame.getLeftAxis(), mouse_sphere_offset * rotation_frame.getAtAxis());
    }
    else
    {
        yaw_angle = F_PI_BY_TWO + asinf(pick_distance_from_rotation_center / camera_distance_from_rotation_center);
        if (mouse_ray * rotation_frame.getLeftAxis() < 0.f)
        {
            yaw_angle *= -1.f;
        }
    }

    if (intersect_ray_with_sphere(
            adjusted_camera_pos,
            old_mouse_ray,
            pick_rotation_center,
            pick_distance_from_rotation_center,
            FAR_INTERSECTION,
            old_mouse_on_sphere))
    {
        LLVector3 mouse_sphere_offset = old_mouse_on_sphere - pick_rotation_center;
        old_yaw_angle = atan2f(mouse_sphere_offset * rotation_frame.getLeftAxis(), mouse_sphere_offset * rotation_frame.getAtAxis());
    }
    else
    {
        old_yaw_angle = F_PI_BY_TWO + asinf(pick_distance_from_rotation_center / camera_distance_from_rotation_center);

        if (mouse_ray * rotation_frame.getLeftAxis() < 0.f)
        {
            old_yaw_angle *= -1.f;
        }
    }

    const F32 delta_angle = yaw_angle - old_yaw_angle;

    if (mClockwise)
    {
        gAgent.yaw(delta_angle);
    }
    else
    {
        gAgent.yaw(-delta_angle);
    }

    mMouseSteerX = x;
    mMouseSteerY = y;
}
