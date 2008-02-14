/** 
 * @file lltoolpie.cpp
 * @brief LLToolPie class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#include "lltoolpie.h"

#include "indra_constants.h"
#include "llclickaction.h"
#include "llmediabase.h"	// for status codes
#include "llparcel.h"

#include "llagent.h"
#include "llviewercontrol.h"
#include "llfirstuse.h"
#include "llfloateravatarinfo.h"
#include "llfloaterland.h"
#include "llfloaterscriptdebug.h"
#include "llhoverview.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llmenugl.h"
#include "llmutelist.h"
#include "llselectmgr.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lltoolselect.h"
#include "llviewercamera.h"
#include "llviewerparcelmedia.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llviewermedia.h"
#include "llvoavatar.h"
#include "llworld.h"
#include "llui.h"
#include "llweb.h"

LLToolPie *gToolPie = NULL;

LLPointer<LLViewerObject> LLToolPie::sClickActionObject;
LLHandle<LLObjectSelection> LLToolPie::sLeftClickSelection = NULL;
U8 LLToolPie::sClickAction = 0;

extern void handle_buy(void*);

extern BOOL gDebugClicks;

static void handle_click_action_play();
static void handle_click_action_open_media(LLPointer<LLViewerObject> objectp);
static ECursorType cursor_from_parcel_media(U8 click_action);


LLToolPie::LLToolPie()
:	LLTool("Select"),
	mPieMouseButtonDown( FALSE ),
	mGrabMouseButtonDown( FALSE ),
	mHitLand( FALSE ),
	mHitObjectID(),
	mMouseOutsideSlop( FALSE )
{ }


BOOL LLToolPie::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (!gCamera) return FALSE;

	gPickFaces = TRUE;
	//left mouse down always picks transparent
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, leftMouseCallback, 
											  TRUE, TRUE);
	mGrabMouseButtonDown = TRUE;
	return TRUE;
}

// static
void LLToolPie::leftMouseCallback(S32 x, S32 y, MASK mask)
{
	gToolPie->pickAndShowMenu(x, y, mask, FALSE);
}

BOOL LLToolPie::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	// Pick faces in case they select "Copy Texture" and need that info.
	gPickFaces = TRUE;
	// don't pick transparent so users can't "pay" transparent objects
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, rightMouseCallback,
											  FALSE, TRUE);
	mPieMouseButtonDown = TRUE; 
	// don't steal focus from UI
	return FALSE;
}

// static
void LLToolPie::rightMouseCallback(S32 x, S32 y, MASK mask)
{
	gToolPie->pickAndShowMenu(x, y, mask, TRUE);
}

// True if you selected an object.
BOOL LLToolPie::pickAndShowMenu(S32 x, S32 y, MASK mask, BOOL always_show)
{
	if (!always_show && gLastHitParcelWall)
	{
		LLParcel* parcel = gParcelMgr->getCollisionParcel();
		if (parcel)
		{
			gParcelMgr->selectCollisionParcel();
			if (parcel->getParcelFlag(PF_USE_PASS_LIST) 
				&& !gParcelMgr->isCollisionBanned())
			{
				// if selling passes, just buy one
				void* deselect_when_done = (void*)TRUE;
				LLPanelLandGeneral::onClickBuyPass(deselect_when_done);
			}
			else
			{
				// not selling passes, get info
				LLFloaterLand::showInstance();
			}
		}

		return LLTool::handleMouseDown(x, y, mask);
	}

	// didn't click in any UI object, so must have clicked in the world
	LLViewerObject *object = gViewerWindow->lastObjectHit();
	LLViewerObject *parent = NULL;

	mHitLand = !object && !gLastHitPosGlobal.isExactlyZero();
	if (!mHitLand)
	{
		gParcelMgr->deselectLand();
	}
	
	if (object)
	{
		mHitObjectID = object->mID;

		parent = object->getRootEdit();
	}
	else
	{
		mHitObjectID.setNull();
	}

	BOOL touchable = (object && object->flagHandleTouch()) 
					 || (parent && parent->flagHandleTouch());

	// If it's a left-click, and we have a special action, do it.
	if (useClickAction(always_show, mask, object, parent))
	{
		sClickAction = 0;
		if (object && object->getClickAction()) 
		{
			sClickAction = object->getClickAction();
		}
		else if (parent && parent->getClickAction()) 
		{
			sClickAction = parent->getClickAction();
		}

		switch(sClickAction)
		{
		case CLICK_ACTION_TOUCH:
		default:
			// nothing
			break;
		case CLICK_ACTION_SIT:
			handle_sit_or_stand();
			return TRUE;
		case CLICK_ACTION_PAY:
			if (object && object->flagTakesMoney()
				|| parent && parent->flagTakesMoney())
			{
				// pay event goes to object actually clicked on
				sClickActionObject = object;
				sLeftClickSelection = LLToolSelect::handleObjectSelection(object, MASK_NONE, FALSE, TRUE);
				return TRUE;
			}
			break;
		case CLICK_ACTION_BUY:
			sClickActionObject = parent;
			sLeftClickSelection = LLToolSelect::handleObjectSelection(parent, MASK_NONE, FALSE, TRUE);
			return TRUE;
		case CLICK_ACTION_OPEN:
			if (parent && parent->allowOpen())
			{
				sClickActionObject = parent;
				sLeftClickSelection = LLToolSelect::handleObjectSelection(parent, MASK_NONE, FALSE, TRUE);
			}
			return TRUE;
		case CLICK_ACTION_PLAY:
			handle_click_action_play();
			return TRUE;
		case CLICK_ACTION_OPEN_MEDIA:
			// sClickActionObject = object;
			handle_click_action_open_media(object);
			return TRUE;
		}
	}

	// Switch to grab tool if physical or triggerable
	if (object && 
		!object->isAvatar() && 
		((object->usePhysics() || (parent && !parent->isAvatar() && parent->usePhysics())) || touchable) && 
		!always_show)
	{
		gGrabTransientTool = this;
		gToolMgr->getCurrentToolset()->selectTool( gToolGrab );
		return gToolGrab->handleObjectHit( object, x, y, mask);
	}
	
	if (!object && gLastHitHUDIcon && gLastHitHUDIcon->getSourceObject())
	{
		LLFloaterScriptDebug::show(gLastHitHUDIcon->getSourceObject()->getID());
	}

	// If left-click never selects or spawns a menu
	// Eat the event.
	if (!gSavedSettings.getBOOL("LeftClickShowMenu")
		&& !always_show)
	{
		// mouse already released
		if (!mGrabMouseButtonDown)
		{
			return TRUE;
		}

		while( object && object->isAttachment() && !object->flagHandleTouch())
		{
			// don't pick avatar through hud attachment
			if (object->isHUDAttachment())
			{
				break;
			}
			object = (LLViewerObject*)object->getParent();
		}
		if (object && object == gAgent.getAvatarObject())
		{
			// we left clicked on avatar, switch to focus mode
			gToolMgr->setTransientTool(gToolCamera);
			gViewerWindow->hideCursor();
			gToolCamera->setMouseCapture(TRUE);
			gToolCamera->pickCallback(gViewerWindow->getCurrentMouseX(), gViewerWindow->getCurrentMouseY(), mask);
			gAgent.setFocusOnAvatar(TRUE, TRUE);

			return TRUE;
		}
		// Could be first left-click on nothing
		LLFirstUse::useLeftClickNoHit();

		// Eat the event
		return LLTool::handleMouseDown(x, y, mask);
	}

	if (!always_show && gAgent.leftButtonGrabbed())
	{
		// if the left button is grabbed, don't put up the pie menu
		return LLTool::handleMouseDown(x, y, mask);
	}

	// Can't ignore children here.
	LLToolSelect::handleObjectSelection(object, mask, FALSE, TRUE);

	// Spawn pie menu
	if (mHitLand)
	{
		LLParcelSelectionHandle selection = gParcelMgr->selectParcelAt( gLastHitPosGlobal );
		gMenuHolder->setParcelSelection(selection);
		gPieLand->show(x, y, mPieMouseButtonDown);

		// VEFFECT: ShowPie
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_SPHERE, TRUE);
		effectp->setPositionGlobal(gLastHitPosGlobal);
		effectp->setColor(LLColor4U(gAgent.getEffectColor()));
		effectp->setDuration(0.25f);
	}
	else if (mHitObjectID == gAgent.getID() )
	{
		gPieSelf->show(x, y, mPieMouseButtonDown);
	}
	else if (object)
	{
		gMenuHolder->setObjectSelection(gSelectMgr->getSelection());

		if (object->isAvatar() 
			|| (object->isAttachment() && !object->isHUDAttachment() && !object->permYouOwner()))
		{
			// Find the attachment's avatar
			while( object && object->isAttachment())
			{
				object = (LLViewerObject*)object->getParent();
			}

			// Object is an avatar, so check for mute by id.
			LLVOAvatar* avatar = (LLVOAvatar*)object;
			LLString name = avatar->getFullname();
			if (gMuteListp->isMuted(avatar->getID(), name))
			{
				gMenuHolder->childSetText("Avatar Mute", LLString("Unmute")); // *TODO:Translate
				//gMutePieMenu->setLabel("Unmute");
			}
			else
			{
				gMenuHolder->childSetText("Avatar Mute", LLString("Mute")); // *TODO:Translate
				//gMutePieMenu->setLabel("Mute");
			}

			gPieAvatar->show(x, y, mPieMouseButtonDown);
		}
		else if (object->isAttachment())
		{
			gPieAttachment->show(x, y, mPieMouseButtonDown);
		}
		else
		{
			// BUG: What about chatting child objects?
			LLString name;
			LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
			if (node)
			{
				name = node->mName;
			}
			if (gMuteListp->isMuted(object->getID(), name))
			{
				gMenuHolder->childSetText("Object Mute", LLString("Unmute")); // *TODO:Translate
				//gMuteObjectPieMenu->setLabel("Unmute");
			}
			else
			{
				gMenuHolder->childSetText("Object Mute", LLString("Mute")); // *TODO:Translate
				//gMuteObjectPieMenu->setLabel("Mute");
			}
			
			gPieObject->show(x, y, mPieMouseButtonDown);

			// VEFFECT: ShowPie object
			// Don't show when you click on someone else, it freaks them
			// out.
			LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_SPHERE, TRUE);
			effectp->setPositionGlobal(gLastHitPosGlobal);
			effectp->setColor(LLColor4U(gAgent.getEffectColor()));
			effectp->setDuration(0.25f);
		}
	}

	if (always_show)
	{
		// ignore return value
		LLTool::handleRightMouseDown(x, y, mask);
	}
	else
	{
		// ignore return value
		LLTool::handleMouseDown(x, y, mask);
	}

	// We handled the event.
	return TRUE;
}

BOOL LLToolPie::useClickAction(BOOL always_show, 
							   MASK mask, 
							   LLViewerObject* object, 
							   LLViewerObject* parent)
{
	return	!always_show
			&& mask == MASK_NONE
			&& object
			&& !object->isAttachment() 
			&& LLPrimitive::isPrimitive(object->getPCode())
			&& (object->getClickAction() 
				|| parent->getClickAction());

}

U8 final_click_action(LLViewerObject* obj)
{
	if (!obj) return CLICK_ACTION_NONE;
	if (obj->isAttachment()) return CLICK_ACTION_NONE;

	U8 click_action = CLICK_ACTION_TOUCH;
	LLViewerObject* parent = obj->getRootEdit();
	if (obj->getClickAction()
	    || (parent && parent->getClickAction()))
	{
		if (obj->getClickAction())
		{
			click_action = obj->getClickAction();
		}
		else if (parent && parent->getClickAction())
		{
			click_action = parent->getClickAction();
		}
	}
	return click_action;
}

ECursorType cursor_from_object(LLViewerObject* object)
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
		cursor = UI_CURSOR_TOOLSIT;
		break;
	case CLICK_ACTION_BUY:
		cursor = UI_CURSOR_TOOLBUY;
		break;
	case CLICK_ACTION_OPEN:
		// Open always opens the parent.
		if (parent && parent->allowOpen())
		{
			cursor = UI_CURSOR_TOOLOPEN;
		}
		break;
	case CLICK_ACTION_PAY:	
		if ((object && object->flagTakesMoney())
			|| (parent && parent->flagTakesMoney()))
		{
			cursor = UI_CURSOR_TOOLPAY;
		}
		break;
	case CLICK_ACTION_PLAY:
	case CLICK_ACTION_OPEN_MEDIA: 
		cursor = cursor_from_parcel_media(click_action);
		break;
	default:
		break;
	}
	return cursor;
}

// When we get object properties after left-clicking on an object
// with left-click = buy, if it's the same object, do the buy.
// static
void LLToolPie::selectionPropertiesReceived()
{
	// Make sure all data has been received.
	// This function will be called repeatedly as the data comes in.
	if (!gSelectMgr->selectGetAllValid())
	{
		return;
	}

	if (!sLeftClickSelection->isEmpty())
	{
		LLViewerObject* selected_object = sLeftClickSelection->getPrimaryObject();
		// since we don't currently have a way to lock a selection, it could have changed
		// after we initially clicked on the object
		if (selected_object == sClickActionObject)
		{
			switch (sClickAction)
			{
			case CLICK_ACTION_BUY:
				handle_buy(NULL);
				break;
			case CLICK_ACTION_PAY:
				handle_give_money_dialog();
				break;
			case CLICK_ACTION_OPEN:
				handle_object_open();
				break;
			default:
				break;
			}
		}
	}
	sLeftClickSelection = NULL;
	sClickActionObject = NULL;
	sClickAction = 0;
}

BOOL LLToolPie::handleHover(S32 x, S32 y, MASK mask)
{
		/*
	// If auto-rotate occurs, tag mouse-outside-slop to make sure the drag
	// gets started.
	const S32 ROTATE_H_MARGIN = (S32) (0.1f * gViewerWindow->getWindowWidth() );
	const F32 ROTATE_ANGLE_PER_SECOND = 30.f * DEG_TO_RAD;
	const F32 rotate_angle = ROTATE_ANGLE_PER_SECOND / gFPSClamped;
	// ...normal modes can only yaw
	if (x < ROTATE_H_MARGIN)
	{
		gAgent.yaw(rotate_angle);
		mMouseOutsideSlop = TRUE;
	}
	else if (x > gViewerWindow->getWindowWidth() - ROTATE_H_MARGIN)
	{
		gAgent.yaw(-rotate_angle);
		mMouseOutsideSlop = TRUE;
	}
	*/
	
	LLViewerObject *object = NULL;
	LLViewerObject *parent = NULL;
	if (gHoverView)
	{
		object = gHoverView->getLastHoverObject();
	}

	if (object)
	{
		parent = object->getRootEdit();
	}

	if (object && useClickAction(FALSE, mask, object, parent))
	{
		ECursorType cursor = cursor_from_object(object);
		gViewerWindow->getWindow()->setCursor(cursor);
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolPie (inactive)" << llendl;
	}
	else if ((object && !object->isAvatar() && object->usePhysics()) 
			 || (parent && !parent->isAvatar() && parent->usePhysics()))
	{
		gViewerWindow->getWindow()->setCursor(UI_CURSOR_TOOLGRAB);
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolPie (inactive)" << llendl;
	}
	else if ( (object && object->flagHandleTouch()) 
			  || (parent && parent->flagHandleTouch()))
	{
		gViewerWindow->getWindow()->setCursor(UI_CURSOR_HAND);
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolPie (inactive)" << llendl;
	}
	else
	{
		gViewerWindow->getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolPie (inactive)" << llendl;
	}

	return TRUE;
}

