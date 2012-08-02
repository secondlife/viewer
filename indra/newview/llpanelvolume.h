/** 
 * @file llpanelvolume.h
 * @brief Object editing (position, scale, etc.) in the tools floater
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

#ifndef LL_LLPANELVOLUME_H
#define LL_LLPANELVOLUME_H

#include "v3math.h"
#include "llpanel.h"
#include "llpointer.h"
#include "llvolume.h"

class LLSpinCtrl;
class LLCheckBoxCtrl;
class LLTextBox;
class LLUICtrl;
class LLButton;
class LLViewerObject;
class LLComboBox;
class LLColorSwatchCtrl;

class LLPanelVolume : public LLPanel
{
public:
	LLPanelVolume();
	virtual ~LLPanelVolume();

	virtual void	draw();
	virtual void 	clearCtrls();

	virtual BOOL	postBuild();

	void			refresh();

	void			sendIsLight();
	void			sendIsFlexible();

	static bool		precommitValidate(const LLSD& data);
	
	static void 	onCommitIsLight(		LLUICtrl* ctrl, void* userdata);
	static void 	onCommitLight(			LLUICtrl* ctrl, void* userdata);
	void 			onCommitIsFlexible(		LLUICtrl* ctrl, void* userdata);
	static void 	onCommitFlexible(		LLUICtrl* ctrl, void* userdata);
	static void     onCommitPhysicsParam(       LLUICtrl* ctrl, void* userdata);
	static void 	onCommitMaterial(		LLUICtrl* ctrl, void* userdata);

	void		onLightCancelColor(const LLSD& data);
	void		onLightSelectColor(const LLSD& data);

	void		onLightCancelTexture(const LLSD& data); 
	void		onLightSelectTexture(const LLSD& data);


protected:
	void			getState();
	void			refreshCost();

protected:
	void            sendPhysicsShapeType(LLUICtrl* ctrl, void* userdata);
	void            sendPhysicsGravity(LLUICtrl* ctrl, void* userdata);
	void            sendPhysicsFriction(LLUICtrl* ctrl, void* userdata);
	void            sendPhysicsRestitution(LLUICtrl* ctrl, void* userdata);
	void            sendPhysicsDensity(LLUICtrl* ctrl, void* userdata);

	void            handleResponseChangeToFlexible(const LLSD &pNotification, const LLSD &pResponse);

/*
	LLTextBox*		mLabelSelectSingleMessage;
	// Light
	LLCheckBoxCtrl*	mCheckLight;
	LLCheckBoxCtrl*	mCheckFlexible1D;
	LLTextBox*		mLabelColor;
	LLColorSwatchCtrl* mLightColorSwatch;
	LLSpinCtrl*		mLightIntensity;
	LLSpinCtrl*		mLightRadius;
	LLSpinCtrl*		mLightFalloff;
	LLSpinCtrl*		mLightCutoff;
	// Flexibile
	LLSpinCtrl*		mSpinSections;
	LLSpinCtrl*		mSpinGravity;
	LLSpinCtrl*		mSpinTension;
	LLSpinCtrl*		mSpinFriction;
	LLSpinCtrl*		mSpinWind;
	LLSpinCtrl*		mSpinForce[3];
*/

	S32			mComboMaterialItemCount;
	LLComboBox*		mComboMaterial;
	

	LLColor4		mLightSavedColor;
	LLUUID			mLightSavedTexture;
	LLPointer<LLViewerObject> mObject;
	LLPointer<LLViewerObject> mRootObject;

	LLComboBox*     mComboPhysicsShapeType;
	LLSpinCtrl*     mSpinPhysicsGravity;
	LLSpinCtrl*     mSpinPhysicsFriction;
	LLSpinCtrl*     mSpinPhysicsDensity;
	LLSpinCtrl*     mSpinPhysicsRestitution;
};

#endif
