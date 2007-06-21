/** 
 * @file llpanelobject.cpp
 * @brief Object editing (position, scale, etc.) in the tools floater
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

// file include
#include "llpanelobject.h"

// linden library includes
#include "lleconomy.h"
#include "llerror.h"
#include "llfontgl.h"
#include "llpermissionsflags.h"
#include "llstring.h"
#include "llvolume.h"
#include "m3math.h"

// project includes
#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "llfocusmgr.h"
#include "llmanipscale.h"
#include "llpanelinventory.h"
#include "llpreviewscript.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "lltexturectrl.h"
#include "lltextbox.h"
#include "lltool.h"
#include "lltoolcomp.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvovolume.h"
#include "llworld.h"
#include "pipeline.h"
#include "viewer.h"
#include "llvieweruictrlfactory.h"
#include "llfirstuse.h"

#include "lldrawpool.h"

//
// Constants
//
enum {
	MI_BOX,
	MI_CYLINDER,
	MI_PRISM,
	MI_SPHERE,
	MI_TORUS,
	MI_TUBE,
	MI_RING,
	MI_SCULPT,
	MI_NONE,
	MI_VOLUME_COUNT
};

enum {
	MI_HOLE_SAME,
	MI_HOLE_CIRCLE,
	MI_HOLE_SQUARE,
	MI_HOLE_TRIANGLE,
	MI_HOLE_COUNT
};

//XUI:translate (depricated, so very low priority)
static const LLString LEGACY_FULLBRIGHT_DESC("Fullbright (Legacy)");

BOOL	LLPanelObject::postBuild()
{
	setMouseOpaque(FALSE);

	//--------------------------------------------------------
	// Top
	//--------------------------------------------------------
	
	// Lock checkbox
	mCheckLock = gUICtrlFactory->getCheckBoxByName(this,"checkbox locked");
	childSetCommitCallback("checkbox locked",onCommitLock,this);

	// Physical checkbox
	mCheckPhysics = gUICtrlFactory->getCheckBoxByName(this,"Physical Checkbox Ctrl");
	childSetCommitCallback("Physical Checkbox Ctrl",onCommitPhysics,this);

	// Temporary checkbox
	mCheckTemporary = gUICtrlFactory->getCheckBoxByName(this,"Temporary Checkbox Ctrl");
	childSetCommitCallback("Temporary Checkbox Ctrl",onCommitTemporary,this);

	// Phantom checkbox
	mCheckPhantom = gUICtrlFactory->getCheckBoxByName(this,"Phantom Checkbox Ctrl");
	childSetCommitCallback("Phantom Checkbox Ctrl",onCommitPhantom,this);
	
	// Position
	mLabelPosition = gUICtrlFactory->getTextBoxByName(this,"label position");
	mCtrlPosX = gUICtrlFactory->getSpinnerByName(this,"Pos X");
	childSetCommitCallback("Pos X",onCommitPosition,this);
	mCtrlPosY = gUICtrlFactory->getSpinnerByName(this,"Pos Y");
	childSetCommitCallback("Pos Y",onCommitPosition,this);
	mCtrlPosZ = gUICtrlFactory->getSpinnerByName(this,"Pos Z");
	childSetCommitCallback("Pos Z",onCommitPosition,this);

	// Scale
	mLabelSize = gUICtrlFactory->getTextBoxByName(this,"label size");
	mCtrlScaleX = gUICtrlFactory->getSpinnerByName(this,"Scale X");
	childSetCommitCallback("Scale X",onCommitScale,this);

	// Scale Y
	mCtrlScaleY = gUICtrlFactory->getSpinnerByName(this,"Scale Y");
	childSetCommitCallback("Scale Y",onCommitScale,this);

	// Scale Z
	mCtrlScaleZ = gUICtrlFactory->getSpinnerByName(this,"Scale Z");
	childSetCommitCallback("Scale Z",onCommitScale,this);

	// Rotation
	mLabelRotation = gUICtrlFactory->getTextBoxByName(this,"label rotation");
	mCtrlRotX = gUICtrlFactory->getSpinnerByName(this,"Rot X");
	childSetCommitCallback("Rot X",onCommitRotation,this);
	mCtrlRotY = gUICtrlFactory->getSpinnerByName(this,"Rot Y");
	childSetCommitCallback("Rot Y",onCommitRotation,this);
	mCtrlRotZ = gUICtrlFactory->getSpinnerByName(this,"Rot Z");
	childSetCommitCallback("Rot Z",onCommitRotation,this);

	//--------------------------------------------------------
		
	// material type popup
	mLabelMaterial = gUICtrlFactory->getTextBoxByName(this,"label material");
	mComboMaterial = gUICtrlFactory->getComboBoxByName(this,"material");
	childSetCommitCallback("material",onCommitMaterial,this);
	mComboMaterial->removeall();
	// XUI:translate
	LLMaterialInfo *minfop;
	for (minfop = LLMaterialTable::basic.mMaterialInfoList.getFirstData(); 
		 minfop != NULL; 
		 minfop = LLMaterialTable::basic.mMaterialInfoList.getNextData())
	{
		if (minfop->mMCode != LL_MCODE_LIGHT)
		{
			mComboMaterial->add(minfop->mName);
		}
	}
	mComboMaterialItemCount = mComboMaterial->getItemCount();

	// Base Type
	mLabelBaseType = gUICtrlFactory->getTextBoxByName(this,"label basetype");
	mComboBaseType = gUICtrlFactory->getComboBoxByName(this,"comboBaseType");
	childSetCommitCallback("comboBaseType",onCommitParametric,this);

	// Cut
	mLabelCut = gUICtrlFactory->getTextBoxByName(this,"text cut");
	mSpinCutBegin = gUICtrlFactory->getSpinnerByName(this,"cut begin");
	childSetCommitCallback("cut begin",onCommitParametric,this);
	mSpinCutBegin->setValidateBeforeCommit( precommitValidate );
	mSpinCutEnd = gUICtrlFactory->getSpinnerByName(this,"cut end");
	childSetCommitCallback("cut end",onCommitParametric,this);
	mSpinCutEnd->setValidateBeforeCommit( &precommitValidate );

	// Hollow / Skew
	mLabelHollow = gUICtrlFactory->getTextBoxByName(this,"text hollow");
	mLabelSkew = gUICtrlFactory->getTextBoxByName(this,"text skew");
	mSpinHollow = gUICtrlFactory->getSpinnerByName(this,"Scale 1");
	childSetCommitCallback("Scale 1",onCommitParametric,this);
	mSpinHollow->setValidateBeforeCommit( &precommitValidate );
	mSpinSkew = gUICtrlFactory->getSpinnerByName(this,"Skew");
	childSetCommitCallback("Skew",onCommitParametric,this);
	mSpinSkew->setValidateBeforeCommit( &precommitValidate );
	mLabelHoleType = gUICtrlFactory->getTextBoxByName(this,"Hollow Shape");

	// Hole Type
	mComboHoleType = gUICtrlFactory->getComboBoxByName(this,"hole");
	childSetCommitCallback("hole",onCommitParametric,this);

	// Twist
	mLabelTwist = gUICtrlFactory->getTextBoxByName(this,"text twist");
	mSpinTwistBegin = gUICtrlFactory->getSpinnerByName(this,"Twist Begin");
	childSetCommitCallback("Twist Begin",onCommitParametric,this);
	mSpinTwistBegin->setValidateBeforeCommit( precommitValidate );
	mSpinTwist = gUICtrlFactory->getSpinnerByName(this,"Twist End");
	childSetCommitCallback("Twist End",onCommitParametric,this);
	mSpinTwist->setValidateBeforeCommit( &precommitValidate );

	// Scale
	mSpinScaleX = gUICtrlFactory->getSpinnerByName(this,"Taper Scale X");
	childSetCommitCallback("Taper Scale X",onCommitParametric,this);
	mSpinScaleX->setValidateBeforeCommit( &precommitValidate );
	mSpinScaleY = gUICtrlFactory->getSpinnerByName(this,"Taper Scale Y");
	childSetCommitCallback("Taper Scale Y",onCommitParametric,this);
	mSpinScaleY->setValidateBeforeCommit( &precommitValidate );

	// Shear
	mLabelShear = gUICtrlFactory->getTextBoxByName(this,"text topshear");
	mSpinShearX = gUICtrlFactory->getSpinnerByName(this,"Shear X");
	childSetCommitCallback("Shear X",onCommitParametric,this);
	mSpinShearX->setValidateBeforeCommit( &precommitValidate );
	mSpinShearY = gUICtrlFactory->getSpinnerByName(this,"Shear Y");
	childSetCommitCallback("Shear Y",onCommitParametric,this);
	mSpinShearY->setValidateBeforeCommit( &precommitValidate );

	// Path / Profile
	mCtrlPathBegin = gUICtrlFactory->getSpinnerByName(this,"Path Limit Begin");
	childSetCommitCallback("Path Limit Begin",onCommitParametric,this);
	mCtrlPathBegin->setValidateBeforeCommit( &precommitValidate );
	mCtrlPathEnd = gUICtrlFactory->getSpinnerByName(this,"Path Limit End");
	childSetCommitCallback("Path Limit End",onCommitParametric,this);
	mCtrlPathEnd->setValidateBeforeCommit( &precommitValidate );

	// Taper
	mLabelTaper = gUICtrlFactory->getTextBoxByName(this,"text taper2");
	mSpinTaperX = gUICtrlFactory->getSpinnerByName(this,"Taper X");
	childSetCommitCallback("Taper X",onCommitParametric,this);
	mSpinTaperX->setValidateBeforeCommit( precommitValidate );
	mSpinTaperY = gUICtrlFactory->getSpinnerByName(this,"Taper Y");
	childSetCommitCallback("Taper Y",onCommitParametric,this);
	mSpinTaperY->setValidateBeforeCommit( precommitValidate );
	
	// Radius Offset / Revolutions
	mLabelRadiusOffset = gUICtrlFactory->getTextBoxByName(this,"text radius delta");
	mLabelRevolutions = gUICtrlFactory->getTextBoxByName(this,"text revolutions");
	mSpinRadiusOffset = gUICtrlFactory->getSpinnerByName(this,"Radius Offset");
	childSetCommitCallback("Radius Offset",onCommitParametric,this);
	mSpinRadiusOffset->setValidateBeforeCommit( &precommitValidate );
	mSpinRevolutions = gUICtrlFactory->getSpinnerByName(this,"Revolutions");
	childSetCommitCallback("Revolutions",onCommitParametric,this);
	mSpinRevolutions->setValidateBeforeCommit( &precommitValidate );

	// Sculpt
	mCtrlSculptTexture = LLUICtrlFactory::getTexturePickerByName(this,"sculpt texture control");
	if (mCtrlSculptTexture)
	{
		mCtrlSculptTexture->setDefaultImageAssetID(LLUUID(SCULPT_DEFAULT_TEXTURE));
		mCtrlSculptTexture->setCommitCallback( LLPanelObject::onCommitSculpt );
		mCtrlSculptTexture->setOnCancelCallback( LLPanelObject::onCancelSculpt );
		mCtrlSculptTexture->setOnSelectCallback( LLPanelObject::onSelectSculpt );
		mCtrlSculptTexture->setDropCallback(LLPanelObject::onDropSculpt);
		mCtrlSculptTexture->setCallbackUserData( this );
		// Don't allow (no copy) or (no transfer) textures to be selected during immediate mode
		mCtrlSculptTexture->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
		// Allow any texture to be used during non-immediate mode.
		mCtrlSculptTexture->setNonImmediateFilterPermMask(PERM_NONE);
		LLAggregatePermissions texture_perms;
		if (gSelectMgr->selectGetAggregateTexturePermissions(texture_perms))
		{
			BOOL can_copy =
				texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_EMPTY ||
				texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_ALL;
			BOOL can_transfer =
				texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_EMPTY ||
				texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_ALL;
			mCtrlSculptTexture->setCanApplyImmediately(can_copy && can_transfer);
		}
		else
		{
			mCtrlSculptTexture->setCanApplyImmediately(FALSE);
		}
	}

	
	// Start with everyone disabled
	clearCtrls();

	return TRUE;
}

LLPanelObject::LLPanelObject(const std::string& name)
:	LLPanel(name),
	mIsPhysical(FALSE),
	mIsTemporary(FALSE),
	mIsPhantom(FALSE),
	mCastShadows(TRUE),
	mSelectedType(MI_BOX)
{
}


LLPanelObject::~LLPanelObject()
{
	// Children all cleaned up by default view destructor.
}

void LLPanelObject::getState( )
{
	LLViewerObject* objectp = gSelectMgr->getSelection()->getFirstRootObject();
	LLViewerObject* root_objectp = objectp;
	if(!objectp)
	{
		objectp = gSelectMgr->getSelection()->getFirstObject();
		// *FIX: shouldn't we just keep the child?
		if (objectp)
		{
			LLViewerObject* parentp = objectp->getSubParent();

			if (parentp)
			{
				root_objectp = parentp;
			}
			else
			{
				root_objectp = objectp;
			}
		}
	}

	LLVOVolume *volobjp = NULL;
	if ( objectp && (objectp->getPCode() == LL_PCODE_VOLUME))
	{
		volobjp = (LLVOVolume *)objectp;
	}

	if( !objectp )
	{
		//forfeit focus
		if (gFocusMgr.childHasKeyboardFocus(this))
		{
			gFocusMgr.setKeyboardFocus(NULL, NULL);
		}

		// Disable all text input fields
		clearCtrls();
		return;
	}

	// can move or rotate only linked group with move permissions, or sub-object with move and modify perms
	BOOL enable_move	= objectp->permMove() && !objectp->isAttachment() && (objectp->permModify() || !gSavedSettings.getBOOL("EditLinkedParts"));
	BOOL enable_scale	= objectp->permMove() && objectp->permModify();
	BOOL enable_rotate	= objectp->permMove() && ( (objectp->permModify() && !objectp->isAttachment()) || !gSavedSettings.getBOOL("EditLinkedParts"));

	LLVector3 vec;
	if (enable_move)
	{
		vec = objectp->getPositionEdit();
		mCtrlPosX->set( vec.mV[VX] );
		mCtrlPosY->set( vec.mV[VY] );
		mCtrlPosZ->set( vec.mV[VZ] );
	}
	else
	{
		mCtrlPosX->clear();
		mCtrlPosY->clear();
		mCtrlPosZ->clear();
	}


	mLabelPosition->setEnabled( enable_move );
	mCtrlPosX->setEnabled(enable_move);
	mCtrlPosY->setEnabled(enable_move);
	mCtrlPosZ->setEnabled(enable_move);

	if (enable_scale)
	{
		vec = objectp->getScale();
		mCtrlScaleX->set( vec.mV[VX] );
		mCtrlScaleY->set( vec.mV[VY] );
		mCtrlScaleZ->set( vec.mV[VZ] );
	}
	else
	{
		mCtrlScaleX->clear();
		mCtrlScaleY->clear();
		mCtrlScaleZ->clear();
	}

	mLabelSize->setEnabled( enable_scale );
	mCtrlScaleX->setEnabled( enable_scale );
	mCtrlScaleY->setEnabled( enable_scale );
	mCtrlScaleZ->setEnabled( enable_scale );

	LLQuaternion object_rot = objectp->getRotationEdit();
	object_rot.getEulerAngles(&(mCurEulerDegrees.mV[VX]), &(mCurEulerDegrees.mV[VY]), &(mCurEulerDegrees.mV[VZ]));
	mCurEulerDegrees *= RAD_TO_DEG;
	mCurEulerDegrees.mV[VX] = fmod(llround(mCurEulerDegrees.mV[VX], OBJECT_ROTATION_PRECISION) + 360.f, 360.f);
	mCurEulerDegrees.mV[VY] = fmod(llround(mCurEulerDegrees.mV[VY], OBJECT_ROTATION_PRECISION) + 360.f, 360.f);
	mCurEulerDegrees.mV[VZ] = fmod(llround(mCurEulerDegrees.mV[VZ], OBJECT_ROTATION_PRECISION) + 360.f, 360.f);

	if (enable_rotate)
	{
		mCtrlRotX->set( mCurEulerDegrees.mV[VX] );
		mCtrlRotY->set( mCurEulerDegrees.mV[VY] );
		mCtrlRotZ->set( mCurEulerDegrees.mV[VZ] );
	}
	else
	{
		mCtrlRotX->clear();
		mCtrlRotY->clear();
		mCtrlRotZ->clear();
	}

	mLabelRotation->setEnabled( enable_rotate );
	mCtrlRotX->setEnabled( enable_rotate );
	mCtrlRotY->setEnabled( enable_rotate );
	mCtrlRotZ->setEnabled( enable_rotate );

	BOOL owners_identical;
	LLUUID owner_id;
	LLString owner_name;
	owners_identical = gSelectMgr->selectGetOwner(owner_id, owner_name);

	// BUG? Check for all objects being editable?
	S32 roots_selected = gSelectMgr->getSelection()->getRootObjectCount();
	BOOL editable = root_objectp->permModify();
	S32 selected_count = gSelectMgr->getSelection()->getObjectCount();
	BOOL single_volume = (gSelectMgr->selectionAllPCode( LL_PCODE_VOLUME ))
						 && (selected_count == 1);

	// Select Single Message
	childSetVisible("select_single", FALSE);
	childSetVisible("edit_object", FALSE);
	if (!editable || single_volume || selected_count <= 1)
	{
		childSetVisible("edit_object", TRUE);
		childSetEnabled("edit_object", TRUE);
	}
	else
	{
		childSetVisible("select_single", TRUE);
		childSetEnabled("select_single", TRUE);
	}
	// Lock checkbox - only modifiable if you own the object.
	BOOL self_owned = (gAgent.getID() == owner_id);
	mCheckLock->setEnabled( roots_selected > 0 && self_owned );

	// More lock and debit checkbox - get the values
	BOOL valid;
	U32 owner_mask_on;
	U32 owner_mask_off;
	valid = gSelectMgr->selectGetPerm(PERM_OWNER, &owner_mask_on, &owner_mask_off);

	if(valid)
	{
		if(owner_mask_on & PERM_MOVE)
		{
			// owner can move, so not locked
			mCheckLock->set(FALSE);
			mCheckLock->setTentative(FALSE);
		}
		else if(owner_mask_off & PERM_MOVE)
		{
			// owner can't move, so locked
			mCheckLock->set(TRUE);
			mCheckLock->setTentative(FALSE);
		}
		else
		{
			// some locked, some not locked
			mCheckLock->set(FALSE);
			mCheckLock->setTentative(TRUE);
		}
	}

	BOOL is_flexible = volobjp && volobjp->isFlexible();

	// Physics checkbox
	mIsPhysical = root_objectp->usePhysics();
	mCheckPhysics->set( mIsPhysical );
	mCheckPhysics->setEnabled( roots_selected>0 
								&& (editable || gAgent.isGodlike()) 
								&& !is_flexible);

	mIsTemporary = root_objectp->flagTemporaryOnRez();
	mCheckTemporary->set( mIsTemporary );
	mCheckTemporary->setEnabled( roots_selected>0 && editable );

	mIsPhantom = root_objectp->flagPhantom();
	mCheckPhantom->set( mIsPhantom );
	mCheckPhantom->setEnabled( roots_selected>0 && editable && !is_flexible );

#if 0 // 1.9.2
	mCastShadows = root_objectp->flagCastShadows();
	mCheckCastShadows->set( mCastShadows );
	mCheckCastShadows->setEnabled( roots_selected==1 && editable );
#endif
	
	// Update material part
	U8 material_code;
	BOOL material_same = gSelectMgr->selectionGetMaterial(&material_code);
	if (editable && single_volume && material_same)
	{
		mComboMaterial->setEnabled( TRUE );
		mLabelMaterial->setEnabled( TRUE );
		if (material_code == LL_MCODE_LIGHT)
		{
			if (mComboMaterial->getItemCount() == mComboMaterialItemCount)
			{
				mComboMaterial->add(LEGACY_FULLBRIGHT_DESC);
			}
			mComboMaterial->setSimple(LEGACY_FULLBRIGHT_DESC);
		}
		else
		{
			if (mComboMaterial->getItemCount() != mComboMaterialItemCount)
			{
				mComboMaterial->remove(LEGACY_FULLBRIGHT_DESC);
			}
			mComboMaterial->setSimple(LLMaterialTable::basic.getName(material_code));
		}
	}
	else
	{
		mComboMaterial->setEnabled( FALSE );
		mLabelMaterial->setEnabled( FALSE );	
	}
	//----------------------------------------------------------------------------

	S32 selected_item	= MI_BOX;
	S32	selected_hole	= MI_HOLE_SAME;
	BOOL enabled = FALSE;
	BOOL hole_enabled = FALSE;
	F32 scale_x=1.f, scale_y=1.f;
	
	if( !objectp || !objectp->getVolume() || !editable || !single_volume)
	{
		// Clear out all geometry fields.
		mComboBaseType->clear();
		mSpinHollow->clear();
		mSpinCutBegin->clear();
		mSpinCutEnd->clear();
		mCtrlPathBegin->clear();
		mCtrlPathEnd->clear();
		mSpinScaleX->clear();
		mSpinScaleY->clear();
		mSpinTwist->clear();
		mSpinTwistBegin->clear();
		mComboHoleType->clear();
		mSpinShearX->clear();
		mSpinShearY->clear();
		mSpinTaperX->clear();
		mSpinTaperY->clear();
		mSpinRadiusOffset->clear();
		mSpinRevolutions->clear();
		mSpinSkew->clear();
		
		mSelectedType = MI_NONE;
	}
	else
	{
		// Only allowed to change these parameters for objects
		// that you have permissions on AND are not attachments.
		enabled = root_objectp->permModify();

		const LLVolumeParams &volume_params = objectp->getVolume()->getParams();

		// Volume type
		U8 path = volume_params.getPathParams().getCurveType();
		U8 profile_and_hole = volume_params.getProfileParams().getCurveType();
		U8 profile	= profile_and_hole & LL_PCODE_PROFILE_MASK;
		U8 hole		= profile_and_hole & LL_PCODE_HOLE_MASK;
		
		// Scale goes first so we can differentiate between a sphere and a torus,
		// which have the same profile and path types.

		// Scale
		scale_x = volume_params.getRatioX();
		scale_y = volume_params.getRatioY();

		BOOL linear_path = (path == LL_PCODE_PATH_LINE) || (path == LL_PCODE_PATH_FLEXIBLE);
		if ( linear_path && profile == LL_PCODE_PROFILE_CIRCLE )
		{
			selected_item = MI_CYLINDER;
		}
		else if ( linear_path && profile == LL_PCODE_PROFILE_SQUARE )
		{
			selected_item = MI_BOX;
		}
		else if ( linear_path && profile == LL_PCODE_PROFILE_ISOTRI )
		{
			selected_item = MI_PRISM;
		}
		else if ( linear_path && profile == LL_PCODE_PROFILE_EQUALTRI )
		{
			selected_item = MI_PRISM;
		}
		else if ( linear_path && profile == LL_PCODE_PROFILE_RIGHTTRI )
		{
			selected_item = MI_PRISM;
		}
		else if (path == LL_PCODE_PATH_FLEXIBLE) // shouldn't happen
		{
			selected_item = MI_CYLINDER; // reasonable default
		}
		else if ( path == LL_PCODE_PATH_CIRCLE && profile == LL_PCODE_PROFILE_CIRCLE && scale_y > 0.75f)
		{
			selected_item = MI_SPHERE;
		}
		else if ( path == LL_PCODE_PATH_CIRCLE && profile == LL_PCODE_PROFILE_CIRCLE && scale_y <= 0.75f)
		{
			selected_item = MI_TORUS;
		}
		else if ( path == LL_PCODE_PATH_CIRCLE && profile == LL_PCODE_PROFILE_CIRCLE_HALF)
		{
			selected_item = MI_SPHERE;
		}
		else if ( path == LL_PCODE_PATH_CIRCLE2 && profile == LL_PCODE_PROFILE_CIRCLE )
		{
			// Spirals aren't supported.  Make it into a sphere.  JC
			selected_item = MI_SPHERE;
		}
		else if ( path == LL_PCODE_PATH_CIRCLE && profile == LL_PCODE_PROFILE_EQUALTRI )
		{
			selected_item = MI_RING;
		}
		else if ( path == LL_PCODE_PATH_CIRCLE && profile == LL_PCODE_PROFILE_SQUARE && scale_y <= 0.75f)
		{
			selected_item = MI_TUBE;
		}
		else
		{
			llinfos << "Unknown path " << (S32) path << " profile " << (S32) profile << " in getState" << llendl;
			selected_item = MI_BOX;
		}


		if (objectp->getParameterEntryInUse(LLNetworkData::PARAMS_SCULPT))
		{
			selected_item = MI_SCULPT;
			LLFirstUse::useSculptedPrim();
		}

		
		mComboBaseType	->setCurrentByIndex( selected_item );
		mSelectedType = selected_item;
		
		// Grab S path
		F32 begin_s	= volume_params.getBeginS();
		F32 end_s	= volume_params.getEndS();

		// Compute cut and advanced cut from S and T
		F32 begin_t = volume_params.getBeginT();
		F32 end_t	= volume_params.getEndT();

		// Hollowness
		F32 hollow = volume_params.getHollow();
		mSpinHollow->set( 100.f * hollow );

		// All hollow objects allow a shape to be selected.
		if (hollow > 0.f)
		{
			switch (hole)
			{
			case LL_PCODE_HOLE_CIRCLE:
				selected_hole = MI_HOLE_CIRCLE;
				break;
			case LL_PCODE_HOLE_SQUARE:
				selected_hole = MI_HOLE_SQUARE;
				break;
			case LL_PCODE_HOLE_TRIANGLE:
				selected_hole = MI_HOLE_TRIANGLE;
				break;
			case LL_PCODE_HOLE_SAME:
			default:
				selected_hole = MI_HOLE_SAME;
				break;
			}
			mComboHoleType->setCurrentByIndex( selected_hole );
			hole_enabled = enabled;
		}
		else
		{
			mComboHoleType->setCurrentByIndex( MI_HOLE_SAME );
			hole_enabled = FALSE;
		}

		// Cut interpretation varies based on base object type
		F32 cut_begin, cut_end, adv_cut_begin, adv_cut_end;

		if ( selected_item == MI_SPHERE || selected_item == MI_TORUS || 
			 selected_item == MI_TUBE   || selected_item == MI_RING )
		{
			cut_begin		= begin_t;
			cut_end			= end_t;
			adv_cut_begin	= begin_s;
			adv_cut_end		= end_s;
		}
		else
		{
			cut_begin       = begin_s;
			cut_end         = end_s;
			adv_cut_begin   = begin_t;
			adv_cut_end     = end_t;
		}

		mSpinCutBegin	->set( cut_begin );
		mSpinCutEnd		->set( cut_end );
		mCtrlPathBegin	->set( adv_cut_begin );
		mCtrlPathEnd	->set( adv_cut_end );

		// Twist
		F32 twist		= volume_params.getTwist();
		F32 twist_begin = volume_params.getTwistBegin();
		// Check the path type for conversion.
		if (path == LL_PCODE_PATH_LINE || path == LL_PCODE_PATH_FLEXIBLE)
		{
			twist		*= OBJECT_TWIST_LINEAR_MAX;
			twist_begin	*= OBJECT_TWIST_LINEAR_MAX;
		}
		else
		{
			twist		*= OBJECT_TWIST_MAX;
			twist_begin	*= OBJECT_TWIST_MAX;
		}

		mSpinTwist		->set( twist );
		mSpinTwistBegin	->set( twist_begin );

		// Shear
		F32 shear_x = volume_params.getShearX();
		F32 shear_y = volume_params.getShearY();
		mSpinShearX->set( shear_x );
		mSpinShearY->set( shear_y );

		// Taper
		F32 taper_x	= volume_params.getTaperX();
		F32 taper_y = volume_params.getTaperY();
		mSpinTaperX->set( taper_x );
		mSpinTaperY->set( taper_y );

		// Radius offset.
		F32 radius_offset = volume_params.getRadiusOffset();
		// Limit radius offset, based on taper and hole size y.
		F32 radius_mag = fabs(radius_offset);
		F32 hole_y_mag = fabs(scale_y);
		F32 taper_y_mag  = fabs(taper_y);
		// Check to see if the taper effects us.
		if ( (radius_offset > 0.f && taper_y < 0.f) ||
			 (radius_offset < 0.f && taper_y > 0.f) )
		{
			// The taper does not help increase the radius offset range.
			taper_y_mag = 0.f;
		}
		F32 max_radius_mag = 1.f - hole_y_mag * (1.f - taper_y_mag) / (1.f - hole_y_mag);
		// Enforce the maximum magnitude.
		if (radius_mag > max_radius_mag)
		{
			// Check radius offset sign.
			if (radius_offset < 0.f)
			{
				radius_offset = -max_radius_mag;
			}
			else
			{
				radius_offset = max_radius_mag;
			}
		}
		mSpinRadiusOffset->set( radius_offset);

		// Revolutions
		F32 revolutions = volume_params.getRevolutions();
		mSpinRevolutions->set( revolutions );
		
		// Skew
		F32 skew	= volume_params.getSkew();
		// Limit skew, based on revolutions hole size x.
		F32 skew_mag= fabs(skew);
		F32 min_skew_mag = 1.0f - 1.0f / (revolutions * scale_x + 1.0f);
		// Discontinuity; A revolution of 1 allows skews below 0.5.
		if ( fabs(revolutions - 1.0f) < 0.001)
			min_skew_mag = 0.0f;

		// Clip skew.
		if (skew_mag < min_skew_mag)
		{
			// Check skew sign.
			if (skew < 0.0f)
			{
				skew = -min_skew_mag;
			}
			else 
			{
				skew = min_skew_mag;
			}
		}
		mSpinSkew->set( skew );
	}

	// Compute control visibility, label names, and twist range.
	// Start with defaults.
	BOOL cut_visible                = TRUE;
	BOOL hollow_visible             = TRUE;
	BOOL top_size_x_visible			= TRUE;
	BOOL top_size_y_visible			= TRUE;
	BOOL top_shear_x_visible		= TRUE;
	BOOL top_shear_y_visible		= TRUE;
	BOOL twist_visible				= TRUE;
	BOOL advanced_cut_visible		= FALSE;
	BOOL taper_visible				= FALSE;
	BOOL skew_visible				= FALSE;
	BOOL radius_offset_visible		= FALSE;
	BOOL revolutions_visible		= FALSE;
	BOOL sculpt_texture_visible     = FALSE;
	F32	 twist_min					= OBJECT_TWIST_LINEAR_MIN;
	F32	 twist_max					= OBJECT_TWIST_LINEAR_MAX;
	F32	 twist_inc					= OBJECT_TWIST_LINEAR_INC;

	BOOL advanced_is_dimple = FALSE;
	BOOL size_is_hole = FALSE;

	// Tune based on overall volume type
	switch (selected_item)
	{
	case MI_SPHERE:
		top_size_x_visible		= FALSE;
		top_size_y_visible		= FALSE;
		top_shear_x_visible		= FALSE;
		top_shear_y_visible		= FALSE;
		//twist_visible			= FALSE;
		advanced_cut_visible	= TRUE;
		advanced_is_dimple		= TRUE;
		twist_min				= OBJECT_TWIST_MIN;
		twist_max				= OBJECT_TWIST_MAX;
		twist_inc				= OBJECT_TWIST_INC;
		break;

	case MI_TORUS:
	case MI_TUBE:	
	case MI_RING:
		//top_size_x_visible		= FALSE;
		//top_size_y_visible		= FALSE;
	  	size_is_hole 			= TRUE;
		skew_visible			= TRUE;
		advanced_cut_visible	= TRUE;
		taper_visible			= TRUE;
		radius_offset_visible	= TRUE;
		revolutions_visible		= TRUE;
		twist_min				= OBJECT_TWIST_MIN;
		twist_max				= OBJECT_TWIST_MAX;
		twist_inc				= OBJECT_TWIST_INC;

		break;

	case MI_SCULPT:
		cut_visible             = FALSE;
		hollow_visible          = FALSE;
		twist_visible           = FALSE;
		top_size_x_visible      = FALSE;
		top_size_y_visible      = FALSE;
		top_shear_x_visible     = FALSE;
		top_shear_y_visible     = FALSE;
		skew_visible            = FALSE;
		advanced_cut_visible    = FALSE;
		taper_visible           = FALSE;
		radius_offset_visible   = FALSE;
		revolutions_visible     = FALSE;
		sculpt_texture_visible  = TRUE;

		break;
		
	case MI_BOX:
	case MI_CYLINDER:
	case MI_PRISM:
	default:
		break;
	}

	// Check if we need to change top size/hole size params.
	switch (selected_item)
	{
	case MI_SPHERE:
	case MI_TORUS:
	case MI_TUBE:
	case MI_RING:
		mSpinScaleX->set( scale_x );
		mSpinScaleY->set( scale_y );
		mSpinScaleX->setMinValue(OBJECT_MIN_HOLE_SIZE);
		mSpinScaleX->setMaxValue(OBJECT_MAX_HOLE_SIZE_X);
		mSpinScaleY->setMinValue(OBJECT_MIN_HOLE_SIZE);
		mSpinScaleY->setMaxValue(OBJECT_MAX_HOLE_SIZE_Y);
		break;
	default:
		mSpinScaleX->set( 1.f - scale_x );
		mSpinScaleY->set( 1.f - scale_y );
		mSpinScaleX->setMinValue(-1.f);
		mSpinScaleX->setMaxValue(1.f);
		mSpinScaleY->setMinValue(-1.f);
		mSpinScaleY->setMaxValue(1.f);
		break;
	}

	// Check if we need to limit the hollow based on the hole type.
	if (  selected_hole == MI_HOLE_SQUARE && 
		  ( selected_item == MI_CYLINDER || selected_item == MI_TORUS ||
		    selected_item == MI_PRISM    || selected_item == MI_RING  ||
			selected_item == MI_SPHERE ) )
	{
		mSpinHollow->setMinValue(0.f);
		mSpinHollow->setMaxValue(70.f);
	}
	else 
	{
		mSpinHollow->setMinValue(0.f);
		mSpinHollow->setMaxValue(95.f);
	}

	// Update field enablement
	mLabelBaseType	->setEnabled( enabled );
	mComboBaseType	->setEnabled( enabled );

	mLabelCut		->setEnabled( enabled );
	mSpinCutBegin	->setEnabled( enabled );
	mSpinCutEnd		->setEnabled( enabled );

	mLabelHollow	->setEnabled( enabled );
	mSpinHollow		->setEnabled( enabled );
	mLabelHoleType	->setEnabled( hole_enabled );
	mComboHoleType	->setEnabled( hole_enabled );

	mLabelTwist		->setEnabled( enabled );
	mSpinTwist		->setEnabled( enabled );
	mSpinTwistBegin	->setEnabled( enabled );

	mLabelSkew		->setEnabled( enabled );
	mSpinSkew		->setEnabled( enabled );

	childSetVisible("scale_hole", FALSE);
	childSetVisible("scale_taper", FALSE);
	if (top_size_x_visible || top_size_y_visible)
	{
		if (size_is_hole)
		{
			childSetVisible("scale_hole", TRUE);
			childSetEnabled("scale_hole", enabled);
		}
		else
		{
			childSetVisible("scale_taper", TRUE);
			childSetEnabled("scale_taper", enabled);
		}
	}
	
	mSpinScaleX		->setEnabled( enabled );
	mSpinScaleY		->setEnabled( enabled );

	mLabelShear		->setEnabled( enabled );
	mSpinShearX		->setEnabled( enabled );
	mSpinShearY		->setEnabled( enabled );

	childSetVisible("advanced_cut", FALSE);
	childSetVisible("advanced_dimple", FALSE);
	if (advanced_cut_visible)
	{
		if (advanced_is_dimple)
		{
			childSetVisible("advanced_dimple", TRUE);
			childSetEnabled("advanced_dimple", enabled);
		}
		else
		{
			childSetVisible("advanced_cut", TRUE);
			childSetEnabled("advanced_cut", enabled);
		}
	}
	
	mCtrlPathBegin	->setEnabled( enabled );
	mCtrlPathEnd	->setEnabled( enabled );

	mLabelTaper		->setEnabled( enabled );
	mSpinTaperX		->setEnabled( enabled );
	mSpinTaperY		->setEnabled( enabled );

	mLabelRadiusOffset->setEnabled( enabled );
	mSpinRadiusOffset ->setEnabled( enabled );

	mLabelRevolutions->setEnabled( enabled );
	mSpinRevolutions ->setEnabled( enabled );

	// Update field visibility
	mLabelCut		->setVisible( cut_visible );
	mSpinCutBegin	->setVisible( cut_visible );
	mSpinCutEnd		->setVisible( cut_visible ); 

	mLabelHollow	->setVisible( hollow_visible );
	mSpinHollow		->setVisible( hollow_visible );
	mLabelHoleType	->setVisible( hollow_visible );
	mComboHoleType	->setVisible( hollow_visible );
	
	mLabelTwist		->setVisible( twist_visible );
	mSpinTwist		->setVisible( twist_visible );
	mSpinTwistBegin	->setVisible( twist_visible );
	mSpinTwist		->setMinValue(  twist_min );
	mSpinTwist		->setMaxValue(  twist_max );
	mSpinTwist		->setIncrement( twist_inc );
	mSpinTwistBegin	->setMinValue(  twist_min );
	mSpinTwistBegin	->setMaxValue(  twist_max );
	mSpinTwistBegin	->setIncrement( twist_inc );

	mSpinScaleX		->setVisible( top_size_x_visible );
	mSpinScaleY		->setVisible( top_size_y_visible );

	mLabelSkew		->setVisible( skew_visible );
	mSpinSkew		->setVisible( skew_visible );

	mLabelShear		->setVisible( top_shear_x_visible || top_shear_y_visible );
	mSpinShearX		->setVisible( top_shear_x_visible );
	mSpinShearY		->setVisible( top_shear_y_visible );

	mCtrlPathBegin	->setVisible( advanced_cut_visible );
	mCtrlPathEnd	->setVisible( advanced_cut_visible );

	mLabelTaper		->setVisible( taper_visible );
	mSpinTaperX		->setVisible( taper_visible );
	mSpinTaperY		->setVisible( taper_visible );

	mLabelRadiusOffset->setVisible( radius_offset_visible );
	mSpinRadiusOffset ->setVisible( radius_offset_visible );

	mLabelRevolutions->setVisible( revolutions_visible );
	mSpinRevolutions ->setVisible( revolutions_visible );

	mCtrlSculptTexture->setVisible( sculpt_texture_visible );


	// sculpt texture

	if (selected_item == MI_SCULPT)
	{
        LLUUID id;
		LLSculptParams *sculpt_params = (LLSculptParams *)objectp->getParameterEntry(LLNetworkData::PARAMS_SCULPT);

		LLTextureCtrl*  mTextureCtrl = LLViewerUICtrlFactory::getTexturePickerByName(this,"sculpt texture control");
		if((mTextureCtrl) && (sculpt_params))
		{
			mTextureCtrl->setTentative(FALSE);
			mTextureCtrl->setEnabled(editable);
			if (editable)
				mTextureCtrl->setImageAssetID(sculpt_params->getSculptTexture());
			else
				mTextureCtrl->setImageAssetID(LLUUID::null);
				

			if (mObject != objectp)  // we've just selected a new object, so save for undo
				mSculptTextureRevert = sculpt_params->getSculptTexture();
		}

	}

	
	//----------------------------------------------------------------------------

	mObject = objectp;
	mRootObject = root_objectp;
}

// static
BOOL LLPanelObject::precommitValidate( LLUICtrl* ctrl, void* userdata )
{
	// TODO: Richard will fill this in later.  
	return TRUE; // FALSE means that validation failed and new value should not be commited.
}

void LLPanelObject::sendIsPhysical()
{
	BOOL value = mCheckPhysics->get();
	if( mIsPhysical != value )
	{
		gSelectMgr->selectionUpdatePhysics(value);
		mIsPhysical = value;

		llinfos << "update physics sent" << llendl;
	}
	else
	{
		llinfos << "update physics not changed" << llendl;
	}
}

void LLPanelObject::sendIsTemporary()
{
	BOOL value = mCheckTemporary->get();
	if( mIsTemporary != value )
	{
		gSelectMgr->selectionUpdateTemporary(value);
		mIsTemporary = value;

		llinfos << "update temporary sent" << llendl;
	}
	else
	{
		llinfos << "update temporary not changed" << llendl;
	}
}


void LLPanelObject::sendIsPhantom()
{
	BOOL value = mCheckPhantom->get();
	if( mIsPhantom != value )
	{
		gSelectMgr->selectionUpdatePhantom(value);
		mIsPhantom = value;

		llinfos << "update phantom sent" << llendl;
	}
	else
	{
		llinfos << "update phantom not changed" << llendl;
	}
}

void LLPanelObject::sendCastShadows()
{
	BOOL value = mCheckCastShadows->get();
	if( mCastShadows != value )
	{
		gSelectMgr->selectionUpdateCastShadows(value);
		mCastShadows = value;

		llinfos << "update cast shadows sent" << llendl;
	}
	else
	{
		llinfos << "update cast shadows not changed" << llendl;
	}
}

// static
void LLPanelObject::onCommitMaterial( LLUICtrl* ctrl, void* userdata )
{
	//LLPanelObject* self = (LLPanelObject*) userdata;
	LLComboBox* box = (LLComboBox*) ctrl;

	if (box)
	{
		// apply the currently selected material to the object
		const LLString& material_name = box->getSimple();
		if (material_name != LEGACY_FULLBRIGHT_DESC)
		{
			U8 material_code = LLMaterialTable::basic.getMCode(material_name.c_str());
			gSelectMgr->selectionSetMaterial(material_code);
		}
	}
}

// static
void LLPanelObject::onCommitParametric( LLUICtrl* ctrl, void* userdata )
{
	LLPanelObject* self = (LLPanelObject*) userdata;

	if (self->mObject.isNull())
	{
		return;
	}

	if (self->mObject->getPCode() != LL_PCODE_VOLUME)
	{
		// Don't allow modification of non-volume objects.
		return;
	}

	LLVolume *volume = self->mObject->getVolume();
	if (!volume)
	{
		return;
	}

	LLVolumeParams volume_params;
	self->getVolumeParams(volume_params);
	

	
	// set sculpting
	S32 selected_type = self->mComboBaseType->getCurrentIndex();

	if (selected_type == MI_SCULPT)
	{
		self->mObject->setParameterEntryInUse(LLNetworkData::PARAMS_SCULPT, TRUE, TRUE);
		LLSculptParams *sculpt_params = (LLSculptParams *)self->mObject->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		if (sculpt_params)
			volume_params.setSculptID(sculpt_params->getSculptTexture(), 0);
	}
	else
	{
		LLSculptParams *sculpt_params = (LLSculptParams *)self->mObject->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		if (sculpt_params)
			self->mObject->setParameterEntryInUse(LLNetworkData::PARAMS_SCULPT, FALSE, TRUE);
	}

	// Update the volume, if necessary.
	self->mObject->updateVolume(volume_params);


	// This was added to make sure thate when changes are made, the UI
	// adjusts to present valid options.
	// *FIX: only some changes, ie, hollow or primitive type changes,
	// require a refresh.
	self->refresh();

}

void LLPanelObject::getVolumeParams(LLVolumeParams& volume_params)
{
	// Figure out what type of volume to make
	S32 was_selected_type = mSelectedType;
	S32 selected_type = mComboBaseType->getCurrentIndex();
	U8 profile;
	U8 path;
	switch ( selected_type )
	{
	case MI_CYLINDER:
		profile = LL_PCODE_PROFILE_CIRCLE;
		path = LL_PCODE_PATH_LINE;
		break;

	case MI_BOX:
		profile = LL_PCODE_PROFILE_SQUARE;
		path = LL_PCODE_PATH_LINE;
		break;

	case MI_PRISM:
		profile = LL_PCODE_PROFILE_EQUALTRI;
		path = LL_PCODE_PATH_LINE;
		break;

	case MI_SPHERE:
		profile = LL_PCODE_PROFILE_CIRCLE_HALF;
		path = LL_PCODE_PATH_CIRCLE;
		break;

	case MI_TORUS:
		profile = LL_PCODE_PROFILE_CIRCLE;
		path = LL_PCODE_PATH_CIRCLE;
		break;

	case MI_TUBE:
		profile = LL_PCODE_PROFILE_SQUARE;
		path = LL_PCODE_PATH_CIRCLE;
		break;

	case MI_RING:
		profile = LL_PCODE_PROFILE_EQUALTRI;
		path = LL_PCODE_PATH_CIRCLE;
		break;

	case MI_SCULPT:
		profile = LL_PCODE_PROFILE_CIRCLE;
		path = LL_PCODE_PATH_CIRCLE;
		break;
		
	default:
		llwarns << "Unknown base type " << selected_type 
			<< " in getVolumeParams()" << llendl;
		// assume a box
		selected_type = MI_BOX;
		profile = LL_PCODE_PROFILE_SQUARE;
		path = LL_PCODE_PATH_LINE;
		break;
	}


	if (path == LL_PCODE_PATH_LINE)
	{
		LLVOVolume *volobjp = (LLVOVolume *)(LLViewerObject*)(mObject);
		if (volobjp->isFlexible())
		{
			path = LL_PCODE_PATH_FLEXIBLE;
		}
	}
	
	S32 selected_hole = mComboHoleType->getCurrentIndex();
	U8 hole;
	switch (selected_hole)
	{
	case MI_HOLE_CIRCLE: 
		hole = LL_PCODE_HOLE_CIRCLE;
		break;
	case MI_HOLE_SQUARE:
		hole = LL_PCODE_HOLE_SQUARE;
		break;
	case MI_HOLE_TRIANGLE:
		hole = LL_PCODE_HOLE_TRIANGLE;
		break;
	case MI_HOLE_SAME:
	default:
		hole = LL_PCODE_HOLE_SAME;
		break;
	}

	volume_params.setType(profile | hole, path);
	mSelectedType = selected_type;
	
	// Compute cut start/end
	F32 cut_begin	= mSpinCutBegin->get();
	F32 cut_end		= mSpinCutEnd->get();

	// Make sure at least OBJECT_CUT_INC of the object survives
	if (cut_begin > cut_end - OBJECT_MIN_CUT_INC)
	{
		cut_begin = cut_end - OBJECT_MIN_CUT_INC;
		mSpinCutBegin->set(cut_begin);
	}

	F32 adv_cut_begin	= mCtrlPathBegin->get();
	F32 adv_cut_end		= mCtrlPathEnd->get();

	// Make sure at least OBJECT_CUT_INC of the object survives
	if (adv_cut_begin > adv_cut_end - OBJECT_MIN_CUT_INC)
	{
		adv_cut_begin = adv_cut_end - OBJECT_MIN_CUT_INC;
		mCtrlPathBegin->set(adv_cut_begin);
	}

	F32 begin_s, end_s;
	F32 begin_t, end_t;

	if (selected_type == MI_SPHERE || selected_type == MI_TORUS || 
		selected_type == MI_TUBE   || selected_type == MI_RING)
	{
		begin_s = adv_cut_begin;
		end_s	= adv_cut_end;

		begin_t = cut_begin;
		end_t	= cut_end;
	}
	else
	{
		begin_s = cut_begin;
		end_s	= cut_end;

		begin_t = adv_cut_begin;
		end_t	= adv_cut_end;
	}

	volume_params.setBeginAndEndS(begin_s, end_s);
	volume_params.setBeginAndEndT(begin_t, end_t);

	// Hollowness
	F32 hollow = mSpinHollow->get() / 100.f;

	if (  selected_hole == MI_HOLE_SQUARE && 
		( selected_type == MI_CYLINDER || selected_type == MI_TORUS ||
		  selected_type == MI_PRISM    || selected_type == MI_RING  ||
		  selected_type == MI_SPHERE ) )
	{
		if (hollow > 0.7f) hollow = 0.7f;
	}

	volume_params.setHollow( hollow );

	// Twist Begin,End
	F32 twist_begin = mSpinTwistBegin->get();
	F32 twist		= mSpinTwist->get();
	// Check the path type for twist conversion.
	if (path == LL_PCODE_PATH_LINE || path == LL_PCODE_PATH_FLEXIBLE)
	{
		twist_begin	/= OBJECT_TWIST_LINEAR_MAX;
		twist		/= OBJECT_TWIST_LINEAR_MAX;
	}
	else
	{
		twist_begin	/= OBJECT_TWIST_MAX;
		twist		/= OBJECT_TWIST_MAX;
	}

	volume_params.setTwistBegin(twist_begin);
	volume_params.setTwist(twist);

	// Scale X,Y
	F32 scale_x = mSpinScaleX->get();
	F32 scale_y = mSpinScaleY->get();
	if ( was_selected_type == MI_BOX || was_selected_type == MI_CYLINDER || was_selected_type == MI_PRISM)
	{
		scale_x = 1.f - scale_x;
		scale_y = 1.f - scale_y;
	}
	
	// Skew
	F32 skew = mSpinSkew->get();

	// Taper X,Y
	F32 taper_x = mSpinTaperX->get();
	F32 taper_y = mSpinTaperY->get();

	// Radius offset
	F32 radius_offset = mSpinRadiusOffset->get();

	// Revolutions
	F32 revolutions	  = mSpinRevolutions->get();

	if ( selected_type == MI_SPHERE )
	{
		// Snap values to valid sphere parameters.
		scale_x			= 1.0f;
		scale_y			= 1.0f;
		skew			= 0.0f;
		taper_x			= 0.0f;
		taper_y			= 0.0f;
		radius_offset	= 0.0f;
		revolutions		= 1.0f;
	}
	else if ( selected_type == MI_TORUS || selected_type == MI_TUBE ||
			  selected_type == MI_RING )
	{
		scale_x = llclamp(
			scale_x,
			OBJECT_MIN_HOLE_SIZE,
			OBJECT_MAX_HOLE_SIZE_X);
		scale_y = llclamp(
			scale_y,
			OBJECT_MIN_HOLE_SIZE,
			OBJECT_MAX_HOLE_SIZE_Y);

		// Limit radius offset, based on taper and hole size y.
		F32 radius_mag = fabs(radius_offset);
		F32 hole_y_mag = fabs(scale_y);
		F32 taper_y_mag  = fabs(taper_y);
		// Check to see if the taper effects us.
		if ( (radius_offset > 0.f && taper_y < 0.f) ||
			 (radius_offset < 0.f && taper_y > 0.f) )
		{
			// The taper does not help increase the radius offset range.
			taper_y_mag = 0.f;
		}
		F32 max_radius_mag = 1.f - hole_y_mag * (1.f - taper_y_mag) / (1.f - hole_y_mag);
		// Enforce the maximum magnitude.
		if (radius_mag > max_radius_mag)
		{
			// Check radius offset sign.
			if (radius_offset < 0.f)
			{
				radius_offset = -max_radius_mag;
			}
			else
			{
				radius_offset = max_radius_mag;
			}
		}
			
		// Check the skew value against the revolutions.
		F32 skew_mag= fabs(skew);
		F32 min_skew_mag = 1.0f - 1.0f / (revolutions * scale_x + 1.0f);
		// Discontinuity; A revolution of 1 allows skews below 0.5.
		if ( fabs(revolutions - 1.0f) < 0.001)
			min_skew_mag = 0.0f;

		// Clip skew.
		if (skew_mag < min_skew_mag)
		{
			// Check skew sign.
			if (skew < 0.0f)
			{
				skew = -min_skew_mag;
			}
			else 
			{
				skew = min_skew_mag;
			}
		}
	}

	volume_params.setRatio( scale_x, scale_y );
	volume_params.setSkew(skew);
	volume_params.setTaper( taper_x, taper_y );
	volume_params.setRadiusOffset(radius_offset);
	volume_params.setRevolutions(revolutions);

	// Shear X,Y
	F32 shear_x = mSpinShearX->get();
	F32 shear_y = mSpinShearY->get();
	volume_params.setShear( shear_x, shear_y );

	if (selected_type == MI_SCULPT)
	{
		volume_params.setSculptID(LLUUID::null, 0);
		volume_params.setBeginAndEndT   (0, 1);
		volume_params.setBeginAndEndS   (0, 1);
		volume_params.setHollow         (0);
		volume_params.setTwistBegin     (0);
		volume_params.setTwistEnd       (0);
		volume_params.setRatio          (1, 0.5);
		volume_params.setShear          (0, 0);
		volume_params.setTaper          (0, 0);
		volume_params.setRevolutions    (1);
		volume_params.setRadiusOffset   (0);
		volume_params.setSkew           (0);
	}

}

// BUG: Make work with multiple objects
void LLPanelObject::sendRotation()
{
	if (mObject.isNull()) return;

	LLVector3 new_rot(mCtrlRotX->get(), mCtrlRotY->get(), mCtrlRotZ->get());
	new_rot.mV[VX] = llround(new_rot.mV[VX], OBJECT_ROTATION_PRECISION);
	new_rot.mV[VY] = llround(new_rot.mV[VY], OBJECT_ROTATION_PRECISION);
	new_rot.mV[VZ] = llround(new_rot.mV[VZ], OBJECT_ROTATION_PRECISION);

	// Note: must compare before conversion to radians
	LLVector3 delta = new_rot - mCurEulerDegrees;

	if (delta.magVec() >= 0.0005f)
	{
		mCurEulerDegrees = new_rot;
		new_rot *= DEG_TO_RAD;

		LLQuaternion rotation;
		rotation.setQuat(new_rot.mV[VX], new_rot.mV[VY], new_rot.mV[VZ]);

		if (mRootObject != mObject)
		{
			rotation = rotation * ~mRootObject->getRotationRegion();
		}

		mObject->setRotation(rotation, TRUE );

		gSelectMgr->sendMultipleUpdate(UPD_ROTATION);
	}
}


// BUG: Make work with multiple objects
void LLPanelObject::sendScale()
{
	if (mObject.isNull()) return;

	LLVector3 newscale(mCtrlScaleX->get(), mCtrlScaleY->get(), mCtrlScaleZ->get());

	LLVector3 delta = newscale - mObject->getScale();
	if (delta.magVec() >= 0.0005f)
	{
		// scale changed by more than 1/2 millimeter

		// check to see if we aren't scaling the textures
		// (in which case the tex coord's need to be recomputed)
		BOOL dont_stretch_textures = !LLManipScale::getStretchTextures();
		if (dont_stretch_textures)
		{
			gSelectMgr->saveSelectedObjectTransform(SELECT_ACTION_TYPE_SCALE);
		}

		mObject->setScale(newscale, TRUE);
		gSelectMgr->sendMultipleUpdate(UPD_SCALE | UPD_POSITION);

		gSelectMgr->adjustTexturesByScale(TRUE, !dont_stretch_textures);
//		llinfos << "scale sent" << llendl;
	}
	else
	{
//		llinfos << "scale not changed" << llendl;
	}
}


void LLPanelObject::sendPosition()
{	
	if (mObject.isNull()) return;

	LLVector3 newpos(mCtrlPosX->get(), mCtrlPosY->get(), mCtrlPosZ->get());
	LLViewerRegion* regionp = mObject->getRegion();
		
	// Clamp the Z height
	const F32 height = newpos.mV[VZ];
	const F32 min_height = gWorldp->getMinAllowedZ(mObject);
	const F32 max_height = gWorldPointer->getRegionMaxHeight();

	if (!mObject->isAttachment())
	{
		if ( height < min_height)
		{
			newpos.mV[VZ] = min_height;
			mCtrlPosZ->set( min_height );
		}
		else if ( height > max_height )
		{
			newpos.mV[VZ] = max_height;
			mCtrlPosZ->set( max_height );
		}

		// Grass is always drawn on the ground, so clamp its position to the ground
		if (mObject->getPCode() == LL_PCODE_LEGACY_GRASS)
		{
			mCtrlPosZ->set(gWorldp->resolveLandHeightAgent(newpos) + 1.f);
		}
	}

	// Make sure new position is in a valid region, so the object
	// won't get dumped by the simulator.
	LLVector3d new_pos_global = regionp->getPosGlobalFromRegion(newpos);

	if ( gWorldPointer->positionRegionValidGlobal(new_pos_global) )
	{
		// send only if the position is changed, that is, the delta vector is not zero
		LLVector3d old_pos_global = mObject->getPositionGlobal();
		LLVector3d delta = new_pos_global - old_pos_global;
		// moved more than 1/2 millimeter
		if (delta.magVec() >= 0.0005f)
		{
			if (mRootObject != mObject)
			{
				newpos = newpos - mRootObject->getPositionRegion();
				newpos = newpos * ~mRootObject->getRotationRegion();
				mObject->setPositionParent(newpos);
			}
			else
			{
				mObject->setPositionEdit(newpos);
			}
			gSelectMgr->sendMultipleUpdate(UPD_POSITION);
			//mRootObject->sendPositionUpdate();

			gSelectMgr->updateSelectionCenter();

//			llinfos << "position sent" << llendl;
		}
		else
		{
//			llinfos << "position not changed" << llendl;
		}
	}
	else
	{
		// move failed, so we update the UI with the correct values
		LLVector3 vec = mRootObject->getPositionRegion();
		mCtrlPosX->set(vec.mV[VX]);
		mCtrlPosY->set(vec.mV[VY]);
		mCtrlPosZ->set(vec.mV[VZ]);
	}
}

void LLPanelObject::sendSculpt()
{
	LLTextureCtrl* mTextureCtrl = gUICtrlFactory->getTexturePickerByName(this,"sculpt texture control");
	if(!mTextureCtrl)
		return;

	LLSculptParams sculpt_params;
	sculpt_params.setSculptTexture(mTextureCtrl->getImageAssetID());
	sculpt_params.setSculptType(LL_SCULPT_TYPE_SPHERE);
	
	mObject->setParameterEntry(LLNetworkData::PARAMS_SCULPT, sculpt_params, TRUE);
}

void LLPanelObject::refresh()
{
	getState();
	if (mObject.notNull() && mObject->isDead())
	{
		mObject = NULL;
	}

	if (mRootObject.notNull() && mRootObject->isDead())
	{
		mRootObject = NULL;
	}
}


void LLPanelObject::draw()
{
	const LLColor4	white(	1.0f,	1.0f,	1.0f,	1);
	const LLColor4	red(	1.0f,	0.25f,	0.f,	1);
	const LLColor4	green(	0.f,	1.0f,	0.f,	1);
	const LLColor4	blue(	0.f,	0.5f,	1.0f,	1);

	// Tune the colors of the labels
	LLTool* tool = gToolMgr->getCurrentTool();

	if (tool == gToolTranslate)
	{
		mCtrlPosX	->setLabelColor(red);
		mCtrlPosY	->setLabelColor(green);
		mCtrlPosZ	->setLabelColor(blue);

		mCtrlScaleX	->setLabelColor(white);
		mCtrlScaleY	->setLabelColor(white);
		mCtrlScaleZ	->setLabelColor(white);

		mCtrlRotX	->setLabelColor(white);
		mCtrlRotY	->setLabelColor(white);
		mCtrlRotZ	->setLabelColor(white);
	}
	else if ( tool == gToolStretch )
	{
		mCtrlPosX	->setLabelColor(white);
		mCtrlPosY	->setLabelColor(white);
		mCtrlPosZ	->setLabelColor(white);

		mCtrlScaleX	->setLabelColor(red);
		mCtrlScaleY	->setLabelColor(green);
		mCtrlScaleZ	->setLabelColor(blue);

		mCtrlRotX	->setLabelColor(white);
		mCtrlRotY	->setLabelColor(white);
		mCtrlRotZ	->setLabelColor(white);
	}
	else if ( tool == gToolRotate )
	{
		mCtrlPosX	->setLabelColor(white);
		mCtrlPosY	->setLabelColor(white);
		mCtrlPosZ	->setLabelColor(white);

		mCtrlScaleX	->setLabelColor(white);
		mCtrlScaleY	->setLabelColor(white);
		mCtrlScaleZ	->setLabelColor(white);

		mCtrlRotX	->setLabelColor(red);
		mCtrlRotY	->setLabelColor(green);
		mCtrlRotZ	->setLabelColor(blue);
	}
	else
	{
		mCtrlPosX	->setLabelColor(white);
		mCtrlPosY	->setLabelColor(white);
		mCtrlPosZ	->setLabelColor(white);

		mCtrlScaleX	->setLabelColor(white);
		mCtrlScaleY	->setLabelColor(white);
		mCtrlScaleZ	->setLabelColor(white);

		mCtrlRotX	->setLabelColor(white);
		mCtrlRotY	->setLabelColor(white);
		mCtrlRotZ	->setLabelColor(white);
	}

	LLPanel::draw();
}

// virtual
void LLPanelObject::clearCtrls()
{
	LLPanel::clearCtrls();

	mCheckLock		->set(FALSE);
	mCheckLock		->setEnabled( FALSE );
	mCheckPhysics	->set(FALSE);
	mCheckPhysics	->setEnabled( FALSE );
	mCheckTemporary	->set(FALSE);
	mCheckTemporary	->setEnabled( FALSE );
	mCheckPhantom	->set(FALSE);
	mCheckPhantom	->setEnabled( FALSE );
#if 0 // 1.9.2
	mCheckCastShadows->set(FALSE);
	mCheckCastShadows->setEnabled( FALSE );
#endif
	mComboMaterial	->setEnabled( FALSE );
	mLabelMaterial	->setEnabled( FALSE );
	// Disable text labels
	mLabelPosition	->setEnabled( FALSE );
	mLabelSize		->setEnabled( FALSE );
	mLabelRotation	->setEnabled( FALSE );
	mLabelBaseType	->setEnabled( FALSE );
	mLabelCut		->setEnabled( FALSE );
	mLabelHollow	->setEnabled( FALSE );
	mLabelHoleType	->setEnabled( FALSE );
	mLabelTwist		->setEnabled( FALSE );
	mLabelSkew		->setEnabled( FALSE );
	mLabelShear		->setEnabled( FALSE );
	mLabelTaper		->setEnabled( FALSE );
	mLabelRadiusOffset->setEnabled( FALSE );
	mLabelRevolutions->setEnabled( FALSE );

	childSetVisible("select_single", TRUE);
	childSetVisible("edit_object", TRUE);	
	childSetEnabled("edit_object", FALSE);
	
	childSetEnabled("scale_hole", FALSE);
	childSetEnabled("scale_taper", FALSE);
	childSetEnabled( "advanced_cut", FALSE );
	childSetEnabled( "advanced_dimple", FALSE );
}

//
// Static functions
//

// static
void LLPanelObject::onCommitLock(LLUICtrl *ctrl, void *data)
{
	// Checkbox will have toggled itself
	LLPanelObject *self = (LLPanelObject *)data;

	if(self->mRootObject.isNull()) return;

	BOOL new_state = self->mCheckLock->get();
	
	gSelectMgr->selectionSetObjectPermissions(PERM_OWNER, !new_state, PERM_MOVE | PERM_MODIFY);
}

// static
void LLPanelObject::onCommitPosition( LLUICtrl* ctrl, void* userdata )
{
	LLPanelObject* self = (LLPanelObject*) userdata;
	self->sendPosition();
}

// static
void LLPanelObject::onCommitScale( LLUICtrl* ctrl, void* userdata )
{
	LLPanelObject* self = (LLPanelObject*) userdata;
	self->sendScale();
}

// static
void LLPanelObject::onCommitRotation( LLUICtrl* ctrl, void* userdata )
{
	LLPanelObject* self = (LLPanelObject*) userdata;
	self->sendRotation();
}

// static
void LLPanelObject::onCommitPhysics( LLUICtrl* ctrl, void* userdata )
{
	LLPanelObject* self = (LLPanelObject*) userdata;
	self->sendIsPhysical();
}

// static
void LLPanelObject::onCommitTemporary( LLUICtrl* ctrl, void* userdata )
{
	LLPanelObject* self = (LLPanelObject*) userdata;
	self->sendIsTemporary();
}

// static
void LLPanelObject::onCommitPhantom( LLUICtrl* ctrl, void* userdata )
{
	LLPanelObject* self = (LLPanelObject*) userdata;
	self->sendIsPhantom();
}

// static
void LLPanelObject::onCommitCastShadows( LLUICtrl* ctrl, void* userdata )
{
	LLPanelObject* self = (LLPanelObject*) userdata;
	self->sendCastShadows();
}


// static
void LLPanelObject::onSelectSculpt(LLUICtrl* ctrl, void* userdata)
{
	LLPanelObject* self = (LLPanelObject*) userdata;

    LLTextureCtrl* mTextureCtrl = gUICtrlFactory->getTexturePickerByName(self, "sculpt texture control");

	if (mTextureCtrl)
		self->mSculptTextureRevert = mTextureCtrl->getImageAssetID();
	
	self->sendSculpt();
}


void LLPanelObject::onCommitSculpt( LLUICtrl* ctrl, void* userdata )
{
	LLPanelObject* self = (LLPanelObject*) userdata;

	self->sendSculpt();
}

// static
BOOL LLPanelObject::onDropSculpt(LLUICtrl*, LLInventoryItem* item, void* userdata)
{
	LLPanelObject* self = (LLPanelObject*) userdata;

    LLTextureCtrl* mTextureCtrl = gUICtrlFactory->getTexturePickerByName(self, "sculpt texture control");

	if (mTextureCtrl)
	{
		LLUUID asset = item->getAssetUUID();

		mTextureCtrl->setImageAssetID(asset);
		self->mSculptTextureRevert = asset;
	}
	

	return TRUE;
}


// static
void LLPanelObject::onCancelSculpt(LLUICtrl* ctrl, void* userdata)
{
	LLPanelObject* self = (LLPanelObject*) userdata;

	LLTextureCtrl* mTextureCtrl = gUICtrlFactory->getTexturePickerByName(self,"sculpt texture control");
	if(!mTextureCtrl)
		return;
	
	mTextureCtrl->setImageAssetID(self->mSculptTextureRevert);
	
	self->sendSculpt();
}
