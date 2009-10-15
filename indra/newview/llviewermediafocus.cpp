/** 
 * @file llviewermediafocus.cpp
 * @brief Governs focus on Media prims
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#include "llviewermediafocus.h"

//LLViewerMediaFocus
#include "llviewerobjectlist.h"
#include "llpanelmediahud.h"
#include "llpluginclassmedia.h"
#include "llagent.h"
#include "lltoolpie.h"
#include "llviewercamera.h"
#include "llviewermedia.h"
#include "llhudview.h"
#include "lluictrlfactory.h"
#include "lldrawable.h"
#include "llparcel.h"
#include "llviewerparcelmgr.h"
#include "llweb.h"
#include "llmediaentry.h"
//
// LLViewerMediaFocus
//

LLViewerMediaFocus::LLViewerMediaFocus()
: mMouseOverFlag(false)
{
}

LLViewerMediaFocus::~LLViewerMediaFocus()
{
	// The destructor for LLSingletons happens at atexit() time, which is too late to do much.
	// Clean up in cleanupClass() instead.
}

void LLViewerMediaFocus::cleanupClass()
{
	LLViewerMediaFocus *self = LLViewerMediaFocus::getInstance();
	
	if(self)
	{
		// mMediaHUD will have been deleted by this point -- don't try to delete it.

		/* Richard says:
			all widgets are supposed to be destroyed at the same time
			you shouldn't hold on to pointer to them outside of ui code		
			you can use the LLHandle approach
			if you want to be type safe, you'll need to add a LLRootHandle to whatever derived class you are pointing to
			look at llview::gethandle
			its our version of a weak pointer			
		*/
		if(self->mMediaHUD.get())
		{
			self->mMediaHUD.get()->setMediaImpl(NULL);
		}
		self->mMediaImpl = NULL;
	}
	
}


void LLViewerMediaFocus::setFocusFace( BOOL b, LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl )
{
	LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

	if(mMediaImpl.notNull())
	{
		mMediaImpl->focus(false);
	}

	if (b && media_impl.notNull())
	{
		bool face_auto_zoom = false;
		mMediaImpl = media_impl;
		mMediaImpl->focus(true);

		LLSelectMgr::getInstance()->deselectAll();
		LLSelectMgr::getInstance()->selectObjectOnly(objectp, face);

		if(objectp.notNull())
		{
			LLTextureEntry* tep = objectp->getTE(face);
			if(! tep->hasMedia())
			{
				// Error condition
			}
			LLMediaEntry* mep = tep->getMediaData();
			face_auto_zoom = mep->getAutoZoom();
			if(! mep->getAutoPlay())
			{
				std::string url = mep->getCurrentURL().empty() ? mep->getHomeURL() : mep->getCurrentURL();
				media_impl->navigateTo(url, "", true);
			}
		}
		mFocus = LLSelectMgr::getInstance()->getSelection();
		if(mMediaHUD.get() && face_auto_zoom && ! parcel->getMediaPreventCameraZoom())
		{
			mMediaHUD.get()->resetZoomLevel();
			mMediaHUD.get()->nextZoomLevel();
		}
		if (!mFocus->isEmpty())
		{
			gFocusMgr.setKeyboardFocus(this);
		}
		mObjectID = objectp->getID();
		mObjectFace = face;
		// LLViewerMedia::addObserver(this, mObjectID);


	}
	else
	{
		gFocusMgr.setKeyboardFocus(NULL);
		if(! parcel->getMediaPreventCameraZoom())
		{
			if (!mFocus->isEmpty())
			{
				gAgent.setFocusOnAvatar(TRUE, ANIMATE);
			}
		}
		mFocus = NULL;
		// LLViewerMedia::remObserver(this, mObjectID);
		
		// Null out the media hud media pointer
		if(mMediaHUD.get())
		{
			mMediaHUD.get()->setMediaImpl(NULL);
		}

		// and null out the media impl
		mMediaImpl = NULL;
		mObjectID = LLUUID::null;
		mObjectFace = 0;
	}
	if(mMediaHUD.get())
	{
		mMediaHUD.get()->setMediaFocus(b);
	}
}
bool LLViewerMediaFocus::getFocus()
{
	if (gFocusMgr.getKeyboardFocus() == this)
	{
		return true;
	}
	return false;
}