BOOL LLToolPie::handleMouseUp(S32 x, S32 y, MASK mask)
{
	LLViewerObject* obj = gViewerWindow->lastObjectHit();
	U8 click_action = final_click_action(obj);
	if (click_action != CLICK_ACTION_NONE)
	{
		switch(click_action)
		{
		case CLICK_ACTION_BUY:
		case CLICK_ACTION_PAY:
		case CLICK_ACTION_OPEN:
			// Because these actions open UI dialogs, we won't change
			// the cursor again until the next hover and GL pick over
			// the world.  Keep the cursor an arrow, assuming that 
			// after the user moves off the UI, they won't be on the
			// same object anymore.
			gViewerWindow->getWindow()->setCursor(UI_CURSOR_ARROW);
			// Make sure the hover-picked object is ignored.
			gHoverView->resetLastHoverObject();
			break;
		default:
			break;
		}
	}
	mGrabMouseButtonDown = FALSE;
	gToolMgr->clearTransientTool();
	gAgent.setLookAt(LOOKAT_TARGET_CONVERSATION, obj); // maybe look at object/person clicked on
	return LLTool::handleMouseUp(x, y, mask);
}

BOOL LLToolPie::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	mPieMouseButtonDown = FALSE; 
	gToolMgr->clearTransientTool();
	return LLTool::handleRightMouseUp(x, y, mask);
}


