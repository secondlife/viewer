/** 
 * @file lltoolcomp.h
 * @brief Composite tools
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

#ifndef LL_TOOLCOMP_H
#define LL_TOOLCOMP_H

#include "lltool.h"

class LLManip;
class LLToolSelectRect;
class LLToolPlacer;
class LLPickInfo;

class LLView;
class LLTextBox;

//-----------------------------------------------------------------------
// LLToolComposite

class LLToolComposite : public LLTool
{
public:
	LLToolComposite(const std::string& name);

    virtual bool			handleMouseDown(S32 x, S32 y, MASK mask) = 0;	// Sets the current tool
    virtual bool			handleMouseUp(S32 x, S32 y, MASK mask);			// Returns to the default tool
	virtual bool			handleDoubleClick(S32 x, S32 y, MASK mask) = 0;

	// Map virtual functions to the currently active internal tool
    virtual bool			handleHover(S32 x, S32 y, MASK mask)			{ return mCur->handleHover( x, y, mask ); }
	virtual bool			handleScrollWheel(S32 x, S32 y, S32 clicks)		{ return mCur->handleScrollWheel( x, y, clicks ); }
	virtual bool			handleRightMouseDown(S32 x, S32 y, MASK mask)	{ return mCur->handleRightMouseDown( x, y, mask ); }

	virtual LLViewerObject*	getEditingObject()								{ return mCur->getEditingObject(); }
	virtual LLVector3d		getEditingPointGlobal()							{ return mCur->getEditingPointGlobal(); }
	virtual bool			isEditing()										{ return mCur->isEditing(); }
	virtual void			stopEditing()									{ mCur->stopEditing(); mCur = mDefault; }

	virtual bool			clipMouseWhenDown()								{ return mCur->clipMouseWhenDown(); }

	virtual void			handleSelect();
	virtual void			handleDeselect();

	virtual void			render()										{ mCur->render(); }
	virtual void			draw()											{ mCur->draw(); }

	virtual bool			handleKey(KEY key, MASK mask)					{ return mCur->handleKey( key, mask ); }

	virtual void			onMouseCaptureLost();

	virtual void			screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const
								{ mCur->screenPointToLocal(screen_x, screen_y, local_x, local_y); }

	virtual void			localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const
								{ mCur->localPointToScreen(local_x, local_y, screen_x, screen_y); }

	bool					isSelecting();
	LLTool*					getCurrentTool()								{ return mCur; }

protected:
	void					setCurrentTool( LLTool* new_tool );
	// In hover handler, call this to auto-switch tools
	void					setToolFromMask( MASK mask, LLTool *normal );

protected:
	LLTool*					mCur;		// The tool to which we're delegating.
	LLTool*					mDefault;
	bool					mSelected;
	bool					mMouseDown;
	LLManip*				mManip;
	LLToolSelectRect*		mSelectRect;

public:
	static const std::string sNameComp;
};


//-----------------------------------------------------------------------
// LLToolCompTranslate

class LLToolCompInspect : public LLToolComposite, public LLSingleton<LLToolCompInspect>
{
	LLSINGLETON(LLToolCompInspect);
	virtual ~LLToolCompInspect();
public:

	// Overridden from LLToolComposite
    virtual bool		handleMouseDown(S32 x, S32 y, MASK mask) override;
	virtual bool		handleMouseUp(S32 x, S32 y, MASK mask) override;
    virtual bool		handleDoubleClick(S32 x, S32 y, MASK mask) override;
	virtual bool		handleKey(KEY key, MASK mask) override;
	virtual void		onMouseCaptureLost() override;
			void		keyUp(KEY key, MASK mask);

	static void pickCallback(const LLPickInfo& pick_info);

	bool isToolCameraActive() const { return mIsToolCameraActive; }

private:
	bool mIsToolCameraActive;
};

//-----------------------------------------------------------------------
// LLToolCompTranslate

class LLToolCompTranslate : public LLToolComposite, public LLSingleton<LLToolCompTranslate>
{
	LLSINGLETON(LLToolCompTranslate);
	virtual ~LLToolCompTranslate();
public:

	// Overridden from LLToolComposite
	virtual bool		handleMouseDown(S32 x, S32 y, MASK mask) override;
	virtual bool		handleDoubleClick(S32 x, S32 y, MASK mask) override;
	virtual bool		handleHover(S32 x, S32 y, MASK mask) override;
	virtual bool		handleMouseUp(S32 x, S32 y, MASK mask) override;			// Returns to the default tool
	virtual void		render() override;

	virtual LLTool*		getOverrideTool(MASK mask) override;

	static void pickCallback(const LLPickInfo& pick_info);
};

//-----------------------------------------------------------------------
// LLToolCompScale

class LLToolCompScale : public LLToolComposite, public LLSingleton<LLToolCompScale>
{
	LLSINGLETON(LLToolCompScale);
	virtual ~LLToolCompScale();
public:

	// Overridden from LLToolComposite
    virtual bool		handleMouseDown(S32 x, S32 y, MASK mask) override;
    virtual bool		handleDoubleClick(S32 x, S32 y, MASK mask) override;
    virtual bool		handleHover(S32 x, S32 y, MASK mask) override;
	virtual bool		handleMouseUp(S32 x, S32 y, MASK mask) override;			// Returns to the default tool
	virtual void		render() override;

	virtual LLTool*		getOverrideTool(MASK mask) override;
	
	static void pickCallback(const LLPickInfo& pick_info);
};


//-----------------------------------------------------------------------
// LLToolCompRotate

class LLToolCompRotate : public LLToolComposite, public LLSingleton<LLToolCompRotate>
{
	LLSINGLETON(LLToolCompRotate);
	virtual ~LLToolCompRotate();
public:

	// Overridden from LLToolComposite
    virtual bool		handleMouseDown(S32 x, S32 y, MASK mask) override;
    virtual bool		handleDoubleClick(S32 x, S32 y, MASK mask) override;
    virtual bool		handleHover(S32 x, S32 y, MASK mask) override;
	virtual bool		handleMouseUp(S32 x, S32 y, MASK mask) override;
	virtual void		render() override;

	virtual LLTool*		getOverrideTool(MASK mask) override;

	static void pickCallback(const LLPickInfo& pick_info);

protected:
};

//-----------------------------------------------------------------------
// LLToolCompCreate

class LLToolCompCreate : public LLToolComposite, public LLSingleton<LLToolCompCreate>
{
	LLSINGLETON(LLToolCompCreate);
	virtual ~LLToolCompCreate();
public:

	// Overridden from LLToolComposite
    virtual bool		handleMouseDown(S32 x, S32 y, MASK mask) override;
    virtual bool		handleDoubleClick(S32 x, S32 y, MASK mask) override;
	virtual bool		handleMouseUp(S32 x, S32 y, MASK mask) override;
	
	static void pickCallback(const LLPickInfo& pick_info);
protected:
	LLToolPlacer*		mPlacer;
	bool				mObjectPlacedOnMouseDown;
};


//-----------------------------------------------------------------------
// LLToolCompGun

class LLToolGun;
class LLToolGrabBase;
class LLToolSelect;

class LLToolCompGun : public LLToolComposite, public LLSingleton<LLToolCompGun>
{
	LLSINGLETON(LLToolCompGun);
	virtual ~LLToolCompGun();
public:

	// Overridden from LLToolComposite
    virtual bool			handleHover(S32 x, S32 y, MASK mask) override;
	virtual bool			handleMouseDown(S32 x, S32 y, MASK mask) override;
	virtual bool			handleDoubleClick(S32 x, S32 y, MASK mask) override;
	virtual bool			handleRightMouseDown(S32 x, S32 y, MASK mask) override;
	virtual bool			handleMouseUp(S32 x, S32 y, MASK mask) override;
	virtual bool			handleScrollWheel(S32 x, S32 y, S32 clicks) override;
	virtual void			onMouseCaptureLost() override;
	virtual void			handleSelect() override;
	virtual void			handleDeselect() override;
	virtual LLTool*			getOverrideTool(MASK mask) override { return NULL; }

protected:
	LLToolGun*			mGun;
	LLToolGrabBase*		mGrab;
	LLTool*				mNull;
};


#endif  // LL_TOOLCOMP_H
