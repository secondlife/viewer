/** 
 * @file lljoystickbutton.h
 * @brief LLJoystick class definition
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
			label = "";
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
			held_down_delay.seconds(0.0);
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


// Zoom the camera in and out
class LLJoystickCameraZoom
:	public LLJoystick
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLJoystick::Params>
	{
		Optional<LLUIImage*>	plus_image;
		Optional<LLUIImage*>	minus_image;

		Params()
		: plus_image ("plus_image", NULL),
		  minus_image ("minus_image", NULL)
		{
			held_down_delay.seconds(0.0);
		}
	};
	LLJoystickCameraZoom(const Params&);

	virtual void	setToggleState( BOOL top, BOOL bottom );

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual void	onHeldDown();
	virtual void	draw();

protected:
	virtual void updateSlop();
	F32				getOrbitRate();

protected:
	BOOL			mInTop;
	BOOL			mInBottom;
	LLUIImagePtr	mPlusInImage;
	LLUIImagePtr	mMinusInImage;
};



#endif  // LL_LLJOYSTICKBUTTON_H
