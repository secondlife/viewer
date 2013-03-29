/** 
 * @file llcontrol.cpp
 * @brief Holds global state for viewer.
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

#include "linden_common.h"

#include <iostream>
#include <fstream>
#include <algorithm>

#include "llcontrol.h"

#include "llstl.h"

#include "llstring.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4coloru.h"
#include "v4color.h"
#include "v3color.h"
#include "llrect.h"
#include "llxmltree.h"
#include "llsdserialize.h"

#if LL_RELEASE_WITH_DEBUG_INFO || LL_DEBUG
#define CONTROL_ERRS LL_ERRS("ControlErrors")
#else
#define CONTROL_ERRS LL_WARNS("ControlErrors")
#endif


template <> eControlType get_control_type<U32>();
template <> eControlType get_control_type<S32>();
template <> eControlType get_control_type<F32>();
template <> eControlType get_control_type<bool>();
// Yay BOOL, its really an S32.
//template <> eControlType get_control_type<BOOL> () ;
template <> eControlType get_control_type<std::string>();

template <> eControlType get_control_type<LLVector3>();
template <> eControlType get_control_type<LLVector3d>();
template <> eControlType get_control_type<LLRect>();
template <> eControlType get_control_type<LLColor4>();
template <> eControlType get_control_type<LLColor3>();
template <> eControlType get_control_type<LLColor4U>();
template <> eControlType get_control_type<LLSD>();

template <> LLSD convert_to_llsd<U32>(const U32& in);
template <> LLSD convert_to_llsd<LLVector3>(const LLVector3& in);
template <> LLSD convert_to_llsd<LLVector3d>(const LLVector3d& in);
template <> LLSD convert_to_llsd<LLRect>(const LLRect& in);
template <> LLSD convert_to_llsd<LLColor4>(const LLColor4& in);
template <> LLSD convert_to_llsd<LLColor3>(const LLColor3& in);
template <> LLSD convert_to_llsd<LLColor4U>(const LLColor4U& in);

template <> bool convert_from_llsd<bool>(const LLSD& sd, eControlType type, const std::string& control_name);
template <> S32 convert_from_llsd<S32>(const LLSD& sd, eControlType type, const std::string& control_name);
template <> U32 convert_from_llsd<U32>(const LLSD& sd, eControlType type, const std::string& control_name);
template <> F32 convert_from_llsd<F32>(const LLSD& sd, eControlType type, const std::string& control_name);
template <> std::string convert_from_llsd<std::string>(const LLSD& sd, eControlType type, const std::string& control_name);
template <> LLWString convert_from_llsd<LLWString>(const LLSD& sd, eControlType type, const std::string& control_name);
template <> LLVector3 convert_from_llsd<LLVector3>(const LLSD& sd, eControlType type, const std::string& control_name);
template <> LLVector3d convert_from_llsd<LLVector3d>(const LLSD& sd, eControlType type, const std::string& control_name);
template <> LLRect convert_from_llsd<LLRect>(const LLSD& sd, eControlType type, const std::string& control_name);
template <> LLColor4 convert_from_llsd<LLColor4>(const LLSD& sd, eControlType type, const std::string& control_name);
template <> LLColor4U convert_from_llsd<LLColor4U>(const LLSD& sd, eControlType type, const std::string& control_name);
template <> LLColor3 convert_from_llsd<LLColor3>(const LLSD& sd, eControlType type, const std::string& control_name);
template <> LLSD convert_from_llsd<LLSD>(const LLSD& sd, eControlType type, const std::string& control_name);

//this defines the current version of the settings file
const S32 CURRENT_VERSION = 101;

bool LLControlVariable::llsd_compare(const LLSD& a, const LLSD & b)
{
	bool result = false;
	switch (mType)
	{
	case TYPE_U32:
	case TYPE_S32:
		result = a.asInteger() == b.asInteger();
		break;
	case TYPE_BOOLEAN:
		result = a.asBoolean() == b.asBoolean();
		break;
	case TYPE_F32:
		result = a.asReal() == b.asReal();
		break;
	case TYPE_VEC3:
	case TYPE_VEC3D:
		result = LLVector3d(a) == LLVector3d(b);
		break;
	case TYPE_RECT:
		result = LLRect(a) == LLRect(b);
		break;
	case TYPE_COL4:
		result = LLColor4(a) == LLColor4(b);
		break;
	case TYPE_COL3:
		result = LLColor3(a) == LLColor3(b);
		break;
	case TYPE_STRING:
		result = a.asString() == b.asString();
		break;
	default:
		break;
	}

	return result;
}

LLControlVariable::LLControlVariable(const std::string& name, eControlType type,
							 LLSD initial, const std::string& comment,
							 bool persist, bool hidefromsettingseditor)
	: mName(name),
	  mComment(comment),
	  mType(type),
	  mPersist(persist),
	  mHideFromSettingsEditor(hidefromsettingseditor)
{
	if (mPersist && mComment.empty())
	{
		llerrs << "Must supply a comment for control " << mName << llendl;
	}
	//Push back versus setValue'ing here, since we don't want to call a signal yet
	mValues.push_back(initial);
}



LLControlVariable::~LLControlVariable()
{
}

LLSD LLControlVariable::getComparableValue(const LLSD& value)
{
	// *FIX:MEP - The following is needed to make the LLSD::ImplString 
	// work with boolean controls...
	LLSD storable_value;
	if(TYPE_BOOLEAN == type() && value.isString())
	{
		BOOL temp;
		if(LLStringUtil::convertToBOOL(value.asString(), temp)) 
		{
			storable_value = (bool)temp;
		}
		else
		{
			storable_value = false;
		}
	}
	else if (TYPE_LLSD == type() && value.isString())
	{
		LLPointer<LLSDNotationParser> parser = new LLSDNotationParser;
		LLSD result;
		std::stringstream value_stream(value.asString());
		if (parser->parse(value_stream, result, LLSDSerialize::SIZE_UNLIMITED) != LLSDParser::PARSE_FAILURE)
		{
			storable_value = result;
		}
		else
		{
			storable_value = value;
		}
	}
	else
	{
		storable_value = value;
	}

	return storable_value;
}

void LLControlVariable::setValue(const LLSD& new_value, bool saved_value)
{
	if (mValidateSignal(this, new_value) == false)
	{
		// can not set new value, exit
		return;
	}
	
	LLSD storable_value = getComparableValue(new_value);
	LLSD original_value = getValue();
	bool value_changed = llsd_compare(original_value, storable_value) == FALSE;
	if(saved_value)
	{
    	// If we're going to save this value, return to default but don't fire
		resetToDefault(false);
	    if (llsd_compare(mValues.back(), storable_value) == FALSE)
	    {
		    mValues.push_back(storable_value);
	    }
	}
    else
    {
        // This is an unsaved value. Its needs to reside at
        // mValues[2] (or greater). It must not affect 
        // the result of getSaveValue()
	    if (llsd_compare(mValues.back(), storable_value) == FALSE)
	    {
            while(mValues.size() > 2)
            {
                // Remove any unsaved values.
                mValues.pop_back();
            }

            if(mValues.size() < 2)
            {
                // Add the default to the 'save' value.
                mValues.push_back(mValues[0]);
            }

            // Add the 'un-save' value.
            mValues.push_back(storable_value);
	    }
    }

    if(value_changed)
    {
		firePropertyChanged(original_value);
    }
}

void LLControlVariable::setDefaultValue(const LLSD& value)
{
	// Set the control variables value and make it 
	// the default value. If the active value is changed,
	// send the signal.
	// *NOTE: Default values are not saved, only read.

	LLSD comparable_value = getComparableValue(value);
	LLSD original_value = getValue();
	bool value_changed = (llsd_compare(original_value, comparable_value) == FALSE);
	resetToDefault(false);
	mValues[0] = comparable_value;
	if(value_changed)
	{
		firePropertyChanged(original_value);
	}
}

void LLControlVariable::setPersist(bool state)
{
	mPersist = state;
}

void LLControlVariable::setHiddenFromSettingsEditor(bool hide)
{
	mHideFromSettingsEditor = hide;
}

void LLControlVariable::setComment(const std::string& comment)
{
	mComment = comment;
}

void LLControlVariable::resetToDefault(bool fire_signal)
{
	//The first setting is always the default
	//Pop to it and fire off the listener
	LLSD originalValue = mValues.back();

	while(mValues.size() > 1)
	{
		mValues.pop_back();
	}
	
	if(fire_signal) 
	{
		firePropertyChanged(originalValue);
	}
}

bool LLControlVariable::isSaveValueDefault()
{ 
    return (mValues.size() ==  1) 
        || ((mValues.size() > 1) && llsd_compare(mValues[1], mValues[0]));
}

LLSD LLControlVariable::getSaveValue() const
{
	//The first level of the stack is default
	//We assume that the second level is user preferences that should be saved
	if(mValues.size() > 1) return mValues[1];
	return mValues[0];
}

LLPointer<LLControlVariable> LLControlGroup::getControl(const std::string& name)
{
	ctrl_name_table_t::iterator iter = mNameTable.find(name);
	return iter == mNameTable.end() ? LLPointer<LLControlVariable>() : iter->second;
}


////////////////////////////////////////////////////////////////////////////

LLControlGroup::LLControlGroup(const std::string& name)
:	LLInstanceTracker<LLControlGroup, std::string>(name)
{
	mTypeString[TYPE_U32] = "U32";
	mTypeString[TYPE_S32] = "S32";
	mTypeString[TYPE_F32] = "F32";
	mTypeString[TYPE_BOOLEAN] = "Boolean";
	mTypeString[TYPE_STRING] = "String";
	mTypeString[TYPE_VEC3] = "Vector3";
    mTypeString[TYPE_VEC3D] = "Vector3D";
	mTypeString[TYPE_RECT] = "Rect";
	mTypeString[TYPE_COL4] = "Color4";
	mTypeString[TYPE_COL3] = "Color3";
	mTypeString[TYPE_LLSD] = "LLSD";
}

LLControlGroup::~LLControlGroup()
{
	cleanup();
}

void LLControlGroup::cleanup()
{
	mNameTable.clear();
}

eControlType LLControlGroup::typeStringToEnum(const std::string& typestr)
{
	for(int i = 0; i < (int)TYPE_COUNT; ++i)
	{
		if(mTypeString[i] == typestr) return (eControlType)i;
	}
	return (eControlType)-1;
}

std::string LLControlGroup::typeEnumToString(eControlType typeenum)
{
	return mTypeString[typeenum];
}

BOOL LLControlGroup::declareControl(const std::string& name, eControlType type, const LLSD initial_val, const std::string& comment, BOOL persist, BOOL hidefromsettingseditor)
{
	LLControlVariable* existing_control = getControl(name);
	if (existing_control)
 	{
		if (persist && existing_control->isType(type))
		{
			if (!existing_control->llsd_compare(existing_control->getDefault(), initial_val))
			{
				// Sometimes we need to declare a control *after* it has been loaded from a settings file.
				LLSD cur_value = existing_control->getValue(); // get the current value
				existing_control->setDefaultValue(initial_val); // set the default to the declared value
				existing_control->setValue(cur_value); // now set to the loaded value
			}
		}
		else
		{
			llwarns << "Control named " << name << " already exists, ignoring new declaration." << llendl;
		}
 		return TRUE;
	}

	// if not, create the control and add it to the name table
	LLControlVariable* control = new LLControlVariable(name, type, initial_val, comment, persist, hidefromsettingseditor);
	mNameTable[name] = control;	
	return TRUE;
}

BOOL LLControlGroup::declareU32(const std::string& name, const U32 initial_val, const std::string& comment, BOOL persist)
{
	return declareControl(name, TYPE_U32, (LLSD::Integer) initial_val, comment, persist);
}

BOOL LLControlGroup::declareS32(const std::string& name, const S32 initial_val, const std::string& comment, BOOL persist)
{
	return declareControl(name, TYPE_S32, initial_val, comment, persist);
}

BOOL LLControlGroup::declareF32(const std::string& name, const F32 initial_val, const std::string& comment, BOOL persist)
{
	return declareControl(name, TYPE_F32, initial_val, comment, persist);
}

BOOL LLControlGroup::declareBOOL(const std::string& name, const BOOL initial_val, const std::string& comment, BOOL persist)
{
	return declareControl(name, TYPE_BOOLEAN, initial_val, comment, persist);
}

BOOL LLControlGroup::declareString(const std::string& name, const std::string& initial_val, const std::string& comment, BOOL persist)
{
	return declareControl(name, TYPE_STRING, initial_val, comment, persist);
}

BOOL LLControlGroup::declareVec3(const std::string& name, const LLVector3 &initial_val, const std::string& comment, BOOL persist)
{
	return declareControl(name, TYPE_VEC3, initial_val.getValue(), comment, persist);
}

BOOL LLControlGroup::declareVec3d(const std::string& name, const LLVector3d &initial_val, const std::string& comment, BOOL persist)
{
	return declareControl(name, TYPE_VEC3D, initial_val.getValue(), comment, persist);
}

BOOL LLControlGroup::declareRect(const std::string& name, const LLRect &initial_val, const std::string& comment, BOOL persist)
{
	return declareControl(name, TYPE_RECT, initial_val.getValue(), comment, persist);
}

BOOL LLControlGroup::declareColor4(const std::string& name, const LLColor4 &initial_val, const std::string& comment, BOOL persist )
{
	return declareControl(name, TYPE_COL4, initial_val.getValue(), comment, persist);
}

BOOL LLControlGroup::declareColor3(const std::string& name, const LLColor3 &initial_val, const std::string& comment, BOOL persist )
{
	return declareControl(name, TYPE_COL3, initial_val.getValue(), comment, persist);
}

BOOL LLControlGroup::declareLLSD(const std::string& name, const LLSD &initial_val, const std::string& comment, BOOL persist )
{
	return declareControl(name, TYPE_LLSD, initial_val, comment, persist);
}

BOOL LLControlGroup::getBOOL(const std::string& name)
{
	return (BOOL)get<bool>(name);
}

S32 LLControlGroup::getS32(const std::string& name)
{
	return get<S32>(name);
}

U32 LLControlGroup::getU32(const std::string& name)
{
	return get<U32>(name);
}

F32 LLControlGroup::getF32(const std::string& name)
{
	return get<F32>(name);
}

std::string LLControlGroup::getString(const std::string& name)
{
	return get<std::string>(name);
}

LLWString LLControlGroup::getWString(const std::string& name)
{
	return get<LLWString>(name);
}

std::string LLControlGroup::getText(const std::string& name)
{
	std::string utf8_string = getString(name);
	LLStringUtil::replaceChar(utf8_string, '^', '\n');
	LLStringUtil::replaceChar(utf8_string, '%', ' ');
	return (utf8_string);
}

LLVector3 LLControlGroup::getVector3(const std::string& name)
{
	return get<LLVector3>(name);
}

LLVector3d LLControlGroup::getVector3d(const std::string& name)
{
	return get<LLVector3d>(name);
}

LLRect LLControlGroup::getRect(const std::string& name)
{
	return get<LLRect>(name);
}


LLColor4 LLControlGroup::getColor(const std::string& name)
{
	return get<LLColor4>(name);
}

LLColor4 LLControlGroup::getColor4(const std::string& name)
{
	return get<LLColor4>(name);
}

LLColor3 LLControlGroup::getColor3(const std::string& name)
{
	return get<LLColor3>(name);
}

LLSD LLControlGroup::getLLSD(const std::string& name)
{
	return get<LLSD>(name);
}

BOOL LLControlGroup::controlExists(const std::string& name)
{
	ctrl_name_table_t::iterator iter = mNameTable.find(name);
	return iter != mNameTable.end();
}


//-------------------------------------------------------------------
// Set functions
//-------------------------------------------------------------------

void LLControlGroup::setBOOL(const std::string& name, BOOL val)
{
	set<bool>(name, val);
}


void LLControlGroup::setS32(const std::string& name, S32 val)
{
	set(name, val);
}


void LLControlGroup::setF32(const std::string& name, F32 val)
{
	set(name, val);
}


void LLControlGroup::setU32(const std::string& name, U32 val)
{
	set(name, val);
}


void LLControlGroup::setString(const std::string& name, const std::string &val)
{
	set(name, val);
}


void LLControlGroup::setVector3(const std::string& name, const LLVector3 &val)
{
	set(name, val);
}

void LLControlGroup::setVector3d(const std::string& name, const LLVector3d &val)
{
	set(name, val);
}

void LLControlGroup::setRect(const std::string& name, const LLRect &val)
{
	set(name, val);
}

void LLControlGroup::setColor4(const std::string& name, const LLColor4 &val)
{
	set(name, val);
}

void LLControlGroup::setLLSD(const std::string& name, const LLSD& val)
{
	set(name, val);
}

void LLControlGroup::setUntypedValue(const std::string& name, const LLSD& val)
{
	if (name.empty())
	{
		return;
	}

	LLControlVariable* control = getControl(name);
	
	if (control)
	{
		control->setValue(val);
	}
	else
	{
		CONTROL_ERRS << "Invalid control " << name << llendl;
	}
}


//---------------------------------------------------------------
// Load and save
//---------------------------------------------------------------

// Returns number of controls loaded, so 0 if failure
U32 LLControlGroup::loadFromFileLegacy(const std::string& filename, BOOL require_declaration, eControlType declare_as)
{
	std::string name;

	LLXmlTree xml_controls;

	if (!xml_controls.parseFile(filename))
	{
		llwarns << "Unable to open control file " << filename << llendl;
		return 0;
	}

	LLXmlTreeNode* rootp = xml_controls.getRoot();
	if (!rootp || !rootp->hasAttribute("version"))
	{
		llwarns << "No valid settings header found in control file " << filename << llendl;
		return 0;
	}

	U32		item = 0;
	U32		validitems = 0;
	S32 version;
	
	rootp->getAttributeS32("version", version);

	// Check file version
	if (version != CURRENT_VERSION)
	{
		llinfos << filename << " does not appear to be a version " << CURRENT_VERSION << " controls file" << llendl;
		return 0;
	}

	LLXmlTreeNode* child_nodep = rootp->getFirstChild();
	while(child_nodep)
	{
		name = child_nodep->getName();		
		
		BOOL declared = controlExists(name);

		if (require_declaration && !declared)
		{
			// Declaration required, but this name not declared.
			// Complain about non-empty names.
			if (!name.empty())
			{
				//read in to end of line
				llwarns << "LLControlGroup::loadFromFile() : Trying to set \"" << name << "\", setting doesn't exist." << llendl;
			}
			child_nodep = rootp->getNextChild();
			continue;
		}

		// Got an item.  Load it up.
		item++;

		// If not declared, assume it's a string
		if (!declared)
		{
			switch(declare_as)
			{
			case TYPE_COL4:
				declareColor4(name, LLColor4::white, LLStringUtil::null, NO_PERSIST);
				break;
			case TYPE_STRING:
			default:
				declareString(name, LLStringUtil::null, LLStringUtil::null, NO_PERSIST);
				break;
			}
		}

		// Control name has been declared in code.
		LLControlVariable *control = getControl(name);

		llassert(control);
		
		switch(control->mType)
		{
		case TYPE_F32:
			{
				F32 initial = 0.f;

				child_nodep->getAttributeF32("value", initial);

				control->set(initial);
				validitems++;
			}
			break;
		case TYPE_S32:
			{
				S32 initial = 0;

				child_nodep->getAttributeS32("value", initial);

				control->set(initial);
				validitems++;
			}
			break;
		case TYPE_U32:
			{
				U32 initial = 0;
				child_nodep->getAttributeU32("value", initial);
				control->set((LLSD::Integer) initial);
				validitems++;
			}
			break;
		case TYPE_BOOLEAN:
			{
				BOOL initial = FALSE;

				child_nodep->getAttributeBOOL("value", initial);
				control->set(initial);

				validitems++;
			}
			break;
		case TYPE_STRING:
			{
				std::string string;
				child_nodep->getAttributeString("value", string);
				control->set(string);
				validitems++;
			}
			break;
		case TYPE_VEC3:
			{
				LLVector3 vector;

				child_nodep->getAttributeVector3("value", vector);
				control->set(vector.getValue());
				validitems++;
			}
			break;
		case TYPE_VEC3D:
			{
				LLVector3d vector;

				child_nodep->getAttributeVector3d("value", vector);

				control->set(vector.getValue());
				validitems++;
			}
			break;
		case TYPE_RECT:
			{
				//RN: hack to support reading rectangles from a string
				std::string rect_string;

				child_nodep->getAttributeString("value", rect_string);
				std::istringstream istream(rect_string);
				S32 left, bottom, width, height;

				istream >> left >> bottom >> width >> height;

				LLRect rect;
				rect.setOriginAndSize(left, bottom, width, height);

				control->set(rect.getValue());
				validitems++;
			}
			break;
		case TYPE_COL4:
			{
				LLColor4 color;
				
				child_nodep->getAttributeColor4("value", color);
				control->set(color.getValue());
				validitems++;
			}
			break;
		case TYPE_COL3:
			{
				LLVector3 color;
				
				child_nodep->getAttributeVector3("value", color);
				control->set(LLColor3(color.mV).getValue());
				validitems++;
			}
			break;

		default:
		  break;

		}
	
		child_nodep = rootp->getNextChild();
	}

	return validitems;
}

U32 LLControlGroup::saveToFile(const std::string& filename, BOOL nondefault_only)
{
	LLSD settings;
	int num_saved = 0;
	for (ctrl_name_table_t::iterator iter = mNameTable.begin();
		 iter != mNameTable.end(); iter++)
	{
		LLControlVariable* control = iter->second;
		if (!control)
		{
			llwarns << "Tried to save invalid control: " << iter->first << llendl;
		}

		if( control && control->isPersisted() )
		{
			if (!(nondefault_only && (control->isSaveValueDefault())))
			{
				settings[iter->first]["Type"] = typeEnumToString(control->type());
				settings[iter->first]["Comment"] = control->getComment();
				settings[iter->first]["Value"] = control->getSaveValue();
				++num_saved;
			}
			else
			{
				// Debug spam
				// llinfos << "Skipping " << control->getName() << llendl;
			}
		}
	}
	llofstream file;
	file.open(filename);
	if (file.is_open())
	{
		LLSDSerialize::toPrettyXML(settings, file);
		file.close();
		llinfos << "Saved to " << filename << llendl;
	}
	else
	{
        // This is a warning because sometime we want to use settings files which can't be written...
		llwarns << "Unable to open settings file: " << filename << llendl;
		return 0;
	}
	return num_saved;
}

U32 LLControlGroup::loadFromFile(const std::string& filename, bool set_default_values, bool save_values)
{
	LLSD settings;
	llifstream infile;
	infile.open(filename);
	if(!infile.is_open())
	{
		llwarns << "Cannot find file " << filename << " to load." << llendl;
		return 0;
	}

	S32 ret = LLSDSerialize::fromXML(settings, infile);

	if (ret <= 0)
	{
		infile.close();
		llwarns << "Unable to open LLSD control file " << filename << ". Trying Legacy Method." << llendl;		
		return loadFromFileLegacy(filename, TRUE, TYPE_STRING);
	}

	U32	validitems = 0;
	bool hidefromsettingseditor = false;
	
	for(LLSD::map_const_iterator itr = settings.beginMap(); itr != settings.endMap(); ++itr)
	{
		bool persist = true;
		std::string const & name = itr->first;
		LLSD const & control_map = itr->second;
		
		if(control_map.has("Persist")) 
		{
			persist = control_map["Persist"].asInteger();
		}
		
		// Sometimes we want to use the settings system to provide cheap persistence, but we
		// don't want the settings themselves to be easily manipulated in the UI because 
		// doing so can cause support problems. So we have this option:
		if(control_map.has("HideFromEditor"))
		{
			hidefromsettingseditor = control_map["HideFromEditor"].asInteger();
		}
		else
		{
			hidefromsettingseditor = false;
		}
		
		// If the control exists just set the value from the input file.
		LLControlVariable* existing_control = getControl(name);
		if(existing_control)
		{
			if(set_default_values)
			{
				// Override all previously set properties of this control.
				// ... except for type. The types must match.
				eControlType new_type = typeStringToEnum(control_map["Type"].asString());
				if(existing_control->isType(new_type))
				{
					existing_control->setDefaultValue(control_map["Value"]);
					existing_control->setPersist(persist);
					existing_control->setHiddenFromSettingsEditor(hidefromsettingseditor);
					existing_control->setComment(control_map["Comment"].asString());
				}
				else
				{
					llerrs << "Mismatched type of control variable '"
						   << name << "' found while loading '"
						   << filename << "'." << llendl;
				}
			}
			else if(existing_control->isPersisted())
			{
				existing_control->setValue(control_map["Value"], save_values);
			}
			// *NOTE: If not persisted and not setting defaults, 
			// the value should not get loaded.
		}
		else
		{
			declareControl(name, 
						   typeStringToEnum(control_map["Type"].asString()), 
						   control_map["Value"], 
						   control_map["Comment"].asString(), 
						   persist,
						   hidefromsettingseditor
						   );
		}
		
		++validitems;
	}

	return validitems;
}

void LLControlGroup::resetToDefaults()
{
	ctrl_name_table_t::iterator control_iter;
	for (control_iter = mNameTable.begin();
		control_iter != mNameTable.end();
		++control_iter)
	{
		LLControlVariable* control = (*control_iter).second;
		control->resetToDefault();
	}
}

void LLControlGroup::applyToAll(ApplyFunctor* func)
{
	for (ctrl_name_table_t::iterator iter = mNameTable.begin();
		 iter != mNameTable.end(); iter++)
	{
		func->apply(iter->first, iter->second);
	}
}

//============================================================================

#ifdef TEST_HARNESS
void main()
{
	F32_CONTROL foo, getfoo;

	S32_CONTROL bar, getbar;
	
	BOOL_CONTROL baz;

	U32 count = gGlobals.loadFromFile("controls.ini");
	llinfos << "Loaded " << count << " controls" << llendl;

	// test insertion
	foo = new LLControlVariable<F32>("gFoo", 5.f, 1.f, 20.f);
	gGlobals.addEntry("gFoo", foo);

	bar = new LLControlVariable<S32>("gBar", 10, 2, 22);
	gGlobals.addEntry("gBar", bar);

	baz = new LLControlVariable<BOOL>("gBaz", FALSE);
	gGlobals.addEntry("gBaz", baz);

	// test retrieval
	getfoo = (LLControlVariable<F32>*) gGlobals.resolveName("gFoo");
	getfoo->dump();

	getbar = (S32_CONTROL) gGlobals.resolveName("gBar");
	getbar->dump();

	// change data
	getfoo->set(10.f);
	getfoo->dump();

	// Failure modes

	// ...min > max
	// badfoo = new LLControlVariable<F32>("gFoo2", 100.f, 20.f, 5.f);

	// ...initial > max
	// badbar = new LLControlVariable<S32>("gBar2", 10, 20, 100000);

	// ...misspelled name
	// getfoo = (F32_CONTROL) gGlobals.resolveName("fooMisspelled");
	// getfoo->dump();

	// ...invalid data type
	getfoo = (F32_CONTROL) gGlobals.resolveName("gFoo");
	getfoo->set(TRUE);
	getfoo->dump();

	// ...out of range data
	// getfoo->set(100000000.f);
	// getfoo->dump();

	// Clean Up
	delete foo;
	delete bar;
	delete baz;
}
#endif


template <> eControlType get_control_type<U32>() 
{ 
	return TYPE_U32; 
}

template <> eControlType get_control_type<S32>() 
{ 
	return TYPE_S32; 
}

template <> eControlType get_control_type<F32>() 
{ 
	return TYPE_F32; 
}

template <> eControlType get_control_type<bool> () 
{ 
	return TYPE_BOOLEAN; 
}
/*
// Yay BOOL, its really an S32.
template <> eControlType get_control_type<BOOL> () 
{ 
	return TYPE_BOOLEAN; 
}
*/
template <> eControlType get_control_type<std::string>() 
{ 
	return TYPE_STRING; 
}

