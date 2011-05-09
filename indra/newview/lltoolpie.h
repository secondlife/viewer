/** 
 * @file lltoolpie.h
 * @brief LLToolPie class header file
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

#ifndef LL_TOOLPIE_H
#define LL_TOOLPIE_H

#include "lltool.h"
#include "lluuid.h"
#include "llviewerwindow.h" // for LLPickInfo
#include "llhudeffectblob.h" // for LLPointer<LLHudEffectBlob>, apparently

class LLViewerObject;
class LLObjectSelection;

class LLToolPie : public LLTool, public LLSingleton<LLToolPie>
{
	LOG_CLASS(LLToolPie);
public:
	LLToolPie( );

	// Virtual functions inherited from LLMouseHandler
	virtual BOOL		handleAnyMouseClick(S32 x, S32 y, MASK mask, EClickType clicktype, BOOL down);
	virtual BOOL		handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL		handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL		handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL		handleRightMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL		handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL		handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual BOOL		handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual BOOL		handleToolTip(S32 x, S32 y, MASK mask);

	virtual void		render();

	virtual void		stopEditing();

	virtual void		onMouseCaptureLost();
	virtual void		handleSelect();
	virtual void		handleDeselect();
	virtual LLTool*		getOverrideTool(MASK mask);

	LLPickInfo&			getPick() { return mPick; }
	U8					getClickAction() { return mClickAction; }
	LLViewerObject*		getClickActionObject() { return mClickActionObject; }
	LLObjectSelection*	getLeftClickSelection() { return (LLObjectSelection*)mLeftClickSelection; }
	void 				resetSelection();
	void				blockClickToWalk() { mBlockClickToWalk = true; }
	void				stopClickToWalk();
	
	static void			selectionPropertiesReceived();

	static void			showAvatarInspector(const LLUUID& avatar_id);
	static void			showObjectInspector(const LLUUID& object_id);
	static void			showObjectInspector(const LLUUID& object_id, const S32& object_face);
	static void			playCurrentMedia(const LLPickInfo& info);
	static void			VisitHomePage(const LLPickInfo& info);
	
private:
	BOOL outsideSlop		(S32 x, S32 y, S32 start_x, S32 start_y);
	BOOL handleLeftClickPick();
	BOOL handleRightClickPick();
	BOOL useClickAction		(MASK mask, LLViewerObject* object,LLViewerObject* parent);
	
	void showVisualContextMenuEffect();
	ECursorType cursorFromObject(LLViewerObject* object);

	bool handleMediaClick(const LLPickInfo& info);
	bool handleMediaHover(const LLPickInfo& info);
	bool handleMediaMouseUp(); 
	BOOL handleTooltipLand(std::string line, std::string tooltip_msg);
	BOOL handleTooltipObject( LLViewerObject* hover_object, std::string line, std::string tooltip_msg);

	void steerCameraWithMouse(S32 x, S32 y);
	void startCameraSteering();
	void stopCameraSteering();
	bool inCameraSteerMode();

private:
	bool				mMouseButtonDown;
	bool				mMouseOutsideSlop;		// for this drag, has mouse moved outside slop region
	S32					mMouseDownX;
	S32					mMouseDownY;
	S32					mMouseSteerX;
	S32					mMouseSteerY;
	LLPointer<LLHUDEffectBlob>	mAutoPilotDestination;
	LLPointer<LLHUDEffectBlob>	mMouseSteerGrabPoint;
	bool				mClockwise;			
	bool				mBlockClickToWalk;
	LLUUID				mMediaMouseCaptureID;
	LLPickInfo			mPick;
	LLPickInfo			mHoverPick;
	LLPickInfo			mSteerPick;
	LLPointer<LLViewerObject> mClickActionObject;
	U8					mClickAction;
	LLSafeHandle<LLObjectSelection> mLeftClickSelection;
	BOOL				mClickActionBuyEnabled;
	BOOL				mClickActionPayEnabled;
};

#endif