BOOL LLToolPie::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (gDebugClicks)
	{
		llinfos << "LLToolPie handleDoubleClick (becoming mouseDown)" << llendl;
	}

	if (gSavedSettings.getBOOL("DoubleClickAutoPilot"))
	{
		if (gLastHitLand
			&& !gLastHitPosGlobal.isExactlyZero())
		{
			handle_go_to();
			return TRUE;
		}
		else if (gLastHitObjectID.notNull()
				 && !gLastHitPosGlobal.isExactlyZero())
		{
			// Hit an object
			// HACK: Call the last hit position the point we hit on the object
			gLastHitPosGlobal += gLastHitObjectOffset;
			handle_go_to();
			return TRUE;
		}
	}

	return FALSE;

	/* JC - don't do go-there, because then double-clicking on physical
	objects gets you into trouble.

	// If double-click on object or land, go there.
	LLViewerObject *object = gViewerWindow->lastObjectHit();
	if (object)
	{
		if (object->isAvatar())
		{
			LLFloaterAvatarInfo::showFromAvatar(object->getID());
		}
		else
		{
			handle_go_to(NULL);
		}
	}
	else if (!gLastHitPosGlobal.isExactlyZero())
	{
		handle_go_to(NULL);
	}

	return TRUE;
	*/
}


