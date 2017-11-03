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

LLSettingsBase::ptr_t LLSettingsBase::buildBlend(const ptr_t &begin, const ptr_t &end, F32 blendf)
{
//     if (begin->getSettingType() != end->getSettingType())
//     {
//         LL_WARNS("SETTINGS") << "Attempt to blend settings of different types! " << 
//             begin->getSettingType() << "<->" << end->getSettingType() << LL_ENDL;
// 
//         return LLSettingsBase::ptr_t();
//     }

//    return begin->blend(end, blendf);
    return LLSettingsBase::ptr_t();
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
