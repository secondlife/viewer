/**
 * @file llpanelobject.h
 * @brief Object editing (position, scale, etc.) in the tools floater
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLPANELOBJECT_H
#define LL_LLPANELOBJECT_H

#include "v3math.h"
#include "llpanel.h"
#include "llpointer.h"
#include "llvolume.h"

class LLSpinCtrl;
class LLCheckBoxCtrl;
class LLTextBox;
class LLUICtrl;
class LLButton;
class LLMenuButton;
class LLViewerObject;
class LLComboBox;
class LLColorSwatchCtrl;
class LLTextureCtrl;
class LLInventoryItem;
class LLUUID;

class LLPanelObject : public LLPanel
{
public:
    LLPanelObject();
    virtual ~LLPanelObject();

    virtual bool    postBuild();
    virtual void    draw();
    virtual void    clearCtrls();

    void            refresh();

    static bool     precommitValidate(const LLSD& data);

    static void     onCommitLock(LLUICtrl *ctrl, void *data);
    static void     onCommitPosition(       LLUICtrl* ctrl, void* userdata);
    static void     onCommitScale(          LLUICtrl* ctrl, void* userdata);
    static void     onCommitRotation(       LLUICtrl* ctrl, void* userdata);
    static void     onCommitTemporary(      LLUICtrl* ctrl, void* userdata);
    static void     onCommitPhantom(        LLUICtrl* ctrl, void* userdata);
    static void     onCommitPhysics(        LLUICtrl* ctrl, void* userdata);

    void            onCopyPos();
    void            onPastePos();
    void            onCopySize();
    void            onPasteSize();
    void            onCopyRot();
    void            onPasteRot();
    void            onCopyParams();
    void            onPasteParams();
    static void     onCommitParametric(LLUICtrl* ctrl, void* userdata);


    void            onCommitSculpt(const LLSD& data);
    void            onCancelSculpt(const LLSD& data);
    void            onSelectSculpt(const LLSD& data);
    bool            onDropSculpt(LLInventoryItem* item);
    static void     onCommitSculptType(    LLUICtrl *ctrl, void* userdata);

    void            menuDoToSelected(const LLSD& userdata);
    bool            menuEnableItem(const LLSD& userdata);

protected:
    void            getState();

    void            sendRotation(bool btn_down);
    void            sendScale(bool btn_down);
    void            sendPosition(bool btn_down);
    void            sendIsPhysical();
    void            sendIsTemporary();
    void            sendIsPhantom();

    void            sendSculpt();

    void            getVolumeParams(LLVolumeParams& volume_params);

protected:
    // Per-object options
    LLComboBox*     mComboBaseType;
    LLMenuButton*   mMenuClipboardParams;

    LLTextBox*      mLabelCut;
    LLSpinCtrl*     mSpinCutBegin;
    LLSpinCtrl*     mSpinCutEnd;

    LLTextBox*      mLabelHollow;
    LLSpinCtrl*     mSpinHollow;

    LLTextBox*      mLabelHoleType;
    LLComboBox*     mComboHoleType;

    LLTextBox*      mLabelTwist;
    LLSpinCtrl*     mSpinTwist;
    LLSpinCtrl*     mSpinTwistBegin;

    LLSpinCtrl*     mSpinScaleX;
    LLSpinCtrl*     mSpinScaleY;

    LLTextBox*      mLabelSkew;
    LLSpinCtrl*     mSpinSkew;

    LLTextBox*      mLabelShear;
    LLSpinCtrl*     mSpinShearX;
    LLSpinCtrl*     mSpinShearY;

    // Advanced Path
    LLSpinCtrl*     mCtrlPathBegin;
    LLSpinCtrl*     mCtrlPathEnd;

    LLTextBox*      mLabelTaper;
    LLSpinCtrl*     mSpinTaperX;
    LLSpinCtrl*     mSpinTaperY;

    LLTextBox*      mLabelRadiusOffset;
    LLSpinCtrl*     mSpinRadiusOffset;

    LLTextBox*      mLabelRevolutions;
    LLSpinCtrl*     mSpinRevolutions;

    LLMenuButton*   mMenuClipboardPos;
    LLTextBox*      mLabelPosition;
    LLSpinCtrl*     mCtrlPosX;
    LLSpinCtrl*     mCtrlPosY;
    LLSpinCtrl*     mCtrlPosZ;

    LLMenuButton*   mMenuClipboardSize;
    LLTextBox*      mLabelSize;
    LLSpinCtrl*     mCtrlScaleX;
    LLSpinCtrl*     mCtrlScaleY;
    LLSpinCtrl*     mCtrlScaleZ;
    bool            mSizeChanged;

    LLMenuButton*   mMenuClipboardRot;
    LLTextBox*      mLabelRotation;
    LLSpinCtrl*     mCtrlRotX;
    LLSpinCtrl*     mCtrlRotY;
    LLSpinCtrl*     mCtrlRotZ;

    LLCheckBoxCtrl  *mCheckLock;
    LLCheckBoxCtrl  *mCheckPhysics;
    LLCheckBoxCtrl  *mCheckTemporary;
    LLCheckBoxCtrl  *mCheckPhantom;

    LLTextureCtrl   *mCtrlSculptTexture;
    LLTextBox       *mLabelSculptType;
    LLComboBox      *mCtrlSculptType;
    LLCheckBoxCtrl  *mCtrlSculptMirror;
    LLCheckBoxCtrl  *mCtrlSculptInvert;

    LLVector3       mCurEulerDegrees;       // to avoid sending rotation when not changed
    bool            mIsPhysical;            // to avoid sending "physical" when not changed
    bool            mIsTemporary;           // to avoid sending "temporary" when not changed
    bool            mIsPhantom;             // to avoid sending "phantom" when not changed
    S32             mSelectedType;          // So we know what selected type we last were

    LLUUID          mSculptTextureRevert;   // so we can revert the sculpt texture on cancel
    U8              mSculptTypeRevert;      // so we can revert the sculpt type on cancel

    LLVector3       mClipboardPos;
    LLVector3       mClipboardSize;
    LLVector3       mClipboardRot;
    LLSD            mClipboardParams;

    bool            mHasClipboardPos;
    bool            mHasClipboardSize;
    bool            mHasClipboardRot;

    LLPointer<LLViewerObject> mObject;
    LLPointer<LLViewerObject> mRootObject;
};

#endif