void LLToolPie::handleDeselect()
{
	if(	hasMouseCapture() )
	{
		setMouseCapture( FALSE );  // Calls onMouseCaptureLost() indirectly
	}
	// remove temporary selection for pie menu
	gSelectMgr->validateSelection();
}

LLTool* LLToolPie::getOverrideTool(MASK mask)
{
	if (mask == MASK_CONTROL)
	{
		return gToolGrab;
	}
	else if (mask == (MASK_CONTROL | MASK_SHIFT))
	{
		return gToolGrab;
	}

	return LLTool::getOverrideTool(mask);
}

void LLToolPie::stopEditing()
{
	if(	hasMouseCapture() )
	{
		setMouseCapture( FALSE );  // Calls onMouseCaptureLost() indirectly
	}
}

void LLToolPie::onMouseCaptureLost()
{
	mMouseOutsideSlop = FALSE;
}


// true if x,y outside small box around start_x,start_y
BOOL LLToolPie::outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y)
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
	LLParcel* parcel = gParcelMgr->getAgentParcel();
	if (!parcel) return;

	LLMediaBase::EStatus status = LLViewerParcelMedia::getStatus();
	switch(status)
	{
		case LLMediaBase::STATUS_STARTED:
			LLViewerParcelMedia::pause();
			break;

		case LLMediaBase::STATUS_PAUSED:
			LLViewerParcelMedia::start();
			break;

		default:
			LLViewerParcelMedia::play(parcel);
			break;
	}
}

