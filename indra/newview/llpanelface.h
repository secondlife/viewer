/** 
 * @file llpanelface.h
 * @brief Panel in the tools floater for editing face textures, colors, etc.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPANELFACE_H
#define LL_LLPANELFACE_H

#include "v4color.h"
#include "llpanel.h"

class LLButton;
class LLCheckBoxCtrl;
class LLColorSwatchCtrl;
class LLComboBox;
class LLInventoryItem;
class LLLineEditor;
class LLSpinCtrl;
class LLTextBox;
class LLTextureCtrl;
class LLUICtrl;
class LLViewerObject;

class LLPanelFace : public LLPanel
{
public:
	virtual BOOL	postBuild();
	LLPanelFace(const std::string& name);
	virtual ~LLPanelFace();

	void			refresh();

protected:
	// Given a callback function that returns an F32, figures out
	// if that F32 is the same for all selected faces.  "value"
	// contains the identical value, or the first object's value.
	BOOL			allFacesSameValue( F32 (get_face_value(LLViewerObject*, S32)), F32 *value);

	void			getState();

	void			sendTexture();			// applies and sends texture
	void			sendTextureInfo();		// applies and sends texture scale, offset, etc.
	void			sendColor();			// applies and sends color
	void			sendAlpha();			// applies and sends transparency
	void			sendBump();				// applies and sends bump map
	void			sendTexGen();				// applies and sends bump map
	void			sendShiny();			// applies and sends shininess
	void			sendFullbright();		// applies and sends full bright
	#if LL_MOZILLA_ENABLED
	void			sendMediaInfo();		// web page settings and URL
	#endif

	// this function is to return TRUE if the dra should succeed.
	static BOOL onDragTexture(LLUICtrl* ctrl, LLInventoryItem* item, void* ud);

	static void 	onCommitTexture(		LLUICtrl* ctrl, void* userdata);
	static void 	onCancelTexture(		LLUICtrl* ctrl, void* userdata);
	static void 	onSelectTexture(		LLUICtrl* ctrl, void* userdata);
	static void 	onCommitTextureInfo(	LLUICtrl* ctrl, void* userdata);
	static void 	onCommitColor(			LLUICtrl* ctrl, void* userdata);
	static void 	onCommitAlpha(			LLUICtrl* ctrl, void* userdata);
	static void 	onCancelColor(			LLUICtrl* ctrl, void* userdata);
	static void 	onSelectColor(			LLUICtrl* ctrl, void* userdata);
	static void		onCommitBump(			LLUICtrl* ctrl, void* userdata);
	static void		onCommitTexGen(			LLUICtrl* ctrl, void* userdata);
	static void		onCommitShiny(			LLUICtrl* ctrl, void* userdata);
	static void		onCommitFullbright(		LLUICtrl* ctrl, void* userdata);
	#if LL_MOZILLA_ENABLED
	static void		onCommitMediaInfo(		LLUICtrl* ctrl, void* data);
	#endif

	static void		onClickApply(void*);
	static void		onClickAutoFix(void*);

	static F32		valueScaleS(LLViewerObject* object, S32 face);
	static F32		valueScaleT(LLViewerObject* object, S32 face);
	static F32		valueOffsetS(LLViewerObject* object, S32 face);
	static F32		valueOffsetT(LLViewerObject* object, S32 face);
	static F32		valueTexRotation(LLViewerObject* object, S32 face);
	static F32		valueRepeatsPerMeter(LLViewerObject* object, S32 face);
	static F32		valueBump(LLViewerObject* object, S32 face);
	static F32		valueTexGen(LLViewerObject* object, S32 face);
	static F32		valueShiny(LLViewerObject* object, S32 face);
	static F32		valueFullbright(LLViewerObject* object, S32 face);

protected:
	//LLTextureCtrl*	mTextureCtrl;
	//LLColorSwatchCtrl*	mColorSwatch;

	//#if LL_MOZILLA_ENABLED
	//LLTextBox*		mLabelMediaType;
	//LLComboBox*		mComboMediaType;
	//LLTextBox*		mLabelMediaURL;
	//LLLineEditor*	mLineMediaURL;
	//#endif

	//LLTextBox		*mLabelTexScale;
	//LLSpinCtrl		*mCtrlTexScaleS;
	//LLSpinCtrl		*mCtrlTexScaleT;

	//LLCheckBoxCtrl	*mCheckFlipScaleS;
	//LLCheckBoxCtrl	*mCheckFlipScaleT;

	//LLTextBox		*mLabelTexOffset;
	//LLSpinCtrl		*mCtrlTexOffsetS;
	//LLSpinCtrl		*mCtrlTexOffsetT;

	//LLTextBox		*mLabelTexRotation;
	//LLSpinCtrl		*mCtrlTexRotation;

	//LLTextBox*		mLabelTexGen;
	//LLComboBox*		mComboTexGen;

	//LLTextBox*		mLabelShininess;
	//LLComboBox*		mComboShininess;

	//LLTextBox*		mLabelBumpiness;
	//LLComboBox*		mComboBumpiness;

	//LLCheckBoxCtrl	*mCheckFullbright;
	//
	//LLTextBox*		mLabelColorTransp;
	//LLSpinCtrl*		mCtrlColorTransp;		// transparency = 1 - alpha

	//LLTextBox*		mLabelRepeatsPerMeter;
	//LLSpinCtrl*		mCtrlRepeatsPerMeter;
	//LLButton*		mBtnApply;

	//LLTextBox*		mLabelTexAutoFix;
	//LLButton*		mBtnAutoFix;
};

#endif
