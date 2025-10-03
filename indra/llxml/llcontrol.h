/**
 * @file llcontrol.h
 * @brief A mechanism for storing "control state" for a program
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLCONTROL_H
#define LL_LLCONTROL_H

#include "llboost.h"
#include "llevent.h"
#include "llstring.h"
#include "llrect.h"
#include "llrefcount.h"
#include "llinstancetracker.h"

#include <vector>

#include <boost/bind.hpp>
#include <boost/signals2.hpp>

class LLVector3;
class LLVector3d;
class LLQuaternion;
class LLColor4;
class LLColor3;

// if this is changed, also modify mTypeString in llcontrol.h
typedef enum e_control_type
{
    TYPE_U32 = 0,
    TYPE_S32,
    TYPE_F32,
    TYPE_BOOLEAN,
    TYPE_STRING,
    TYPE_VEC3,
    TYPE_VEC3D,
    TYPE_QUAT,
    TYPE_RECT,
    TYPE_COL4,
    TYPE_COL3,
    TYPE_LLSD,
    TYPE_COUNT
} eControlType;

class LLControlVariable : public LLRefCount
{
    LOG_CLASS(LLControlVariable);

    friend class LLControlGroup;

public:
    typedef boost::signals2::signal<bool(LLControlVariable* control, const LLSD&), boost_boolean_combiner> validate_signal_t;
    typedef boost::signals2::signal<void(LLControlVariable* control, const LLSD&, const LLSD&)> commit_signal_t;

    enum ePersist
    {
        PERSIST_NO,                 // don't save this var
        PERSIST_NONDFT,             // save this var if differs from default
        PERSIST_ALWAYS              // save this var even if has default value
    };

private:
    std::string     mName;
    std::string     mComment;
    eControlType    mType;
    ePersist        mPersist;
    bool            mHideFromSettingsEditor;
    std::vector<LLSD> mValues;

    commit_signal_t mCommitSignal;
    validate_signal_t mValidateSignal;

public:
    LLControlVariable(const std::string& name, eControlType type,
                      LLSD initial, const std::string& comment,
                      ePersist persist = PERSIST_NONDFT, bool hidefromsettingseditor = false);

    virtual ~LLControlVariable();

    const std::string& getName() const { return mName; }
    const std::string& getComment() const { return mComment; }

    eControlType type()     { return mType; }
    bool isType(eControlType tp) { return tp == mType; }

    void resetToDefault(bool fire_signal = false);

    commit_signal_t* getSignal() { return &mCommitSignal; } // shorthand for commit signal
    commit_signal_t* getCommitSignal() { return &mCommitSignal; }
    validate_signal_t* getValidateSignal() { return &mValidateSignal; }

    bool isDefault() { return (mValues.size() == 1); }
    bool shouldSave(bool nondefault_only);
    bool isPersisted() { return mPersist != PERSIST_NO; }
    bool isHiddenFromSettingsEditor() { return mHideFromSettingsEditor; }
    LLSD get()          const   { return getValue(); }
    LLSD getValue()     const   { return mValues.back(); }
    LLSD getDefault()   const   { return mValues.front(); }
    LLSD getSaveValue() const;

    void set(const LLSD& val)   { setValue(val); }
    void setValue(const LLSD& value, bool saved_value = true);
    void setDefaultValue(const LLSD& value);
    void setPersist(ePersist);
    void setHiddenFromSettingsEditor(bool hide);
    void setComment(const std::string& comment);

private:
    void firePropertyChanged(const LLSD &pPreviousValue)
    {
        mCommitSignal(this, mValues.back(), pPreviousValue);
    }
    LLSD getComparableValue(const LLSD& value);
    bool llsd_compare(const LLSD& a, const LLSD & b);
};

typedef LLPointer<LLControlVariable> LLControlVariablePtr;

//! Helper functions for converting between static types and LLControl values
template <class T>
eControlType get_control_type()
{
    LL_WARNS() << "Usupported control type: " << typeid(T).name() << "." << LL_ENDL;
    return TYPE_COUNT;
}

template <class T>
LLSD convert_to_llsd(const T& in)
{
    // default implementation
    return LLSD(in);
}

template <class T>
T convert_from_llsd(const LLSD& sd, eControlType type, std::string_view control_name)
{
    // needs specialization
    return T(sd);
}

//const U32 STRING_CACHE_SIZE = 10000;
class LLControlGroup : public LLInstanceTracker<LLControlGroup, std::string>
{
    LOG_CLASS(LLControlGroup);

protected:
    typedef std::map<std::string, LLControlVariablePtr, std::less<> > ctrl_name_table_t;
    ctrl_name_table_t mNameTable;
    static const std::string mTypeString[TYPE_COUNT];

public:
    static eControlType typeStringToEnum(const std::string& typestr);
    static std::string typeEnumToString(eControlType typeenum);

    LLControlGroup(const std::string& name);
    ~LLControlGroup();
    void cleanup();

    LLControlVariablePtr getControl(std::string_view name);

    struct ApplyFunctor
    {
        virtual ~ApplyFunctor() {};
        virtual void apply(const std::string& name, LLControlVariable* control) = 0;
    };
    void applyToAll(ApplyFunctor* func);

    LLControlVariable* declareControl(const std::string& name, eControlType type, const LLSD initial_val, const std::string& comment, LLControlVariable::ePersist persist, bool hidefromsettingseditor = false);
    LLControlVariable* declareU32(const std::string& name, U32 initial_val, const std::string& comment, LLControlVariable::ePersist persist = LLControlVariable::PERSIST_NONDFT);
    LLControlVariable* declareS32(const std::string& name, S32 initial_val, const std::string& comment, LLControlVariable::ePersist persist = LLControlVariable::PERSIST_NONDFT);
    LLControlVariable* declareF32(const std::string& name, F32 initial_val, const std::string& comment, LLControlVariable::ePersist persist = LLControlVariable::PERSIST_NONDFT);
    LLControlVariable* declareBOOL(const std::string& name, bool initial_val, const std::string& comment, LLControlVariable::ePersist persist = LLControlVariable::PERSIST_NONDFT);
    LLControlVariable* declareString(const std::string& name, const std::string &initial_val, const std::string& comment, LLControlVariable::ePersist persist = LLControlVariable::PERSIST_NONDFT);
    LLControlVariable* declareVec3(const std::string& name, const LLVector3 &initial_val,const std::string& comment,  LLControlVariable::ePersist persist = LLControlVariable::PERSIST_NONDFT);
    LLControlVariable* declareVec3d(const std::string& name, const LLVector3d &initial_val, const std::string& comment, LLControlVariable::ePersist persist = LLControlVariable::PERSIST_NONDFT);
    LLControlVariable* declareQuat(const std::string& name, const LLQuaternion &initial_val, const std::string& comment, LLControlVariable::ePersist persist = LLControlVariable::PERSIST_NONDFT);
    LLControlVariable* declareRect(const std::string& name, const LLRect &initial_val, const std::string& comment, LLControlVariable::ePersist persist = LLControlVariable::PERSIST_NONDFT);
    LLControlVariable* declareColor4(const std::string& name, const LLColor4 &initial_val, const std::string& comment, LLControlVariable::ePersist persist = LLControlVariable::PERSIST_NONDFT);
    LLControlVariable* declareColor3(const std::string& name, const LLColor3 &initial_val, const std::string& comment, LLControlVariable::ePersist persist = LLControlVariable::PERSIST_NONDFT);
    LLControlVariable* declareLLSD(const std::string& name, const LLSD &initial_val, const std::string& comment, LLControlVariable::ePersist persist = LLControlVariable::PERSIST_NONDFT);

    std::string getString(std::string_view name);
    std::string getText(std::string_view name);
    bool        getBOOL(std::string_view name);
    S32         getS32(std::string_view name);
    F32         getF32(std::string_view name);
    U32         getU32(std::string_view name);

    LLWString   getWString(std::string_view name);
    LLVector3   getVector3(std::string_view name);
    LLVector3d  getVector3d(std::string_view name);
    LLRect      getRect(std::string_view name);
    LLSD        getLLSD(std::string_view name);
    LLQuaternion    getQuaternion(std::string_view name);

    LLColor4    getColor(std::string_view name);
    LLColor4    getColor4(std::string_view name);
    LLColor3    getColor3(std::string_view name);

    LLSD        asLLSD(bool diffs_only);

    // generic getter
    template<typename T> T get(std::string_view name)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_LLSD;
        LLControlVariable* control = getControl(name);
        LLSD value;
        eControlType type = TYPE_COUNT;

        if (control)
        {
            value = control->get();
            type = control->type();
        }
        else
        {
            LL_WARNS() << "Control " << name << " not found." << LL_ENDL;
            return T();
        }
        return convert_from_llsd<T>(value, type, name);
    }

    void    setBOOL(std::string_view name, bool val);
    void    setS32(std::string_view name, S32 val);
    void    setF32(std::string_view name, F32 val);
    void    setU32(std::string_view name, U32 val);
    void    setString(std::string_view  name, const std::string& val);
    void    setVector3(std::string_view name, const LLVector3 &val);
    void    setVector3d(std::string_view name, const LLVector3d &val);
    void    setQuaternion(std::string_view name, const LLQuaternion &val);
    void    setRect(std::string_view name, const LLRect &val);
    void    setColor4(std::string_view name, const LLColor4 &val);
    void    setLLSD(std::string_view name, const LLSD& val);

    // type agnostic setter that takes LLSD
    void    setUntypedValue(std::string_view name, const LLSD& val);

    // generic setter
    template<typename T> void set(std::string_view name, const T& val)
    {
        LLControlVariable* control = getControl(name);

        if (control && control->isType(get_control_type<T>()))
        {
            control->set(convert_to_llsd(val));
        }
        else
        {
            LL_WARNS() << "Invalid control " << name << LL_ENDL;
        }
    }

    bool    controlExists(std::string_view name);

    // Returns number of controls loaded, 0 if failed
    // If require_declaration is false, will auto-declare controls it finds
    // as the given type.
    U32 loadFromFileLegacy(const std::string& filename, bool require_declaration = true, eControlType declare_as = TYPE_STRING);
    U32 saveToFile(const std::string& filename, bool nondefault_only);
    U32 loadFromFile(const std::string& filename, bool default_values = false, bool save_values = true);
    void    resetToDefaults();
    void    incrCount(std::string_view name);

    bool    mSettingsProfile;
};


//! Publish/Subscribe object to interact with LLControlGroups.

//! Use an LLCachedControl instance to connect to a LLControlVariable
//! without have to manually create and bind a listener to a local
//! object.
template <class T>
class LLControlCache : public LLRefCount, public LLInstanceTracker<LLControlCache<T>, std::string>
{
public:
    // This constructor will declare a control if it doesn't exist in the contol group
    LLControlCache(LLControlGroup& group,
                    const std::string& name,
                    const T& default_value,
                    const std::string& comment)
    :   LLInstanceTracker<LLControlCache<T>, std::string >(name)
    {
        if(!group.controlExists(name))
        {
            if(!declareTypedControl(group, name, default_value, comment))
            {
                LL_ERRS() << "The control could not be created!!!" << LL_ENDL;
            }
        }

        bindToControl(group, name);
    }

    LLControlCache(LLControlGroup& group,
                    const std::string& name)
    :   LLInstanceTracker<LLControlCache<T>, std::string >(name)
    {
        if(!group.controlExists(name))
        {
            LL_ERRS() << "Control named " << name << "not found." << LL_ENDL;
        }

        bindToControl(group, name);
    }

    ~LLControlCache()
    {
    }

    const T& getValue() const { return mCachedValue; }

private:
    void bindToControl(LLControlGroup& group, const std::string& name)
    {
        LLControlVariablePtr controlp = group.getControl(name);
        mType = controlp->type();
        mCachedValue = convert_from_llsd<T>(controlp->get(), mType, name);

        // Add a listener to the controls signal...
        // NOTE: All listeners connected to 0 group, for guaranty that variable handlers (gSavedSettings) call last
        mConnection = controlp->getSignal()->connect(0,
            boost::bind(&LLControlCache<T>::handleValueChange, this, _2)
            );
        mType = controlp->type();
    }
    bool declareTypedControl(LLControlGroup& group,
                            const std::string& name,
                             const T& default_value,
                             const std::string& comment)
    {
        LLSD init_value;
        eControlType type = get_control_type<T>();
        init_value = convert_to_llsd(default_value);
        if(type < TYPE_COUNT)
        {
            group.declareControl(name, type, init_value, comment, LLControlVariable::PERSIST_NO);
            return true;
        }
        return false;
    }

    bool handleValueChange(const LLSD& newvalue)
    {
        mCachedValue = convert_from_llsd<T>(newvalue, mType, "");
        return true;
    }

private:
    T                           mCachedValue;
    eControlType                mType;
    boost::signals2::scoped_connection  mConnection;
};

template <typename T>
class LLCachedControl
{
public:
    LLCachedControl(LLControlGroup& group,
                    const std::string& name,
                    const T& default_value,
                    const std::string& comment = "Declared In Code")
    {
        mCachedControlPtr = LLControlCache<T>::getInstance(name).get();
        if (! mCachedControlPtr)
        {
            mCachedControlPtr = new LLControlCache<T>(group, name, default_value, comment);
        }
    }

    LLCachedControl(LLControlGroup& group,
                    const std::string& name)
    {
        mCachedControlPtr = LLControlCache<T>::getInstance(name).get();
        if (! mCachedControlPtr)
        {
            mCachedControlPtr = new LLControlCache<T>(group, name);
        }
    }

    operator const T&() const { return mCachedControlPtr->getValue(); }
    operator boost::function<const T&()> () const { return boost::function<const T&()>(*this); }
    const T& operator()() { return mCachedControlPtr->getValue(); }

private:
    LLPointer<LLControlCache<T> > mCachedControlPtr;
};

template <> eControlType get_control_type<U32>();
template <> eControlType get_control_type<S32>();
template <> eControlType get_control_type<F32>();
template <> eControlType get_control_type<bool>();
template <> eControlType get_control_type<std::string>();
template <> eControlType get_control_type<LLVector3>();
template <> eControlType get_control_type<LLVector3d>();
template <> eControlType get_control_type<LLQuaternion>();
template <> eControlType get_control_type<LLRect>();
template <> eControlType get_control_type<LLColor4>();
template <> eControlType get_control_type<LLColor3>();
template <> eControlType get_control_type<LLSD>();

template <> LLSD convert_to_llsd<U32>(const U32& in);
template <> LLSD convert_to_llsd<LLVector3>(const LLVector3& in);
template <> LLSD convert_to_llsd<LLVector3d>(const LLVector3d& in);
template <> LLSD convert_to_llsd<LLQuaternion>(const LLQuaternion& in);
template <> LLSD convert_to_llsd<LLRect>(const LLRect& in);
template <> LLSD convert_to_llsd<LLColor4>(const LLColor4& in);
template <> LLSD convert_to_llsd<LLColor3>(const LLColor3& in);

template<> std::string convert_from_llsd<std::string>(const LLSD& sd, eControlType type, std::string_view control_name);
template<> LLWString convert_from_llsd<LLWString>(const LLSD& sd, eControlType type, std::string_view control_name);
template<> LLVector3 convert_from_llsd<LLVector3>(const LLSD& sd, eControlType type, std::string_view control_name);
template<> LLVector3d convert_from_llsd<LLVector3d>(const LLSD& sd, eControlType type, std::string_view control_name);
template<> LLQuaternion convert_from_llsd<LLQuaternion>(const LLSD& sd, eControlType type, std::string_view control_name);
template<> LLRect convert_from_llsd<LLRect>(const LLSD& sd, eControlType type, std::string_view control_name);
template<> bool convert_from_llsd<bool>(const LLSD& sd, eControlType type, std::string_view control_name);
template<> S32 convert_from_llsd<S32>(const LLSD& sd, eControlType type, std::string_view control_name);
template<> F32 convert_from_llsd<F32>(const LLSD& sd, eControlType type, std::string_view control_name);
template<> U32 convert_from_llsd<U32>(const LLSD& sd, eControlType type, std::string_view control_name);
template<> LLColor3 convert_from_llsd<LLColor3>(const LLSD& sd, eControlType type, std::string_view control_name);
template<> LLColor4 convert_from_llsd<LLColor4>(const LLSD& sd, eControlType type, std::string_view control_name);
template<> LLSD convert_from_llsd<LLSD>(const LLSD& sd, eControlType type, std::string_view control_name);

//#define TEST_CACHED_CONTROL 1
#ifdef TEST_CACHED_CONTROL
void test_cached_control();
#endif // TEST_CACHED_CONTROL

#endif
