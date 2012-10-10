/** 
* @file   llfloaterdebugmaterials.h
* @brief  Header file for llfloaterdebugmaterials
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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
#ifndef LL_LLFLOATERDEBUGMATERIALS_H
#define LL_LLFLOATERDEBUGMATERIALS_H

#include <string>

#include <boost/signals2.hpp>

#include "llfloater.h"
#include "lluuid.h"
#include "v4color.h"

class LLButton;
class LLColorSwatchCtrl;
class LLLineEditor;
class LLMaterialID;
class LLScrollListCtrl;
class LLSD;
class LLTextBase;
class LLTextureCtrl;
class LLUICtrl;

class LLFloaterDebugMaterials : public LLFloater
{
public:
	virtual BOOL postBuild();

	virtual void onOpen(const LLSD& pKey);
	virtual void onClose(bool pIsAppQuitting);

protected:

private:
	friend class LLFloaterReg;

	typedef enum {
		kNoRegion,
		kCapabilitiesLoading,
		kReady,
		kRequestStarted,
		kRequestCompleted,
		kNotEnabled,
		kError
	} EState;

	LLFloaterDebugMaterials(const LLSD& pParams);
	virtual ~LLFloaterDebugMaterials();

	void          onGetClicked();
	void          onValueEntered(LLUICtrl* pUICtrl);
	void          onPutSetClicked();
	void          onPutClearClicked();
	void          onResetPutValuesClicked();
	void          onQueryVisibleObjectsClicked();
	void          onGoodPostClicked();
	void          onBadPostClicked();
	void          onRegionCross();
	void          onInWorldSelectionChange();
	void          onGetScrollListSelectionChange(LLUICtrl* pUICtrl);
	void          onDeferredCheckRegionMaterialStatus(LLUUID regionId);
	void          onDeferredRequestGetMaterials(LLUUID regionId);
	void          onDeferredRequestPutMaterials(LLUUID regionId, bool pIsDoSet);
	void          onDeferredRequestPostMaterials(LLUUID regionId, bool pUseGoodData);
	void          onGetResponse(bool pRequestStatus, const LLSD& pContent);
	void          onPutResponse(bool pRequestStatus, const LLSD& pContent);
	void          onPostResponse(bool pRequestStatus, const LLSD& pContent);

	void          checkRegionMaterialStatus();
	void          checkRegionMaterialStatus(const LLUUID& regionId);

	void          requestGetMaterials();
	void          requestGetMaterials(const LLUUID& regionId);

	void          requestPutMaterials(bool pIsDoSet);
	void          requestPutMaterials(const LLUUID& regionId, bool pIsDoSet);

	void          requestPostMaterials(bool pUseGoodData);
	void          requestPostMaterials(const LLUUID& regionId, bool pUseGoodData);

	void          queryViewableObjects();

	void          parseGetResponse(const LLSD& pContent);
	void          parsePutResponse(const LLSD& pContent);
	void          parsePostResponse(const LLSD& pContent);
	void          printResponse(const std::string& pRequestType, const LLSD& pContent) const;

	void          setState(EState pState);
	inline EState getState() const;

	void          resetObjectEditInputs();
	void          clearGetResults();
	void          clearPutResults();
	void          clearPostResults();
	void          clearViewableObjectsResults();

	void          updateStatusMessage();
	void          updateControls();
	std::string   convertToPrintableMaterialID(const LLSD& pBinaryHash) const;
	std::string   convertToPrintableMaterialID(const LLMaterialID& pMaterialID) const;

	S32           getNormalMapOffsetX() const;
	S32           getNormalMapOffsetY() const;
	S32           getNormalMapRepeatX() const;
	S32           getNormalMapRepeatY() const;
	S32           getNormalMapRotation() const;

	S32           getSpecularMapOffsetX() const;
	S32           getSpecularMapOffsetY() const;
	S32           getSpecularMapRepeatX() const;
	S32           getSpecularMapRepeatY() const;
	S32           getSpecularMapRotation() const;

	S32           getSpecularExponent() const;
	S32           getEnvironmentExponent() const;
	S32           getAlphMaskCutoff() const;
	S32           getDiffuseAlphaMode() const;
	S32           getLineEditorValue(const LLLineEditor *pLineEditor) const;

	LLTextBase*                 mStatusText;
	LLButton*                   mGetButton;
	LLScrollListCtrl*           mGetNormalMapScrollList;
	LLScrollListCtrl*           mGetSpecularMapScrollList;
	LLScrollListCtrl*           mGetOtherDataScrollList;
	LLTextureCtrl*              mNormalMap;
	LLLineEditor*               mNormalMapOffsetX;
	LLLineEditor*               mNormalMapOffsetY;
	LLLineEditor*               mNormalMapRepeatX;
	LLLineEditor*               mNormalMapRepeatY;
	LLLineEditor*               mNormalMapRotation;
	LLTextureCtrl*              mSpecularMap;
	LLLineEditor*               mSpecularMapOffsetX;
	LLLineEditor*               mSpecularMapOffsetY;
	LLLineEditor*               mSpecularMapRepeatX;
	LLLineEditor*               mSpecularMapRepeatY;
	LLLineEditor*               mSpecularMapRotation;
	LLColorSwatchCtrl*          mSpecularColor;
	LLLineEditor*               mSpecularExponent;
	LLLineEditor*               mEnvironmentExponent;
	LLLineEditor*               mAlphaMaskCutoff;
	LLLineEditor*               mDiffuseAlphaMode;
	LLButton*                   mPutSetButton;
	LLButton*                   mPutClearButton;
	LLScrollListCtrl*           mPutScrollList;
	LLButton*                   mQueryViewableObjectsButton;
	LLScrollListCtrl*           mViewableObjectsScrollList;
	LLButton*                   mGoodPostButton;
	LLButton*                   mBadPostButton;
	LLScrollListCtrl*           mPostNormalMapScrollList;
	LLScrollListCtrl*           mPostSpecularMapScrollList;
	LLScrollListCtrl*           mPostOtherDataScrollList;

	LLColor4                    mDefaultSpecularColor;

	EState                      mState;
	LLColor4                    mWarningColor;
	LLColor4                    mErrorColor;

	boost::signals2::connection mRegionCrossConnection;
	boost::signals2::connection mTeleportFailedConnection;
	boost::signals2::connection mSelectionUpdateConnection;
};



LLFloaterDebugMaterials::EState LLFloaterDebugMaterials::getState() const
{
	return mState;
}

#endif // LL_LLFLOATERDEBUGMATERIALS_H
