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

#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>

#include "llfloater.h"
#include "lluuid.h"
#include "v4color.h"

class LLButton;
class LLColorSwatchCtrl;
class LLColor4U;
class LLLineEditor;
class LLMaterialID;
class LLScrollListCtrl;
class LLSD;
class LLSpinCtrl;
class LLTextBase;
class LLTextureCtrl;
class LLUICtrl;
class MultiMaterialsResponder;

typedef boost::shared_ptr<MultiMaterialsResponder> MultiMaterialsResponderPtr;

class LLFloaterDebugMaterials : public LLFloater
{
public:
	virtual BOOL postBuild();

	virtual void onOpen(const LLSD& pKey);
	virtual void onClose(bool pIsAppQuitting);

	virtual void draw();

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
	void          onPostClicked();
	void          onRegionCross();
	void          onInWorldSelectionChange();
	void          onGetScrollListSelectionChange(LLUICtrl* pUICtrl);
	void          onPostScrollListSelectionChange(LLUICtrl* pUICtrl);
	void          onViewableObjectsScrollListSelectionChange();
	void          onDeferredCheckRegionMaterialStatus(LLUUID regionId);
	void          onDeferredRequestGetMaterials(LLUUID regionId);
	void          onDeferredRequestPutMaterials(LLUUID regionId, bool pIsDoSet);
	void          onGetResponse(bool pRequestStatus, const LLSD& pContent);
	void          onPutResponse(bool pRequestStatus, const LLSD& pContent);
	void          onPostResponse(bool pRequestStatus, const LLSD& pContent);

	void          checkRegionMaterialStatus();
	void          checkRegionMaterialStatus(const LLUUID& regionId);

	void          requestGetMaterials();
	void          requestGetMaterials(const LLUUID& regionId);

	void          requestPutMaterials(bool pIsDoSet);
	void          requestPutMaterials(const LLUUID& regionId, bool pIsDoSet);

	void          requestPostMaterials();

	void          parseGetResponse();
	void          parsePutResponse(const LLSD& pContent);
	void          parseQueryViewableObjects();
	void          parsePostResponse(const LLSD& pMultiContent);

	void          setState(EState pState);
	inline EState getState() const;

	void          resetObjectEditInputs();
	void          clearGetResults();
	void          clearPutResults();
	void          clearPostResults();
	void          clearViewableObjectsResults();

	void          setUnparsedGetData(const LLSD& pGetData);
	void          clearUnparsedGetData();
	void          updateGetParsingStatus();

	void          setUnparsedQueryData();
	void          clearUnparsedQueryData();
	void          updateQueryParsingStatus();

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

	LLColor4U     getSpecularColor() const;
	S32           getSpecularExponent() const;
	S32           getEnvironmentExponent() const;
	S32           getAlphMaskCutoff() const;
	S32           getDiffuseAlphaMode() const;
	S32           getLineEditorValue(const LLLineEditor *pLineEditor) const;

	LLTextBase*                 mStatusText;
	LLButton*                   mGetButton;
	LLTextBase*                 mParsingStatusText;
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
	LLSpinCtrl*                 mSpecularColorAlpha;
	LLLineEditor*               mSpecularExponent;
	LLLineEditor*               mEnvironmentExponent;
	LLLineEditor*               mAlphaMaskCutoff;
	LLLineEditor*               mDiffuseAlphaMode;
	LLButton*                   mPutSetButton;
	LLButton*                   mPutClearButton;
	LLScrollListCtrl*           mPutScrollList;
	LLButton*                   mQueryViewableObjectsButton;
	LLTextBase*                 mQueryStatusText;
	LLScrollListCtrl*           mViewableObjectsScrollList;
	LLButton*                   mPostButton;
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

	LLSD                        mUnparsedGetData;
	S32                         mNextUnparsedGetDataIndex;

	S32                         mNextUnparsedQueryDataIndex;
	MultiMaterialsResponderPtr  mMultiMaterialsResponder;
};


LLFloaterDebugMaterials::EState LLFloaterDebugMaterials::getState() const
{
	return mState;
}

#endif // LL_LLFLOATERDEBUGMATERIALS_H
