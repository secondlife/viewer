/** 
 * @file llfloatermodelwizard.h
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LLFLOATERMODELWIZARD_H
#define LLFLOATERMODELWIZARD_H


#include "llmeshrepository.h"
#include "llmodel.h"
#include "llthread.h"


class LLModelPreview;


class LLFloaterModelWizard : public LLFloater
{
public:
	
	class DecompRequest : public LLPhysicsDecomp::Request
	{
	public:
		S32 mContinue;
		LLPointer<LLModel> mModel;
		
		DecompRequest(const std::string& stage, LLModel* mdl);
		virtual S32 statusCallback(const char* status, S32 p1, S32 p2);
		virtual void completed();
		
	};
	
	static LLFloaterModelWizard* sInstance;

	LLFloaterModelWizard(const LLSD& key);
	virtual ~LLFloaterModelWizard();
	/*virtual*/	BOOL	postBuild();
	void			draw();
	
	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	BOOL handleHover(S32 x, S32 y, MASK mask);
	BOOL handleScrollWheel(S32 x, S32 y, S32 clicks); 

	
	void initDecompControls();
	
	LLPhysicsDecomp::decomp_params mDecompParams;
	std::set<LLPointer<DecompRequest> > mCurRequest;
	std::string mStatusMessage;
	static void executePhysicsStage(std::string stage_name);

private:
	enum EWizardState
	{
		CHOOSE_FILE = 0,
		OPTIMIZE,
		PHYSICS,
		REVIEW,
		UPLOAD
	};

	void setState(int state);
	void setButtons(int state);
	void onClickCancel();
	void onClickBack();
	void onClickNext();
	bool onEnableNext();
	bool onEnableBack();
	void loadModel();
	void onPreviewLODCommit(LLUICtrl*);
	void onAccuracyPerformance(const LLSD& data);
	void onUpload();

	LLModelPreview*	mModelPreview;
	LLRect			mPreviewRect;
	int mState;

	S32				mLastMouseX;
	S32				mLastMouseY;


};


#endif