template <> eControlType get_control_type<LLVector3>() 
{ 
	return TYPE_VEC3; 
}

template <> eControlType get_control_type<LLVector3d>() 
{ 
	return TYPE_VEC3D; 
}

template <> eControlType get_control_type<LLRect>() 
{ 
	return TYPE_RECT; 
}

template <> eControlType get_control_type<LLColor4>() 
{ 
	return TYPE_COL4; 
}

template <> eControlType get_control_type<LLColor3>() 
{ 
	return TYPE_COL3; 
}

template <> eControlType get_control_type<LLSD>() 
{ 
	return TYPE_LLSD; 
}


template <> LLSD convert_to_llsd<U32>(const U32& in) 
{ 
	return (LLSD::Integer)in; 
}

template <> LLSD convert_to_llsd<LLVector3>(const LLVector3& in) 
{ 
	return in.getValue(); 
}

template <> LLSD convert_to_llsd<LLVector3d>(const LLVector3d& in) 
{ 
	return in.getValue(); 
}

template <> LLSD convert_to_llsd<LLRect>(const LLRect& in) 
{ 
	return in.getValue(); 
}

template <> LLSD convert_to_llsd<LLColor4>(const LLColor4& in) 
{ 
	return in.getValue(); 
}

template <> LLSD convert_to_llsd<LLColor3>(const LLColor3& in) 
{ 
	return in.getValue(); 
}

