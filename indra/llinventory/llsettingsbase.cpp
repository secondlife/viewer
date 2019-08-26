/**
* @file llsettingsbase.cpp
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

#include "llsettingsbase.h"

#include "llmath.h"
#include <algorithm>

#include "llsdserialize.h"

//=========================================================================
namespace
{
    const LLSettingsBase::TrackPosition BREAK_POINT = 0.5;
}

const LLSettingsBase::TrackPosition LLSettingsBase::INVALID_TRACKPOS(-1.0);

//=========================================================================
std::ostream &operator <<(std::ostream& os, LLSettingsBase &settings)
{
    LLSDSerialize::serialize(settings.getSettings(), os, LLSDSerialize::LLSD_NOTATION);

    return os;
}

//=========================================================================
const std::string LLSettingsBase::SETTING_ID("id");
const std::string LLSettingsBase::SETTING_NAME("name");
const std::string LLSettingsBase::SETTING_HASH("hash");
const std::string LLSettingsBase::SETTING_TYPE("type");
const std::string LLSettingsBase::SETTING_ASSETID("asset_id");
const std::string LLSettingsBase::SETTING_FLAGS("flags");

const U32 LLSettingsBase::FLAG_NOCOPY(0x01 << 0);
const U32 LLSettingsBase::FLAG_NOMOD(0x01 << 1);
const U32 LLSettingsBase::FLAG_NOTRANS(0x01 << 2);
const U32 LLSettingsBase::FLAG_NOSAVE(0x01 << 3);

const U32 LLSettingsBase::Validator::VALIDATION_PARTIAL(0x01 << 0);

//=========================================================================
LLSettingsBase::LLSettingsBase():
    mSettings(LLSD::emptyMap()),
    mDirty(true),
    mAssetID(),
    mBlendedFactor(0.0)
{
}

LLSettingsBase::LLSettingsBase(const LLSD setting) :
    mSettings(setting),
    mDirty(true),
    mAssetID(),
    mBlendedFactor(0.0)
{
}

//=========================================================================
void LLSettingsBase::lerpSettings(const LLSettingsBase &other, F64 mix) 
{
    mSettings = interpolateSDMap(mSettings, other.mSettings, other.getParameterMap(), mix);
    setDirtyFlag(true);
}

LLSD LLSettingsBase::combineSDMaps(const LLSD &settings, const LLSD &other) const
{
    LLSD newSettings;

    for (LLSD::map_const_iterator it = settings.beginMap(); it != settings.endMap(); ++it)
    {
        std::string key_name = (*it).first;
        LLSD value = (*it).second;

        LLSD::Type setting_type = value.type();
        switch (setting_type)
        {
        case LLSD::TypeMap:
            newSettings[key_name] = combineSDMaps(value, LLSD());
            break;
        case LLSD::TypeArray:
            newSettings[key_name] = LLSD::emptyArray();
            for (LLSD::array_const_iterator ita = value.beginArray(); ita != value.endArray(); ++ita)
            {
                newSettings[key_name].append(*ita);
            }
            break;
        //case LLSD::TypeInteger:
        //case LLSD::TypeReal:
        //case LLSD::TypeBoolean:
        //case LLSD::TypeString:
        //case LLSD::TypeUUID:
        //case LLSD::TypeURI:
        //case LLSD::TypeDate:
        //case LLSD::TypeBinary:
        default:
            newSettings[key_name] = value;
            break;
        }
    }

    if (!other.isUndefined())
    {
        for (LLSD::map_const_iterator it = other.beginMap(); it != other.endMap(); ++it)
        {
            std::string key_name = (*it).first;
            LLSD value = (*it).second;

            LLSD::Type setting_type = value.type();
            switch (setting_type)
            {
            case LLSD::TypeMap:
                newSettings[key_name] = combineSDMaps(value, LLSD());
                break;
            case LLSD::TypeArray:
                newSettings[key_name] = LLSD::emptyArray();
                for (LLSD::array_const_iterator ita = value.beginArray(); ita != value.endArray(); ++ita)
                {
                    newSettings[key_name].append(*ita);
                }
                break;
            //case LLSD::TypeInteger:
            //case LLSD::TypeReal:
            //case LLSD::TypeBoolean:
            //case LLSD::TypeString:
            //case LLSD::TypeUUID:
            //case LLSD::TypeURI:
            //case LLSD::TypeDate:
            //case LLSD::TypeBinary:
            default:
                newSettings[key_name] = value;
                break;
            }
        }
    }

    return newSettings;
}

LLSD LLSettingsBase::interpolateSDMap(const LLSD &settings, const LLSD &other, const parammapping_t& defaults, F64 mix) const
{
    LLSD newSettings;

    stringset_t skip = getSkipInterpolateKeys();
    stringset_t slerps = getSlerpKeys();

    llassert(mix >= 0.0f && mix <= 1.0f);

    for (LLSD::map_const_iterator it = settings.beginMap(); it != settings.endMap(); ++it)
    {
        std::string key_name = (*it).first;
        LLSD value = (*it).second;

        if (skip.find(key_name) != skip.end())
            continue;

        LLSD other_value;
        if (other.has(key_name))
        {
            other_value = other[key_name];
        }
        else
        {
            parammapping_t::const_iterator def_iter = defaults.find(key_name);
            if (def_iter != defaults.end())
            {
                other_value = def_iter->second.getDefaultValue();
            }
            else if (value.type() == LLSD::TypeMap)
            {
                // interpolate in case there are defaults inside (part of legacy)
                other_value = LLSDMap();
            }
            else
            {
                // The other or defaults does not contain this setting, keep the original value 
                // TODO: Should I blend this out instead?
                newSettings[key_name] = value;
                continue;
            }
        }

        newSettings[key_name] = interpolateSDValue(key_name, value, other_value, defaults, mix, slerps);
    }

    // Special handling cases
    // Flags
    if (settings.has(SETTING_FLAGS))
    {
        U32 flags = (U32)settings[SETTING_FLAGS].asInteger();
        if (other.has(SETTING_FLAGS))
            flags |= (U32)other[SETTING_FLAGS].asInteger();

        newSettings[SETTING_FLAGS] = LLSD::Integer(flags);
    }

    // Now add anything that is in other but not in the settings
    for (LLSD::map_const_iterator it = other.beginMap(); it != other.endMap(); ++it)
    {
        std::string key_name = (*it).first;

        if (skip.find(key_name) != skip.end())
            continue;

        if (settings.has(key_name))
            continue;

        parammapping_t::const_iterator def_iter = defaults.find(key_name);
        if (def_iter != defaults.end())
        {
            // Blend against default value
            newSettings[key_name] = interpolateSDValue(key_name, def_iter->second.getDefaultValue(), (*it).second, defaults, mix, slerps);
        }
        else if ((*it).second.type() == LLSD::TypeMap)
        {
            // interpolate in case there are defaults inside (part of legacy)
            newSettings[key_name] = interpolateSDValue(key_name, LLSDMap(), (*it).second, defaults, mix, slerps);
        }
        // else do nothing when no known defaults
        // TODO: Should I blend this out instead?
    }

    // Note: writes variables from skip list, bug?
    for (LLSD::map_const_iterator it = other.beginMap(); it != other.endMap(); ++it)
    {
        // TODO: Should I blend this in instead?
        if (skip.find((*it).first) == skip.end())
            continue;

        if (!settings.has((*it).first))
            continue;
    
        newSettings[(*it).first] = (*it).second;
    }

    return newSettings;
}

LLSD LLSettingsBase::interpolateSDValue(const std::string& key_name, const LLSD &value, const LLSD &other_value, const parammapping_t& defaults, BlendFactor mix, const stringset_t& slerps) const
{
    LLSD new_value;

    LLSD::Type setting_type = value.type();

    if (other_value.type() != setting_type)
    {
        // The data type mismatched between this and other. Hard switch when we pass the break point
        // but issue a warning.
        LL_WARNS("SETTINGS") << "Setting lerp between mismatched types for '" << key_name << "'." << LL_ENDL;
        new_value = (mix > BREAK_POINT) ? other_value : value;
    }

    switch (setting_type)
    {
        case LLSD::TypeInteger:
            // lerp between the two values rounding the result to the nearest integer. 
            new_value = LLSD::Integer(llroundf(lerp(value.asReal(), other_value.asReal(), mix)));
            break;
        case LLSD::TypeReal:
            // lerp between the two values.
            new_value = LLSD::Real(lerp(value.asReal(), other_value.asReal(), mix));
            break;
        case LLSD::TypeMap:
            // deep copy.
            new_value = interpolateSDMap(value, other_value, defaults, mix);
            break;

        case LLSD::TypeArray:
        {
            LLSD new_array(LLSD::emptyArray());

            if (slerps.find(key_name) != slerps.end())
            {
                LLQuaternion a(value);
                LLQuaternion b(other_value);
                LLQuaternion q = slerp(mix, a, b);
                new_array = q.getValue();
            }
            else
            {   // TODO: We could expand this to inspect the type and do a deep lerp based on type. 
                // for now assume a heterogeneous array of reals. 
                size_t len = std::max(value.size(), other_value.size());

                for (size_t i = 0; i < len; ++i)
                {

                    new_array[i] = lerp(value[i].asReal(), other_value[i].asReal(), mix);
                }
            }

            new_value = new_array;
        }

        break;

        case LLSD::TypeUUID:
            new_value = value.asUUID();
            break;

            //      case LLSD::TypeBoolean:
            //      case LLSD::TypeString:
            //      case LLSD::TypeURI:
            //      case LLSD::TypeBinary:
            //      case LLSD::TypeDate:
        default:
            // atomic or unknown data types. Lerping between them does not make sense so switch at the break.
            new_value = (mix > BREAK_POINT) ? other_value : value;
            break;
    }

    return new_value;
}

LLSettingsBase::stringset_t LLSettingsBase::getSkipInterpolateKeys() const
{
    static stringset_t skipSet;

    if (skipSet.empty())
    {
        skipSet.insert(SETTING_FLAGS);
        skipSet.insert(SETTING_HASH);
    }

    return skipSet;
}

LLSD LLSettingsBase::getSettings() const
{
    return mSettings;
}

LLSD LLSettingsBase::cloneSettings() const
{
    U32 flags = getFlags();
    LLSD settings (combineSDMaps(getSettings(), LLSD()));
    if (flags)
        settings[SETTING_FLAGS] = LLSD::Integer(flags);
    return settings;
}

size_t LLSettingsBase::getHash() const
{   // get a shallow copy of the LLSD filtering out values to not include in the hash
    LLSD hash_settings = llsd_shallow(getSettings(), 
        LLSDMap(SETTING_NAME, false)(SETTING_ID, false)(SETTING_HASH, false)("*", true));

    boost::hash<LLSD> hasher;
    return hasher(hash_settings);
}

bool LLSettingsBase::validate()
{
    validation_list_t validations = getValidationList();

    if (!mSettings.has(SETTING_TYPE))
    {
        mSettings[SETTING_TYPE] = getSettingsType();
    }

    LLSD result = LLSettingsBase::settingValidation(mSettings, validations);

    if (result["errors"].size() > 0)
    {
        LL_WARNS("SETTINGS") << "Validation errors: " << result["errors"] << LL_ENDL;
    }
    if (result["warnings"].size() > 0)
    {
        LL_DEBUGS("SETTINGS") << "Validation warnings: " << result["warnings"] << LL_ENDL;
    }

    return result["success"].asBoolean();
}

LLSD LLSettingsBase::settingValidation(LLSD &settings, validation_list_t &validations, bool partial)
{
    static Validator  validateName(SETTING_NAME, false, LLSD::TypeString, boost::bind(&Validator::verifyStringLength, _1, 63));
    static Validator  validateId(SETTING_ID, false, LLSD::TypeUUID);
    static Validator  validateHash(SETTING_HASH, false, LLSD::TypeInteger);
    static Validator  validateType(SETTING_TYPE, false, LLSD::TypeString);
    static Validator  validateAssetId(SETTING_ASSETID, false, LLSD::TypeUUID);
    static Validator  validateFlags(SETTING_FLAGS, false, LLSD::TypeInteger);
    stringset_t       validated;
    stringset_t       strip;
    bool              isValid(true);
    LLSD              errors(LLSD::emptyArray());
    LLSD              warnings(LLSD::emptyArray());
    U32               flags(0);
    
    if (partial)
        flags |= Validator::VALIDATION_PARTIAL;

    // Fields common to all settings.
    if (!validateName.verify(settings, flags))
    {
        errors.append( LLSD::String("Unable to validate 'name'.") );
        isValid = false;
    }
    validated.insert(validateName.getName());

    if (!validateId.verify(settings, flags))
    {
        errors.append( LLSD::String("Unable to validate 'id'.") );
        isValid = false;
    }
    validated.insert(validateId.getName());

    if (!validateHash.verify(settings, flags))
    {
        errors.append( LLSD::String("Unable to validate 'hash'.") );
        isValid = false;
    }
    validated.insert(validateHash.getName());

    if (!validateAssetId.verify(settings, flags))
    {
        errors.append(LLSD::String("Invalid asset Id"));
        isValid = false;
    }
    validated.insert(validateAssetId.getName());

    if (!validateType.verify(settings, flags))
    {
        errors.append( LLSD::String("Unable to validate 'type'.") );
        isValid = false;
    }
    validated.insert(validateType.getName());

    if (!validateFlags.verify(settings, flags))
    {
        errors.append(LLSD::String("Unable to validate 'flags'."));
        isValid = false;
    }
    validated.insert(validateFlags.getName());

    // Fields for specific settings.
    for (validation_list_t::iterator itv = validations.begin(); itv != validations.end(); ++itv)
    {
#ifdef VALIDATION_DEBUG
        LLSD oldvalue;
        if (settings.has((*itv).getName()))
        {
            oldvalue = llsd_clone(mSettings[(*itv).getName()]);
        }
#endif

        if (!(*itv).verify(settings, flags))
        {
            std::stringstream errtext;

            errtext << "Settings LLSD fails validation and could not be corrected for '" << (*itv).getName() << "'!\n";
            errors.append( errtext.str() );
            isValid = false;
        }
        validated.insert((*itv).getName());

#ifdef VALIDATION_DEBUG
        if (!oldvalue.isUndefined())
        {
            if (!compare_llsd(settings[(*itv).getName()], oldvalue))
            {
                LL_WARNS("SETTINGS") << "Setting '" << (*itv).getName() << "' was changed: " << oldvalue << " -> " << settings[(*itv).getName()] << LL_ENDL;
            }
        }
#endif
    }

    // strip extra entries
    for (LLSD::map_const_iterator itm = settings.beginMap(); itm != settings.endMap(); ++itm)
    {
        if (validated.find((*itm).first) == validated.end())
        {
            std::stringstream warntext;

            warntext << "Stripping setting '" << (*itm).first << "'";
            warnings.append( warntext.str() );
            strip.insert((*itm).first);
        }
    }

    for (stringset_t::iterator its = strip.begin(); its != strip.end(); ++its)
    {
        settings.erase(*its);
    }

    return LLSDMap("success", LLSD::Boolean(isValid))
        ("errors", errors)
        ("warnings", warnings);
}

//=========================================================================

bool LLSettingsBase::Validator::verify(LLSD &data, U32 flags)
{
    if (!data.has(mName) || (data.has(mName) && data[mName].isUndefined()))
    {
        if ((flags & VALIDATION_PARTIAL) != 0) // we are doing a partial validation.  Do no attempt to set a default if missing (or fail even if required)
            return true;    
        
        if (!mDefault.isUndefined())
        {
            data[mName] = mDefault;
            return true;
        }
        if (mRequired)
            LL_WARNS("SETTINGS") << "Missing required setting '" << mName << "' with no default." << LL_ENDL;
        return !mRequired;
    }

    if (data[mName].type() != mType)
    {
        LL_WARNS("SETTINGS") << "Setting '" << mName << "' is incorrect type." << LL_ENDL;
        return false;
    }

    if (!mVerify.empty() && !mVerify(data[mName]))
    {
        LL_WARNS("SETTINGS") << "Setting '" << mName << "' fails validation." << LL_ENDL;
        return false;
    }

    return true;
}

bool LLSettingsBase::Validator::verifyColor(LLSD &value)
{
    return (value.size() == 3 || value.size() == 4);
}

bool LLSettingsBase::Validator::verifyVector(LLSD &value, S32 length)
{
    return (value.size() == length);
}

bool LLSettingsBase::Validator::verifyVectorNormalized(LLSD &value, S32 length)
{
    if (value.size() != length)
        return false;

    LLSD newvector;

    switch (length)
    {
    case 2:
    {
        LLVector2 vect(value);

        if (is_approx_equal(vect.normalize(), 1.0f))
            return true;
        newvector = vect.getValue();
        break;
    }
    case 3:
    {
        LLVector3 vect(value);

        if (is_approx_equal(vect.normalize(), 1.0f))
            return true;
        newvector = vect.getValue();
        break;
    }
    case 4:
    {
        LLVector4 vect(value);

        if (is_approx_equal(vect.normalize(), 1.0f))
            return true;
        newvector = vect.getValue();
        break;
    }
    default:
        return false;
    }

    return true;
}

bool LLSettingsBase::Validator::verifyVectorMinMax(LLSD &value, LLSD minvals, LLSD maxvals)
{
    for (S32 index = 0; index < value.size(); ++index)
    {
        if (minvals[index].asString() != "*")
        {
            if (minvals[index].asReal() > value[index].asReal())
            {
                value[index] = minvals[index].asReal();
            }
        }
        if (maxvals[index].asString() != "*") 
        {
            if (maxvals[index].asReal() < value[index].asReal())
            {
                value[index] = maxvals[index].asReal();
            }
        }
    }

    return true;
}

bool LLSettingsBase::Validator::verifyQuaternion(LLSD &value)
{
    return (value.size() == 4);
}

bool LLSettingsBase::Validator::verifyQuaternionNormal(LLSD &value)
{
    if (value.size() != 4)
        return false;

    LLQuaternion quat(value);

    if (is_approx_equal(quat.normalize(), 1.0f))
        return true;

    LLSD newquat = quat.getValue();
    for (S32 index = 0; index < 4; ++index)
    {
        value[index] = newquat[index];
    }
    return true;
}

bool LLSettingsBase::Validator::verifyFloatRange(LLSD &value, LLSD range)
{
    F64 real = value.asReal();

    F64 clampedval = llclamp(LLSD::Real(real), range[0].asReal(), range[1].asReal());

    if (is_approx_equal(clampedval, real))
        return true;

    value = LLSD::Real(clampedval);
    return true;
}

bool LLSettingsBase::Validator::verifyIntegerRange(LLSD &value, LLSD range)
{
    S32 ival = value.asInteger();

    S32 clampedval = llclamp(LLSD::Integer(ival), range[0].asInteger(), range[1].asInteger());

    if (clampedval == ival)
        return true;

    value = LLSD::Integer(clampedval);
    return true;
}

bool LLSettingsBase::Validator::verifyStringLength(LLSD &value, S32 length)
{
    std::string sval = value.asString();

    if (!sval.empty())
    {
        sval = sval.substr(0, length);
        value = LLSD::String(sval);
    }
    return true;
}

//=========================================================================
void LLSettingsBlender::update(const LLSettingsBase::BlendFactor& blendf)
{
    F64 res = setBlendFactor(blendf);
    llassert(res >= 0.0 && res <= 1.0);
    (void)res;
    mTarget->update();
}

F64 LLSettingsBlender::setBlendFactor(const LLSettingsBase::BlendFactor& blendf_in)
{
    LLSettingsBase::TrackPosition blendf = blendf_in;
    if (blendf >= 1.0)
    {
        triggerComplete();
    }
    blendf = llclamp(blendf, 0.0f, 1.0f);

    if (mTarget)
    {
        mTarget->replaceSettings(mInitial->getSettings());
        mTarget->blend(mFinal, blendf);
    }
    else
    {
        LL_WARNS("SETTINGS") << "No target for settings blender." << LL_ENDL;
    }

    return blendf;
}

void LLSettingsBlender::triggerComplete()
{
    if (mTarget)
        mTarget->replaceSettings(mFinal->getSettings());
    LLSettingsBlender::ptr_t hold = shared_from_this();   // prevents this from deleting too soon
    mTarget->update();
    mOnFinished(shared_from_this());
}

//-------------------------------------------------------------------------
const LLSettingsBase::BlendFactor LLSettingsBlenderTimeDelta::MIN_BLEND_DELTA(FLT_EPSILON);

LLSettingsBase::BlendFactor LLSettingsBlenderTimeDelta::calculateBlend(const LLSettingsBase::TrackPosition& spanpos, const LLSettingsBase::TrackPosition& spanlen) const
{
    return LLSettingsBase::BlendFactor(fmod((F64)spanpos, (F64)spanlen) / (F64)spanlen);
}

bool LLSettingsBlenderTimeDelta::applyTimeDelta(const LLSettingsBase::Seconds& timedelta)
{
    mTimeSpent += timedelta;

    if (mTimeSpent > mBlendSpan)
    {
        triggerComplete();
        return false;
    }

    LLSettingsBase::BlendFactor blendf = calculateBlend(mTimeSpent, mBlendSpan);

    if (fabs(mLastBlendF - blendf) < mBlendFMinDelta)
    {
        return false;
    }

    mLastBlendF = blendf;
    update(blendf);
    return true;
}
