/** 
 * @file llpanelface.cpp
 * @brief Panel in the tools floater for editing face textures, colors, etc.
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

#include "llviewerprecompiledheaders.h"

// file include
#include "llpanelface.h"
 
// library includes
#include "llcalc.h"
#include "llerror.h"
#include "llfocusmgr.h"
#include "llrect.h"
#include "llstring.h"
#include "llfontgl.h"

// project includes
#include "llagentdata.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "lldrawpoolbump.h"
#include "llface.h"
#include "lllineeditor.h"
#include "llmaterialmgr.h"
#include "llmediaentry.h"
#include "llnotificationsutil.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "lltextureentry.h"
#include "lltooldraganddrop.h"
#include "lltrans.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llvovolume.h"
#include "lluictrlfactory.h"
#include "llpluginclassmedia.h"
#include "llviewertexturelist.h"

//
// Constant definitions for comboboxes
// Must match the commbobox definitions in panel_tools_texture.xml
//
const S32 MATMEDIA_MATERIAL = 0;	// Material
const S32 MATMEDIA_MEDIA = 1;		// Media
const S32 MATTYPE_DIFFUSE = 0;		// Diffuse material texture
const S32 MATTYPE_NORMAL = 1;		// Normal map
const S32 MATTYPE_SPECULAR = 2;		// Specular map
const S32 ALPHAMODE_NONE = 0;		// No alpha mask applied
const S32 ALPHAMODE_BLEND = 1;		// Alpha blending mode
const S32 ALPHAMODE_MASK = 2;		// Alpha masking mode
const S32 BUMPY_TEXTURE = 18;		// use supplied normal map
const S32 SHINY_TEXTURE = 4;		// use supplied specular map

//
// "Use texture" label for normal/specular type comboboxes
// Filled in at initialization from translated strings
//
std::string USE_TEXTURE;

//
// Methods
//

BOOL	LLPanelFace::postBuild()
{
	childSetCommitCallback("combobox shininess",&LLPanelFace::onCommitShiny,this);
	childSetCommitCallback("combobox bumpiness",&LLPanelFace::onCommitBump,this);
	childSetCommitCallback("combobox alphamode",&LLPanelFace::onCommitAlphaMode,this);
	childSetCommitCallback("TexScaleU",&LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexScaleV",&LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexRot",&LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("rptctrl",&LLPanelFace::onCommitRepeatsPerMeter, this);
	childSetCommitCallback("checkbox planar align",&LLPanelFace::onCommitPlanarAlign, this);
	childSetCommitCallback("TexOffsetU",LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexOffsetV",LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("bumpyScaleU",&LLPanelFace::onCommitMaterial, this);
	childSetCommitCallback("bumpyScaleV",&LLPanelFace::onCommitMaterial, this);
	childSetCommitCallback("bumpyRot",&LLPanelFace::onCommitMaterial, this);
	childSetCommitCallback("bumpyOffsetU",&LLPanelFace::onCommitMaterial, this);
	childSetCommitCallback("bumpyOffsetV",&LLPanelFace::onCommitMaterial, this);
	childSetCommitCallback("shinyScaleU",&LLPanelFace::onCommitMaterial, this);
	childSetCommitCallback("shinyScaleV",&LLPanelFace::onCommitMaterial, this);
	childSetCommitCallback("shinyRot",&LLPanelFace::onCommitMaterial, this);
	childSetCommitCallback("shinyOffsetU",&LLPanelFace::onCommitMaterial, this);
	childSetCommitCallback("shinyOffsetV",&LLPanelFace::onCommitMaterial, this);
	childSetCommitCallback("glossiness",&LLPanelFace::onCommitMaterial, this);
	childSetCommitCallback("environment",&LLPanelFace::onCommitMaterial, this);
	childSetCommitCallback("maskcutoff",&LLPanelFace::onCommitMaterial, this);
	childSetAction("button align",&LLPanelFace::onClickAutoFix,this);

	LLRect	rect = this->getRect();
	LLTextureCtrl*	mTextureCtrl;
	LLTextureCtrl*	mShinyTextureCtrl;
	LLTextureCtrl*	mBumpyTextureCtrl;
	LLColorSwatchCtrl*	mColorSwatch;
	LLColorSwatchCtrl*	mShinyColorSwatch;

	LLComboBox*		mComboTexGen;
	LLComboBox*		mComboMatMedia;
	LLComboBox*		mComboMatType;

	LLCheckBoxCtrl	*mCheckFullbright;
	
	LLTextBox*		mLabelColorTransp;
	LLSpinCtrl*		mCtrlColorTransp;		// transparency = 1 - alpha

	LLSpinCtrl*     mCtrlGlow;

	setMouseOpaque(FALSE);

	mTextureCtrl = getChild<LLTextureCtrl>("texture control");
	if(mTextureCtrl)
	{
		mTextureCtrl->setDefaultImageAssetID(LLUUID( gSavedSettings.getString( "DefaultObjectTexture" )));
		mTextureCtrl->setCommitCallback( boost::bind(&LLPanelFace::onCommitTexture, this, _2) );
		mTextureCtrl->setOnCancelCallback( boost::bind(&LLPanelFace::onCancelTexture, this, _2) );
		mTextureCtrl->setOnSelectCallback( boost::bind(&LLPanelFace::onSelectTexture, this, _2) );
		mTextureCtrl->setDragCallback(boost::bind(&LLPanelFace::onDragTexture, this, _2));
		mTextureCtrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onTextureSelectionChanged, this, _1));
		mTextureCtrl->setFollowsTop();
		mTextureCtrl->setFollowsLeft();
		mTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
		mTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);
	}

	mShinyTextureCtrl = getChild<LLTextureCtrl>("shinytexture control");
	if(mShinyTextureCtrl)
	{
		mShinyTextureCtrl->setDefaultImageAssetID(LLUUID( gSavedSettings.getString( "DefaultObjectTexture" )));
		mShinyTextureCtrl->setCommitCallback( boost::bind(&LLPanelFace::onCommitMaterialTexture, this, _2) );
		mShinyTextureCtrl->setOnCancelCallback( boost::bind(&LLPanelFace::onCancelMaterialTexture, this, _2) );
		mShinyTextureCtrl->setOnSelectCallback( boost::bind(&LLPanelFace::onSelectMaterialTexture, this, _2) );
		mShinyTextureCtrl->setDragCallback(boost::bind(&LLPanelFace::onDragTexture, this, _2));
		mShinyTextureCtrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onTextureSelectionChanged, this, _1));
		mShinyTextureCtrl->setFollowsTop();
		mShinyTextureCtrl->setFollowsLeft();
		mShinyTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
		mShinyTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);
	}

	mBumpyTextureCtrl = getChild<LLTextureCtrl>("bumpytexture control");
	if(mBumpyTextureCtrl)
	{
		mBumpyTextureCtrl->setDefaultImageAssetID(LLUUID( gSavedSettings.getString( "DefaultObjectTexture" )));
		mBumpyTextureCtrl->setCommitCallback( boost::bind(&LLPanelFace::onCommitMaterialTexture, this, _2) );
		mBumpyTextureCtrl->setOnCancelCallback( boost::bind(&LLPanelFace::onCancelMaterialTexture, this, _2) );
		mBumpyTextureCtrl->setOnSelectCallback( boost::bind(&LLPanelFace::onSelectMaterialTexture, this, _2) );
		mBumpyTextureCtrl->setDragCallback(boost::bind(&LLPanelFace::onDragTexture, this, _2));
		mBumpyTextureCtrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onTextureSelectionChanged, this, _1));
		mBumpyTextureCtrl->setFollowsTop();
		mBumpyTextureCtrl->setFollowsLeft();
		mBumpyTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
		mBumpyTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);
	}

	mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	if(mColorSwatch)
	{
		mColorSwatch->setCommitCallback(boost::bind(&LLPanelFace::onCommitColor, this, _2));
		mColorSwatch->setOnCancelCallback(boost::bind(&LLPanelFace::onCancelColor, this, _2));
		mColorSwatch->setOnSelectCallback(boost::bind(&LLPanelFace::onSelectColor, this, _2));
		mColorSwatch->setFollowsTop();
		mColorSwatch->setFollowsLeft();
		mColorSwatch->setCanApplyImmediately(TRUE);
	}

	mShinyColorSwatch = getChild<LLColorSwatchCtrl>("shinycolorswatch");
	if(mShinyColorSwatch)
	{
		mShinyColorSwatch->setCommitCallback(boost::bind(&LLPanelFace::onCommitShinyColor, this, _2));
		mShinyColorSwatch->setFollowsTop();
		mShinyColorSwatch->setFollowsLeft();
		mShinyColorSwatch->setCanApplyImmediately(TRUE);
	}

	mLabelColorTransp = getChild<LLTextBox>("color trans");
	if(mLabelColorTransp)
	{
		mLabelColorTransp->setFollowsTop();
		mLabelColorTransp->setFollowsLeft();
	}

	mCtrlColorTransp = getChild<LLSpinCtrl>("ColorTrans");
	if(mCtrlColorTransp)
	{
		mCtrlColorTransp->setCommitCallback(boost::bind(&LLPanelFace::onCommitAlpha, this, _2));
		mCtrlColorTransp->setPrecision(0);
		mCtrlColorTransp->setFollowsTop();
		mCtrlColorTransp->setFollowsLeft();
	}

	mCheckFullbright = getChild<LLCheckBoxCtrl>("checkbox fullbright");
	if (mCheckFullbright)
	{
		mCheckFullbright->setCommitCallback(LLPanelFace::onCommitFullbright, this);
	}

	mComboTexGen = getChild<LLComboBox>("combobox texgen");
	if(mComboTexGen)
	{
		mComboTexGen->setCommitCallback(LLPanelFace::onCommitTexGen, this);
		mComboTexGen->setFollows(FOLLOWS_LEFT | FOLLOWS_TOP);	
	}

	mComboMatMedia = getChild<LLComboBox>("combobox matmedia");
	if(mComboMatMedia)
	{
		mComboMatMedia->setCommitCallback(LLPanelFace::onCommitMaterialsMedia,this);
		mComboMatMedia->selectNthItem(MATMEDIA_MATERIAL);
	}

	mComboMatType = getChild<LLComboBox>("combobox mattype");
	if(mComboMatType)
	{
		mComboMatType->setCommitCallback(LLPanelFace::onCommitMaterialType, this);
		mComboMatType->selectNthItem(MATTYPE_DIFFUSE);
	}

	mCtrlGlow = getChild<LLSpinCtrl>("glow");
	if(mCtrlGlow)
	{
		mCtrlGlow->setCommitCallback(LLPanelFace::onCommitGlow, this);
	}
	

	clearCtrls();

	return TRUE;
}

LLPanelFace::LLPanelFace()
:	LLPanel(),
	mMaterialID(LLMaterialID::null),
	mMaterial(LLMaterialPtr()),
	mIsAlpha(false)
{
	USE_TEXTURE = LLTrans::getString("use_texture");
}


LLPanelFace::~LLPanelFace()
{
	// Children all cleaned up by default view destructor.
}


void LLPanelFace::sendTexture()
{
	LLTextureCtrl* mTextureCtrl = getChild<LLTextureCtrl>("texture control");
	if(!mTextureCtrl) return;
	if( !mTextureCtrl->getTentative() )
	{
		// we grab the item id first, because we want to do a
		// permissions check in the selection manager. ARGH!
		LLUUID id = mTextureCtrl->getImageItemID();
		if(id.isNull())
		{
			id = mTextureCtrl->getImageAssetID();
		}
		LLSelectMgr::getInstance()->selectionSetImage(id);
	}
}

void LLPanelFace::sendBump()
{	
	LLComboBox*	mComboBumpiness = getChild<LLComboBox>("combobox bumpiness");
	if(!mComboBumpiness)return;
	U32 bumpiness = mComboBumpiness->getCurrentIndex();
	if (bumpiness < BUMPY_TEXTURE)
	{
		LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
		//texture_ctrl->setImageAssetID(LLUUID());
		texture_ctrl->clear();
		LLSD dummy_data;
		onSelectMaterialTexture(dummy_data);
	}
	U8 bump = (U8) bumpiness & TEM_BUMP_MASK;
	LLSelectMgr::getInstance()->selectionSetBumpmap( bump );

	//refresh material state (in case this change impacts material params)
	LLSD dummy_data;
	onCommitMaterialTexture(dummy_data);
}

void LLPanelFace::sendTexGen()
{
	LLComboBox*	mComboTexGen = getChild<LLComboBox>("combobox texgen");
	if(!mComboTexGen)return;
	U8 tex_gen = (U8) mComboTexGen->getCurrentIndex() << TEM_TEX_GEN_SHIFT;
	LLSelectMgr::getInstance()->selectionSetTexGen( tex_gen );
}

void LLPanelFace::sendShiny()
{
	LLComboBox*	mComboShininess = getChild<LLComboBox>("combobox shininess");
	if(!mComboShininess)return;
	U32 shininess = mComboShininess->getCurrentIndex();
	if (shininess < SHINY_TEXTURE)
	{
		LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("shinytexture control");
		texture_ctrl->setImageAssetID(LLUUID());
	}
	U8 shiny = (U8) shininess & TEM_SHINY_MASK;
	LLSelectMgr::getInstance()->selectionSetShiny( shiny );
}

void LLPanelFace::sendFullbright()
{
	LLCheckBoxCtrl*	mCheckFullbright = getChild<LLCheckBoxCtrl>("checkbox fullbright");
	if(!mCheckFullbright)return;
	U8 fullbright = mCheckFullbright->get() ? TEM_FULLBRIGHT_MASK : 0;
	LLSelectMgr::getInstance()->selectionSetFullbright( fullbright );
}

void LLPanelFace::sendColor()
{
	
	LLColorSwatchCtrl*	mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	if(!mColorSwatch)return;
	LLColor4 color = mColorSwatch->get();

	LLSelectMgr::getInstance()->selectionSetColorOnly( color );
}

void LLPanelFace::sendAlpha()
{	
	LLSpinCtrl*	mCtrlColorTransp = getChild<LLSpinCtrl>("ColorTrans");
	if(!mCtrlColorTransp)return;
	F32 alpha = (100.f - mCtrlColorTransp->get()) / 100.f;

	LLSelectMgr::getInstance()->selectionSetAlphaOnly( alpha );
}


void LLPanelFace::sendGlow()
{
	LLSpinCtrl* mCtrlGlow = getChild<LLSpinCtrl>("glow");
	llassert(mCtrlGlow);
	if (mCtrlGlow)
	{
		F32 glow = mCtrlGlow->get();
		LLSelectMgr::getInstance()->selectionSetGlow( glow );
	}
}

struct LLPanelFaceSetTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceSetTEFunctor(LLPanelFace* panel) : mPanel(panel) {}
	virtual bool apply(LLViewerObject* object, S32 te)
	{
		BOOL valid;
		F32 value;
		LLSpinCtrl*	ctrlTexScaleS = mPanel->getChild<LLSpinCtrl>("TexScaleU");
		LLSpinCtrl*	ctrlTexScaleT = mPanel->getChild<LLSpinCtrl>("TexScaleV");
		LLSpinCtrl*	ctrlTexOffsetS = mPanel->getChild<LLSpinCtrl>("TexOffsetU");
		LLSpinCtrl*	ctrlTexOffsetT = mPanel->getChild<LLSpinCtrl>("TexOffsetV");
		LLSpinCtrl*	ctrlTexRotation = mPanel->getChild<LLSpinCtrl>("TexRot");
		LLComboBox*		comboTexGen = mPanel->getChild<LLComboBox>("combobox texgen");
		llassert(comboTexGen);
		llassert(object);

		if (ctrlTexScaleS)
		{
			valid = !ctrlTexScaleS->getTentative(); // || !checkFlipScaleS->getTentative();
			if (valid)
			{
				value = ctrlTexScaleS->get();
				//if( checkFlipScaleS->get() )
				//{
				//	value = -value;
				//}
				if (comboTexGen &&
				    comboTexGen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleS( te, value );
			}
		}

		if (ctrlTexScaleT)
		{
			valid = !ctrlTexScaleT->getTentative(); // || !checkFlipScaleT->getTentative();
			if (valid)
			{
				value = ctrlTexScaleT->get();
				//if( checkFlipScaleT->get() )
				//{
				//	value = -value;
				//}
				if (comboTexGen &&
				    comboTexGen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleT( te, value );
			}
		}

		if (ctrlTexOffsetS)
		{
			valid = !ctrlTexOffsetS->getTentative();
			if (valid)
			{
				value = ctrlTexOffsetS->get();
				object->setTEOffsetS( te, value );
			}
		}

		if (ctrlTexOffsetT)
		{
			valid = !ctrlTexOffsetT->getTentative();
			if (valid)
			{
				value = ctrlTexOffsetT->get();
				object->setTEOffsetT( te, value );
			}
		}

		if (ctrlTexRotation)
		{
			valid = !ctrlTexRotation->getTentative();
			if (valid)
			{
				value = ctrlTexRotation->get() * DEG_TO_RAD;
				object->setTERotation( te, value );
			}
		}
		return true;
	}
private:
	LLPanelFace* mPanel;
};

// Functor that aligns a face to mCenterFace
struct LLPanelFaceSetAlignedTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceSetAlignedTEFunctor(LLPanelFace* panel, LLFace* center_face) :
		mPanel(panel),
		mCenterFace(center_face) {}

	virtual bool apply(LLViewerObject* object, S32 te)
	{
		LLFace* facep = object->mDrawable->getFace(te);
		if (!facep)
		{
			return true;
		}

		if (facep->getViewerObject()->getVolume()->getNumVolumeFaces() <= te)
		{
			return true;
		}

		bool set_aligned = true;
		if (facep == mCenterFace)
		{
			set_aligned = false;
		}
		if (set_aligned)
		{
			LLVector2 uv_offset, uv_scale;
			F32 uv_rot;
			set_aligned = facep->calcAlignedPlanarTE(mCenterFace, &uv_offset, &uv_scale, &uv_rot);
			if (set_aligned)
			{
				object->setTEOffset(te, uv_offset.mV[VX], uv_offset.mV[VY]);
				object->setTEScale(te, uv_scale.mV[VX], uv_scale.mV[VY]);
				object->setTERotation(te, uv_rot);
			}
		}
		if (!set_aligned)
		{
			LLPanelFaceSetTEFunctor setfunc(mPanel);
			setfunc.apply(object, te);
		}
		return true;
	}
private:
	LLPanelFace* mPanel;
	LLFace* mCenterFace;
};

// Functor that tests if a face is aligned to mCenterFace
struct LLPanelFaceGetIsAlignedTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceGetIsAlignedTEFunctor(LLFace* center_face) :
		mCenterFace(center_face) {}

	virtual bool apply(LLViewerObject* object, S32 te)
	{
		LLFace* facep = object->mDrawable->getFace(te);
		if (!facep)
		{
			return false;
		}

		if (facep->getViewerObject()->getVolume()->getNumVolumeFaces() <= te)
		{ //volume face does not exist, can't be aligned
			return false;
		}

		if (facep == mCenterFace)
		{
			return true;
		}
		
		LLVector2 aligned_st_offset, aligned_st_scale;
		F32 aligned_st_rot;
		if ( facep->calcAlignedPlanarTE(mCenterFace, &aligned_st_offset, &aligned_st_scale, &aligned_st_rot) )
		{
			const LLTextureEntry* tep = facep->getTextureEntry();
			LLVector2 st_offset, st_scale;
			tep->getOffset(&st_offset.mV[VX], &st_offset.mV[VY]);
			tep->getScale(&st_scale.mV[VX], &st_scale.mV[VY]);
			F32 st_rot = tep->getRotation();
			// needs a fuzzy comparison, because of fp errors
			if (is_approx_equal_fraction(st_offset.mV[VX], aligned_st_offset.mV[VX], 12) && 
				is_approx_equal_fraction(st_offset.mV[VY], aligned_st_offset.mV[VY], 12) && 
				is_approx_equal_fraction(st_scale.mV[VX], aligned_st_scale.mV[VX], 12) &&
				is_approx_equal_fraction(st_scale.mV[VY], aligned_st_scale.mV[VY], 12) &&
				is_approx_equal_fraction(st_rot, aligned_st_rot, 14))
			{
				return true;
			}
		}
		return false;
	}
private:
	LLFace* mCenterFace;
};

struct LLPanelFaceSendFunctor : public LLSelectedObjectFunctor
{
	virtual bool apply(LLViewerObject* object)
	{
		object->sendTEUpdate();
		return true;
	}
};

void LLPanelFace::sendTextureInfo()
{
	if ((bool)childGetValue("checkbox planar align").asBoolean())
	{
		struct f1 : public LLSelectedTEGetFunctor<LLFace *>
		{
			LLFace* get(LLViewerObject* object, S32 te)
			{
				return (object->mDrawable) ? object->mDrawable->getFace(te): NULL;
			}
		} get_last_face_func;
		LLFace* last_face;
		LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&get_last_face_func, last_face);

		LLPanelFaceSetAlignedTEFunctor setfunc(this, last_face);
		LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
	}
	else
	{
		LLPanelFaceSetTEFunctor setfunc(this);
		LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
	}

	LLPanelFaceSendFunctor sendfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
}

void LLPanelFace::getState()
{ //set state of UI to match state of texture entry(ies)  (calls setEnabled, setValue, etc, but NOT setVisible)
	LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();

	if( objectp
		&& objectp->getPCode() == LL_PCODE_VOLUME
		&& objectp->permModify())
	{
		BOOL editable = objectp->permModify() && !objectp->isPermanentEnforced();

		// only turn on auto-adjust button if there is a media renderer and the media is loaded
		getChildView("button align")->setEnabled(editable);

		LLComboBox* combobox_matmedia = getChild<LLComboBox>("combobox matmedia");
		if (combobox_matmedia)
		{
			if (combobox_matmedia->getCurrentIndex() < MATMEDIA_MATERIAL)
			{
				combobox_matmedia->selectNthItem(MATMEDIA_MATERIAL);
			}
		}
		else
		{
			llwarns << "failed getChild for 'combobox matmedia'" << llendl;
		}
		getChildView("combobox matmedia")->setEnabled(editable);

		LLComboBox* combobox_mattype = getChild<LLComboBox>("combobox mattype");
		if (combobox_mattype)
		{
			if (combobox_mattype->getCurrentIndex() < MATTYPE_DIFFUSE)
			{
				combobox_mattype->selectNthItem(MATTYPE_DIFFUSE);
			}
		}
		else
		{
			llwarns << "failed getChild for 'combobox mattype'" << llendl;
		}
		getChildView("combobox mattype")->setEnabled(editable);

		onCommitMaterialsMedia(NULL, this);
		
		//if ( LLMediaEngine::getInstance()->getMediaRenderer () )
		//	if ( LLMediaEngine::getInstance()->getMediaRenderer ()->isLoaded () )
		//	{	
		//		
		//		//mLabelTexAutoFix->setEnabled ( editable );
		//		
		//		//mBtnAutoFix->setEnabled ( editable );
		//	}

		bool identical;
        bool identical_diffuse;
        bool identical_norm;
        bool identical_spec;
        
		LLTextureCtrl*	texture_ctrl = getChild<LLTextureCtrl>("texture control");
		LLTextureCtrl*	shinytexture_ctrl = getChild<LLTextureCtrl>("shinytexture control");
		LLTextureCtrl*	bumpytexture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
		
		LLUUID id;
		LLUUID normmap_id;
		LLUUID specmap_id;

		// Texture
		{
			struct f1 : public LLSelectedTEGetFunctor<LLUUID>
			{
				LLUUID get(LLViewerObject* object, S32 te_index)
				{
					LLUUID id;
					
					LLViewerTexture* image = object->getTEImage(te_index);
					if (image) id = image->getID();
					
					if (!id.isNull() && LLViewerMedia::textureHasMedia(id))
					{
						LLTextureEntry *te = object->getTE(te_index);
						if (te)
						{
							LLViewerTexture* tex = te->getID().notNull() ? gTextureList.findImage(te->getID()) : NULL ;
							if(!tex)
							{
								tex = LLViewerFetchedTexture::sDefaultImagep;
							}
							if (tex)
							{
								id = tex->getID();
							}
						}
					}
					return id;
				}
			} func;
			identical_diffuse = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, id );

			// Normal map
			struct norm_get : public LLSelectedTEGetFunctor<LLUUID>
			{
				LLUUID get(LLViewerObject* object, S32 te_index)
				{
					LLUUID id;
					
					LLMaterial* mat = object->getTE(te_index)->getMaterialParams().get();

					if (mat)
					{
						id = mat->getNormalID();
					}
									
					return id;
				}
			} norm_get_func;
			identical_norm = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &norm_get_func, normmap_id );

			// Specular map
			struct spec_get : public LLSelectedTEGetFunctor<LLUUID>
			{
				LLUUID get(LLViewerObject* object, S32 te_index)
				{
					LLUUID id;
					
					LLMaterial* mat = object->getTE(te_index)->getMaterialParams().get();

					if (mat)
					{
						id = mat->getSpecularID();
					}
									
					return id;
				}
			} spec_get_func;
			identical_spec = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &spec_get_func, specmap_id );

			mIsAlpha = FALSE;
			LLGLenum image_format;
			struct f2 : public LLSelectedTEGetFunctor<LLGLenum>
			{
				LLGLenum get(LLViewerObject* object, S32 te_index)
				{
					LLGLenum image_format = GL_RGB;
					
					LLViewerTexture* image = object->getTEImage(te_index);
					if (image) image_format  = image->getPrimaryFormat();
					return image_format;
				}
			} func2;
			LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func2, image_format );
            
            mIsAlpha = FALSE;
            switch (image_format)
            {
                case GL_RGBA:
                case GL_ALPHA:
                {
                    mIsAlpha = TRUE;
                }
                break;

                case GL_RGB: break;
                default:
                {
                    llwarns << "Unexpected tex format in LLPanelFace...resorting to no alpha" << llendl;
                }
                break;
            }

			if(LLViewerMedia::textureHasMedia(id))
			{
				getChildView("button align")->setEnabled(editable);
			}
			
			// Specular map
			struct alpha_get : public LLSelectedTEGetFunctor<U8>
			{
				U8 get(LLViewerObject* object, S32 te_index)
			{
					U8 ret = 1;
					
					LLMaterial* mat = object->getTE(te_index)->getMaterialParams().get();

					if (mat)
				{
						ret = mat->getDiffuseAlphaMode();
				}
									
					return ret;
				}
			} alpha_get_func;
			
			U8 alpha_mode = 1;
			LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &alpha_get_func, alpha_mode);
			
			{
				LLCtrlSelectionInterface* combobox_alphamode =
				      childGetSelectionInterface("combobox alphamode");

				if (combobox_alphamode)
				{
					if (!mIsAlpha)
					{
						alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_NONE;
					}
					
					combobox_alphamode->selectNthItem(alpha_mode);
				}
				else
				{
					llwarns << "failed childGetSelectionInterface for 'combobox alphamode'" << llendl;
				}

				updateAlphaControls(getChild<LLComboBox>("combobox alphamode"),this);
			}
			
			if(texture_ctrl)
			{
                if (identical_diffuse)
				{
					texture_ctrl->setTentative( FALSE );
					texture_ctrl->setEnabled( editable );
					texture_ctrl->setImageAssetID( id );
					shinytexture_ctrl->setTentative( FALSE );
					shinytexture_ctrl->setEnabled( editable );
					if (!editable)
					{
						shinytexture_ctrl->setImageAssetID( LLUUID::null );
						bumpytexture_ctrl->setImageAssetID( LLUUID::null );
					}
					bumpytexture_ctrl->setTentative( FALSE );
					bumpytexture_ctrl->setEnabled( editable );
					getChildView("combobox alphamode")->setEnabled(editable && mIsAlpha);
					getChildView("label alphamode")->setEnabled(editable && mIsAlpha);
					getChildView("maskcutoff")->setEnabled(editable && mIsAlpha);
					getChildView("label maskcutoff")->setEnabled(editable && mIsAlpha);
				}
                else if (id.isNull())
					{
						// None selected
						texture_ctrl->setTentative( FALSE );
						texture_ctrl->setEnabled( FALSE );
						texture_ctrl->setImageAssetID( LLUUID::null );
						shinytexture_ctrl->setTentative( FALSE );
						shinytexture_ctrl->setEnabled( FALSE );
						shinytexture_ctrl->setImageAssetID( LLUUID::null );
						bumpytexture_ctrl->setTentative( FALSE );
						bumpytexture_ctrl->setEnabled( FALSE );
						bumpytexture_ctrl->setImageAssetID( LLUUID::null );
						getChildView("combobox alphamode")->setEnabled( FALSE );
						getChildView("label alphamode")->setEnabled( FALSE );
						getChildView("maskcutoff")->setEnabled( FALSE);
						getChildView("label maskcutoff")->setEnabled( FALSE );
					}
					else
					{
						// Tentative: multiple selected with different textures
						texture_ctrl->setTentative( TRUE );
						texture_ctrl->setEnabled( editable );
						texture_ctrl->setImageAssetID( id );
                    getChildView("combobox alphamode")->setEnabled(editable && mIsAlpha);
                    getChildView("label alphamode")->setEnabled(editable && mIsAlpha);
                    getChildView("maskcutoff")->setEnabled(editable && mIsAlpha);
                    getChildView("label maskcutoff")->setEnabled(editable && mIsAlpha);
                }
            }
            
            if (shinytexture_ctrl)
            {
                if (identical_spec)
                {
                    shinytexture_ctrl->setTentative( FALSE );
                    shinytexture_ctrl->setEnabled( editable );
                    shinytexture_ctrl->setImageAssetID( specmap_id );
                }
                else if (specmap_id.isNull())
                {
                    shinytexture_ctrl->setTentative( FALSE );
                    shinytexture_ctrl->setEnabled( FALSE );
                    shinytexture_ctrl->setImageAssetID( LLUUID::null );
                }
                else
                {
					shinytexture_ctrl->setTentative( TRUE );
					shinytexture_ctrl->setEnabled( editable );
                    shinytexture_ctrl->setImageAssetID( specmap_id );
                }
            }

            if (bumpytexture_ctrl)
            {
                if (identical_norm)
				{
                    bumpytexture_ctrl->setTentative( FALSE );
                    bumpytexture_ctrl->setEnabled( editable );
                    bumpytexture_ctrl->setImageAssetID( normmap_id );
                }
                else if (normmap_id.isNull())
                {
                    bumpytexture_ctrl->setTentative( FALSE );
                    bumpytexture_ctrl->setEnabled( FALSE );
					bumpytexture_ctrl->setImageAssetID( LLUUID::null );
				}
                else
                {
					bumpytexture_ctrl->setTentative( TRUE );
					bumpytexture_ctrl->setEnabled( editable );
                    bumpytexture_ctrl->setImageAssetID( normmap_id );
				}
			}
		}
		
		// planar align
		bool align_planar = false;
		bool identical_planar_aligned = false;
		bool is_planar = false;
		{
			LLCheckBoxCtrl*	cb_planar_align = getChild<LLCheckBoxCtrl>("checkbox planar align");
			align_planar = (cb_planar_align && cb_planar_align->get());
			struct f1 : public LLSelectedTEGetFunctor<bool>
			{
				bool get(LLViewerObject* object, S32 face)
				{
					return (object->getTE(face)->getTexGen() == LLTextureEntry::TEX_GEN_PLANAR);
				}
			} func;

			bool texgens_identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, is_planar );
			bool enabled = (editable && texgens_identical && is_planar);
			childSetValue("checkbox planar align", align_planar && enabled);
			childSetEnabled("checkbox planar align", enabled);

			if (align_planar && enabled)
			{
				struct f2 : public LLSelectedTEGetFunctor<LLFace *>
				{
					LLFace* get(LLViewerObject* object, S32 te)
					{
						return (object->mDrawable) ? object->mDrawable->getFace(te): NULL;
					}
				} get_te_face_func;
				LLFace* last_face;
				LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&get_te_face_func, last_face);
				LLPanelFaceGetIsAlignedTEFunctor get_is_aligend_func(last_face);
				// this will determine if the texture param controls are tentative:
				identical_planar_aligned = LLSelectMgr::getInstance()->getSelection()->applyToTEs(&get_is_aligend_func);
			}
		}
		
		// Needs to be public and before tex scale settings below to properly reflect
		// behavior when in planar vs default texgen modes in the
		// NORSPEC-84 et al
		//
		LLTextureEntry::e_texgen selected_texgen = LLTextureEntry::TEX_GEN_DEFAULT;
		bool identical_texgen = true;		
		bool identical_planar_texgen = false;

		{			
			struct f11 : public LLSelectedTEGetFunctor<LLTextureEntry::e_texgen>
			{
				LLTextureEntry::e_texgen get(LLViewerObject* object, S32 face)
				{
					return (LLTextureEntry::e_texgen)(object->getTE(face)->getTexGen());
				}
			} func;
			identical_texgen = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, selected_texgen );
			identical_planar_texgen = (identical_texgen && (selected_texgen == LLTextureEntry::TEX_GEN_PLANAR));
		}

		// Texture scale
		{
			F32 scale_s = 1.f;
			struct f2 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mScaleS;
				}
			} func;

			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, scale_s );
			identical = align_planar ? identical_planar_aligned : identical;

			F32 scale_u = editable ? scale_s : 0;
			scale_u *= identical_planar_texgen ? 2.0f : 1.0f;

			getChild<LLUICtrl>("TexScaleU")->setValue(scale_u);
			getChild<LLUICtrl>("TexScaleU")->setTentative(LLSD((BOOL)(!identical)));
			getChildView("TexScaleU")->setEnabled(editable);
		
			scale_s = 1.f;
			struct f3 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					F32 s = 1.f, t = 1.f;

					LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
					if (mat)
					{
						mat->getSpecularRepeat(s, t);
					}
					return s;
				}
			} shiny_func;
			
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &shiny_func, scale_s );
			identical = align_planar ? identical_planar_aligned : identical;

			F32 scale_s_value = editable ? scale_s : 0;
			scale_s_value *= identical_planar_texgen ? 2.0f : 1.0f;

			getChild<LLUICtrl>("shinyScaleU")->setValue(scale_s_value);
			getChild<LLUICtrl>("shinyScaleU")->setTentative(LLSD((BOOL)(!identical)));
			getChildView("shinyScaleU")->setEnabled(editable && specmap_id.notNull());

			scale_s = 1.f;
			struct f4 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					F32 s = 1.f, t = 1.f;

					LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
					if (mat)
					{
						mat->getNormalRepeat(s, t);
					}
					return s;
				}
			} bump_func;
			
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &bump_func, scale_s );
			identical = align_planar ? identical_planar_aligned : identical;

			scale_s_value = editable ? scale_s : 0;
			scale_s_value *= identical_planar_texgen ? 2.0f : 1.0f;

			getChild<LLUICtrl>("bumpyScaleU")->setValue(scale_s_value);
			getChild<LLUICtrl>("bumpyScaleU")->setTentative(LLSD((BOOL)(!identical)));
			getChildView("bumpyScaleU")->setEnabled(editable && normmap_id.notNull());
		}

		{
			F32 scale_t = 1.f;
			struct f3 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mScaleT;
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, scale_t );
			identical = align_planar ? identical_planar_aligned : identical;

			F32 scale_t_value = editable ? scale_t : 0;
			scale_t_value *= identical_planar_texgen ? 2.0f : 1.0f;

			getChild<LLUICtrl>("TexScaleV")->setValue(scale_t_value);
			getChild<LLUICtrl>("TexScaleV")->setTentative(LLSD((BOOL)(!identical)));
			getChildView("TexScaleV")->setEnabled(editable);
			
			scale_t = 1.f;
			struct f4 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					F32 s = 1.f, t = 1.f;

					LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
					if (mat)
					{
						mat->getSpecularRepeat(s, t);
					}
					return t;
				}
			} shiny_func;
			
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &shiny_func, scale_t );
			identical = align_planar ? identical_planar_aligned : identical;

			scale_t_value = editable ? scale_t : 0;
			scale_t_value *= identical_planar_texgen ? 2.0f : 1.0f;

			getChild<LLUICtrl>("shinyScaleV")->setValue(scale_t_value);
			getChild<LLUICtrl>("shinyScaleV")->setTentative(LLSD((BOOL)(!identical)));
			getChildView("shinyScaleV")->setEnabled(editable && specmap_id.notNull());

			scale_t = 1.f;
			struct f5 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					F32 s = 1.f, t = 1.f;

					LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
					if (mat)
					{
						mat->getNormalRepeat(s, t);
					}
					return t;
				}
			} bump_func;
			
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &bump_func, scale_t );
			identical = align_planar ? identical_planar_aligned : identical;

			scale_t_value = editable ? scale_t : 0.0f;
			scale_t_value *= identical_planar_texgen ? 2.0f : 1.0f;

			getChild<LLUICtrl>("bumpyScaleV")->setValue(scale_t_value);
			getChild<LLUICtrl>("bumpyScaleV")->setTentative(LLSD((BOOL)(!identical)));
			getChildView("bumpyScaleV")->setEnabled(editable && normmap_id.notNull());
			
		}

		// Texture offset
		{
			getChildView("tex offset")->setEnabled(editable);
			F32 offset_s = 0.f;
			struct f4 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mOffsetS;
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, offset_s );
			identical = align_planar ? identical_planar_aligned : identical;
			getChild<LLUICtrl>("TexOffsetU")->setValue(editable ? offset_s : 0);
			getChild<LLUICtrl>("TexOffsetU")->setTentative(!identical);
			getChildView("TexOffsetU")->setEnabled(editable);

			offset_s = 1.f;
			struct f3 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					F32 s = 0.f, t = 0.f;

					LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
					if (mat)
					{
						mat->getSpecularOffset(s, t);
					}
					return s;
				}
			} shiny_func;
			
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &shiny_func, offset_s );
			identical = align_planar ? identical_planar_aligned : identical;
			getChild<LLUICtrl>("shinyOffsetU")->setValue(editable ? offset_s : 0);
			getChild<LLUICtrl>("shinyOffsetU")->setTentative(LLSD((BOOL)(!identical)));
			getChildView("shinyOffsetU")->setEnabled(editable && specmap_id.notNull());

			offset_s = 1.f;
			struct f5 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					F32 s = 0.f, t = 0.f;

					LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
					if (mat)
					{
						mat->getNormalOffset(s, t);
					}
					return s;
				}
			} bump_func;
			
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &bump_func, offset_s );
			identical = align_planar ? identical_planar_aligned : identical;

			getChild<LLUICtrl>("bumpyOffsetU")->setValue(editable ? offset_s : 0);
			getChild<LLUICtrl>("bumpyOffsetU")->setTentative(LLSD((BOOL)(!identical)));
			getChildView("bumpyOffsetU")->setEnabled(editable && normmap_id.notNull());
		}

		{
			F32 offset_t = 0.f;
			struct f5 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mOffsetT;
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, offset_t );
			identical = align_planar ? identical_planar_aligned : identical;
			getChild<LLUICtrl>("TexOffsetV")->setValue(editable ? offset_t : 0);
			getChild<LLUICtrl>("TexOffsetV")->setTentative(!identical);
			getChildView("TexOffsetV")->setEnabled(editable);
			
			
			offset_t = 1.f;
			struct f3 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					F32 s = 1.f, t = 1.f;

					LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
					if (mat)
					{
						mat->getSpecularOffset(s, t);
					}
					return t;
				}
			} shiny_func;
			
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &shiny_func, offset_t );
			identical = align_planar ? identical_planar_aligned : identical;
			getChild<LLUICtrl>("shinyOffsetV")->setValue(editable ? offset_t : 0);
			getChild<LLUICtrl>("shinyOffsetV")->setTentative(LLSD((BOOL)(!identical)));
			getChildView("shinyOffsetV")->setEnabled(editable && specmap_id.notNull());

			offset_t = 1.f;
			struct f4 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					F32 s = 1.f, t = 1.f;

					LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
					if (mat)
					{
						mat->getNormalOffset(s, t);
					}
					return t;
				}
			} bump_func;
			
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &bump_func, offset_t );
			identical = align_planar ? identical_planar_aligned : identical;

			getChild<LLUICtrl>("bumpyOffsetV")->setValue(editable ? offset_t : 0);
			getChild<LLUICtrl>("bumpyOffsetV")->setTentative(LLSD((BOOL)(!identical)));
			getChildView("bumpyOffsetV")->setEnabled(editable && normmap_id.notNull());
		}

		// Texture rotation
		{
			F32 rotation = 0.f;
			struct f6 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mRotation;
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, rotation );
			identical = align_planar ? identical_planar_aligned : identical;
			getChild<LLUICtrl>("TexRot")->setValue(editable ? rotation * RAD_TO_DEG : 0);
			getChild<LLUICtrl>("TexRot")->setTentative(!identical);
			getChildView("TexRot")->setEnabled(editable);


			
			rotation = 1.f;
			struct f3 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					F32 ret = 0.f;

					LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
					if (mat)
					{
						ret = mat->getSpecularRotation();
					}
					return ret;
				}
			} shiny_func;
			
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &shiny_func, rotation );
			identical = align_planar ? identical_planar_aligned : identical;
			getChild<LLUICtrl>("shinyRot")->setValue(editable ? rotation * RAD_TO_DEG : 0);
			getChild<LLUICtrl>("shinyRot")->setTentative(LLSD((BOOL)(!identical)));
			getChildView("shinyRot")->setEnabled(editable && specmap_id.notNull());

			rotation = 1.f;
			struct f4 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					F32 ret = 0.f;

					LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
					if (mat)
					{
						ret = mat->getNormalRotation();
					}
					return ret;
				}
			} bump_func;
			
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &bump_func, rotation );
			identical = align_planar ? identical_planar_aligned : identical;

			getChild<LLUICtrl>("bumpyRot")->setValue(editable ? rotation * RAD_TO_DEG : 0);
			getChild<LLUICtrl>("bumpyRot")->setTentative(LLSD((BOOL)(!identical)));
			getChildView("bumpyRot")->setEnabled(editable && normmap_id.notNull());
		}

		// Color swatch
		{
			getChildView("color label")->setEnabled(editable);
		}
		LLColorSwatchCtrl*	mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
		LLColor4 color = LLColor4::white;
		if(mColorSwatch)
		{
			struct f7 : public LLSelectedTEGetFunctor<LLColor4>
			{
				LLColor4 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->getColor();
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, color );
			
			mColorSwatch->setOriginal(color);
			mColorSwatch->set(color, TRUE);

			mColorSwatch->setValid(editable);
			mColorSwatch->setEnabled( editable );
			mColorSwatch->setCanApplyImmediately( editable );
		}
		// Color transparency
		{
			getChildView("color trans")->setEnabled(editable);
		}

		F32 transparency = (1.f - color.mV[VALPHA]) * 100.f;
		{
			getChild<LLUICtrl>("ColorTrans")->setValue(editable ? transparency : 0);
			getChildView("ColorTrans")->setEnabled(editable);
		}

		{
			F32 glow = 0.f;
			struct f8 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->getGlow();
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, glow );

			getChild<LLUICtrl>("glow")->setValue(glow);
			getChildView("glow")->setEnabled(editable);
			getChild<LLUICtrl>("glow")->setTentative(!identical);
			getChildView("glow label")->setEnabled(editable);

		}

		// Shiny
		{
			U8 shiny = 0;
			struct f9 : public LLSelectedTEGetFunctor<U8>
			{
				U8 get(LLViewerObject* object, S32 face)
				{
					return (U8)(object->getTE(face)->getShiny());
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, shiny );

			LLUUID spec_map_id = getChild<LLTextureCtrl>("shinytexture control")->getImageAssetID();

			LLCtrlSelectionInterface* combobox_shininess =
			      childGetSelectionInterface("combobox shininess");
			if (combobox_shininess)
			{
				combobox_shininess->selectNthItem(spec_map_id.isNull() ? (S32)shiny : SHINY_TEXTURE);
			}
			else
			{
				llwarns << "failed childGetSelectionInterface for 'combobox shininess'" << llendl;
			}
			getChildView("combobox shininess")->setEnabled(editable);
			getChild<LLUICtrl>("combobox shininess")->setTentative(!identical);
			getChildView("label shininess")->setEnabled(editable);
			getChildView("glossiness")->setEnabled(editable);
			getChild<LLUICtrl>("glossiness")->setTentative(!identical);
			getChildView("label glossiness")->setEnabled(editable);
			getChildView("environment")->setEnabled(editable);
			getChild<LLUICtrl>("environment")->setTentative(!identical);
			getChildView("label environment")->setEnabled(editable);
			getChildView("shinycolorswatch")->setEnabled(editable);
			getChild<LLUICtrl>("shinycolorswatch")->setTentative(!identical);
			getChildView("label shinycolor")->setEnabled(editable);
		}

		// Bumpy
		{
			U8 bumpy = 0;
			struct f10 : public LLSelectedTEGetFunctor<U8>
			{
				U8 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->getBumpmap();
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, bumpy );

			LLUUID norm_map_id = getChild<LLTextureCtrl>("shinytexture control")->getImageAssetID();

			LLCtrlSelectionInterface* combobox_bumpiness = childGetSelectionInterface("combobox bumpiness");
			if (combobox_bumpiness)
			{
				combobox_bumpiness->selectNthItem(norm_map_id.isNull() ? (S32)bumpy : BUMPY_TEXTURE);
			}
			else
			{
				llwarns << "failed childGetSelectionInterface for 'combobox bumpiness'" << llendl;
			}
			getChildView("combobox bumpiness")->setEnabled(editable);
			getChild<LLUICtrl>("combobox bumpiness")->setTentative(!identical);
			getChildView("label bumpiness")->setEnabled(editable);
		}

		{			
			LLCtrlSelectionInterface* combobox_texgen =
			      childGetSelectionInterface("combobox texgen");
			if (combobox_texgen)
			{
				combobox_texgen->selectNthItem(((S32)selected_texgen) >> 1); // Maps from enum to combobox entry index
			}
			else
			{
				llwarns << "failed childGetSelectionInterface for 'combobox texgen'" << llendl;
			}
			getChildView("combobox texgen")->setEnabled(editable);
			getChild<LLUICtrl>("combobox texgen")->setTentative(!identical);
			getChildView("tex gen")->setEnabled(editable);

			if (selected_texgen == LLTextureEntry::TEX_GEN_PLANAR)
			{
				// EXP-1507 (change label based on the mapping mode)
				getChild<LLUICtrl>("rpt")->setValue(getString("string repeats per meter"));
			}
			else
			if (selected_texgen == LLTextureEntry::TEX_GEN_DEFAULT)
			{
				getChild<LLUICtrl>("rpt")->setValue(getString("string repeats per face"));
			}
		}

		{
			U8 fullbright_flag = 0;
			struct f12 : public LLSelectedTEGetFunctor<U8>
			{
				U8 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->getFullbright();
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, fullbright_flag );

			getChild<LLUICtrl>("checkbox fullbright")->setValue((S32)(fullbright_flag != 0));
			getChildView("checkbox fullbright")->setEnabled(editable);
			getChild<LLUICtrl>("checkbox fullbright")->setTentative(!identical);
		}
		
		// Repeats per meter
		{
			F32 repeats = 1.f;
			struct f13 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					U32 s_axis = VX;
					U32 t_axis = VY;
					// BUG: Only repeats along S axis
					// BUG: Only works for boxes.
					LLPrimitive::getTESTAxes(face, &s_axis, &t_axis);
					return object->getTE(face)->mScaleS / object->getScale().mV[s_axis];
				}
			} func;			
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, repeats );
			
			getChild<LLUICtrl>("rptctrl")->setValue(editable ? repeats : 0);
			getChild<LLUICtrl>("rptctrl")->setTentative(!identical);
			LLComboBox*	mComboTexGen = getChild<LLComboBox>("combobox texgen");
			if (mComboTexGen)
			{
				BOOL enabled = editable && (!mComboTexGen || mComboTexGen->getCurrentIndex() != 1);
				getChildView("rptctrl")->setEnabled(enabled);
			}
		}

		// Materials
		{
			mMaterialID = LLMaterialID::null;
			mMaterial = NULL;

			struct f1 : public LLSelectedTEGetFunctor<LLMaterialID>
			{
				LLMaterialID get(LLViewerObject* object, S32 te_index)
				{
					LLMaterialID material_id;
					
					return object->getTE(te_index)->getMaterialID();
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, mMaterialID );
			llinfos << "Material ID returned: '"
				<< mMaterialID.asString() << "', isNull? "
				<< (mMaterialID.isNull()?"TRUE":"FALSE") << llendl;
			if (!mMaterialID.isNull() && editable)
			{
				llinfos << "Requesting material ID " << mMaterialID.asString() << llendl;
				LLMaterialMgr::getInstance()->get(objectp->getRegion()->getRegionID(),mMaterialID,boost::bind(&LLPanelFace::onMaterialLoaded, this, _1, _2));
			}
		}

		// Set variable values for numeric expressions
		LLCalc* calcp = LLCalc::getInstance();
		calcp->setVar(LLCalc::TEX_U_SCALE, childGetValue("TexScaleU").asReal());
		calcp->setVar(LLCalc::TEX_V_SCALE, childGetValue("TexScaleV").asReal());
		calcp->setVar(LLCalc::TEX_U_OFFSET, childGetValue("TexOffsetU").asReal());
		calcp->setVar(LLCalc::TEX_V_OFFSET, childGetValue("TexOffsetV").asReal());
		calcp->setVar(LLCalc::TEX_ROTATION, childGetValue("TexRot").asReal());
		calcp->setVar(LLCalc::TEX_TRANSPARENCY, childGetValue("ColorTrans").asReal());
		calcp->setVar(LLCalc::TEX_GLOW, childGetValue("glow").asReal());
	}
	else
	{
		// Disable all UICtrls
		clearCtrls();

		// Disable non-UICtrls
		LLTextureCtrl*	texture_ctrl = getChild<LLTextureCtrl>("texture control"); 
		if(texture_ctrl)
		{
			texture_ctrl->setImageAssetID( LLUUID::null );
			texture_ctrl->setEnabled( FALSE );  // this is a LLUICtrl, but we don't want it to have keyboard focus so we add it as a child, not a ctrl.
// 			texture_ctrl->setValid(FALSE);
		}
		LLColorSwatchCtrl* mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
		if(mColorSwatch)
		{
			mColorSwatch->setEnabled( FALSE );			
			mColorSwatch->setFallbackImageName("locked_image.j2c" );
			mColorSwatch->setValid(FALSE);
		}
		getChildView("color trans")->setEnabled(FALSE);
		getChildView("rpt")->setEnabled(FALSE);
		getChildView("tex offset")->setEnabled(FALSE);
		getChildView("tex gen")->setEnabled(FALSE);
		getChildView("label shininess")->setEnabled(FALSE);
		getChildView("label bumpiness")->setEnabled(FALSE);

		getChildView("button align")->setEnabled(FALSE);
		//getChildView("has media")->setEnabled(FALSE);
		//getChildView("media info set")->setEnabled(FALSE);

		onCommitMaterialsMedia(NULL,this);

		// Set variable values for numeric expressions
		LLCalc* calcp = LLCalc::getInstance();
		calcp->clearVar(LLCalc::TEX_U_SCALE);
		calcp->clearVar(LLCalc::TEX_V_SCALE);
		calcp->clearVar(LLCalc::TEX_U_OFFSET);
		calcp->clearVar(LLCalc::TEX_V_OFFSET);
		calcp->clearVar(LLCalc::TEX_ROTATION);
		calcp->clearVar(LLCalc::TEX_TRANSPARENCY);
		calcp->clearVar(LLCalc::TEX_GLOW);		
	}
}


void LLPanelFace::refresh()
{
	getState();
}

void LLPanelFace::onMaterialLoaded(const LLMaterialID& material_id, const LLMaterialPtr material)
{ //laying out UI based on material parameters (calls setVisible on various components)
	LL_DEBUGS("Materials") << "Loaded material " << material_id.asString() << material->asLLSD() << LL_ENDL;
	
	//make a local copy of the material for editing 
	// (prevents local edits from overwriting client state on shared materials)
	mMaterial   = new LLMaterial(*material);
	mMaterialID = material_id;

	// Alpha
	LLCtrlSelectionInterface* combobox_alphamode =
	      childGetSelectionInterface("combobox alphamode");
	if (combobox_alphamode)
	{
		combobox_alphamode->selectNthItem(material->getDiffuseAlphaMode());
	}
	else
	{
		llwarns << "failed childGetSelectionInterface for 'combobox alphamode'" << llendl;
	}
	getChild<LLUICtrl>("maskcutoff")->setValue(material->getAlphaMaskCutoff());
	updateAlphaControls(getChild<LLComboBox>("combobox alphamode"),this);

	LLTextureEntry::e_texgen selected_texgen = LLTextureEntry::TEX_GEN_DEFAULT;
	bool identical_texgen = true;		
	bool identical_planar_texgen = false;

	struct f44 : public LLSelectedTEGetFunctor<LLTextureEntry::e_texgen>
	{
		LLTextureEntry::e_texgen get(LLViewerObject* object, S32 face)
		{
			return (LLTextureEntry::e_texgen)(object->getTE(face)->getTexGen());
		}
	} func;
	identical_texgen = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, selected_texgen );
	identical_planar_texgen = (identical_texgen && (selected_texgen == LLTextureEntry::TEX_GEN_PLANAR));

	// Shiny (specular)
	F32 offset_x, offset_y, repeat_x, repeat_y, rot;
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("shinytexture control");
	texture_ctrl->setImageAssetID(material->getSpecularID());
	LLComboBox* combobox_shininess = getChild<LLComboBox>("combobox shininess");
	if (!material->getSpecularID().isNull())
	{
		material->getSpecularOffset(offset_x,offset_y);
		material->getSpecularRepeat(repeat_x,repeat_y);

		if (identical_planar_texgen)
		{
			repeat_x *= 2.0f;
			repeat_y *= 2.0f;
		}

		rot = material->getSpecularRotation();
		getChild<LLUICtrl>("shinyScaleU")->setValue(repeat_x);
		getChild<LLUICtrl>("shinyScaleV")->setValue(repeat_y);
		getChild<LLUICtrl>("shinyRot")->setValue(rot*RAD_TO_DEG);
		getChild<LLUICtrl>("shinyOffsetU")->setValue(offset_x);
		getChild<LLUICtrl>("shinyOffsetV")->setValue(offset_y);
		getChild<LLColorSwatchCtrl>("shinycolorswatch")->setOriginal(material->getSpecularLightColor());
		getChild<LLColorSwatchCtrl>("shinycolorswatch")->set(material->getSpecularLightColor(),TRUE);
		getChild<LLUICtrl>("glossiness")->setValue((F32)(material->getSpecularLightExponent())/255.0);
		getChild<LLUICtrl>("environment")->setValue((F32)(material->getEnvironmentIntensity())/255.0);
	}
	updateShinyControls(combobox_shininess,this, true);

	// Bumpy (normal)
	texture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
	texture_ctrl->setImageAssetID(material->getNormalID());
	LLComboBox* combobox_bumpiness = getChild<LLComboBox>("combobox bumpiness");
	if (!material->getNormalID().isNull())
	{
		material->getNormalOffset(offset_x,offset_y);
		material->getNormalRepeat(repeat_x,repeat_y);

		if (identical_planar_texgen)
		{
			repeat_x *= 2.0f;
			repeat_y *= 2.0f;
		}

		rot = material->getNormalRotation();
		getChild<LLUICtrl>("bumpyScaleU")->setValue(repeat_x);
		getChild<LLUICtrl>("bumpyScaleV")->setValue(repeat_y);
		getChild<LLUICtrl>("bumpyRot")->setValue(rot*RAD_TO_DEG);
		getChild<LLUICtrl>("bumpyOffsetU")->setValue(offset_x);
		getChild<LLUICtrl>("bumpyOffsetV")->setValue(offset_y);
	}
	updateBumpyControls(combobox_bumpiness,this, true);
}

void LLPanelFace::updateMaterial()
{ // assign current state of UI to material definition for submit to sim
	LL_DEBUGS("Materials") << "Entered." << LL_ENDL;
	LLComboBox* comboAlphaMode = getChild<LLComboBox>("combobox alphamode");
	LLComboBox* comboBumpiness = getChild<LLComboBox>("combobox bumpiness");
	LLComboBox* comboShininess = getChild<LLComboBox>("combobox shininess");
	if (!comboAlphaMode || !comboBumpiness || !comboShininess)
	{
		return;
	}
	U32 alpha_mode = comboAlphaMode->getCurrentIndex();
	U32 bumpiness = comboBumpiness->getCurrentIndex();
	U32 shininess = comboShininess->getCurrentIndex();

	LLTextureEntry::e_texgen selected_texgen = LLTextureEntry::TEX_GEN_DEFAULT;
	struct f45 : public LLSelectedTEGetFunctor<LLTextureEntry::e_texgen>
	{
		LLTextureEntry::e_texgen get(LLViewerObject* object, S32 face)
		{
			return (LLTextureEntry::e_texgen)(object->getTE(face)->getTexGen());
		}
	} func;
	bool identical_texgen = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, selected_texgen );
	bool identical_planar_texgen = (identical_texgen && (selected_texgen == LLTextureEntry::TEX_GEN_PLANAR));

	if ((mIsAlpha && (alpha_mode != LLMaterial::DIFFUSE_ALPHA_MODE_BLEND))
		|| (bumpiness == BUMPY_TEXTURE)
		|| (shininess == SHINY_TEXTURE))
	{
		// The user's specified something that needs a material.
		bool new_material = false;
		if (!mMaterial)
		{
			new_material = true;
			mMaterial = LLMaterialPtr(new LLMaterial());
		}

		mMaterial->setDiffuseAlphaMode(getChild<LLComboBox>("combobox alphamode")->getCurrentIndex());
		mMaterial->setAlphaMaskCutoff((U8)(getChild<LLUICtrl>("maskcutoff")->getValue().asInteger()));

		LLUUID norm_map_id = getChild<LLTextureCtrl>("bumpytexture control")->getImageAssetID();
		if (bumpiness == BUMPY_TEXTURE)
		{
			LL_DEBUGS("Materials") << "Setting bumpy texture, bumpiness = " << bumpiness  << LL_ENDL;
			mMaterial->setNormalID(norm_map_id);

			F32 bumpy_scale_u = getChild<LLUICtrl>("bumpyScaleU")->getValue().asReal();
			F32 bumpy_scale_v = getChild<LLUICtrl>("bumpyScaleV")->getValue().asReal();

			if (identical_planar_texgen)
			{
				bumpy_scale_u *= 0.5f;
				bumpy_scale_v *= 0.5f;
			}

			mMaterial->setNormalOffset(getChild<LLUICtrl>("bumpyOffsetU")->getValue().asReal(),
							getChild<LLUICtrl>("bumpyOffsetV")->getValue().asReal());
			mMaterial->setNormalRepeat(bumpy_scale_u, bumpy_scale_v);
			mMaterial->setNormalRotation(getChild<LLUICtrl>("bumpyRot")->getValue().asReal()*DEG_TO_RAD);
		}
		else
		{
			LL_DEBUGS("Materials") << "Removing bumpy texture, bumpiness = " << bumpiness  << LL_ENDL;
			mMaterial->setNormalID(LLUUID());
			mMaterial->setNormalOffset(0.0f,0.0f);
			mMaterial->setNormalRepeat(1.0f,1.0f);
			mMaterial->setNormalRotation(0.0f);
		}
        
		LLUUID spec_map_id = getChild<LLTextureCtrl>("shinytexture control")->getImageAssetID();

		if (shininess == SHINY_TEXTURE)
		{
			LL_DEBUGS("Materials") << "Setting shiny texture, shininess = " << shininess  << LL_ENDL;
			mMaterial->setSpecularID(spec_map_id);
			mMaterial->setSpecularOffset(getChild<LLUICtrl>("shinyOffsetU")->getValue().asReal(),
							getChild<LLUICtrl>("shinyOffsetV")->getValue().asReal());

			F32 shiny_scale_u = getChild<LLUICtrl>("shinyScaleU")->getValue().asReal();
			F32 shiny_scale_v = getChild<LLUICtrl>("shinyScaleV")->getValue().asReal();

			if (identical_planar_texgen)
			{
				shiny_scale_u *= 0.5f;
				shiny_scale_v *= 0.5f;
			}

			mMaterial->setSpecularRepeat(shiny_scale_u, shiny_scale_v);
			mMaterial->setSpecularRotation(getChild<LLUICtrl>("shinyRot")->getValue().asReal()*DEG_TO_RAD);

			//override shininess to 0.2f if this is a new material
			if (!new_material)
			{
				mMaterial->setSpecularLightColor(getChild<LLColorSwatchCtrl>("shinycolorswatch")->get());
				mMaterial->setSpecularLightExponent((U8)(255*getChild<LLUICtrl>("glossiness")->getValue().asReal()));
				mMaterial->setEnvironmentIntensity((U8)(255*getChild<LLUICtrl>("environment")->getValue().asReal()));
			}
		}
		else
		{
			LL_DEBUGS("Materials") << "Removing shiny texture, shininess = " << shininess << LL_ENDL;
			mMaterial->setSpecularID(LLUUID());
			mMaterial->setSpecularOffset(0.0f,0.0f);
			mMaterial->setSpecularRepeat(1.0f,1.0f);
			mMaterial->setSpecularRotation(0.0f);
			mMaterial->setSpecularLightColor(LLMaterial::DEFAULT_SPECULAR_LIGHT_COLOR);
			mMaterial->setSpecularLightExponent(LLMaterial::DEFAULT_SPECULAR_LIGHT_EXPONENT);
			mMaterial->setEnvironmentIntensity(0);
		}
		
		LL_DEBUGS("Materials") << "Updating material: " << mMaterial->asLLSD() << LL_ENDL;
		LLSelectMgr::getInstance()->selectionSetMaterial( mMaterial );
	}
	else
	{
		// The user has specified settings that don't need a material.
		if (mMaterial || !mMaterialID.isNull())
		{
			LL_DEBUGS("Materials") << "Resetting material entry" << LL_ENDL;
			mMaterial = NULL;
			mMaterialID = LLMaterialID::null;
			// Delete existing material entry...
			LLSelectMgr::getInstance()->selectionRemoveMaterial();
		}
	}
}

//
// Static functions
//

// static
F32 LLPanelFace::valueGlow(LLViewerObject* object, S32 face)
{
	return (F32)(object->getTE(face)->getGlow());
}


void LLPanelFace::onCommitColor(const LLSD& data)
{
	sendColor();
}

void LLPanelFace::onCommitShinyColor(const LLSD& data)
{
	updateMaterial();
}

void LLPanelFace::onCommitAlpha(const LLSD& data)
{
	sendAlpha();
}

void LLPanelFace::onCancelColor(const LLSD& data)
{
	LLSelectMgr::getInstance()->selectionRevertColors();
}

void LLPanelFace::onSelectColor(const LLSD& data)
{
	LLSelectMgr::getInstance()->saveSelectedObjectColors();
	sendColor();
}

// static
void LLPanelFace::onCommitMaterialsMedia(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	LLComboBox* combo_matmedia = self->getChild<LLComboBox>("combobox matmedia");
	LLComboBox* combo_mattype = self->getChild<LLComboBox>("combobox mattype");
	LLComboBox* combo_shininess = self->getChild<LLComboBox>("combobox shininess");
	LLComboBox* combo_bumpiness = self->getChild<LLComboBox>("combobox bumpiness");
	if (!combo_mattype || !combo_matmedia || !combo_shininess || !combo_bumpiness)
	{
		llwarns << "Combo box not found...exiting." << llendl;
		return;
	}
	U32 materials_media = combo_matmedia->getCurrentIndex();
	U32 material_type = combo_mattype->getCurrentIndex();
	bool show_media = (materials_media == MATMEDIA_MEDIA) && combo_matmedia->getEnabled();
	bool show_texture = (!show_media) && (material_type == MATTYPE_DIFFUSE) && combo_matmedia->getEnabled();
	bool show_bumpiness = (!show_media) && (material_type == MATTYPE_NORMAL) && combo_matmedia->getEnabled();
	bool show_shininess = (!show_media) && (material_type == MATTYPE_SPECULAR) && combo_matmedia->getEnabled();
	self->getChildView("combobox mattype")->setVisible(!show_media);
	self->getChildView("rptctrl")->setVisible(!show_media);

	// Media controls
	self->getChildView("media_info")->setVisible(show_media);
	self->getChildView("add_media")->setVisible(show_media);
	self->getChildView("delete_media")->setVisible(show_media);
	self->getChildView("button align")->setVisible(show_media);

	// Diffuse texture controls
	self->getChildView("texture control")->setVisible(show_texture);
	self->getChildView("label alphamode")->setVisible(show_texture);
	self->getChildView("combobox alphamode")->setVisible(show_texture);
	self->getChildView("label maskcutoff")->setVisible(false);
	self->getChildView("maskcutoff")->setVisible(false);
	if (show_texture)
	{
		updateAlphaControls(ctrl, userdata);
	}
	self->getChildView("TexScaleU")->setVisible(show_texture);
	self->getChildView("TexScaleV")->setVisible(show_texture);
	self->getChildView("TexRot")->setVisible(show_texture);
	self->getChildView("TexOffsetU")->setVisible(show_texture);
	self->getChildView("TexOffsetV")->setVisible(show_texture);

	// Specular map controls
	self->getChildView("shinytexture control")->setVisible(show_shininess);
	self->getChildView("combobox shininess")->setVisible(show_shininess);
	self->getChildView("label shininess")->setVisible(show_shininess);
	self->getChildView("label glossiness")->setVisible(false);
	self->getChildView("glossiness")->setVisible(false);
	self->getChildView("label environment")->setVisible(false);
	self->getChildView("environment")->setVisible(false);
	self->getChildView("label shinycolor")->setVisible(false);
	self->getChildView("shinycolorswatch")->setVisible(false);
	if (show_shininess)
	{
		updateShinyControls(ctrl, userdata);
	}
	self->getChildView("shinyScaleU")->setVisible(show_shininess);
	self->getChildView("shinyScaleV")->setVisible(show_shininess);
	self->getChildView("shinyRot")->setVisible(show_shininess);
	self->getChildView("shinyOffsetU")->setVisible(show_shininess);
	self->getChildView("shinyOffsetV")->setVisible(show_shininess);

	// Normal map controls
	if (show_bumpiness)
	{
		updateBumpyControls(ctrl, userdata);
	}
	self->getChildView("bumpytexture control")->setVisible(show_bumpiness);
	self->getChildView("combobox bumpiness")->setVisible(show_bumpiness);
	self->getChildView("label bumpiness")->setVisible(show_bumpiness);
	self->getChildView("bumpyScaleU")->setVisible(show_bumpiness);
	self->getChildView("bumpyScaleV")->setVisible(show_bumpiness);
	self->getChildView("bumpyRot")->setVisible(show_bumpiness);
	self->getChildView("bumpyOffsetU")->setVisible(show_bumpiness);
	self->getChildView("bumpyOffsetV")->setVisible(show_bumpiness);

	// Enable texture scale/rotation/offset parameters if there's one
	// present to set for
	bool texParmsEnable = show_texture ||
		(show_shininess && (combo_shininess->getCurrentIndex() == SHINY_TEXTURE)) ||
		(show_bumpiness && (combo_bumpiness->getCurrentIndex() == BUMPY_TEXTURE));
	self->getChildView("tex gen")->setEnabled(texParmsEnable);
	self->getChildView("combobox texgen")->setEnabled(texParmsEnable);
	self->getChildView("rptctrl")->setEnabled(texParmsEnable);
	self->getChildView("TexScaleU")->setEnabled(texParmsEnable);
	self->getChildView("TexScaleV")->setEnabled(texParmsEnable);
	self->getChildView("TexRot")->setEnabled(texParmsEnable);
	self->getChildView("TexOffsetU")->setEnabled(texParmsEnable);
	self->getChildView("TexOffsetV")->setEnabled(texParmsEnable);
	self->getChildView("shinyScaleU")->setEnabled(texParmsEnable);
	self->getChildView("shinyScaleV")->setEnabled(texParmsEnable);
	self->getChildView("shinyRot")->setEnabled(texParmsEnable);
	self->getChildView("shinyOffsetU")->setEnabled(texParmsEnable);
	self->getChildView("shinyOffsetV")->setEnabled(texParmsEnable);
	self->getChildView("bumpyScaleU")->setEnabled(texParmsEnable);
	self->getChildView("bumpyScaleV")->setEnabled(texParmsEnable);
	self->getChildView("bumpyRot")->setEnabled(texParmsEnable);
	self->getChildView("bumpyOffsetU")->setEnabled(texParmsEnable);
	self->getChildView("bumpyOffsetV")->setEnabled(texParmsEnable);
	self->getChildView("checkbox planar align")->setEnabled(texParmsEnable);
}

// static
void LLPanelFace::onCommitMaterialType(LLUICtrl* ctrl, void* userdata)
{
    LLPanelFace* self = (LLPanelFace*) userdata;
    // This is here to insure that we properly update shared UI elements
    // like the texture ctrls for diffuse/norm/spec so that they are correct
    // when switching modes
    //
    self->getState();
}

// static
void LLPanelFace::onCommitBump(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendBump();

	LLComboBox* combo_bumpy = self->getChild<LLComboBox>("combobox bumpiness");
	// Need 'true' here to insure that the 'Use Texture' choice is removed
	// when we select something other than a spec texture
	//
	updateBumpyControls(combo_bumpy,self, true);
	self->updateMaterial();
}

// static
void LLPanelFace::onCommitTexGen(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendTexGen();
}

// static
void LLPanelFace::updateShinyControls(LLUICtrl* ctrl, void* userdata, bool mess_with_shiny_combobox)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	LLTextureCtrl* texture_ctrl = self->getChild<LLTextureCtrl>("shinytexture control");
	LLUUID shiny_texture_ID = texture_ctrl->getImageAssetID();
	LL_DEBUGS("Materials") << "Shiny texture selected: " << shiny_texture_ID << LL_ENDL;
	LLComboBox* comboShiny = self->getChild<LLComboBox>("combobox shininess");

	if(mess_with_shiny_combobox)
	{
		if (!comboShiny)
		{
			return;
		}
		if (!shiny_texture_ID.isNull())
		{
			if (!comboShiny->itemExists(USE_TEXTURE))
			{
				comboShiny->add(USE_TEXTURE);
			}
			comboShiny->setSimple(USE_TEXTURE);
		}
		else
		{
			if (comboShiny->itemExists(USE_TEXTURE))
			{
				// HACK: This depends on adding the "Use texture"
				//	item at the end of a list of known length.
				comboShiny->remove(SHINY_TEXTURE);
			}
		}
	}

	LLComboBox* combo_matmedia = self->getChild<LLComboBox>("combobox matmedia");
	LLComboBox* combo_mattype = self->getChild<LLComboBox>("combobox mattype");
	U32 materials_media = combo_matmedia->getCurrentIndex();
	U32 material_type = combo_mattype->getCurrentIndex();
	bool show_media = (materials_media == MATMEDIA_MEDIA) && combo_matmedia->getEnabled();
	bool show_shininess = (!show_media) && (material_type == MATTYPE_SPECULAR) && combo_matmedia->getEnabled();
	U32 shiny_value = comboShiny->getCurrentIndex();
	bool show_shinyctrls = (shiny_value == SHINY_TEXTURE) && show_shininess; // Use texture
	self->getChildView("label glossiness")->setVisible(show_shinyctrls);
	self->getChildView("glossiness")->setVisible(show_shinyctrls);
	self->getChildView("label environment")->setVisible(show_shinyctrls);
	self->getChildView("environment")->setVisible(show_shinyctrls);
	self->getChildView("label shinycolor")->setVisible(show_shinyctrls);
	self->getChildView("shinycolorswatch")->setVisible(show_shinyctrls);
}

// static
void LLPanelFace::updateBumpyControls(LLUICtrl* ctrl, void* userdata, bool mess_with_combobox)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	LLTextureCtrl* texture_ctrl = self->getChild<LLTextureCtrl>("bumpytexture control");
	LLUUID bumpy_texture_ID = texture_ctrl->getImageAssetID();
	LL_DEBUGS("Materials") << "Bumpy texture selected: " << bumpy_texture_ID << LL_ENDL;
	LLComboBox* comboBumpy = self->getChild<LLComboBox>("combobox bumpiness");
	if (!comboBumpy)
	{
		return;
	}

	if (mess_with_combobox)
	{
		if (!bumpy_texture_ID.isNull())
		{
			if (!comboBumpy->itemExists(USE_TEXTURE))
			{
				comboBumpy->add(USE_TEXTURE);
			}
			comboBumpy->setSimple(USE_TEXTURE);
		}
		else
		{
			if (comboBumpy->itemExists(USE_TEXTURE))
			{
				// HACK: This depends on adding the "Use texture"
				//	item at the end of a list of known length.
				comboBumpy->remove(BUMPY_TEXTURE);
			}
		}
	}
}

// static
void LLPanelFace::onCommitShiny(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendShiny();	
	LLComboBox* combo_shiny = self->getChild<LLComboBox>("combobox shininess");
	// Need 'true' here to insure that the 'Use Texture' choice is removed
	// when we select something other than a spec texture
	//
	updateShinyControls(combo_shiny,self, true);
	self->updateMaterial();
}

// static
void LLPanelFace::updateAlphaControls(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	LLComboBox* comboAlphaMode = self->getChild<LLComboBox>("combobox alphamode");
	if (!comboAlphaMode)
	{
		return;
	}
	U32 alpha_value = comboAlphaMode->getCurrentIndex();
	bool show_alphactrls = (alpha_value == ALPHAMODE_MASK); // Alpha masking
    
    LLComboBox* combobox_matmedia = self->getChild<LLComboBox>("combobox matmedia");
    U32 mat_media = MATMEDIA_MATERIAL;
    if (combobox_matmedia)
    {
        mat_media = combobox_matmedia->getCurrentIndex();
    }
    
    LLComboBox* combobox_mattype = self->getChild<LLComboBox>("combobox mattype");
    U32 mat_type = MATTYPE_DIFFUSE;
    if (combobox_mattype)
    {
        mat_type = combobox_mattype->getCurrentIndex();
    }

    show_alphactrls = show_alphactrls && (mat_media == MATMEDIA_MATERIAL);
    show_alphactrls = show_alphactrls && (mat_type == MATTYPE_DIFFUSE);
    
	self->getChildView("label maskcutoff")->setVisible(show_alphactrls);
	self->getChildView("maskcutoff")->setVisible(show_alphactrls);
}

// static
void LLPanelFace::onCommitAlphaMode(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	updateAlphaControls(ctrl,userdata);
	self->updateMaterial();
}

// static
void LLPanelFace::onCommitFullbright(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendFullbright();
}

// static
void LLPanelFace::onCommitGlow(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendGlow();
}

// static
BOOL LLPanelFace::onDragTexture(LLUICtrl*, LLInventoryItem* item)
{
	BOOL accept = TRUE;
	for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* obj = node->getObject();
		if(!LLToolDragAndDrop::isInventoryDropAcceptable(obj, item))
		{
			accept = FALSE;
			break;
		}
	}
	return accept;
}

void LLPanelFace::onCommitTexture( const LLSD& data )
{
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_EDIT_TEXTURE_COUNT );
	sendTexture();
}

void LLPanelFace::onCancelTexture(const LLSD& data)
{
	LLSelectMgr::getInstance()->selectionRevertTextures();
}

void LLPanelFace::onSelectTexture(const LLSD& data)
{
	LLSelectMgr::getInstance()->saveSelectedObjectTextures();
	sendTexture();
}

void LLPanelFace::onCommitMaterialTexture( const LLSD& data )
{
	LL_DEBUGS("Materials") << data << LL_ENDL;
	updateMaterial();
	LLComboBox* combo_shiny = getChild<LLComboBox>("combobox shininess");
	updateShinyControls(combo_shiny,this, true);
	LLComboBox* combo_bumpy = getChild<LLComboBox>("combobox bumpiness");
	updateBumpyControls(combo_bumpy,this, true);
}

void LLPanelFace::onCancelMaterialTexture(const LLSD& data)
{
	// not sure what to do here other than
	updateMaterial();
	LLComboBox* combo_shiny = getChild<LLComboBox>("combobox shininess");
	updateShinyControls(combo_shiny,this, true);
	LLComboBox* combo_bumpy = getChild<LLComboBox>("combobox bumpiness");
	updateBumpyControls(combo_bumpy,this, true);
}

void LLPanelFace::onSelectMaterialTexture(const LLSD& data)
{
	LL_DEBUGS("Materials") << data << LL_ENDL;
	updateMaterial();
	LLComboBox* combo_shiny = getChild<LLComboBox>("combobox shininess");
	updateShinyControls(combo_shiny,this, true);
	LLComboBox* combo_bumpy = getChild<LLComboBox>("combobox bumpiness");
	updateBumpyControls(combo_bumpy,this, true);
}

//static
void LLPanelFace::onCommitMaterial(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->updateMaterial();
}

// static
void LLPanelFace::onCommitTextureInfo( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendTextureInfo();
}

// Commit the number of repeats per meter
// static
void LLPanelFace::onCommitRepeatsPerMeter(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	
	gFocusMgr.setKeyboardFocus( NULL );

	//F32 repeats_per_meter = self->mCtrlRepeatsPerMeter->get();
	F32 repeats_per_meter = (F32)self->getChild<LLUICtrl>("rptctrl")->getValue().asReal();//self->mCtrlRepeatsPerMeter->get();

	LLComboBox* combo_mattype = self->getChild<LLComboBox>("combobox mattype");
	
	U32 material_type = combo_mattype->getCurrentIndex();

	switch (material_type)
	{
		case MATTYPE_DIFFUSE:
		{
			LLSelectMgr::getInstance()->selectionTexScaleAutofit( repeats_per_meter );
		}
		break;

		case MATTYPE_NORMAL:
		{
			LLUICtrl* bumpy_scale_u = self->getChild<LLUICtrl>("bumpyScaleU");
			LLUICtrl* bumpy_scale_v = self->getChild<LLUICtrl>("bumpyScaleV");
			bumpy_scale_u->setValue(bumpy_scale_u->getValue().asReal() * repeats_per_meter);
			bumpy_scale_v->setValue(bumpy_scale_v->getValue().asReal() * repeats_per_meter);
			self->updateMaterial();
		}
		break;

		case MATTYPE_SPECULAR:
		{
			LLUICtrl* shiny_scale_u = self->getChild<LLUICtrl>("shinyScaleU");
			LLUICtrl* shiny_scale_v = self->getChild<LLUICtrl>("shinyScaleV");
			shiny_scale_u->setValue(shiny_scale_u->getValue().asReal() * repeats_per_meter);
			shiny_scale_v->setValue(shiny_scale_v->getValue().asReal() * repeats_per_meter);
			self->updateMaterial();
		}
		break;

		default:
			llassert(false);
		break;
	}
}

struct LLPanelFaceSetMediaFunctor : public LLSelectedTEFunctor
{
	virtual bool apply(LLViewerObject* object, S32 te)
	{
		viewer_media_t pMediaImpl;
				
		const LLTextureEntry* tep = object->getTE(te);
		const LLMediaEntry* mep = tep->hasMedia() ? tep->getMediaData() : NULL;
		if ( mep )
		{
			pMediaImpl = LLViewerMedia::getMediaImplFromTextureID(mep->getMediaID());
		}
		
		if ( pMediaImpl.isNull())
		{
			// If we didn't find face media for this face, check whether this face is showing parcel media.
			pMediaImpl = LLViewerMedia::getMediaImplFromTextureID(tep->getID());
		}
		
		if ( pMediaImpl.notNull())
		{
			LLPluginClassMedia *media = pMediaImpl->getMediaPlugin();
			if(media)
			{
				S32 media_width = media->getWidth();
				S32 media_height = media->getHeight();
				S32 texture_width = media->getTextureWidth();
				S32 texture_height = media->getTextureHeight();
				F32 scale_s = (F32)media_width / (F32)texture_width;
				F32 scale_t = (F32)media_height / (F32)texture_height;

				// set scale and adjust offset
				object->setTEScaleS( te, scale_s );
				object->setTEScaleT( te, scale_t );	// don't need to flip Y anymore since QT does this for us now.
				object->setTEOffsetS( te, -( 1.0f - scale_s ) / 2.0f );
				object->setTEOffsetT( te, -( 1.0f - scale_t ) / 2.0f );
			}
		}
		return true;
	};
};

void LLPanelFace::onClickAutoFix(void* userdata)
{
	LLPanelFaceSetMediaFunctor setfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);

	LLPanelFaceSendFunctor sendfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
}



// TODO: I don't know who put these in or what these are for???
void LLPanelFace::setMediaURL(const std::string& url)
{
}
void LLPanelFace::setMediaType(const std::string& mime_type)
{
}

// static
void LLPanelFace::onCommitPlanarAlign(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->getState();
	self->sendTextureInfo();
}

void LLPanelFace::onTextureSelectionChanged(LLInventoryItem* itemp)
{
	LLComboBox* combo_mattype = getChild<LLComboBox>("combobox mattype");
	if (!combo_mattype)
	{
		return;
	}
	U32 mattype = combo_mattype->getCurrentIndex();
	std::string which_control="texture control";
	switch (mattype)
	{
		case MATTYPE_SPECULAR:
			which_control = "shinytexture control";
			break;
		case MATTYPE_NORMAL:
			which_control = "bumpytexture control";
			break;
		// no default needed
	}
		
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>(which_control);
	if (texture_ctrl)
	{
		LLUUID obj_owner_id;
		std::string obj_owner_name;
		LLSelectMgr::instance().selectGetOwner(obj_owner_id, obj_owner_name);

		LLSaleInfo sale_info;
		LLSelectMgr::instance().selectGetSaleInfo(sale_info);

		bool can_copy = itemp->getPermissions().allowCopyBy(gAgentID); // do we have perm to copy this texture?
		bool can_transfer = itemp->getPermissions().allowOperationBy(PERM_TRANSFER, gAgentID); // do we have perm to transfer this texture?
		bool is_object_owner = gAgentID == obj_owner_id; // does object for which we are going to apply texture belong to the agent?
		bool not_for_sale = !sale_info.isForSale(); // is object for which we are going to apply texture not for sale?

		if (can_copy && can_transfer)
		{
			texture_ctrl->setCanApply(true, true);
			return;
		}

		// if texture has (no-transfer) attribute it can be applied only for object which we own and is not for sale
		texture_ctrl->setCanApply(false, can_transfer ? true : is_object_owner && not_for_sale);

		if (gSavedSettings.getBOOL("TextureLivePreview"))
		{
			LLNotificationsUtil::add("LivePreviewUnavailable");
		}
	}
}

