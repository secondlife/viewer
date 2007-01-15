/** 
 * @file lljoystickbutton.h
 * @brief LLJoystick class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLJOYSTICKBUTTON_H
#define LL_LLJOYSTICKBUTTON_H

#include "llbutton.h"
#include "llcoord.h"

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
	LLJoystick(const LLString& name, LLRect rect,	const LLString &default_image,	const LLString &selected_image, EJoystickQuadrant initial);

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);

	virtual void	onMouseUp() {}
	virtual void	onHeldDown() = 0;
	F32				getElapsedHeldDownTime();

	static void		onHeldDown(void *userdata);		// called by llbutton callback handler
	void            setInitialQuadrant(EJoystickQuadrant initial) { mInitialQuadrant = initial; };
	
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLString nameFromQuadrant(const EJoystickQuadrant quadrant);
	static EJoystickQuadrant quadrantFromName(const LLString& name);
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
	LLJoystickAgentTurn(const LLString& name, LLRect rect, const LLString &default_image, const LLString &selected_image, EJoystickQuadrant initial)
		: LLJoystick(name, rect, default_image, selected_image, initial)
	{ }

	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_JOYSTICK_TURN; }
	virtual LLString getWidgetTag() const { return LL_JOYSTICK_TURN; }

	virtual void	onHeldDown();

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

};


// Slide left and right, move forward and back
class LLJoystickAgentSlide
:	public LLJoystick
{
public:
	LLJoystickAgentSlide(const LLString& name, LLRect rect, const LLString &default_image, const LLString &selected_image, EJoystickQuadrant initial)
		: LLJoystick(name, rect, default_image, selected_image, initial)
	{ }
	
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_JOYSTICK_SLIDE; }
	virtual LLString getWidgetTag() const { return LL_JOYSTICK_SLIDE; }


	virtual void	onHeldDown();
	virtual void	onMouseUp();

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
};


// Rotate camera around the focus point
class LLJoystickCameraRotate
:	public LLJoystick
{
public:
	LLJoystickCameraRotate(const LLString& name, LLRect rect, const LLString &out_img, const LLString &in_img);

	virtual void	setToggleState( BOOL left, BOOL top, BOOL right, BOOL bottom );

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual void	onHeldDown();
	virtual void	draw();

protected:
	F32				getOrbitRate();
	virtual void	updateSlop();
	void			drawRotatedImage( LLImageGL* image, S32 rotations );

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
	LLJoystickCameraTrack(const LLString& name, LLRect rect, const LLString &out_img, const LLString &in_img)
		: LLJoystickCameraRotate(name, rect, out_img, in_img)
	{ }

	virtual void	onHeldDown();
};


// Zoom the camera in and out
class LLJoystickCameraZoom
:	public LLJoystick
{
public:
	LLJoystickCameraZoom(const LLString& name, LLRect rect, const LLString &out_img, const LLString &plus_in_img, const LLString &minus_in_img);

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
	LLPointer<LLViewerImage>	mPlusInImage;
	LLPointer<LLViewerImage>	mMinusInImage;
};



#endif  // LL_LLJOYSTICKBUTTON_H
