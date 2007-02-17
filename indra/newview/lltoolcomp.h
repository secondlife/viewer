/** 
 * @file lltoolcomp.h
 * @brief Composite tools
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_TOOLCOMP_H
#define LL_TOOLCOMP_H

#include "lltool.h"

class LLManip;
class LLToolSelectRect;
class LLToolPlacer;

class LLView;
class LLTextBox;

//-----------------------------------------------------------------------
// LLToolComposite

class LLToolComposite : public LLTool
{
public:
	LLToolComposite(const LLString& name);

    virtual BOOL			handleMouseDown(S32 x, S32 y, MASK mask) = 0;	// Sets the current tool
    virtual BOOL			handleMouseUp(S32 x, S32 y, MASK mask);			// Returns to the default tool
	virtual BOOL			handleDoubleClick(S32 x, S32 y, MASK mask) = 0;

	// Map virtual functions to the currently active internal tool
    virtual BOOL			handleHover(S32 x, S32 y, MASK mask)			{ return mCur->handleHover( x, y, mask ); }
	virtual BOOL			handleScrollWheel(S32 x, S32 y, S32 clicks)		{ return mCur->handleScrollWheel( x, y, clicks ); }
	virtual BOOL			handleRightMouseDown(S32 x, S32 y, MASK mask)	{ return mCur->handleRightMouseDown( x, y, mask ); }

	virtual LLViewerObject*	getEditingObject()								{ return mCur->getEditingObject(); }
	virtual LLVector3d		getEditingPointGlobal()							{ return mCur->getEditingPointGlobal(); }
	virtual BOOL			isEditing()										{ return mCur->isEditing(); }
	virtual void			stopEditing()									{ mCur->stopEditing(); mCur = mDefault; }

	virtual BOOL			clipMouseWhenDown()								{ return mCur->clipMouseWhenDown(); }

	virtual void			handleSelect();
	virtual void			handleDeselect()								{ mCur->handleDeselect(); mCur = mDefault; mSelected = FALSE; }

	virtual void			render()										{ mCur->render(); }
	virtual void			draw()											{ mCur->draw(); }

	virtual BOOL			handleKey(KEY key, MASK mask)					{ return mCur->handleKey( key, mask ); }

	virtual void			onMouseCaptureLost();

	virtual void			screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const
								{ mCur->screenPointToLocal(screen_x, screen_y, local_x, local_y); }

	virtual void			localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const
								{ mCur->localPointToScreen(local_x, local_y, screen_x, screen_y); }

	BOOL					isSelecting();
protected:
	void					setCurrentTool( LLTool* new_tool );
	LLTool*					getCurrentTool()								{ return mCur; }
	// In hover handler, call this to auto-switch tools
	void					setToolFromMask( MASK mask, LLTool *normal );

protected:
	LLTool*					mCur;		// The tool to which we're delegating.
	LLTool*					mDefault;
	BOOL					mSelected;
	BOOL					mMouseDown;
	LLManip*				mManip;
	LLToolSelectRect*		mSelectRect;

public:
	static const LLString sNameComp;
};


//-----------------------------------------------------------------------
// LLToolCompTranslate

class LLToolCompInspect : public LLToolComposite
{
public:
	LLToolCompInspect();
	virtual ~LLToolCompInspect();

	// Overridden from LLToolComposite
    virtual BOOL		handleMouseDown(S32 x, S32 y, MASK mask);
    virtual BOOL		handleDoubleClick(S32 x, S32 y, MASK mask);

	static void pickCallback(S32 x, S32 y, MASK mask);
};

//-----------------------------------------------------------------------
// LLToolCompTranslate

class LLToolCompTranslate : public LLToolComposite
{
public:
	LLToolCompTranslate();
	virtual ~LLToolCompTranslate();

	// Overridden from LLToolComposite
	virtual BOOL		handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL		handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual BOOL		handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL		handleMouseUp(S32 x, S32 y, MASK mask);			// Returns to the default tool
	virtual void		render();

	virtual LLTool*		getOverrideTool(MASK mask);

	static void pickCallback(S32 x, S32 y, MASK mask);
};

//-----------------------------------------------------------------------
// LLToolCompScale

class LLToolCompScale : public LLToolComposite
{
public:
	LLToolCompScale();
	virtual ~LLToolCompScale();

	// Overridden from LLToolComposite
    virtual BOOL		handleMouseDown(S32 x, S32 y, MASK mask);
    virtual BOOL		handleDoubleClick(S32 x, S32 y, MASK mask);
    virtual BOOL		handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL		handleMouseUp(S32 x, S32 y, MASK mask);			// Returns to the default tool
	virtual void		render();

	virtual LLTool*		getOverrideTool(MASK mask);
	
	static void pickCallback(S32 x, S32 y, MASK mask);
};


//-----------------------------------------------------------------------
// LLToolCompRotate

class LLToolCompRotate : public LLToolComposite
{
public:
	LLToolCompRotate();
	virtual ~LLToolCompRotate();

	// Overridden from LLToolComposite
    virtual BOOL		handleMouseDown(S32 x, S32 y, MASK mask);
    virtual BOOL		handleDoubleClick(S32 x, S32 y, MASK mask);
    virtual BOOL		handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL		handleMouseUp(S32 x, S32 y, MASK mask);
	virtual void		render();

	virtual LLTool*		getOverrideTool(MASK mask);

	static void pickCallback(S32 x, S32 y, MASK mask);

protected:
};

//-----------------------------------------------------------------------
// LLToolCompCreate

class LLToolCompCreate : public LLToolComposite
{
public:
	LLToolCompCreate();
	virtual ~LLToolCompCreate();

	// Overridden from LLToolComposite
    virtual BOOL		handleMouseDown(S32 x, S32 y, MASK mask);
    virtual BOOL		handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual BOOL		handleMouseUp(S32 x, S32 y, MASK mask);
	
	static void pickCallback(S32 x, S32 y, MASK mask);
protected:
	LLToolPlacer*		mPlacer;
	BOOL				mObjectPlacedOnMouseDown;
};


//-----------------------------------------------------------------------
// LLToolCompGun

class LLToolGun;
class LLToolGrab;
class LLToolSelect;

class LLToolCompGun : public LLToolComposite
{
public:
	LLToolCompGun();
	virtual ~LLToolCompGun();

	// Overridden from LLToolComposite
    virtual BOOL			handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL			handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL			handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual BOOL			handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL			handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL			handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual void			onMouseCaptureLost();
	virtual void			handleSelect();
	virtual void			handleDeselect();

protected:
	LLToolGun*			mGun;
	LLToolGrab*			mGrab;
	LLTool*				mNull;
};

extern LLToolCompInspect	*gToolInspect;
extern LLToolCompTranslate	*gToolTranslate;
extern LLToolCompScale		*gToolStretch;
extern LLToolCompRotate		*gToolRotate;
extern LLToolCompCreate		*gToolCreate;
extern LLToolCompGun		*gToolGun;

#endif  // LL_TOOLCOMP_H