static void handle_click_action_open_media(LLPointer<LLViewerObject> objectp)
{
	//FIXME: how do we handle object in different parcel than us?
	LLParcel* parcel = gParcelMgr->getAgentParcel();
	if (!parcel) return;

	// did we hit an object?
	if (objectp.isNull()) return;

	// did we hit a valid face on the object?
	if( gLastHitObjectFace < 0 || gLastHitObjectFace >= objectp->getNumTEs() ) return;
		
	// is media playing on this face?
	if (!LLViewerMedia::isActiveMediaTexture(objectp->getTE(gLastHitObjectFace)->getID()))
	{
		handle_click_action_play();
		return;
	}

	std::string media_url = std::string ( parcel->getMediaURL () );
	std::string media_type = std::string ( parcel->getMediaType() );
	LLString::trim(media_url);

	// Get the scheme, see if that is handled as well.
	LLURI uri(media_url);
	std::string media_scheme = uri.scheme() != "" ? uri.scheme() : "http";

	// HACK: This is directly referencing an impl name.  BAD!
	// This can be removed when we have a truly generic media browser that only 
	// builds an impl based on the type of url it is passed.

	if(	LLMediaManager::getInstance()->supportsMediaType( "LLMediaImplLLMozLib", media_scheme, media_type ) )
	{
		LLWeb::loadURL(media_url);
	}
}

static ECursorType cursor_from_parcel_media(U8 click_action)
{
	// HACK: This is directly referencing an impl name.  BAD!
	// This can be removed when we have a truly generic media browser that only 
	// builds an impl based on the type of url it is passed.
	
	//FIXME: how do we handle object in different parcel than us?
	ECursorType open_cursor = UI_CURSOR_ARROW;
	LLParcel* parcel = gParcelMgr->getAgentParcel();
	if (!parcel) return open_cursor;

	std::string media_url = std::string ( parcel->getMediaURL () );
	std::string media_type = std::string ( parcel->getMediaType() );
	LLString::trim(media_url);

	// Get the scheme, see if that is handled as well.
	LLURI uri(media_url);
	std::string media_scheme = uri.scheme() != "" ? uri.scheme() : "http";

	if(	LLMediaManager::getInstance()->supportsMediaType( "LLMediaImplLLMozLib", media_scheme, media_type ) )
	{
		open_cursor = UI_CURSOR_TOOLMEDIAOPEN;
	}

	LLMediaBase::EStatus status = LLViewerParcelMedia::getStatus();
	switch(status)
	{
		case LLMediaBase::STATUS_STARTED:
			return click_action == CLICK_ACTION_PLAY ? UI_CURSOR_TOOLPAUSE : open_cursor;
		default:
			return UI_CURSOR_TOOLPLAY;
	}
}