template <> LLSD convert_to_llsd<LLColor4U>(const LLColor4U& in) 
{ 
	return in.getValue();
}


template<>
bool convert_from_llsd<bool>(const LLSD& sd, eControlType type, const std::string& control_name)
{
	if (type == TYPE_BOOLEAN)
		return sd.asBoolean();
	else
	{
		CONTROL_ERRS << "Invalid BOOL value for " << control_name << ": " << sd << llendl;
		return FALSE;
	}
}

template<>
S32 convert_from_llsd<S32>(const LLSD& sd, eControlType type, const std::string& control_name)
{
	if (type == TYPE_S32)
		return sd.asInteger();
	else
	{
		CONTROL_ERRS << "Invalid S32 value for " << control_name << ": " << sd << llendl;
		return 0;
	}
}

template<>
U32 convert_from_llsd<U32>(const LLSD& sd, eControlType type, const std::string& control_name)
{
	if (type == TYPE_U32)	
		return sd.asInteger();
	else
	{
		CONTROL_ERRS << "Invalid U32 value for " << control_name << ": " << sd << llendl;
		return 0;
	}
}

template<>
F32 convert_from_llsd<F32>(const LLSD& sd, eControlType type, const std::string& control_name)
{
	if (type == TYPE_F32)
		return (F32) sd.asReal();
	else
	{
		CONTROL_ERRS << "Invalid F32 value for " << control_name << ": " << sd << llendl;
		return 0.0f;
	}
}

