/** 
 * @file llpanelface.cpp
 * @brief Panel in the tools floater for editing face textures, colors, etc.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

// file include
#include "llpanelface.h"
 
// library includes
#include "llerror.h"
#include "llfocusmgr.h"
#include "llrect.h"
#include "llstring.h"
#include "llfontgl.h"

// project includes
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "lldrawpoolbump.h"
#include "lllineeditor.h"
#include "llmediaentry.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "lltextureentry.h"
#include "lltooldraganddrop.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewerobject.h"
#include "llviewerstats.h"
#include "llvovolume.h"
#include "lluictrlfactory.h"
#include "llpluginclassmedia.h"
#include "llviewertexturelist.h"

//
// Methods
//

BOOL	LLPanelFace::postBuild()
{
	childSetCommitCallback("combobox shininess",&LLPanelFace::onCommitShiny,this);
	childSetCommitCallback("combobox bumpiness",&LLPanelFace::onCommitBump,this);
	childSetCommitCallback("TexScaleU",&LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("checkbox flip s",&LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexScaleV",&LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("checkbox flip t",&LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexRot",&LLPanelFace::onCommitTextureInfo, this);
	childSetAction("button apply",&LLPanelFace::onClickApply,this);
	childSetCommitCallback("TexOffsetU",LLPanelFace::onCommitTextureInfo, this);
	childSetCommitCallback("TexOffsetV",LLPanelFace::onCommitTextureInfo, this);
	childSetAction("button align",&LLPanelFace::onClickAutoFix,this);

	LLRect	rect = this->getRect();
	LLTextureCtrl*	mTextureCtrl;
	LLColorSwatchCtrl*	mColorSwatch;

	LLComboBox*		mComboTexGen;

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
		mTextureCtrl->setFollowsTop();
		mTextureCtrl->setFollowsLeft();
		// Don't allow (no copy) or (no transfer) textures to be selected during immediate mode
		mTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
		// Allow any texture to be used during non-immediate mode.
		mTextureCtrl->setNonImmediateFilterPermMask(PERM_NONE);
		LLAggregatePermissions texture_perms;
		if (LLSelectMgr::getInstance()->selectGetAggregateTexturePermissions(texture_perms))
		{
			BOOL can_copy = 
				texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_EMPTY || 
				texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_ALL;
			BOOL can_transfer = 
				texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_EMPTY || 
				texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_ALL;
			mTextureCtrl->setCanApplyImmediately(can_copy && can_transfer);
		}
		else
		{
			mTextureCtrl->setCanApplyImmediately(FALSE);
		}
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

	mCtrlGlow = getChild<LLSpinCtrl>("glow");
	if(mCtrlGlow)
	{
		mCtrlGlow->setCommitCallback(LLPanelFace::onCommitGlow, this);
	}
	

	clearCtrls();

	return TRUE;
}

LLPanelFace::LLPanelFace()
:	LLPanel()
{
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
	U8 bump = (U8) mComboBumpiness->getCurrentIndex() & TEM_BUMP_MASK;
	LLSelectMgr::getInstance()->selectionSetBumpmap( bump );
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
	U8 shiny = (U8) mComboShininess->getCurrentIndex() & TEM_SHINY_MASK;
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
	LLSpinCtrl*	mCtrlGlow = getChild<LLSpinCtrl>("glow");
	F32 glow = mCtrlGlow->get();

	LLSelectMgr::getInstance()->selectionSetGlow( glow );
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
		LLCheckBoxCtrl*	checkFlipScaleS = mPanel->getChild<LLCheckBoxCtrl>("checkbox flip s");
		LLCheckBoxCtrl*	checkFlipScaleT = mPanel->getChild<LLCheckBoxCtrl>("checkbox flip t");
		LLComboBox*		comboTexGen = mPanel->getChild<LLComboBox>("combobox texgen");
		if (ctrlTexScaleS)
		{
			valid = !ctrlTexScaleS->getTentative() || !checkFlipScaleS->getTentative();
			if (valid)
			{
				value = ctrlTexScaleS->get();
				if( checkFlipScaleS->get() )
				{
					value = -value;
				}
				if (comboTexGen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleS( te, value );
			}
		}

		if (ctrlTexScaleT)
		{
			valid = !ctrlTexScaleT->getTentative() || !checkFlipScaleT->getTentative();
			if (valid)
			{
				value = ctrlTexScaleT->get();
				if( checkFlipScaleT->get() )
				{
					value = -value;
				}
				if (comboTexGen->getCurrentIndex() == 1)
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
	LLPanelFaceSetTEFunctor setfunc(this);
	LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);

	LLPanelFaceSendFunctor sendfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
}

void LLPanelFace::getState()
{
	LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();

	if( objectp
		&& objectp->getPCode() == LL_PCODE_VOLUME
		&& objectp->permModify())
	{
		BOOL editable = objectp->permModify();

		// only turn on auto-adjust button if there is a media renderer and the media is loaded
		childSetEnabled("textbox autofix", editable);
		childSetEnabled("button align", editable);
		
		//if ( LLMediaEngine::getInstance()->getMediaRenderer () )
		//	if ( LLMediaEngine::getInstance()->getMediaRenderer ()->isLoaded () )
		//	{	
		//		
		//		//mLabelTexAutoFix->setEnabled ( editable );
		//		
		//		//mBtnAutoFix->setEnabled ( editable );
		//	}
		childSetEnabled("button apply",editable);

		bool identical;
		LLTextureCtrl*	texture_ctrl = getChild<LLTextureCtrl>("texture control");
		
		// Texture
		{
			LLUUID id;
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
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, id );

			if(LLViewerMedia::textureHasMedia(id))
			{
				childSetEnabled("textbox autofix",editable);
				childSetEnabled("button align",editable);
			}
			
			if (identical)
			{
				// All selected have the same texture
				if(texture_ctrl)
				{
					texture_ctrl->setTentative( FALSE );
					texture_ctrl->setEnabled( editable );
					texture_ctrl->setImageAssetID( id );
				}
			}
			else
			{
				if(texture_ctrl)
				{
					if( id.isNull() )
					{
						// None selected
						texture_ctrl->setTentative( FALSE );
						texture_ctrl->setEnabled( FALSE );
						texture_ctrl->setImageAssetID( LLUUID::null );
					}
					else
					{
						// Tentative: multiple selected with different textures
						texture_ctrl->setTentative( TRUE );
						texture_ctrl->setEnabled( editable );
						texture_ctrl->setImageAssetID( id );
					}
				}
			}
		}

		
		LLAggregatePermissions texture_perms;
		if(texture_ctrl)
		{
// 			texture_ctrl->setValid( editable );
		
			if (LLSelectMgr::getInstance()->selectGetAggregateTexturePermissions(texture_perms))
			{
				BOOL can_copy = 
					texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_EMPTY || 
					texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_ALL;
				BOOL can_transfer = 
					texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_EMPTY || 
					texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_ALL;
				texture_ctrl->setCanApplyImmediately(can_copy && can_transfer);
			}
			else
			{
				texture_ctrl->setCanApplyImmediately(FALSE);
			}
		}

		// Texture scale
		{
			childSetEnabled("tex scale",editable);
			//mLabelTexScale->setEnabled( editable );
			F32 scale_s = 1.f;
			struct f2 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mScaleS;
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, scale_s );
			childSetValue("TexScaleU",editable ? llabs(scale_s) : 0);
			childSetTentative("TexScaleU",LLSD((BOOL)(!identical)));
			childSetEnabled("TexScaleU",editable);
			childSetValue("checkbox flip s",LLSD((BOOL)(scale_s < 0 ? TRUE : FALSE )));
			childSetTentative("checkbox flip s",LLSD((BOOL)((!identical) ? TRUE : FALSE )));
			childSetEnabled("checkbox flip s",editable);
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

			childSetValue("TexScaleV",llabs(editable ? llabs(scale_t) : 0));
			childSetTentative("TexScaleV",LLSD((BOOL)(!identical)));
			childSetEnabled("TexScaleV",editable);
			childSetValue("checkbox flip t",LLSD((BOOL)(scale_t< 0 ? TRUE : FALSE )));
			childSetTentative("checkbox flip t",LLSD((BOOL)((!identical) ? TRUE : FALSE )));
			childSetEnabled("checkbox flip t",editable);
		}

		// Texture offset
		{
			childSetEnabled("tex offset",editable);
			F32 offset_s = 0.f;
			struct f4 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mOffsetS;
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, offset_s );
			childSetValue("TexOffsetU", editable ? offset_s : 0);
			childSetTentative("TexOffsetU",!identical);
			childSetEnabled("TexOffsetU",editable);
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
			childSetValue("TexOffsetV", editable ? offset_t : 0);
			childSetTentative("TexOffsetV",!identical);
			childSetEnabled("TexOffsetV",editable);
		}

		// Texture rotation
		{
			childSetEnabled("tex rotate",editable);
			F32 rotation = 0.f;
			struct f6 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mRotation;
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, rotation );
			childSetValue("TexRot", editable ? rotation * RAD_TO_DEG : 0);
			childSetTentative("TexRot",!identical);
			childSetEnabled("TexRot",editable);
		}

		// Color swatch
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
			childSetEnabled("color trans",editable);
		}

		F32 transparency = (1.f - color.mV[VALPHA]) * 100.f;
		{
			childSetValue("ColorTrans", editable ? transparency : 0);
			childSetEnabled("ColorTrans",editable);
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

			childSetValue("glow",glow);
			childSetEnabled("glow",editable);
			childSetTentative("glow",!identical);
			childSetEnabled("glow label",editable);

		}

		// Bump
		{
			F32 shinyf = 0.f;
			struct f9 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return (F32)(object->getTE(face)->getShiny());
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, shinyf );
			LLCtrlSelectionInterface* combobox_shininess =
			      childGetSelectionInterface("combobox shininess");
			if (combobox_shininess)
			{
				combobox_shininess->selectNthItem((S32)shinyf);
			}
			else
			{
				llwarns << "failed childGetSelectionInterface for 'combobox shininess'" << llendl;
			}
			childSetEnabled("combobox shininess",editable);
			childSetTentative("combobox shininess",!identical);
			childSetEnabled("label shininess",editable);
		}

		{
			F32 bumpf = 0.f;
			struct f10 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return (F32)(object->getTE(face)->getBumpmap());
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, bumpf );
			LLCtrlSelectionInterface* combobox_bumpiness =
			      childGetSelectionInterface("combobox bumpiness");
			if (combobox_bumpiness)
			{
				combobox_bumpiness->selectNthItem((S32)bumpf);
			}
			else
			{
				llwarns << "failed childGetSelectionInterface for 'combobox bumpiness'" << llendl;
			}
			childSetEnabled("combobox bumpiness",editable);
			childSetTentative("combobox bumpiness",!identical);
			childSetEnabled("label bumpiness",editable);
		}

		{
			F32 genf = 0.f;
			struct f11 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return (F32)(object->getTE(face)->getTexGen());
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, genf );
			S32 selected_texgen = ((S32) genf) >> TEM_TEX_GEN_SHIFT;
			LLCtrlSelectionInterface* combobox_texgen =
			      childGetSelectionInterface("combobox texgen");
			if (combobox_texgen)
			{
				combobox_texgen->selectNthItem(selected_texgen);
			}
			else
			{
				llwarns << "failed childGetSelectionInterface for 'combobox texgen'" << llendl;
			}
			childSetEnabled("combobox texgen",editable);
			childSetTentative("combobox texgen",!identical);
			childSetEnabled("tex gen",editable);

			if (selected_texgen == 1)
			{
				childSetText("tex scale",getString("string repeats per meter"));
				childSetValue("TexScaleU", 2.0f * childGetValue("TexScaleU").asReal() );
				childSetValue("TexScaleV", 2.0f * childGetValue("TexScaleV").asReal() );
			}
			else
			{
				childSetText("tex scale",getString("string repeats per face"));
			}

		}

		{
			F32 fullbrightf = 0.f;
			struct f12 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return (F32)(object->getTE(face)->getFullbright());
				}
			} func;
			identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, fullbrightf );

			childSetValue("checkbox fullbright",(S32)fullbrightf);
			childSetEnabled("checkbox fullbright",editable);
			childSetTentative("checkbox fullbright",!identical);
		}
		
		// Repeats per meter label
		{
			childSetEnabled("rpt",editable);
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
			
			childSetValue("rptctrl", editable ? repeats : 0);
			childSetTentative("rptctrl",!identical);
			LLComboBox*	mComboTexGen = getChild<LLComboBox>("combobox texgen");
			if (mComboTexGen)
			{
				BOOL enabled = editable && (!mComboTexGen || mComboTexGen->getCurrentIndex() != 1);
				childSetEnabled("rptctrl",enabled);
				childSetEnabled("button apply",enabled);
			}
		}
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
			texture_ctrl->setFallbackImageName( "locked_image.j2c" );
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
		childSetEnabled("color trans",FALSE);
		childSetEnabled("rpt",FALSE);
		childSetEnabled("tex scale",FALSE);
		childSetEnabled("tex offset",FALSE);
		childSetEnabled("tex rotate",FALSE);
		childSetEnabled("tex gen",FALSE);
		childSetEnabled("label shininess",FALSE);
		childSetEnabled("label bumpiness",FALSE);

		childSetEnabled("textbox autofix",FALSE);

		childSetEnabled("button align",FALSE);
		childSetEnabled("button apply",FALSE);
		//childSetEnabled("has media", FALSE);
		//childSetEnabled("media info set", FALSE);
		
	}
}


void LLPanelFace::refresh()
{
	getState();
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
void LLPanelFace::onCommitBump(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendBump();
}

// static
void LLPanelFace::onCommitTexGen(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendTexGen();
}

// static
void LLPanelFace::onCommitShiny(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendShiny();
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


// static
void LLPanelFace::onCommitTextureInfo( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendTextureInfo();
}

// Commit the number of repeats per meter
// static
void LLPanelFace::onClickApply(void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	
	gFocusMgr.setKeyboardFocus( NULL );

	//F32 repeats_per_meter = self->mCtrlRepeatsPerMeter->get();
	F32 repeats_per_meter = (F32)self->childGetValue( "rptctrl" ).asReal();//self->mCtrlRepeatsPerMeter->get();
	LLSelectMgr::getInstance()->selectionTexScaleAutofit( repeats_per_meter );
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

