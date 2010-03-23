/** 
 * @file lljoystickbutton.cpp
 * @brief LLJoystick class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
#include "llrender.h"

// Project includes
#include "llui.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llmoveview.h"

#include "llglheaders.h"

static LLDefaultChildRegistry::Register<LLJoystickAgentSlide> r1("joystick_slide");
static LLDefaultChildRegistry::Register<LLJoystickAgentTurn> r2("joystick_turn");
static LLDefaultChildRegistry::Register<LLJoystickCameraRotate> r3("joystick_rotate");
static LLDefaultChildRegistry::Register<LLJoystickCameraZoom> r4("joystick_zoom");
static LLDefaultChildRegistry::Register<LLJoystickCameraTrack> r5("joystick_track");



const F32 NUDGE_TIME = 0.25f;		// in seconds
const F32 ORBIT_NUDGE_RATE = 0.05f; // fraction of normal speed

//
// Public Methods
//
void QuadrantNames::declareValues()
{
	declare("origin", JQ_ORIGIN);
	declare("up", JQ_UP);
	declare("down", JQ_DOWN);
	declare("left", JQ_LEFT);
	declare("right", JQ_RIGHT);
}


LLJoystick::LLJoystick(const LLJoystick::Params& p)
:	LLButton(p),
	mInitialOffset(0, 0),
	mLastMouse(0, 0),
	mFirstMouse(0, 0),
	mVertSlopNear(0),
	mVertSlopFar(0),
	mHorizSlopNear(0),
	mHorizSlopFar(0),
	mHeldDown(FALSE),
	mHeldDownTimer(),
	mInitialQuadrant(p.quadrant)
{
	setHeldDownCallback(&LLJoystick::onBtnHeldDown, this);
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

bool LLJoystick::pointInCircle(S32 x, S32 y) const 
{ 
	if(this->getLocalRect().getHeight() != this->getLocalRect().getWidth())
	{
		llwarns << "Joystick shape is not square"<<llendl;
		return true;
	}
	//center is x and y coordinates of center of joystick circle, and also its radius
	int center = this->getLocalRect().getHeight()/2;
	bool in_circle = (x - center) * (x - center) + (y - center) * (y - center) <= center * center;
	return in_circle;
}

BOOL LLJoystick::handleMouseDown(S32 x, S32 y, MASK mask)
{
	//llinfos << "joystick mouse down " << x << ", " << y << llendl;
	bool handles = false;

	if(pointInCircle(x, y))
	{
		mLastMouse.set(x, y);
		mFirstMouse.set(x, y);
		mMouseDownTimer.reset();
		handles = LLButton::handleMouseDown(x, y, mask);
	}

	return handles;
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
void LLJoystick::onBtnHeldDown(void *userdata)
{
	LLJoystick *self = (LLJoystick *)userdata;
	if (self)
	{
		self->mHeldDown = TRUE;
		self->onHeldDown();
	}
}

EJoystickQuadrant LLJoystick::selectQuadrant(LLXMLNodePtr node)
{
	
	EJoystickQuadrant quadrant = JQ_RIGHT;

	if (node->hasAttribute("quadrant"))
	{
		std::string quadrant_name;
		node->getAttributeString("quadrant", quadrant_name);

		quadrant = quadrantFromName(quadrant_name);
	}
	return quadrant;
}


std::string LLJoystick::nameFromQuadrant(EJoystickQuadrant	quadrant)
{
	if (quadrant == JQ_ORIGIN)	    return std::string("origin");
	else if (quadrant == JQ_UP)	    return std::string("up");
	else if (quadrant == JQ_DOWN)	return std::string("down");
	else if (quadrant == JQ_LEFT)	return std::string("left");
	else if (quadrant == JQ_RIGHT)	return std::string("right");
	else return std::string();
}


EJoystickQuadrant LLJoystick::quadrantFromName(const std::string& sQuadrant)
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


//-------------------------------------------------------------------------------
// LLJoystickCameraRotate
//-------------------------------------------------------------------------------

LLJoystickCameraRotate::LLJoystickCameraRotate(const LLJoystickCameraRotate::Params& p)
:	LLJoystick(p), 
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
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitLeftKey(getOrbitRate());
	}
	else if (dx < -mHorizSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitRightKey(getOrbitRate());
	}

	// over/under rotation
	if (dy > mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitUpKey(getOrbitRate());
	}
	else if (dy < -mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitDownKey(getOrbitRate());
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
	LLGLSUIDefault gls_ui;

	getImageUnselected()->draw( 0, 0 );
	LLPointer<LLUIImage> image = getImageSelected();

	if( mInTop )
	{
		drawRotatedImage( getImageSelected(), 0 );
	}

	if( mInRight )
	{
		drawRotatedImage( getImageSelected(), 1 );
	}

	if( mInBottom )
	{
		drawRotatedImage( getImageSelected(), 2 );
	}

	if( mInLeft )
	{
		drawRotatedImage( getImageSelected(), 3 );
	}
}

// Draws image rotated by multiples of 90 degrees
void LLJoystickCameraRotate::drawRotatedImage( LLPointer<LLUIImage> image, S32 rotations )
{
	S32 width = image->getWidth();
	S32 height = image->getHeight();
	LLTexture* texture = image->getImage();

	/*
	 * Scale  texture coordinate system 
	 * to handle the different between image size and size of texture.
	 * If we will use default matrix, 
	 * it may break texture mapping after rotation.
	 * see EXT-2023 Camera floater: arrows became shifted when pressed.
	 */ 
	F32 uv[][2] = 
	{
		{ (F32)width/texture->getWidth(), (F32)height/texture->getHeight() },
		{ 0.f, (F32)height/texture->getHeight() },
		{ 0.f, 0.f },
		{ (F32)width/texture->getWidth(), 0.f }
	};

	gGL.getTexUnit(0)->bind(texture);

	gGL.color4fv(UI_VERTEX_COLOR.mV);
	
	gGL.begin(LLRender::QUADS);
	{
		gGL.texCoord2fv( uv[ (rotations + 0) % 4]);
		gGL.vertex2i(width, height );

		gGL.texCoord2fv( uv[ (rotations + 1) % 4]);
		gGL.vertex2i(0, height );

		gGL.texCoord2fv( uv[ (rotations + 2) % 4]);
		gGL.vertex2i(0, 0);

		gGL.texCoord2fv( uv[ (rotations + 3) % 4]);
		gGL.vertex2i(width, 0);
	}
	gGL.end();
}



