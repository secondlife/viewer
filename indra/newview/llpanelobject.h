/** 
 * @file llpanelobject.h
 * @brief Object editing (position, scale, etc.) in the tools floater
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPANELOBJECT_H
#define LL_LLPANELOBJECT_H

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

class LLPanelObject : public LLPanel
{
public:
	LLPanelObject(const std::string& name);
	virtual ~LLPanelObject();

	virtual BOOL	postBuild();
	virtual void	draw();
	virtual void 	clearCtrls();

	void			refresh();

	static BOOL		precommitValidate(LLUICtrl* ctrl,void* userdata);
	
	static void		onCommitLock(LLUICtrl *ctrl, void *data);
	static void 	onCommitPosition(		LLUICtrl* ctrl, void* userdata);
	static void 	onCommitScale(			LLUICtrl* ctrl, void* userdata);
	static void 	onCommitRotation(		LLUICtrl* ctrl, void* userdata);
	static void 	onCommitPhysics(		LLUICtrl* ctrl, void* userdata);
	static void 	onCommitTemporary(		LLUICtrl* ctrl, void* userdata);
	static void 	onCommitPhantom(		LLUICtrl* ctrl, void* userdata);
	static void 	onCommitCastShadows(	LLUICtrl* ctrl, void* userdata);

	static void 	onCommitParametric(LLUICtrl* ctrl, void* userdata);

	static void 	onCommitMaterial(		LLUICtrl* ctrl, void* userdata);
	
protected:
	void			getState();

	void			sendRotation();
	void			sendScale();
	void			sendPosition();
	void			sendIsPhysical();
	void			sendIsTemporary();
	void			sendIsPhantom();
	void			sendCastShadows();

	void 			getVolumeParams(LLVolumeParams& volume_params);
	
protected:
	S32				mComboMaterialItemCount;

	LLTextBox*		mLabelMaterial;
	LLComboBox*		mComboMaterial;
	
	// Per-object options
	LLTextBox*		mLabelBaseType;
	LLComboBox*		mComboBaseType;

	LLTextBox*		mLabelCut;
	LLSpinCtrl*		mSpinCutBegin;
	LLSpinCtrl*		mSpinCutEnd;

	LLTextBox*		mLabelHollow;
	LLSpinCtrl*		mSpinHollow;

	LLTextBox*		mLabelHoleType;
	LLComboBox*		mComboHoleType;

	LLTextBox*		mLabelTwist;
	LLSpinCtrl*		mSpinTwist;
	LLSpinCtrl*		mSpinTwistBegin;

	LLSpinCtrl*		mSpinScaleX;
	LLSpinCtrl*		mSpinScaleY;
	
	LLTextBox*		mLabelSkew;
	LLSpinCtrl*		mSpinSkew;

	LLTextBox*		mLabelShear;
	LLSpinCtrl*		mSpinShearX;
	LLSpinCtrl*		mSpinShearY;

	// Advanced Path
	LLSpinCtrl*		mCtrlPathBegin;
	LLSpinCtrl*		mCtrlPathEnd;

	LLTextBox*		mLabelTaper;
	LLSpinCtrl*		mSpinTaperX;
	LLSpinCtrl*		mSpinTaperY;

	LLTextBox*		mLabelRadiusOffset;
	LLSpinCtrl*		mSpinRadiusOffset;

	LLTextBox*		mLabelRevolutions;
	LLSpinCtrl*		mSpinRevolutions;

	LLTextBox*		mLabelPosition;
	LLSpinCtrl*		mCtrlPosX;
	LLSpinCtrl*		mCtrlPosY;
	LLSpinCtrl*		mCtrlPosZ;

	LLTextBox*		mLabelSize;
	LLSpinCtrl*		mCtrlScaleX;
	LLSpinCtrl*		mCtrlScaleY;
	LLSpinCtrl*		mCtrlScaleZ;

	LLTextBox*		mLabelRotation;
	LLSpinCtrl*		mCtrlRotX;
	LLSpinCtrl*		mCtrlRotY;
	LLSpinCtrl*		mCtrlRotZ;

	LLCheckBoxCtrl	*mCheckLock;
	LLCheckBoxCtrl	*mCheckPhysics;
	LLCheckBoxCtrl	*mCheckTemporary;
	LLCheckBoxCtrl	*mCheckPhantom;
	LLCheckBoxCtrl	*mCheckCastShadows;

	LLVector3		mCurEulerDegrees;		// to avoid sending rotation when not changed
	BOOL			mIsPhysical;			// to avoid sending "physical" when not changed
	BOOL			mIsTemporary;			// to avoid sending "temporary" when not changed
	BOOL			mIsPhantom;				// to avoid sending "phantom" when not changed
	BOOL			mCastShadows;			// to avoid sending "cast shadows" when not changed
	S32				mSelectedType;			// So we know what selected type we last were
	
	LLPointer<LLViewerObject> mObject;
	LLPointer<LLViewerObject> mRootObject;
};

#endif
