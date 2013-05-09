/** 
 * @file lljoystickbutton.h
 * @brief LLJoystick class definition
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

#ifndef LL_LLJOYSTICKBUTTON_H
#define LL_LLJOYSTICKBUTTON_H

#include "llbutton.h"
#include "llcoord.h"
#include "llviewertexture.h"

typedef enum e_joystick_quadrant
{
	JQ_ORIGIN,
	JQ_UP,
	JQ_DOWN,
	JQ_LEFT,
	JQ_RIGHT
} EJoystickQuadrant;

struct QuadrantNames : public LLInitParam::TypeValuesHelper<EJoystickQuadrant, QuadrantNames>
{
	static void declareValues();
};

class LLJoystick
:	public LLButton
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLButton::Params>
	{
		Optional<EJoystickQuadrant, QuadrantNames> quadrant;

		Params()
		:	quadrant("quadrant", JQ_ORIGIN)
		{
			changeDefault(label, "");
		}
	};
	LLJoystick(const Params&);

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);

	virtual void	onMouseUp() {}
	virtual void	onHeldDown() = 0;
	F32				getElapsedHeldDownTime();

	static void		onBtnHeldDown(void *userdata);		// called by llbutton callback handler
	void            setInitialQuadrant(EJoystickQuadrant initial) { mInitialQuadrant = initial; };

	/**
	 * Checks if click location is inside joystick circle.
	 *
	 * Image containing circle is square and this square has adherent points with joystick
	 * circle. Make sure to change method according to shape other than square. 
	 */
	bool			pointInCircle(S32 x, S32 y) const;
	
	static std::string nameFromQuadrant(const EJoystickQuadrant quadrant);
	static EJoystickQuadrant quadrantFromName(const std::string& name);
	static EJoystickQuadrant selectQuadrant(LLXMLNodePtr node);


protected:
	virtual void	updateSlop();					// recompute slop margins

protected:
	EJoystickQuadrant	mInitialQuadrant;			// mousedown = click in this quadrant
	LLCoordGL			mInitialOffset;				// pretend mouse started here
	LLCoordGL			mLastMouse;					// where was mouse on last hover event
	LLCoordGL			mFirstMouse;				// when mouse clicked, where was it
	S32					mVertSlopNear;				// where the slop regions end
	S32					mVertSlopFar;				// where the slop regions end
	S32					mHorizSlopNear;				// where the slop regions end
	S32					mHorizSlopFar;				// where the slop regions end
	BOOL				mHeldDown;
	LLFrameTimer		mHeldDownTimer;
};


// Turn agent left and right, move forward and back
class LLJoystickAgentTurn
:	public LLJoystick
{
public:
	struct Params : public LLJoystick::Params {};
	LLJoystickAgentTurn(const Params& p) : LLJoystick(p) {}
	virtual void	onHeldDown();
};


// Slide left and right, move forward and back
class LLJoystickAgentSlide
:	public LLJoystick
{
public:
	struct Params : public LLJoystick::Params {};
	LLJoystickAgentSlide(const Params& p) : LLJoystick(p) {}

	virtual void	onHeldDown();
	virtual void	onMouseUp();
};


// Rotate camera around the focus point
class LLJoystickCameraRotate
:	public LLJoystick
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLJoystick::Params>
	{
		Params()
		{
			changeDefault(held_down_delay.seconds, 0.0);
		}
	};

	LLJoystickCameraRotate(const LLJoystickCameraRotate::Params&);

	virtual void	setToggleState( BOOL left, BOOL top, BOOL right, BOOL bottom );

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual void	onHeldDown();
	virtual void	draw();

protected:
	F32				getOrbitRate();
	virtual void	updateSlop();
	void			drawRotatedImage( LLPointer<LLUIImage> image, S32 rotations );

protected:
	BOOL			mInLeft;
	BOOL			mInTop;
	BOOL			mInRight;
	BOOL			mInBottom;
};


// Track the camera focus point forward/backward and side to side
class LLJoystickCameraTrack
:	public LLJoystickCameraRotate
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLJoystickCameraRotate::Params>
	{
		Params();
	};

	LLJoystickCameraTrack(const LLJoystickCameraTrack::Params&);
	virtual void	onHeldDown();
};

#endif  // LL_LLJOYSTICKBUTTON_H
