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
#include "llviewerimage.h"

typedef enum e_joystick_quadrant
{
	JQ_ORIGIN,
	JQ_UP,
	JQ_DOWN,
	JQ_LEFT,
	JQ_RIGHT
} EJoystickQuadrant;

class LLJoystick
:	public LLButton
{
public:
	LLJoystick(const std::string& name, LLRect rect,	const std::string &default_image,	const std::string &selected_image, EJoystickQuadrant initial);

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);

	virtual void	onMouseUp() {}
	virtual void	onHeldDown() = 0;
	F32				getElapsedHeldDownTime();

	static void		onHeldDown(void *userdata);		// called by llbutton callback handler
	void            setInitialQuadrant(EJoystickQuadrant initial) { mInitialQuadrant = initial; };
	
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
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
	LLJoystickAgentTurn(const std::string& name, LLRect rect, const std::string &default_image, const std::string &selected_image, EJoystickQuadrant initial)
		: LLJoystick(name, rect, default_image, selected_image, initial)
	{ }

	virtual void	onHeldDown();

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

};


// Slide left and right, move forward and back
class LLJoystickAgentSlide
:	public LLJoystick
{
public:
	LLJoystickAgentSlide(const std::string& name, LLRect rect, const std::string &default_image, const std::string &selected_image, EJoystickQuadrant initial)
		: LLJoystick(name, rect, default_image, selected_image, initial)
	{ }
	
	virtual void	onHeldDown();
	virtual void	onMouseUp();

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
};


// Rotate camera around the focus point
class LLJoystickCameraRotate
:	public LLJoystick
{
public:
	LLJoystickCameraRotate(const std::string& name, LLRect rect, const std::string &out_img, const std::string &in_img);

	virtual void	setToggleState( BOOL left, BOOL top, BOOL right, BOOL bottom );

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual void	onHeldDown();
	virtual void	draw();

protected:
	F32				getOrbitRate();
	virtual void	updateSlop();
	void			drawRotatedImage( const LLImageGL* image, S32 rotations );

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
	LLJoystickCameraTrack(const std::string& name, LLRect rect, const std::string &out_img, const std::string &in_img)
		: LLJoystickCameraRotate(name, rect, out_img, in_img)
	{ }

	virtual void	onHeldDown();
};


// Zoom the camera in and out
class LLJoystickCameraZoom
:	public LLJoystick
{
public:
	LLJoystickCameraZoom(const std::string& name, LLRect rect, const std::string &out_img, const std::string &plus_in_img, const std::string &minus_in_img);

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
