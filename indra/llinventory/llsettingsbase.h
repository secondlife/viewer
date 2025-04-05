/**
* @file llsettingsbase.h
* @author optional
* @brief A base class for asset based settings groups.
*
* $LicenseInfo:2011&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2017, Linden Research, Inc.
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

#ifndef LL_SETTINGS_BASE_H
#define LL_SETTINGS_BASE_H

#include <string>
#include <map>
#include <vector>
#include <boost/signals2.hpp>

#include "llsd.h"
#include "llsdutil.h"
#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "llquaternion.h"
#include "v4color.h"
#include "v3color.h"
#include "llunits.h"

#include "llinventorysettings.h"

#define PTR_NAMESPACE     std
#define SETTINGS_OVERRIDE override

class LLSettingsBase :
    public PTR_NAMESPACE::enable_shared_from_this<LLSettingsBase>,
    private boost::noncopyable
{
    friend class LLEnvironment;
    friend class LLSettingsDay;

    friend std::ostream &operator <<(std::ostream& os, LLSettingsBase &settings);

protected:
    LOG_CLASS(LLSettingsBase);
public:
    typedef F64Seconds Seconds;
    typedef F64        BlendFactor;
    typedef F32        TrackPosition; // 32-bit as these are stored in LLSD as such
    static const TrackPosition INVALID_TRACKPOS;
    static const std::string DEFAULT_SETTINGS_NAME;

    static const std::string SETTING_ID;
    static const std::string SETTING_NAME;
    static const std::string SETTING_HASH;
    static const std::string SETTING_TYPE;
    static const std::string SETTING_ASSETID;
    static const std::string SETTING_FLAGS;

    static const U32 FLAG_NOCOPY;
    static const U32 FLAG_NOMOD;
    static const U32 FLAG_NOTRANS;
    static const U32 FLAG_NOSAVE;

    class DefaultParam
    {
    public:
        DefaultParam(S32 key, const LLSD& value) : mShaderKey(key), mDefaultValue(value) {}
        DefaultParam() : mShaderKey(-1) {}
        S32 getShaderKey() const { return mShaderKey; }
        const LLSD getDefaultValue() const { return mDefaultValue; }

    private:
        S32 mShaderKey;
        LLSD mDefaultValue;
    };
    // Contains settings' names (map key), related shader id-key and default
    // value for revert in case we need to reset shader (no need to search each time)
    typedef std::map<std::string, DefaultParam>  parammapping_t;

    typedef PTR_NAMESPACE::shared_ptr<LLSettingsBase> ptr_t;

    virtual ~LLSettingsBase() { };

    //---------------------------------------------------------------------
    virtual std::string getSettingsType() const = 0;

    virtual LLSettingsType::type_e getSettingsTypeValue() const = 0;

    //---------------------------------------------------------------------
    // Settings status
    inline bool hasSetting(const std::string &param) const { return mSettings.has(param); }
    virtual bool isDirty() const { return mDirty; }
    virtual bool isVeryDirty() const { return mReplaced; }
    inline void setDirtyFlag(bool dirty) { mDirty = dirty; clearAssetId(); }
    inline void setReplaced() { mReplaced = true; }

    size_t getHash(); // Hash will not include Name, ID or a previously stored Hash

    inline LLUUID getId() const
    {
        return mSettingId;
    }

    inline std::string getName() const
    {
        return mSettingName;
    }

    inline void setName(std::string val)
    {
        mSettingName = val;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    inline LLUUID getAssetId() const
    {
        return mAssetId;
    }

    inline U32 getFlags() const
    {
        return mSettingFlags;
    }

    inline void setFlags(U32 value)
    {
        mSettingFlags = value;
        setDirtyFlag(true);
        setLLSDDirty();
    }

    inline bool getFlag(U32 flag) const
    {
        return (mSettingFlags & flag) == flag;
    }

    inline void setFlag(U32 flag)
    {
        mSettingFlags |= flag;
        setLLSDDirty();
    }

    inline void clearFlag(U32 flag)
    {
        mSettingFlags &= ~flag;
        setLLSDDirty();
    }

    virtual void replaceSettings(LLSD settings)
    {
        mBlendedFactor = 0.0;
        setDirtyFlag(true);
        mReplaced = true;
        mSettings = settings;
        loadValuesFromLLSD();
    }

    virtual void replaceSettings(const ptr_t& other)
    {
        mBlendedFactor = 0.0;
        setDirtyFlag(true);
        mReplaced = true;
        mSettingFlags = other->getFlags();
        mSettingName = other->getName();
        mSettingId = other->getId();
        mAssetId = other->getAssetId();
        setLLSDDirty();
    }

    void setSettings(LLSD settings)
    {
        setDirtyFlag(true);
        mSettings = settings;
        loadValuesFromLLSD();
    }

    // if you are using getSettings to edit them, call setSettings(settings),
    // replaceSettings(settings) or loadValuesFromLLSD() afterwards
    virtual LLSD& getSettings();
    virtual void setLLSDDirty()
    {
        mLLSDDirty = true;
    }

    //---------------------------------------------------------------------
    //
    inline void setLLSD(const std::string &name, const LLSD &value)
    {
        saveValuesIfNeeded();
        mSettings[name] = value;
        mDirty = true;
        if (name != SETTING_ASSETID)
            clearAssetId();
    }

    inline void setValue(const std::string &name, const LLSD &value)
    {
        setLLSD(name, value);
    }

    inline LLSD getValue(const std::string &name, const LLSD &deflt = LLSD())
    {
        saveValuesIfNeeded();
        if (!mSettings.has(name))
            return deflt;
        return mSettings[name];
    }

    inline void setValue(const std::string &name, F32 v)
    {
        setLLSD(name, LLSD::Real(v));
    }

    inline void setValue(const std::string &name, const LLVector2 &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLVector3 &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLVector4 &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLQuaternion &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLColor3 &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLColor4 &value)
    {
        setValue(name, value.getValue());
    }

    inline BlendFactor getBlendFactor() const
    {
        return mBlendedFactor;
    }

    // Note this method is marked const but may modify the settings object.
    // (note the internal const cast).  This is so that it may be called without
    // special consideration from getters.
    inline void     update() const
    {
        if ((!mDirty) && (!mReplaced))
            return;
        (const_cast<LLSettingsBase *>(this))->updateSettings();
    }

    virtual void    blend(ptr_t &end, BlendFactor blendf) = 0;

    virtual bool    validate();

    virtual ptr_t   buildDerivedClone() = 0;

    class Validator
    {
    public:
        static const U32 VALIDATION_PARTIAL;

        typedef boost::function<bool(LLSD &, U32)> verify_pr;

        Validator(std::string name, bool required, LLSD::Type type, verify_pr verify = verify_pr(), LLSD defval = LLSD())  :
            mName(name),
            mRequired(required),
            mType(type),
            mVerify(verify),
            mDefault(defval)
        {   }

        std::string getName() const { return mName; }
        bool        isRequired() const { return mRequired; }
        LLSD::Type  getType() const { return mType; }

        bool        verify(LLSD &data, U32 flags);

        // Some basic verifications
        static bool verifyColor(LLSD &value, U32 flags);
        static bool verifyVector(LLSD &value, U32 flags, S32 length);
        static bool verifyVectorMinMax(LLSD &value, U32 flags, LLSD minvals, LLSD maxvals);
        static bool verifyVectorNormalized(LLSD &value, U32 flags, S32 length);
        static bool verifyQuaternion(LLSD &value, U32 flags);
        static bool verifyQuaternionNormal(LLSD &value, U32 flags);
        static bool verifyFloatRange(LLSD &value, U32 flags, LLSD range);
        static bool verifyIntegerRange(LLSD &value, U32 flags, LLSD range);
        static bool verifyStringLength(LLSD &value, U32 flags, S32 length);

    private:
        std::string mName;
        bool        mRequired;
        LLSD::Type  mType;
        verify_pr   mVerify;
        LLSD        mDefault;
    };
    typedef std::vector<Validator> validation_list_t;

    static LLSD settingValidation(LLSD &settings, validation_list_t &validations, bool partial = false);

    inline void setAssetId(LLUUID value)
    {   // note that this skips setLLSD
        mAssetId = value;
        mLLSDDirty = true;
    }

    inline void clearAssetId()
    {
        mAssetId.setNull();
        mLLSDDirty = true;
    }

    // Calculate any custom settings that may need to be cached.
    virtual void updateSettings() { mDirty = false; mReplaced = false; }
    LLSD         cloneSettings();

    static void lerpVector2(LLVector2& a, const LLVector2& b, F32 mix);
    static void lerpVector3(LLVector3& a, const LLVector3& b, F32 mix);
    static void lerpColor(LLColor3& a, const LLColor3& b, F32 mix);

protected:

    LLSettingsBase();
    LLSettingsBase(const LLSD setting);

    typedef std::set<std::string>   stringset_t;

    // combining settings maps where it can based on mix rate
    // @settings initial value (mix==0)
    // @other target value (mix==1)
    // @defaults list of default values for legacy fields and (re)setting shaders
    // @mix from 0 to 1, ratio or rate of transition from initial 'settings' to 'other'
    // return interpolated and combined LLSD map
    static LLSD interpolateSDMap(const LLSD &settings, const LLSD &other, const parammapping_t& defaults, BlendFactor mix, const stringset_t& skip, const stringset_t& slerps);
    static LLSD interpolateSDValue(const std::string& name, const LLSD &value, const LLSD &other, const parammapping_t& defaults, BlendFactor mix, const stringset_t& skip, const stringset_t& slerps);

    /// when lerping between settings, some may require special handling.
    /// Get a list of these key to be skipped by the default settings lerp.
    /// (handling should be performed in the override of lerpSettings.
    virtual stringset_t getSkipInterpolateKeys() const;

    // A list of settings that represent quaternions and should be slerped
    // rather than lerped.
    virtual stringset_t getSlerpKeys() const { return stringset_t(); }

    virtual validation_list_t getValidationList() const = 0;

    // Apply settings.
    virtual void applyToUniforms(void *) { };
    virtual void applySpecial(void*, bool force = false) { };

    virtual parammapping_t getParameterMap() const { return parammapping_t(); }

    inline void setBlendFactor(BlendFactor blendfactor)
    {
        mBlendedFactor = blendfactor;
    }

    virtual void replaceWith(const LLSettingsBase::ptr_t other)
    {
        replaceSettings(other);
        setBlendFactor(other->getBlendFactor());
    }

    virtual void loadValuesFromLLSD();
    virtual void saveValuesToLLSD();
    void saveValuesIfNeeded();

    LLUUID mAssetId;
    LLUUID mSettingId;
    std::string mSettingName;
    U32 mSettingFlags;

private:
    bool        mLLSDDirty;
    bool        mDirty;
    bool        mReplaced; // super dirty!

    static LLSD combineSDMaps(const LLSD &first, const LLSD &other);

    LLSD        mSettings;
    BlendFactor mBlendedFactor;
};


class LLSettingsBlender : public PTR_NAMESPACE::enable_shared_from_this<LLSettingsBlender>
{
    LOG_CLASS(LLSettingsBlender);
public:
    typedef PTR_NAMESPACE::shared_ptr<LLSettingsBlender>      ptr_t;
    typedef boost::signals2::signal<void(const ptr_t )> finish_signal_t;
    typedef boost::signals2::connection     connection_t;

    LLSettingsBlender(const LLSettingsBase::ptr_t &target,
            const LLSettingsBase::ptr_t &initsetting, const LLSettingsBase::ptr_t &endsetting) :
        mOnFinished(),
        mTarget(target),
        mInitial(initsetting),
        mFinal(endsetting)
    {
        if (mInitial && mTarget)
            mTarget->replaceSettings(mInitial->getSettings());

        if (!mFinal)
            mFinal = mInitial;
    }

    virtual ~LLSettingsBlender() {}

    virtual void reset( LLSettingsBase::ptr_t &initsetting, const LLSettingsBase::ptr_t &endsetting, const LLSettingsBase::TrackPosition&)
    {
        // note: the 'span' reset parameter is unused by the base class.
        if (!mInitial)
            LL_WARNS("BLENDER") << "Reseting blender with empty initial setting. Expect badness in the future." << LL_ENDL;

        mInitial = initsetting;
        mFinal = endsetting;

        if (!mFinal)
            mFinal = mInitial;

        if (mTarget)
            mTarget->replaceSettings(mInitial->getSettings());
    }

    LLSettingsBase::ptr_t getTarget() const
    {
        return mTarget;
    }

    LLSettingsBase::ptr_t getInitial() const
    {
        return mInitial;
    }

    LLSettingsBase::ptr_t getFinal() const
    {
        return mFinal;
    }

    connection_t            setOnFinished(const finish_signal_t::slot_type &onfinished)
    {
        return mOnFinished.connect(onfinished);
    }

    virtual void            update(const LLSettingsBase::BlendFactor& blendf);
    virtual bool            applyTimeDelta(const LLSettingsBase::Seconds& timedelta)
    {
        llassert(false);
        // your derived class needs to implement an override of this func
        return false;
    }

    virtual F64             setBlendFactor(const LLSettingsBase::BlendFactor& position);

    virtual void            switchTrack(S32 trackno, const LLSettingsBase::TrackPosition& position) { /*NoOp*/ }

