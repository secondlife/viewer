/** 
 * @file llmaterialeditor.h
 * @brief LLMaterialEditor class header file
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
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

#pragma once

#include "llfloater.h"

class LLTextureCtrl;

class LLMaterialEditor : public LLFloater
{
public:
	LLMaterialEditor(const LLSD& key);

    // open a file dialog and select a gltf/glb file for import
    void importMaterial();

    // for live preview, apply current material to currently selected object
    void applyToSelection();

    void onClickSave();
    void onClickSaveAs();
    void onSaveAsMsgCallback(const LLSD& notification, const LLSD& response);
    void onClickCancel();
    void onCancelMsgCallback(const LLSD& notification, const LLSD& response);

	// llpanel
	BOOL postBuild() override;
    void onClickCloseBtn(bool app_quitting = false) override;

    LLUUID getAlbedoId();
    void setAlbedoId(const LLUUID& id);

    LLColor4 getAlbedoColor();

    // sets both albedo color and transparency
    void    setAlbedoColor(const LLColor4& color);

    F32     getTransparency();
    
    std::string getAlphaMode();
    void setAlphaMode(const std::string& alpha_mode);

    F32 getAlphaCutoff();
    void setAlphaCutoff(F32 alpha_cutoff);
    
    void setMaterialName(const std::string &name);

    LLUUID getMetallicRoughnessId();
    void setMetallicRoughnessId(const LLUUID& id);

    F32 getMetalnessFactor();
    void setMetalnessFactor(F32 factor);

    F32 getRoughnessFactor();
    void setRoughnessFactor(F32 factor);

    LLUUID getEmissiveId();
    void setEmissiveId(const LLUUID& id);

    LLColor4 getEmissiveColor();
    void setEmissiveColor(const LLColor4& color);

    LLUUID getNormalId();
    void setNormalId(const LLUUID& id);

    bool getDoubleSided();
    void setDoubleSided(bool double_sided);

    void setHasUnsavedChanges(bool value);

    void onCommitAlbedoTexture(LLUICtrl* ctrl, const LLSD& data);
    void onCommitMetallicTexture(LLUICtrl* ctrl, const LLSD& data);
    void onCommitEmissiveTexture(LLUICtrl* ctrl, const LLSD& data);
    void onCommitNormalTexture(LLUICtrl* ctrl, const LLSD& data);

private:
    LLTextureCtrl* mAlbedoTextureCtrl;
    LLTextureCtrl* mMetallicTextureCtrl;
    LLTextureCtrl* mEmissiveTextureCtrl;
    LLTextureCtrl* mNormalTextureCtrl;

    // 'Default' texture, unless it's null or from inventory is the one with the fee
    LLUUID mAlbedoTextureUploadId;
    LLUUID mMetallicTextureUploadId;
    LLUUID mEmissiveTextureUploadId;
    LLUUID mNormalTextureUploadId;

    bool mHasUnsavedChanges;
    std::string mMaterialName;
};

