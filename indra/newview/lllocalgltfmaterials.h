/** 
 * @file lllocalrendermaterials.h
 * @brief Local GLTF materials header
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#ifndef LL_LOCALGLTFMATERIALS_H
#define LL_LOCALGLTFMATERIALS_H

#include "lleventtimer.h"
#include "llpointer.h"

class LLScrollListCtrl;
class LLGLTFMaterial;
class LLViewerObject;
class LLViewerFetchedTexture;

class LLLocalGLTFMaterial
{
public: /* main */
    LLLocalGLTFMaterial(std::string filename);
    ~LLLocalGLTFMaterial();

public: /* accessors */
    std::string	getFilename();
    std::string	getShortName();
    LLUUID		getTrackingID();
    LLUUID		getWorldID();
    bool		getValid();

public:
    bool updateSelf();

private:
    bool loadMaterial(LLPointer<LLGLTFMaterial> raw);

private: /* private enums */
    enum ELinkStatus
    {
        LS_ON,
        LS_BROKEN,
    };

    enum EExtension
    {
        ET_MATERIAL_GLTF,
        ET_MATERIAL_GLB,
    };

private: /* members */
    std::string mFilename;
    std::string mShortName;
    LLUUID      mTrackingID;
    LLUUID      mWorldID;
    bool        mValid;
    LLSD        mLastModified;
    EExtension  mExtension;
    ELinkStatus mLinkStatus;
    S32         mUpdateRetries;

    // material needs to maintain textures
    LLPointer<LLViewerFetchedTexture> mBaseColorFetched;
    LLPointer<LLViewerFetchedTexture> mNormalFetched;
    LLPointer<LLViewerFetchedTexture> mMRFetched;
    LLPointer<LLViewerFetchedTexture> mEmissiveFetched;
};

class LLLocalGLTFMaterialTimer : public LLEventTimer
{
public:
    LLLocalGLTFMaterialTimer();
    ~LLLocalGLTFMaterialTimer();

public:
    void startTimer();
    void stopTimer();
    bool isRunning();
    BOOL tick();
};

class LLLocalGLTFMaterialMgr : public LLSingleton<LLLocalGLTFMaterialMgr>
{
    LLSINGLETON(LLLocalGLTFMaterialMgr);
    ~LLLocalGLTFMaterialMgr();
public:
    bool         addUnit(const std::vector<std::string>& filenames);
    LLUUID       addUnit(const std::string& filename);
    void         delUnit(LLUUID tracking_id);

    LLUUID       getWorldID(LLUUID tracking_id);
    bool         isLocal(LLUUID world_id);
    std::string  getFilename(LLUUID tracking_id);

    void         feedScrollList(LLScrollListCtrl* ctrl);
    void         doUpdates();

private:
    std::list<LLLocalGLTFMaterial*>    mMaterialList;
    LLLocalGLTFMaterialTimer           mTimer;
    typedef std::list<LLLocalGLTFMaterial*>::iterator local_list_iter;
};

#endif // LL_LOCALGLTFMATERIALS_H