protected:
    void                    triggerComplete();

    finish_signal_t         mOnFinished;

    LLSettingsBase::ptr_t   mTarget;
    LLSettingsBase::ptr_t   mInitial;
    LLSettingsBase::ptr_t   mFinal;
};

class LLSettingsBlenderTimeDelta : public LLSettingsBlender
{
protected:
    LOG_CLASS(LLSettingsBlenderTimeDelta);
public:
    static const LLSettingsBase::BlendFactor MIN_BLEND_DELTA;

    LLSettingsBlenderTimeDelta(const LLSettingsBase::ptr_t &target,
        const LLSettingsBase::ptr_t &initsetting, const LLSettingsBase::ptr_t &endsetting, const LLSettingsBase::Seconds& blend_span) :
        LLSettingsBlender(target, initsetting, endsetting),
        mBlendSpan((F32)blend_span.value()),
        mLastUpdate(0.0f),
        mTimeSpent(0.0f),
        mBlendFMinDelta(MIN_BLEND_DELTA),
        mLastBlendF(-1.0f)
    {
        mTimeStart = LLSettingsBase::Seconds(LLDate::now().secondsSinceEpoch());
        mLastUpdate = mTimeStart;
    }

    virtual ~LLSettingsBlenderTimeDelta()
    {
    }