template<>
std::string convert_from_llsd<std::string>(const LLSD& sd, eControlType type, const std::string& control_name)
{
	if (type == TYPE_STRING)
		return sd.asString();
	else
	{
		CONTROL_ERRS << "Invalid string value for " << control_name << ": " << sd << llendl;
		return LLStringUtil::null;
	}
}

template<>
LLWString convert_from_llsd<LLWString>(const LLSD& sd, eControlType type, const std::string& control_name)
{
	return utf8str_to_wstring(convert_from_llsd<std::string>(sd, type, control_name));
}

template<>
LLVector3 convert_from_llsd<LLVector3>(const LLSD& sd, eControlType type, const std::string& control_name)
{
	if (type == TYPE_VEC3)
		return (LLVector3)sd;
	else
	{
		CONTROL_ERRS << "Invalid LLVector3 value for " << control_name << ": " << sd << llendl;
		return LLVector3::zero;
	}
}

template<>
LLVector3d convert_from_llsd<LLVector3d>(const LLSD& sd, eControlType type, const std::string& control_name)
{
	if (type == TYPE_VEC3D)
		return (LLVector3d)sd;
	else
	{
		CONTROL_ERRS << "Invalid LLVector3d value for " << control_name << ": " << sd << llendl;
		return LLVector3d::zero;
	}
}

