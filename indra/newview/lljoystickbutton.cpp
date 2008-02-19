/** 
 * @file lljoystickbutton.cpp
 * @brief LLJoystick class implementation
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

#include "lljoystickbutton.h"

// Library includes
#include "llcoord.h"
#include "indra_constants.h"

// Project includes
#include "llui.h"
#include "llagent.h"
#include "llviewerimage.h"
#include "llviewerimagelist.h"
#include "llviewerwindow.h"
#include "llmoveview.h"

#include "llglheaders.h"

const F32 NUDGE_TIME = 0.25f;		// in seconds
const F32 ORBIT_NUDGE_RATE = 0.05f; // fraction of normal speed

//
// Public Methods
//
LLJoystick::LLJoystick(
	const LLString& name, 
	LLRect rect,
	const LLString &default_image,
	const LLString &selected_image,
	EJoystickQuadrant initial_quadrant )
	:	
	LLButton(name, rect, default_image, selected_image, NULL, NULL),
	mInitialQuadrant(initial_quadrant),
	mInitialOffset(0, 0),
	mLastMouse(0, 0),
	mFirstMouse(0, 0),
	mVertSlopNear(0),
	mVertSlopFar(0),
	mHorizSlopNear(0),
	mHorizSlopFar(0),
	mHeldDown(FALSE),
	mHeldDownTimer()
{
	setHeldDownCallback(&LLJoystick::onHeldDown);
	setCallbackUserData(this);
}


void LLJoystick::updateSlop()
{
	mVertSlopNear = getRect().getHeight();
	mVertSlopFar = getRect().getHeight() * 2;

	mHorizSlopNear = getRect().getWidth();
	mHorizSlopFar = getRect().getWidth() * 2;

	// Compute initial mouse offset based on initial quadrant.
	// Place the mouse evenly between the near and far zones.
	switch (mInitialQuadrant)
	{
	case JQ_ORIGIN:
		mInitialOffset.set(0, 0);
		break;

	case JQ_UP:
		mInitialOffset.mX = 0;
		mInitialOffset.mY = (mVertSlopNear + mVertSlopFar) / 2;
		break;

	case JQ_DOWN:
		mInitialOffset.mX = 0;
		mInitialOffset.mY = - (mVertSlopNear + mVertSlopFar) / 2;
		break;

	case JQ_LEFT:
		mInitialOffset.mX = - (mHorizSlopNear + mHorizSlopFar) / 2;
		mInitialOffset.mY = 0;
		break;

	case JQ_RIGHT:
		mInitialOffset.mX = (mHorizSlopNear + mHorizSlopFar) / 2;
		mInitialOffset.mY = 0;
		break;

	default:
		llerrs << "LLJoystick::LLJoystick() - bad switch case" << llendl;
		break;
	}

	return;
}


BOOL LLJoystick::handleMouseDown(S32 x, S32 y, MASK mask)
{
	//llinfos << "joystick mouse down " << x << ", " << y << llendl;

	mLastMouse.set(x, y);
	mFirstMouse.set(x, y);

	mMouseDownTimer.reset();
	return LLButton::handleMouseDown(x, y, mask);
}


BOOL LLJoystick::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// llinfos << "joystick mouse up " << x << ", " << y << llendl;

	if( hasMouseCapture() )
	{
		mLastMouse.set(x, y);
		mHeldDown = FALSE;
		onMouseUp();
	}

	return LLButton::handleMouseUp(x, y, mask);
}


BOOL LLJoystick::handleHover(S32 x, S32 y, MASK mask)
{
	if( hasMouseCapture() )
	{
		mLastMouse.set(x, y);
	}

	return LLButton::handleHover(x, y, mask);
}

F32 LLJoystick::getElapsedHeldDownTime()
{
	if( mHeldDown )
	{
		return getHeldDownTime();
	}
	else
	{
		return 0.f;
	}
}

// static
void LLJoystick::onHeldDown(void *userdata)
{
	LLJoystick *self = (LLJoystick *)userdata;

	// somebody removed this function without checking the
	// build. Removed 2007-03-26.
	//llassert( gViewerWindow->hasMouseCapture( self ) );

	self->mHeldDown = TRUE;
	self->onHeldDown();
}

EJoystickQuadrant LLJoystick::selectQuadrant(LLXMLNodePtr node)
{
	
	EJoystickQuadrant quadrant = JQ_RIGHT;

	if (node->hasAttribute("quadrant"))
	{
		LLString quadrant_name;
		node->getAttributeString("quadrant", quadrant_name);

		quadrant = quadrantFromName(quadrant_name.c_str());
	}
	return quadrant;
}


LLString LLJoystick::nameFromQuadrant(EJoystickQuadrant	quadrant)
{
	if (quadrant == JQ_ORIGIN)	    return LLString("origin");
	else if (quadrant == JQ_UP)	    return LLString("up");
	else if (quadrant == JQ_DOWN)	return LLString("down");
	else if (quadrant == JQ_LEFT)	return LLString("left");
	else if (quadrant == JQ_RIGHT)	return LLString("right");
	else return LLString();
}


EJoystickQuadrant LLJoystick::quadrantFromName(const LLString& sQuadrant)
{
	EJoystickQuadrant quadrant = JQ_RIGHT;

	if (sQuadrant == "origin")
	{
		quadrant = JQ_ORIGIN;
	}
	else if (sQuadrant == "up")
	{
		quadrant = JQ_UP;
	}
	else if (sQuadrant == "down")
	{
		quadrant = JQ_DOWN;
	}
	else if (sQuadrant == "left")
	{
		quadrant = JQ_LEFT;
	}
	else if (sQuadrant == "right")
	{
		quadrant = JQ_RIGHT;
	}

	return quadrant;
}


LLXMLNodePtr LLJoystick::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->createChild("halign", TRUE)->setStringValue(LLFontGL::nameFromHAlign(getHAlign()));
	node->createChild("quadrant", TRUE)->setStringValue(nameFromQuadrant(mInitialQuadrant));

	addImageAttributeToXML(node,getImageUnselectedName(),getImageUnselectedID(),"image_unselected");
	addImageAttributeToXML(node,getImageSelectedName(),getImageSelectedID(),"image_selected");
	
	node->createChild("scale_image", TRUE)->setBoolValue(getScaleImage());

	return node;
}



//-------------------------------------------------------------------------------
// LLJoystickAgentTurn
//-------------------------------------------------------------------------------

void LLJoystickAgentTurn::onHeldDown()
{
	F32 time = getElapsedHeldDownTime();
	updateSlop();

	//llinfos << "move forward/backward (and/or turn)" << llendl;

	S32 dx = mLastMouse.mX - mFirstMouse.mX + mInitialOffset.mX;
	S32 dy = mLastMouse.mY - mFirstMouse.mY + mInitialOffset.mY;

	float m = (float) (dx)/abs(dy);
	
	if (m > 1) {
		m = 1;
	}
	else if (m < -1) {
		m = -1;
	}
	gAgent.moveYaw(-LLFloaterMove::getYawRate(time)*m);
	

	// handle forward/back movement
	if (dy > mVertSlopFar)
	{
		// ...if mouse is forward of run region run forward
		gAgent.moveAt(1);
	}
	else if (dy > mVertSlopNear)
	{
		if( time < NUDGE_TIME )
		{
			gAgent.moveAtNudge(1);
		}
		else
		{
			// ...else if mouse is forward of walk region walk forward
			// JC 9/5/2002 - Always run / move quickly.
			gAgent.moveAt(1);
		}
	}
	else if (dy < -mVertSlopFar)
	{
		// ...else if mouse is behind run region run backward
		gAgent.moveAt(-1);
	}
	else if (dy < -mVertSlopNear)
	{
		if( time < NUDGE_TIME )
		{
			gAgent.moveAtNudge(-1);
		}
		else
		{
			// ...else if mouse is behind walk region walk backward
			// JC 9/5/2002 - Always run / move quickly.
			gAgent.moveAt(-1);
		}
	}
}

LLView* LLJoystickAgentTurn::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("button");
	node->getAttributeString("name", name);

	LLString	image_unselected;
	if (node->hasAttribute("image_unselected")) node->getAttributeString("image_unselected",image_unselected);
	
	LLString	image_selected;
	if (node->hasAttribute("image_selected")) node->getAttributeString("image_selected",image_selected);

	EJoystickQuadrant quad = JQ_ORIGIN;
	if (node->hasAttribute("quadrant")) quad = selectQuadrant(node);
	
	LLJoystickAgentTurn *button = new LLJoystickAgentTurn(name, 
		LLRect(),
		image_unselected,
		image_selected,
		quad);

	if (node->hasAttribute("halign"))
	{
		LLFontGL::HAlign halign = selectFontHAlign(node);
		button->setHAlign(halign);
	}

	if (node->hasAttribute("scale_image"))
	{
		BOOL	needsScale = FALSE;
		node->getAttributeBOOL("scale_image",needsScale);
		button->setScaleImage( needsScale );
	}

	button->initFromXML(node, parent);
	
	return button;
}



//-------------------------------------------------------------------------------
// LLJoystickAgentSlide
//-------------------------------------------------------------------------------

void LLJoystickAgentSlide::onMouseUp()
{
	F32 time = getElapsedHeldDownTime();
	if( time < NUDGE_TIME )
	{
		switch (mInitialQuadrant)
		{
		case JQ_LEFT:
			gAgent.moveLeftNudge(1);
			break;

		case JQ_RIGHT:
			gAgent.moveLeftNudge(-1);
			break;

		default:
			break;
		}
	}
}

void LLJoystickAgentSlide::onHeldDown()
{
	//llinfos << "slide left/right (and/or move forward/backward)" << llendl;

	updateSlop();

	S32 dx = mLastMouse.mX - mFirstMouse.mX + mInitialOffset.mX;
	S32 dy = mLastMouse.mY - mFirstMouse.mY + mInitialOffset.mY;

	// handle left-right sliding
	if (dx > mHorizSlopNear)
	{
		gAgent.moveLeft(-1);
	}
	else if (dx < -mHorizSlopNear)
	{
		gAgent.moveLeft(1);
	}

	// handle forward/back movement
	if (dy > mVertSlopFar)
	{
		// ...if mouse is forward of run region run forward
		gAgent.moveAt(1);
	}
	else if (dy > mVertSlopNear)
	{
		// ...else if mouse is forward of walk region walk forward
		gAgent.moveAtNudge(1);
	}
	else if (dy < -mVertSlopFar)
	{
		// ...else if mouse is behind run region run backward
		gAgent.moveAt(-1);
	}
	else if (dy < -mVertSlopNear)
	{
		// ...else if mouse is behind walk region walk backward
		gAgent.moveAtNudge(-1);
	}
}


// static
LLView* LLJoystickAgentSlide::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("button");
	node->getAttributeString("name", name);

	LLString	image_unselected;
	if (node->hasAttribute("image_unselected")) node->getAttributeString("image_unselected",image_unselected);
	
	LLString	image_selected;
	if (node->hasAttribute("image_selected")) node->getAttributeString("image_selected",image_selected);
	
	
	EJoystickQuadrant quad = JQ_ORIGIN;
	if (node->hasAttribute("quadrant")) quad = selectQuadrant(node);
	
	LLJoystickAgentSlide *button = new LLJoystickAgentSlide(name, 
		LLRect(),
		image_unselected,
		image_selected,
		quad);

	if (node->hasAttribute("halign"))
	{
		LLFontGL::HAlign halign = selectFontHAlign(node);
		button->setHAlign(halign);
	}

	if (node->hasAttribute("scale_image"))
	{
		BOOL	needsScale = FALSE;
		node->getAttributeBOOL("scale_image",needsScale);
		button->setScaleImage( needsScale );
	}
	
	button->initFromXML(node, parent);
	
	return button;
}


//-------------------------------------------------------------------------------
// LLJoystickCameraRotate
//-------------------------------------------------------------------------------

LLJoystickCameraRotate::LLJoystickCameraRotate(const LLString& name, LLRect rect, const LLString &out_img, const LLString &in_img)
	: 
	LLJoystick(name, rect, out_img, in_img, JQ_ORIGIN),
	mInLeft( FALSE ),
	mInTop( FALSE ),
	mInRight( FALSE ),
	mInBottom( FALSE )
{ }


void LLJoystickCameraRotate::updateSlop()
{
	// do the initial offset calculation based on mousedown location

	// small fixed slop region
	mVertSlopNear = 16;
	mVertSlopFar = 32;

	mHorizSlopNear = 16;
	mHorizSlopFar = 32;

	return;
}


BOOL LLJoystickCameraRotate::handleMouseDown(S32 x, S32 y, MASK mask)
{
	updateSlop();

	// Set initial offset based on initial click location
	S32 horiz_center = getRect().getWidth() / 2;
	S32 vert_center = getRect().getHeight() / 2;

	S32 dx = x - horiz_center;
	S32 dy = y - vert_center;

	if (dy > dx && dy > -dx)
	{
		// top
		mInitialOffset.mX = 0;
		mInitialOffset.mY = (mVertSlopNear + mVertSlopFar) / 2;
		mInitialQuadrant = JQ_UP;
	}
	else if (dy > dx && dy <= -dx)
	{
		// left
		mInitialOffset.mX = - (mHorizSlopNear + mHorizSlopFar) / 2;
		mInitialOffset.mY = 0;
		mInitialQuadrant = JQ_LEFT;
	}
	else if (dy <= dx && dy <= -dx)
	{
		// bottom
		mInitialOffset.mX = 0;
		mInitialOffset.mY = - (mVertSlopNear + mVertSlopFar) / 2;
		mInitialQuadrant = JQ_DOWN;
	}
	else
	{
		// right
		mInitialOffset.mX = (mHorizSlopNear + mHorizSlopFar) / 2;
		mInitialOffset.mY = 0;
		mInitialQuadrant = JQ_RIGHT;
	}

	return LLJoystick::handleMouseDown(x, y, mask);
}


void LLJoystickCameraRotate::onHeldDown()
{
	updateSlop();

	S32 dx = mLastMouse.mX - mFirstMouse.mX + mInitialOffset.mX;
	S32 dy = mLastMouse.mY - mFirstMouse.mY + mInitialOffset.mY;

	// left-right rotation
	if (dx > mHorizSlopNear)
	{
		gAgent.unlockView();
		gAgent.setOrbitLeftKey(getOrbitRate());
	}
	else if (dx < -mHorizSlopNear)
	{
		gAgent.unlockView();
		gAgent.setOrbitRightKey(getOrbitRate());
	}

	// over/under rotation
	if (dy > mVertSlopNear)
	{
		gAgent.unlockView();
		gAgent.setOrbitUpKey(getOrbitRate());
	}
	else if (dy < -mVertSlopNear)
	{
		gAgent.unlockView();
		gAgent.setOrbitDownKey(getOrbitRate());
	}
}

F32 LLJoystickCameraRotate::getOrbitRate()
{
	F32 time = getElapsedHeldDownTime();
	if( time < NUDGE_TIME )
	{
		F32 rate = ORBIT_NUDGE_RATE + time * (1 - ORBIT_NUDGE_RATE)/ NUDGE_TIME;
		//llinfos << rate << llendl;
		return rate;
	}
	else
	{
		return 1;
	}
}


// Only used for drawing
void LLJoystickCameraRotate::setToggleState( BOOL left, BOOL top, BOOL right, BOOL bottom )
{
	mInLeft = left;
	mInTop = top;
	mInRight = right;
	mInBottom = bottom;
}

void LLJoystickCameraRotate::draw()
{
	
	if( getVisible() ) 
	{
		LLGLSUIDefault gls_ui;

		getImageUnselected()->draw( 0, 0 );

		if( mInTop )
		{
			drawRotatedImage( getImageSelected()->getImage(), 0 );
		}

		if( mInRight )
		{
			drawRotatedImage( getImageSelected()->getImage(), 1 );
		}

		if( mInBottom )
		{
			drawRotatedImage( getImageSelected()->getImage(), 2 );
		}

		if( mInLeft )
		{
			drawRotatedImage( getImageSelected()->getImage(), 3 );
		}

		if (sDebugRects)
		{
			drawDebugRect();
		}
	}
}

// Draws image rotated by multiples of 90 degrees
void LLJoystickCameraRotate::drawRotatedImage( const LLImageGL* image, S32 rotations )
{
	S32 width = image->getWidth();
	S32 height = image->getHeight();

	F32 uv[][2] = 
	{
		{ 1.f, 1.f },
		{ 0.f, 1.f },
		{ 0.f, 0.f },
		{ 1.f, 0.f }
	};

	image->bind();

	glColor4fv(UI_VERTEX_COLOR.mV);
	
	glBegin(GL_QUADS);
	{
		glTexCoord2fv( uv[ (rotations + 0) % 4]);
		glVertex2i(width, height );

		glTexCoord2fv( uv[ (rotations + 1) % 4]);
		glVertex2i(0, height );

		glTexCoord2fv( uv[ (rotations + 2) % 4]);
		glVertex2i(0, 0);

		glTexCoord2fv( uv[ (rotations + 3) % 4]);
		glVertex2i(width, 0);
	}
	glEnd();
}



//-------------------------------------------------------------------------------
// LLJoystickCameraTrack
//-------------------------------------------------------------------------------


void LLJoystickCameraTrack::onHeldDown()
{
	updateSlop();

	S32 dx = mLastMouse.mX - mFirstMouse.mX + mInitialOffset.mX;
	S32 dy = mLastMouse.mY - mFirstMouse.mY + mInitialOffset.mY;

	if (dx > mVertSlopNear)
	{
		gAgent.unlockView();
		gAgent.setPanRightKey(getOrbitRate());
	}
	else if (dx < -mVertSlopNear)
	{
		gAgent.unlockView();
		gAgent.setPanLeftKey(getOrbitRate());
	}

	// over/under rotation
	if (dy > mVertSlopNear)
	{
		gAgent.unlockView();
		gAgent.setPanUpKey(getOrbitRate());
	}
	else if (dy < -mVertSlopNear)
	{
		gAgent.unlockView();
		gAgent.setPanDownKey(getOrbitRate());
	}
}



//-------------------------------------------------------------------------------
// LLJoystickCameraZoom
//-------------------------------------------------------------------------------

LLJoystickCameraZoom::LLJoystickCameraZoom(const LLString& name, LLRect rect, const LLString &out_img, const LLString &plus_in_img, const LLString &minus_in_img)
	: 
	LLJoystick(name, rect, out_img, "", JQ_ORIGIN),
	mInTop( FALSE ),
	mInBottom( FALSE )
{
	mPlusInImage = gImageList.getImage(LLUI::findAssetUUIDByName(plus_in_img), MIPMAP_FALSE, TRUE);
	mMinusInImage = gImageList.getImage(LLUI::findAssetUUIDByName(minus_in_img), MIPMAP_FALSE, TRUE);
}


BOOL LLJoystickCameraZoom::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLJoystick::handleMouseDown(x, y, mask);

	if( handled )
	{
		if (mFirstMouse.mY > getRect().getHeight() / 2)
		{
			mInitialQuadrant = JQ_UP;
		}
		else
		{
			mInitialQuadrant = JQ_DOWN;
		}
	}
	return handled;
}


void LLJoystickCameraZoom::onHeldDown()
{
	updateSlop();

	const F32 FAST_RATE = 2.5f; // two and a half times the normal rate

	S32 dy = mLastMouse.mY - mFirstMouse.mY + mInitialOffset.mY;

	if (dy > mVertSlopFar)
	{
		// Zoom in fast
		gAgent.unlockView();
		gAgent.setOrbitInKey(FAST_RATE);
	}
	else if (dy > mVertSlopNear)
	{
		// Zoom in slow
		gAgent.unlockView();
		gAgent.setOrbitInKey(getOrbitRate());
	}
	else if (dy < -mVertSlopFar)
	{
		// Zoom out fast
		gAgent.unlockView();
		gAgent.setOrbitOutKey(FAST_RATE);
	}
	else if (dy < -mVertSlopNear)
	{
		// Zoom out slow
		gAgent.unlockView();
		gAgent.setOrbitOutKey(getOrbitRate());
	}
}

// Only used for drawing
void LLJoystickCameraZoom::setToggleState( BOOL top, BOOL bottom )
{
	mInTop = top;
	mInBottom = bottom;
}

void LLJoystickCameraZoom::draw()
{
	if( getVisible() ) 
	{
		if( mInTop )
		{
			gl_draw_image( 0, 0, mPlusInImage );
		}
		else
		if( mInBottom )
		{
			gl_draw_image( 0, 0, mMinusInImage );
		}
		else
		{
			getImageUnselected()->draw( 0, 0 );
		}

		if (sDebugRects)
		{
			drawDebugRect();
		}
	}
}

void LLJoystickCameraZoom::updateSlop()
{
	mVertSlopNear = getRect().getHeight() / 4;
	mVertSlopFar = getRect().getHeight() / 2;

	mHorizSlopNear = getRect().getWidth() / 4;
	mHorizSlopFar = getRect().getWidth() / 2;

	// Compute initial mouse offset based on initial quadrant.
	// Place the mouse evenly between the near and far zones.
	switch (mInitialQuadrant)
	{
	case JQ_ORIGIN:
		mInitialOffset.set(0, 0);
		break;

	case JQ_UP:
		mInitialOffset.mX = 0;
		mInitialOffset.mY = (mVertSlopNear + mVertSlopFar) / 2;
		break;

	case JQ_DOWN:
		mInitialOffset.mX = 0;
		mInitialOffset.mY = - (mVertSlopNear + mVertSlopFar) / 2;
		break;

	case JQ_LEFT:
		mInitialOffset.mX = - (mHorizSlopNear + mHorizSlopFar) / 2;
		mInitialOffset.mY = 0;
		break;

	case JQ_RIGHT:
		mInitialOffset.mX = (mHorizSlopNear + mHorizSlopFar) / 2;
		mInitialOffset.mY = 0;
		break;

	default:
		llerrs << "LLJoystick::LLJoystick() - bad switch case" << llendl;
		break;
	}

	return;
}


F32 LLJoystickCameraZoom::getOrbitRate()
{
	F32 time = getElapsedHeldDownTime();
	if( time < NUDGE_TIME )
	{
		F32 rate = ORBIT_NUDGE_RATE + time * (1 - ORBIT_NUDGE_RATE)/ NUDGE_TIME;
//		llinfos << "rate " << rate << " time " << time << llendl;
		return rate;
	}
	else
	{
		return 1;
	}
}
