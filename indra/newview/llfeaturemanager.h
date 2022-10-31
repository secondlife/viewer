/** 
 * @file llfeaturemanager.h
 * @brief The feature manager is responsible for determining what features are turned on/off in the app.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLFEATUREMANAGER_H
#define LL_LLFEATUREMANAGER_H

#include "stdtypes.h"

#include "llsingleton.h"
#include "llstring.h"
#include <map>
#include "llcoros.h"
#include "lleventcoro.h"

typedef enum EGPUClass
{
    GPU_CLASS_UNKNOWN = -1,
    GPU_CLASS_0 = 0,
    GPU_CLASS_1 = 1,
    GPU_CLASS_2 = 2,
    GPU_CLASS_3 = 3,
    GPU_CLASS_4 = 4,
    GPU_CLASS_5 = 5
} EGPUClass; 


class LLFeatureInfo
{
public:
    LLFeatureInfo() : mValid(FALSE), mAvailable(FALSE), mRecommendedLevel(-1) {}
    LLFeatureInfo(const std::string& name, const BOOL available, const F32 level);

    BOOL isValid() const    { return mValid; };

public:
    BOOL        mValid;
    std::string mName;
    BOOL        mAvailable;
    F32         mRecommendedLevel;
};


class LLFeatureList
{
public:
    typedef std::map<std::string, LLFeatureInfo> feature_map_t;

    LLFeatureList(const std::string& name);
    virtual ~LLFeatureList();

    BOOL isFeatureAvailable(const std::string& name);
    F32 getRecommendedValue(const std::string& name);

    void setFeatureAvailable(const std::string& name, const BOOL available);
    void setRecommendedLevel(const std::string& name, const F32 level);

    bool loadFeatureList(LLFILE *fp);

    BOOL maskList(LLFeatureList &mask);

    void addFeature(const std::string& name, const BOOL available, const F32 level);

    feature_map_t& getFeatures()
    {
        return mFeatures;
    }

    void dump();
protected:
    std::string mName;
    feature_map_t   mFeatures;
};


class LLFeatureManager : public LLFeatureList, public LLSingleton<LLFeatureManager>
{
    LLSINGLETON(LLFeatureManager);
    ~LLFeatureManager() {cleanupFeatureTables();}

    // initialize this by loading feature table and gpu table
    void initSingleton();

public:

    void maskCurrentList(const std::string& name); // Mask the current feature list with the named list

    bool loadFeatureTables();

    EGPUClass getGPUClass()             { return mGPUClass; }
    std::string& getGPUString()         { return mGPUString; }
    BOOL isGPUSupported()               { return mGPUSupported; }
    F32 getExpectedGLVersion()          { return mExpectedGLVersion; }
    
    void cleanupFeatureTables();

    S32 getVersion() const              { return mTableVersion; }
    void setSafe(const BOOL safe)       { mSafe = safe; }
    BOOL isSafe() const                 { return mSafe; }

    LLFeatureList *findMask(const std::string& name);
    BOOL maskFeatures(const std::string& name);

    // set the graphics to low, medium, high, or ultra.
    // skipFeatures forces skipping of mostly hardware settings
    // that we don't want to change when we change graphics
    // settings
    void setGraphicsLevel(U32 level, bool skipFeatures);

    // What 'level' values are valid to pass to setGraphicsLevel()?
    // 0 is the low end...
    U32 getMaxGraphicsLevel() const;
    bool isValidGraphicsLevel(U32 level) const;

    // setGraphicsLevel() levels have names.
    std::string getNameForGraphicsLevel(U32 level) const;
    // returns -1 for unrecognized name (hence S32 rather than U32)
    S32 getGraphicsLevelForName(const std::string& name) const;

    void applyBaseMasks();
    void applyRecommendedSettings();

    // apply the basic masks.  Also, skip one saved
    // in the skip list if true
    void applyFeatures(bool skipFeatures);

    LLSD getRecommendedSettingsMap();

protected:
    bool loadGPUClass();

    bool parseFeatureTable(std::string filename);
    ///< @returns TRUE is file parsed correctly, FALSE if not

    void initBaseMask();

    std::map<std::string, LLFeatureList *> mMaskList;
    std::set<std::string> mSkippedFeatures;
    BOOL        mInited;
    S32         mTableVersion;
    BOOL        mSafe;                  // Reinitialize everything to the "safe" mask
    EGPUClass   mGPUClass;
    F32         mExpectedGLVersion;     //expected GL version according to gpu table
    std::string mGPUString;
    BOOL        mGPUSupported;
};

inline
LLFeatureManager::LLFeatureManager()
:   LLFeatureList("default"),

    mInited(FALSE),
    mTableVersion(0),
    mSafe(FALSE),
    mGPUClass(GPU_CLASS_UNKNOWN),
    mExpectedGLVersion(0.f),
    mGPUSupported(FALSE)
{
}

#endif // LL_LLFEATUREMANAGER_H