//-------------------------------------------------------------------------------
// LLJoystickCameraTrack
//-------------------------------------------------------------------------------

LLJoystickCameraTrack::Params::Params()
{
	held_down_delay.seconds(0.0);
}

LLJoystickCameraTrack::LLJoystickCameraTrack(const LLJoystickCameraTrack::Params& p)
:	LLJoystickCameraRotate(p)
{}


void LLJoystickCameraTrack::onHeldDown()
{
	updateSlop();

	S32 dx = mLastMouse.mX - mFirstMouse.mX + mInitialOffset.mX;
	S32 dy = mLastMouse.mY - mFirstMouse.mY + mInitialOffset.mY;

	if (dx > mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgent.setPanRightKey(getOrbitRate());
	}
	else if (dx < -mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgent.setPanLeftKey(getOrbitRate());
	}

	// over/under rotation
	if (dy > mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgent.setPanUpKey(getOrbitRate());
	}
	else if (dy < -mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgent.setPanDownKey(getOrbitRate());
	}
}



//-------------------------------------------------------------------------------
// LLJoystickCameraZoom
//-------------------------------------------------------------------------------

LLJoystickCameraZoom::LLJoystickCameraZoom(const LLJoystickCameraZoom::Params& p)
:	LLJoystick(p),
	mInTop( FALSE ),
	mInBottom( FALSE ),
	mPlusInImage(p.plus_image),
	mMinusInImage(p.minus_image)
{
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
		gAgentCamera.unlockView();
		gAgent.setOrbitInKey(FAST_RATE);
	}
	else if (dy > mVertSlopNear)
	{
		// Zoom in slow
		gAgentCamera.unlockView();
		gAgent.setOrbitInKey(getOrbitRate());
	}
	else if (dy < -mVertSlopFar)
	{
		// Zoom out fast
		gAgentCamera.unlockView();
		gAgent.setOrbitOutKey(FAST_RATE);
	}
	else if (dy < -mVertSlopNear)
	{
		// Zoom out slow
		gAgentCamera.unlockView();
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
	if( mInTop )
	{
		mPlusInImage->draw(0,0);
	}
	else
	if( mInBottom )
	{
		mMinusInImage->draw(0,0);
	}
	else
	{
		getImageUnselected()->draw( 0, 0 );
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
