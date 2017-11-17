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

#include "llviewerprecompiledheaders.h"
#include "llsettingsbase.h"

#include "llmath.h"
#include <algorithm>

#include "llsdserialize.h"

//=========================================================================
namespace
{
    const F32 BREAK_POINT = 0.5;
}

//=========================================================================
const std::string LLSettingsBase::SETTING_ID("id");
const std::string LLSettingsBase::SETTING_NAME("name");

const F32Seconds LLSettingsBlender::DEFAULT_THRESHOLD(0.01);

//=========================================================================
LLSettingsBase::LLSettingsBase():
    mSettings(LLSD::emptyMap()),
    mDirty(true)
{
}

LLSettingsBase::LLSettingsBase(const LLSD setting) :
    mSettings(setting),
    mDirty(true)
{
}

//=========================================================================
void LLSettingsBase::lerpSettings(const LLSettingsBase &other, F32 mix) 
{
    mSettings = interpolateSDMap(mSettings, other.mSettings, mix);
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

LLSD LLSettingsBase::interpolateSDMap(const LLSD &settings, const LLSD &other, F32 mix) const
{
    LLSD newSettings;

    stringset_t skip = getSkipInterpolateKeys();
    stringset_t slerps = getSlerpKeys();

    for (LLSD::map_const_iterator it = settings.beginMap(); it != settings.endMap(); ++it)
    {
        std::string key_name = (*it).first;
        LLSD value = (*it).second;

        if (skip.find(key_name) != skip.end())
            continue;

        if (!other.has(key_name))
        {   // The other does not contain this setting, keep the original value 
            // TODO: Should I blend this out instead?
            newSettings[key_name] = value;
            continue;
        }
        LLSD::Type setting_type = value.type();
        LLSD other_value = other[key_name];
        
        if (other_value.type() != setting_type)
        {   
            // The data type mismatched between this and other. Hard switch when we pass the break point
            // but issue a warning.
            LL_WARNS("SETTINGS") << "Setting lerp between mismatched types for '" << key_name << "'." << LL_ENDL;
            newSettings[key_name] = (mix > BREAK_POINT) ? other_value : value;
            continue;
        }

        switch (setting_type)
        {
        case LLSD::TypeInteger:
            // lerp between the two values rounding the result to the nearest integer. 
            newSettings[key_name] = LLSD::Integer(llroundf(lerp(value.asReal(), other_value.asReal(), mix)));
            break;
        case LLSD::TypeReal:
            // lerp between the two values.
            newSettings[key_name] = LLSD::Real(lerp(value.asReal(), other_value.asReal(), mix));
            break;
        case LLSD::TypeMap:
            // deep copy.
            newSettings[key_name] = interpolateSDMap(value, other_value, mix);
            break;

        case LLSD::TypeArray:
            {
                LLSD newvalue(LLSD::emptyArray());

                if (slerps.find(key_name) != slerps.end())
                {
                    LLQuaternion q = slerp(mix, LLQuaternion(value), LLQuaternion(other_value));
                    newvalue = q.getValue();
                }
                else
                {   // TODO: We could expand this to inspect the type and do a deep lerp based on type. 
                    // for now assume a heterogeneous array of reals. 
                    size_t len = std::max(value.size(), other_value.size());

                    for (size_t i = 0; i < len; ++i)
                    {

                        newvalue[i] = lerp(value[i].asReal(), other_value[i].asReal(), mix);
                    }
                }
                
                newSettings[key_name] = newvalue;
            }

            break;

//      case LLSD::TypeBoolean:
//      case LLSD::TypeString:
//      case LLSD::TypeUUID:
//      case LLSD::TypeURI:
//      case LLSD::TypeBinary:
//      case LLSD::TypeDate:
        default:
            /* TODO: If the UUID points to an image ID, blend the images. */
            // atomic or unknown data types. Lerping between them does not make sense so switch at the break.
            newSettings[key_name] = (mix > BREAK_POINT) ? other_value : value;
            break;
        }
    }

    // Now add anything that is in other but not in the settings
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

LLSD LLSettingsBase::cloneSettings() const
{
    return combineSDMaps(mSettings, LLSD());
}

void LLSettingsBase::exportSettings(std::string name) const
{
    LLSD exprt = LLSDMap("type", LLSD::String(getSettingType()))
        ("name", LLSD::String(name))
        ("settings", mSettings);

    std::string path_name = gDirUtilp->getExpandedFilename(LL_PATH_DUMP, name + ".settings");

    // write to file
    llofstream presetsXML(path_name.c_str());
    if (presetsXML.is_open())
    {
        LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
        formatter->format(exprt, presetsXML, LLSDFormatter::OPTIONS_PRETTY);
        presetsXML.close();

        LL_DEBUGS() << "saved preset '" << name << "'; " << mSettings.size() << " settings" << LL_ENDL;
    }
    else
    {
        LL_WARNS("Presets") << "Cannot open for output preset file " << path_name << LL_ENDL;
    }
}

#ifdef VALIDATION_DEBUG
namespace
{
    LLSD clone_llsd(LLSD value)
    {
        LLSD clone;

        switch (value.type())
        {
//         case LLSD::TypeMap:
//             newSettings[key_name] = combineSDMaps(value, LLSD());
//             break;
        case LLSD::TypeArray:
            clone = LLSD::emptyArray();
            for (LLSD::array_const_iterator ita = value.beginArray(); ita != value.endArray(); ++ita)
            {
                clone.append( clone_llsd(*ita) );
            }
            break;
        case LLSD::TypeInteger:
            clone = LLSD::Integer(value.asInteger());
            break;
        case LLSD::TypeReal:
            clone = LLSD::Real(value.asReal());
            break;
        case LLSD::TypeBoolean:
            clone = LLSD::Boolean(value.asBoolean());
            break;
        case LLSD::TypeString:
            clone = LLSD::String(value.asString());
            break;
        case LLSD::TypeUUID:
            clone = LLSD::UUID(value.asUUID());
            break;
        case LLSD::TypeURI:
            clone = LLSD::URI(value.asURI());
            break;
        case LLSD::TypeDate:
            clone = LLSD::Date(value.asDate());
            break;
        //case LLSD::TypeBinary:
        //    break;
        //default:
        //    newSettings[key_name] = value;
        //    break;
        }

        return clone;
    }

    bool compare_llsd(LLSD valA, LLSD valB)
    {
        if (valA.type() != valB.type())
            return false;

        switch (valA.type())
        {
        //         case LLSD::TypeMap:
        //             newSettings[key_name] = combineSDMaps(value, LLSD());
        //             break;
        case LLSD::TypeArray:
            if (valA.size() != valB.size())
                return false;

            for (S32 idx = 0; idx < valA.size(); ++idx)
            {
                if (!compare_llsd(valA[idx], valB[idx]))
                    return false;
            }
            return true;

        case LLSD::TypeInteger:
            return valA.asInteger() == valB.asInteger();

        case LLSD::TypeReal:
            return is_approx_equal(valA.asReal(), valB.asReal());

        case LLSD::TypeBoolean:
            return valA.asBoolean() == valB.asBoolean();

        case LLSD::TypeString:
            return valA.asString() == valB.asString();

        case LLSD::TypeUUID:
            return valA.asUUID() == valB.asUUID();

        case LLSD::TypeURI:
            return valA.asString() == valB.asString();

        case LLSD::TypeDate:
            return valA.asDate() == valB.asDate();
        }

        return true;
    }
}
#endif

bool LLSettingsBase::validate()
{
    static Validator  validateName(SETTING_NAME, false, LLSD::TypeString);
    static Validator  validateId(SETTING_ID, false, LLSD::TypeUUID);
    validation_list_t validations = getValidationList();
    stringset_t       validated;
    stringset_t       strip;

    // Fields common to all settings.
    if (!validateName.verify(mSettings))
    {
        LL_WARNS("SETTINGS") << "Unable to validate name." << LL_ENDL;
        mIsValid = false;
        return false;
    }
    validated.insert(validateName.getName());

    if (!validateId.verify(mSettings))
    {
        LL_WARNS("SETTINGS") << "Unable to validate Id." << LL_ENDL;
        mIsValid = false;
        return false;
    }
    validated.insert(validateId.getName());

    // Fields for specific settings.
    for (validation_list_t::iterator itv = validations.begin(); itv != validations.end(); ++itv)
    {
#ifdef VALIDATION_DEBUG
        LLSD oldvalue;
        if (mSettings.has((*itv).getName()))
        {
            oldvalue = clone_llsd(mSettings[(*itv).getName()]);
        }
#endif

        if (!(*itv).verify(mSettings))
        {
            LL_WARNS("SETTINGS") << "Settings LLSD fails validation and could not be corrected!" << LL_ENDL;
            mIsValid = false;
            return false;
        }
        validated.insert((*itv).getName());

#ifdef VALIDATION_DEBUG
        if (!oldvalue.isUndefined())
        {
            if (!compare_llsd(mSettings[(*itv).getName()], oldvalue))
            {
                LL_WARNS("SETTINGS") << "Setting '" << (*itv).getName() << "' was changed: " << oldvalue << " -> " << mSettings[(*itv).getName()] << LL_ENDL;
            }
        }
#endif
    }

    // strip extra entries
    for (LLSD::map_iterator itm = mSettings.beginMap(); itm != mSettings.endMap(); ++itm)
    {
        if (validated.find((*itm).first) == validated.end())
        {
            LL_WARNS("SETTINGS") << "Stripping setting '" << (*itm).first << "'" << LL_ENDL;
            strip.insert((*itm).first);
        }
    }

    for (stringset_t::iterator its = strip.begin(); its != strip.end(); ++its)
    {
        mSettings.erase(*its);
    }

    return true;
}

//=========================================================================
bool LLSettingsBase::Validator::verify(LLSD &data)
{
    if (!data.has(mName))
    {
        if (mRequired)
            LL_WARNS("SETTINGS") << "Missing required setting '" << mName << "'" << LL_ENDL;
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
    F32 real = value.asReal();

    F32 clampedval = llclamp(LLSD::Real(real), range[0].asReal(), range[1].asReal());

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

//=========================================================================

void LLSettingsBlender::update(F32Seconds timedelta)
{
    mTimeSpent += timedelta;

    if (mTimeSpent >= mSeconds)
    {
        LLSettingsBlender::ptr_t hold = shared_from_this();   // prevents this from deleting too soon
        mOnFinished(shared_from_this());
        mOnFinished.disconnect_all_slots(); // prevent from firing more than once.
        return;
    }

    F32 blendf = fmod(mTimeSpent.value(), mSeconds.value()) / mSeconds.value();

    mTarget->replaceSettings(mInitial->getSettings());
    mTarget->blend(mFinal, blendf);
}

