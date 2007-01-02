/** 
 * @file llpanelvolume.h
 * @brief Object editing (position, scale, etc.) in the tools floater
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPANELVOLUME_H
#define LL_LLPANELVOLUME_H

#include "v3math.h"
#include "llpanel.h"
#include "llmemory.h"
#include "llvolume.h"

class LLSpinCtrl;
class LLCheckBoxCtrl;
class LLTextBox;
class LLUICtrl;
class LLButton;
class LLViewerObject;
class LLComboBox;
class LLPanelInventory;
class LLColorSwatchCtrl;

class LLPanelVolume : public LLPanel
{
public:
	LLPanelVolume(const std::string& name);
	virtual ~LLPanelVolume();

	virtual void	draw();
	virtual void 	clearCtrls();

	virtual BOOL	postBuild();

	void			refresh();

	void			sendIsLight();
	void			sendIsFlexible();

	static BOOL		precommitValidate(LLUICtrl* ctrl,void* userdata);
	
	static void 	onCommitIsLight(		LLUICtrl* ctrl, void* userdata);
	static void 	onCommitLight(			LLUICtrl* ctrl, void* userdata);
	static void 	onCommitIsFlexible(		LLUICtrl* ctrl, void* userdata);
	static void 	onCommitFlexible(		LLUICtrl* ctrl, void* userdata);

	static void		onLightCancelColor(LLUICtrl* ctrl, void* userdata);
	static void		onLightSelectColor(LLUICtrl* ctrl, void* userdata);

protected:
	void			getState();

protected:
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

	LLColor4		mLightSavedColor;
	LLPointer<LLViewerObject> mObject;
	LLPointer<LLViewerObject> mRootObject;
};

#endif