template<>
LLRect convert_from_llsd<LLRect>(const LLSD& sd, eControlType type, const std::string& control_name)
{
	if (type == TYPE_RECT)
		return LLRect(sd);
	else
	{
		CONTROL_ERRS << "Invalid rect value for " << control_name << ": " << sd << llendl;
		return LLRect::null;
	}
}


template<>
LLColor4 convert_from_llsd<LLColor4>(const LLSD& sd, eControlType type, const std::string& control_name)
{
	if (type == TYPE_COL4)
	{
		LLColor4 color(sd);
		if (color.mV[VRED] < 0.f || color.mV[VRED] > 1.f)
		{
			llwarns << "Color " << control_name << " red value out of range: " << color << llendl;
		}
		else if (color.mV[VGREEN] < 0.f || color.mV[VGREEN] > 1.f)
		{
			llwarns << "Color " << control_name << " green value out of range: " << color << llendl;
		}
		else if (color.mV[VBLUE] < 0.f || color.mV[VBLUE] > 1.f)
		{
			llwarns << "Color " << control_name << " blue value out of range: " << color << llendl;
		}
		else if (color.mV[VALPHA] < 0.f || color.mV[VALPHA] > 1.f)
		{
			llwarns << "Color " << control_name << " alpha value out of range: " << color << llendl;
		}

		return LLColor4(sd);
	}
	else
	{
		CONTROL_ERRS << "Control " << control_name << " not a color" << llendl;
		return LLColor4::white;
	}
}

