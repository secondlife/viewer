/**
 * @file lllocalbitmaps.h
 * @author Vaalith Jinn
 * @brief Local Bitmaps header
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LOCALBITMAPS_H
#define LL_LOCALBITMAPS_H

#include "llavatarappearancedefines.h"
#include "lleventtimer.h"
#include "llpointer.h"
#include "llwearabletype.h"

class LLScrollListCtrl;
class LLImageRaw;
class LLViewerObject;
class LLGLTFMaterial;

class LLLocalBitmap
{
    public: /* main */
        LLLocalBitmap(std::string filename);
        ~LLLocalBitmap();

    public: /* accessors */
        std::string getFilename() const;
        std::string getShortName() const;
        LLUUID      getTrackingID() const;
        LLUUID      getWorldID() const;
        bool        getValid() const;

    public: /* self update public section */
        enum EUpdateType
        {
            UT_FIRSTUSE,
            UT_REGUPDATE
        };

        bool updateSelf(EUpdateType = UT_REGUPDATE);

        typedef boost::signals2::signal<void(const LLUUID& tracking_id,
                                             const LLUUID& old_id,
                                             const LLUUID& new_id)> LLLocalTextureChangedSignal;
        typedef LLLocalTextureChangedSignal::slot_type LLLocalTextureCallback;
        boost::signals2::connection setChangedCallback(const LLLocalTextureCallback& cb);
        void addGLTFMaterial(LLGLTFMaterial* mat);

    private: /* self update private section */
        bool decodeBitmap(LLPointer<LLImageRaw> raw);
        void replaceIDs(const LLUUID &old_id, LLUUID new_id);
        std::vector<LLViewerObject*> prepUpdateObjects(LLUUID old_id, U32 channel);
        void updateUserPrims(LLUUID old_id, LLUUID new_id, U32 channel);
        void updateUserVolumes(LLUUID old_id, LLUUID new_id, U32 channel);
        void updateUserLayers(LLUUID old_id, LLUUID new_id, LLWearableType::EType type);
        void updateGLTFMaterials(LLUUID old_id, LLUUID new_id);
        LLAvatarAppearanceDefines::ETextureIndex getTexIndex(LLWearableType::EType type, LLAvatarAppearanceDefines::EBakedTextureIndex baked_texind);

    private: /* private enums */
        enum ELinkStatus
        {
            LS_ON,
            LS_BROKEN,
        };

        enum EExtension
        {
            ET_IMG_BMP,
            ET_IMG_TGA,
            ET_IMG_JPG,
            ET_IMG_PNG
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
        LLLocalTextureChangedSignal mChangedSignal;

        // Store a list of accosiated materials
        // Might be a better idea to hold this in LLGLTFMaterialList
        typedef std::list<LLPointer<LLGLTFMaterial> > mat_list_t;
        mat_list_t mGLTFMaterialWithLocalTextures;

};

class LLLocalBitmapTimer : public LLEventTimer
{
    public:
        LLLocalBitmapTimer();
        ~LLLocalBitmapTimer();

    public:
        void startTimer();
        void stopTimer();
        bool isRunning();
        bool tick();

};

class LLLocalBitmapMgr : public LLSingleton<LLLocalBitmapMgr>
{
    LLSINGLETON(LLLocalBitmapMgr);
    ~LLLocalBitmapMgr();
public:
    bool         addUnit(const std::vector<std::string>& filenames);
    LLUUID       addUnit(const std::string& filename);
    void         delUnit(LLUUID tracking_id);
    bool        checkTextureDimensions(std::string filename);

    LLUUID       getTrackingID(const LLUUID& world_id) const;
    LLUUID       getWorldID(const LLUUID &tracking_id) const;
    bool         isLocal(const LLUUID& world_id) const;
    std::string  getFilename(const LLUUID &tracking_id) const;
    boost::signals2::connection setOnChangedCallback(const LLUUID tracking_id, const LLLocalBitmap::LLLocalTextureCallback& cb);
    void associateGLTFMaterial(const LLUUID tracking_id, LLGLTFMaterial* mat);

    void         feedScrollList(LLScrollListCtrl* ctrl);
    void         doUpdates();
    void         setNeedsRebake();
    void         doRebake();

private:
    std::list<LLLocalBitmap*>    mBitmapList;
    LLLocalBitmapTimer           mTimer;
    bool                         mNeedsRebake;
    typedef std::list<LLLocalBitmap*>::iterator local_list_iter;
    typedef std::list<LLLocalBitmap*>::const_iterator local_list_citer;
};

#endif

