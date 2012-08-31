/** 
* @file   llfloaterstinson.h
* @brief  Header file for llfloaterstinson
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
#ifndef LL_LLFLOATERSTINSON_H
#define LL_LLFLOATERSTINSON_H

#include <boost/signals2.hpp>

#include "llfloater.h"
#include "lluuid.h"
#include "v4color.h"

class LLButton;
class LLScrollListCtrl;
class LLSD;
class LLTextBase;

class LLFloaterStinson : public LLFloater
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

	LLFloaterStinson(const LLSD& pParams);
	virtual ~LLFloaterStinson();

	void          onGetClicked();
	void          onPutClicked();
	void          onRegionCross();
	void          onInWorldSelectionChange();
	void          onDeferredCheckRegionMaterialStatus(LLUUID regionId);
	void          onDeferredRequestGetMaterials(LLUUID regionId);
	void          onDeferredRequestPutMaterials(LLUUID regionId);
	void          onGetResponse(bool pRequestStatus, const LLSD& pContent);
	void          onPutResponse(bool pRequestStatus, const LLSD& pContent);

	void          checkRegionMaterialStatus();
	void          checkRegionMaterialStatus(const LLUUID& regionId);

	void          requestGetMaterials();
	void          requestGetMaterials(const LLUUID& regionId);

	void          requestPutMaterials();
	void          requestPutMaterials(const LLUUID& regionId);

	void          parseGetResponse(const LLSD& pContent);
	void          printResponse(const std::string& pRequestType, const LLSD& pContent) const;

	void          setState(EState pState);
	inline EState getState() const;

	void          clearMaterialsList();

	void          updateStatusMessage();
	void          updateControls();

	LLTextBase*                 mStatusText;
	LLButton*                   mGetButton;
	LLButton*                   mPutButton;
	LLScrollListCtrl*           mMaterialsScrollList;

	EState                      mState;
	LLColor4                    mWarningColor;
	LLColor4                    mErrorColor;

	boost::signals2::connection mRegionCrossConnection;
	boost::signals2::connection mTeleportFailedConnection;
	boost::signals2::connection mSelectionUpdateConnection;
};



LLFloaterStinson::EState LLFloaterStinson::getState() const
{
	return mState;
}

#endif // LL_LLFLOATERSTINSON_H