template<>
LLColor3 convert_from_llsd<LLColor3>(const LLSD& sd, eControlType type, const std::string& control_name)
{
	if (type == TYPE_COL3)
		return sd;
	else
	{
		CONTROL_ERRS << "Invalid LLColor3 value for " << control_name << ": " << sd << llendl;
		return LLColor3::white;
	}
}

template<>
LLSD convert_from_llsd<LLSD>(const LLSD& sd, eControlType type, const std::string& control_name)
{
	return sd;
}


#if TEST_CACHED_CONTROL

#define DECL_LLCC(T, V) static LLCachedControl<T> mySetting_##T("TestCachedControl"#T, V)
DECL_LLCC(U32, (U32)666);
DECL_LLCC(S32, (S32)-666);
DECL_LLCC(F32, (F32)-666.666);
DECL_LLCC(bool, true);
DECL_LLCC(BOOL, FALSE);
static LLCachedControl<std::string> mySetting_string("TestCachedControlstring", "Default String Value");
DECL_LLCC(LLVector3, LLVector3(1.0f, 2.0f, 3.0f));
DECL_LLCC(LLVector3d, LLVector3d(6.0f, 5.0f, 4.0f));
DECL_LLCC(LLRect, LLRect(0, 0, 100, 500));
DECL_LLCC(LLColor4, LLColor4(0.0f, 0.5f, 1.0f));
DECL_LLCC(LLColor3, LLColor3(1.0f, 0.f, 0.5f));
DECL_LLCC(LLColor4U, LLColor4U(255, 200, 100, 255));