// This function selects an ideal viewing distance given a selection bounding box, normal, and padding value
void LLViewerMediaFocus::setCameraZoom(F32 padding_factor)
{
	LLPickInfo& pick = LLToolPie::getInstance()->getPick();

	if(LLSelectMgr::getInstance()->getSelection()->isEmpty())
	{
		pick = mPickInfo;
		setFocusFace(true, pick.getObject(), pick.mObjectFace, mMediaImpl);
	}

	if (!LLSelectMgr::getInstance()->getSelection()->isEmpty())
	{
		gAgent.setFocusOnAvatar(FALSE, ANIMATE);

		LLBBox selection_bbox = LLSelectMgr::getInstance()->getBBoxOfSelection();
		F32 height;
		F32 width;
		F32 depth;
		F32 angle_of_view;
		F32 distance;

		// We need the aspect ratio, and the 3 components of the bbox as height, width, and depth.
		F32 aspect_ratio = getBBoxAspectRatio(selection_bbox, pick.mNormal, &height, &width, &depth);
		F32 camera_aspect = LLViewerCamera::getInstance()->getAspect();

		// We will normally use the side of the volume aligned with the short side of the screen (i.e. the height for 
		// a screen in a landscape aspect ratio), however there is an edge case where the aspect ratio of the object is 
		// more extreme than the screen.  In this case we invert the logic, using the longer component of both the object
		// and the screen.  
		bool invert = (camera_aspect > 1.0f && aspect_ratio > camera_aspect) ||
			(camera_aspect < 1.0f && aspect_ratio < camera_aspect);

		// To calculate the optimum viewing distance we will need the angle of the shorter side of the view rectangle.
		// In portrait mode this is the width, and in landscape it is the height.
		// We then calculate the distance based on the corresponding side of the object bbox (width for portrait, height for landscape)
		// We will add half the depth of the bounding box, as the distance projection uses the center point of the bbox.
		if(camera_aspect < 1.0f || invert)
		{
			angle_of_view = llmax(0.1f, LLViewerCamera::getInstance()->getView() * LLViewerCamera::getInstance()->getAspect());
			distance = width * 0.5 * padding_factor / tan(angle_of_view * 0.5f );
		}
		else
		{
			angle_of_view = llmax(0.1f, LLViewerCamera::getInstance()->getView());
			distance = height * 0.5 * padding_factor / tan(angle_of_view * 0.5f );
		}

		distance += depth * 0.5;

		// Finally animate the camera to this new position and focal point
		gAgent.setCameraPosAndFocusGlobal(LLSelectMgr::getInstance()->getSelectionCenterGlobal() + LLVector3d(pick.mNormal * distance), 
			LLSelectMgr::getInstance()->getSelectionCenterGlobal(), LLSelectMgr::getInstance()->getSelection()->getFirstObject()->mID );
	}
}
void LLViewerMediaFocus::onFocusReceived()
{
	if(mMediaImpl.notNull())
		mMediaImpl->focus(true);

	LLFocusableElement::onFocusReceived();
}

void LLViewerMediaFocus::onFocusLost()
{
	if(mMediaImpl.notNull())
		mMediaImpl->focus(false);
	gViewerWindow->focusClient();
	mFocus = NULL;
	LLFocusableElement::onFocusLost();
}
void LLViewerMediaFocus::setMouseOverFlag(bool b, viewer_media_t media_impl)
{
	if (b && media_impl.notNull())
	{
		if(! mMediaHUD.get())
		{
			LLPanelMediaHUD* media_hud = new LLPanelMediaHUD(mMediaImpl);
			mMediaHUD = media_hud->getHandle();
			gHUDView->addChild(media_hud);	
		}
		mMediaHUD.get()->setMediaImpl(media_impl);

		if(mMediaImpl.notNull() && (mMediaImpl != media_impl))
		{
			mMediaImpl->focus(false);
		}

		mMediaImpl = media_impl;
	}
	mMouseOverFlag = b;
}
LLUUID LLViewerMediaFocus::getSelectedUUID() 
{ 
	LLViewerObject* object = mFocus->getFirstObject();
	return object ? object->getID() : LLUUID::null; 
}
#if 0 // Must re-implement when the new media api event system is ready
void LLViewerMediaFocus::onNavigateComplete( const EventType& event_in )
{
	if (hasFocus() && mLastURL != event_in.getStringValue())
	{
		LLViewerMedia::focus(true, mObjectID);
		// spoof mouse event to reassert focus
		LLViewerMedia::mouseDown(1,1, mObjectID);
		LLViewerMedia::mouseUp(1,1, mObjectID);
	}
	mLastURL = event_in.getStringValue();
}
#endif
BOOL LLViewerMediaFocus::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	if(mMediaImpl.notNull())
		mMediaImpl->handleKeyHere(key, mask);

	if (key == KEY_ESCAPE && mMediaHUD.get())
	{
		mMediaHUD.get()->close();
	}
	return true;
}