    virtual void reset(LLSettingsBase::ptr_t &initsetting, const LLSettingsBase::ptr_t &endsetting, const LLSettingsBase::TrackPosition& blend_span) SETTINGS_OVERRIDE
    {
        LLSettingsBlender::reset(initsetting, endsetting, blend_span);

        mBlendSpan  = blend_span;
        mTimeStart  = LLSettingsBase::Seconds(LLDate::now().secondsSinceEpoch());
        mLastUpdate = mTimeStart;
        mTimeSpent  = LLSettingsBase::Seconds(0.0f);
        mLastBlendF = LLSettingsBase::BlendFactor(-1.0f);
    }

    virtual bool applyTimeDelta(const LLSettingsBase::Seconds& timedelta) SETTINGS_OVERRIDE;

    inline void setTimeSpent(LLSettingsBase::Seconds time) { mTimeSpent = time; }
protected:
    LLSettingsBase::BlendFactor calculateBlend(const LLSettingsBase::TrackPosition& spanpos, const LLSettingsBase::TrackPosition& spanlen) const;

    LLSettingsBase::TrackPosition mBlendSpan;
    LLSettingsBase::Seconds mLastUpdate;
    LLSettingsBase::Seconds mTimeSpent;
    LLSettingsBase::Seconds mTimeStart;
    LLSettingsBase::BlendFactor mBlendFMinDelta;
    LLSettingsBase::BlendFactor mLastBlendF;
};


#endif