LLSD test_llsd = LLSD()["testing1"] = LLSD()["testing2"];
DECL_LLCC(LLSD, test_llsd);

static LLCachedControl<std::string> test_BrowserHomePage("BrowserHomePage", "hahahahahha", "Not the real comment");

void test_cached_control()
{
#define TEST_LLCC(T, V) if((T)mySetting_##T != V) llerrs << "Fail "#T << llendl
	TEST_LLCC(U32, 666);
	TEST_LLCC(S32, (S32)-666);
	TEST_LLCC(F32, (F32)-666.666);
	TEST_LLCC(bool, true);
	TEST_LLCC(BOOL, FALSE);
	if((std::string)mySetting_string != "Default String Value") llerrs << "Fail string" << llendl;
	TEST_LLCC(LLVector3, LLVector3(1.0f, 2.0f, 3.0f));
	TEST_LLCC(LLVector3d, LLVector3d(6.0f, 5.0f, 4.0f));
	TEST_LLCC(LLRect, LLRect(0, 0, 100, 500));
	TEST_LLCC(LLColor4, LLColor4(0.0f, 0.5f, 1.0f));
	TEST_LLCC(LLColor3, LLColor3(1.0f, 0.f, 0.5f));
	TEST_LLCC(LLColor4U, LLColor4U(255, 200, 100, 255));
//There's no LLSD comparsion for LLCC yet. TEST_LLCC(LLSD, test_llsd); 

	if((std::string)test_BrowserHomePage != "http://www.secondlife.com") llerrs << "Fail BrowserHomePage" << llendl;
}
#endif // TEST_CACHED_CONTROL