BOOL LLViewerMediaFocus::handleUnicodeChar(llwchar uni_char, BOOL called_from_parent)
{
	if(mMediaImpl.notNull())
		mMediaImpl->handleUnicodeCharHere(uni_char);
	return true;
}
BOOL LLViewerMediaFocus::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	BOOL retval = FALSE;
	if(mFocus.notNull() && mMediaImpl.notNull() && mMediaImpl->hasMedia())
	{
        // the scrollEvent() API's x and y are not the same as handleScrollWheel's x and y.
        // The latter is the position of the mouse at the time of the event
        // The former is the 'scroll amount' in x and y, respectively.
        // All we have for 'scroll amount' here is 'clicks', and no mask.
		mMediaImpl->getMediaPlugin()->scrollEvent(0, clicks, /*mask*/0);
		retval = TRUE;
	}
	return retval;
}

void LLViewerMediaFocus::update()
{
	if (mMediaHUD.get())
	{
		if(mFocus.notNull() || mMouseOverFlag || mMediaHUD.get()->isMouseOver())
		{
			// mMediaHUD.get()->setVisible(true);
			mMediaHUD.get()->updateShape();
		}
		else
		{
			mMediaHUD.get()->setVisible(false);
		}
	}
}
// This function calculates the aspect ratio and the world aligned components of a selection bounding box.
F32 LLViewerMediaFocus::getBBoxAspectRatio(const LLBBox& bbox, const LLVector3& normal, F32* height, F32* width, F32* depth)
{
	// Convert the selection normal and an up vector to local coordinate space of the bbox
	LLVector3 local_normal = bbox.agentToLocalBasis(normal);
	LLVector3 z_vec = bbox.agentToLocalBasis(LLVector3(0.0f, 0.0f, 1.0f));
	
	LLVector3 comp1(0.f,0.f,0.f);
	LLVector3 comp2(0.f,0.f,0.f);
	LLVector3 bbox_max = bbox.getExtentLocal();
	F32 dot1 = 0.f;
	F32 dot2 = 0.f;

	// The largest component of the localized normal vector is the depth component
	// meaning that the other two are the legs of the rectangle.
	local_normal.abs();
	if(local_normal.mV[VX] > local_normal.mV[VY])
	{
		if(local_normal.mV[VX] > local_normal.mV[VZ])
		{
			// Use the y and z comps
			comp1.mV[VY] = bbox_max.mV[VY];
			comp2.mV[VZ] = bbox_max.mV[VZ];
			*depth = bbox_max.mV[VX];
		}
		else
		{
			// Use the x and y comps
			comp1.mV[VY] = bbox_max.mV[VY];
			comp2.mV[VZ] = bbox_max.mV[VZ];
			*depth = bbox_max.mV[VZ];
		}
	}
	else if(local_normal.mV[VY] > local_normal.mV[VZ])
	{
		// Use the x and z comps
		comp1.mV[VX] = bbox_max.mV[VX];
		comp2.mV[VZ] = bbox_max.mV[VZ];
		*depth = bbox_max.mV[VY];
	}
	else
	{
		// Use the x and y comps
		comp1.mV[VY] = bbox_max.mV[VY];
		comp2.mV[VZ] = bbox_max.mV[VZ];
		*depth = bbox_max.mV[VX];
	}
	
	// The height is the vector closest to vertical in the bbox coordinate space (highest dot product value)
	dot1 = comp1 * z_vec;
	dot2 = comp2 * z_vec;
	if(fabs(dot1) > fabs(dot2))
	{
		*height = comp1.length();
		*width = comp2.length();
	}
	else
	{
		*height = comp2.length();
		*width = comp1.length();
	}

	// Return the aspect ratio.
	return *width / *height;
}

bool LLViewerMediaFocus::isFocusedOnFace(LLPointer<LLViewerObject> objectp, S32 face)
{
	return objectp->getID() == mObjectID && face == mObjectFace;
}
