/** 
 * @file llcontrol.cpp
 * @brief Holds global state for viewer.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include <iostream>
#include <fstream>
#include <algorithm>

#include "llcontrol.h"

#include "llstl.h"

#include "linked_lists.h"
#include "llstring.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4coloru.h"
#include "v4color.h"
#include "v3color.h"
#include "llrect.h"
#include "llxmltree.h"
#include "llsdserialize.h"

#if LL_RELEASE_FOR_DOWNLOAD
#define CONTROL_ERRS llwarns
#else
#define CONTROL_ERRS llerrs
#endif

//this defines the current version of the settings file
U32	LLControlBase::sMaxControlNameLength = 0;

//These lists are used to store the ID's of registered event listeners.
std::list<S32>				LLControlBase::mFreeIDs;
std::list<S32>				LLControlBase::mUsedIDs;

S32							LLControlBase::mTopID;

std::set<LLControlBase*>	LLControlBase::mChangedControls;

const S32 CURRENT_VERSION = 101;

BOOL control_insert_before( LLControlBase* first, LLControlBase* second );

BOOL LLControl::llsd_compare(const LLSD& a, const LLSD & b)
{
	switch (mType)
	{
	case TYPE_U32:
	case TYPE_S32:
		return a.asInteger() == b.asInteger();
	case TYPE_BOOLEAN:
		return a.asBoolean() == b.asBoolean();
	case TYPE_F32:
		return a.asReal() == b.asReal();
	case TYPE_VEC3:
	case TYPE_VEC3D:
		return LLVector3d(a) == LLVector3d(b);
	case TYPE_RECT:
		return LLRect(a) == LLRect(b);
	case TYPE_COL4:
		return LLColor4(a) == LLColor4(b);
	case TYPE_COL3:
		return LLColor3(a) == LLColor3(b);
	case TYPE_COL4U:
		return LLColor4U(a) == LLColor4U(b);
	case TYPE_STRING:
		return a.asString() == b.asString();
	default:
		// no-op
		break;
	}

	return FALSE;
}

LLControlBase::~LLControlBase()
{
}

// virtual
void LLControlBase::resetToDefault()
{
}

LLControlGroup::LLControlGroup():	mNameTable()
{
	//mFreeStringOffset = 0;
}

LLControlGroup::~LLControlGroup()
{
}

LLSD LLControlBase::registerListener(LLSimpleListenerObservable *listener, LLSD userdata)
{
	// Symmetric listener relationship
	addListener(listener, "", userdata);
	listener->addListener(this, "", userdata);
	return getValue();
}

void LLControlGroup::cleanup()
{
	mNameTable.clear();
}

LLControlBase*	LLControlGroup::getControl(const LLString& name)
{
	return mNameTable[name];
}

BOOL LLControlGroup::declareControl(const LLString& name, eControlType type, const LLSD initial_val, const LLString& comment, BOOL persist)
{
	if(!mNameTable[name])
	{
		// if not, create the control and add it to the name table
		LLControl* control = new LLControl(name, type, initial_val, comment, persist);
		mNameTable[name] = control;
		return TRUE;
	} else
	{
		llwarns << "LLControlGroup::declareControl: Control named " << name << " already exists." << llendl;
		return FALSE;
	}
}

BOOL LLControlGroup::declareU32(const LLString& name, const U32 initial_val, const LLString& comment, BOOL persist)
{
	return declareControl(name, TYPE_U32, (LLSD::Integer) initial_val, comment, persist);
}

BOOL LLControlGroup::declareS32(const LLString& name, const S32 initial_val, const LLString& comment, BOOL persist)
{
	return declareControl(name, TYPE_S32, initial_val, comment, persist);
}

BOOL LLControlGroup::declareF32(const LLString& name, const F32 initial_val, const LLString& comment, BOOL persist)
{
	return declareControl(name, TYPE_F32, initial_val, comment, persist);
}

BOOL LLControlGroup::declareBOOL(const LLString& name, const BOOL initial_val, const LLString& comment, BOOL persist)
{
	return declareControl(name, TYPE_BOOLEAN, initial_val, comment, persist);
}

BOOL LLControlGroup::declareString(const LLString& name, const LLString& initial_val, const LLString& comment, BOOL persist)
{
	return declareControl(name, TYPE_STRING, initial_val, comment, persist);
}

BOOL LLControlGroup::declareVec3(const LLString& name, const LLVector3 &initial_val, const LLString& comment, BOOL persist)
{
	return declareControl(name, TYPE_VEC3, initial_val.getValue(), comment, persist);
}

BOOL LLControlGroup::declareVec3d(const LLString& name, const LLVector3d &initial_val, const LLString& comment, BOOL persist)
{
	return declareControl(name, TYPE_VEC3D, initial_val.getValue(), comment, persist);
}

BOOL LLControlGroup::declareRect(const LLString& name, const LLRect &initial_val, const LLString& comment, BOOL persist)
{
	return declareControl(name, TYPE_RECT, initial_val.getValue(), comment, persist);
}

BOOL LLControlGroup::declareColor4U(const LLString& name, const LLColor4U &initial_val, const LLString& comment, BOOL persist )
{
	return declareControl(name, TYPE_COL4U, initial_val.getValue(), comment, persist);
}

BOOL LLControlGroup::declareColor4(const LLString& name, const LLColor4 &initial_val, const LLString& comment, BOOL persist )
{
	return declareControl(name, TYPE_COL4, initial_val.getValue(), comment, persist);
}

BOOL LLControlGroup::declareColor3(const LLString& name, const LLColor3 &initial_val, const LLString& comment, BOOL persist )
{
	return declareControl(name, TYPE_COL3, initial_val.getValue(), comment, persist);
}

LLSD LLControlGroup::registerListener(const LLString& name, LLSimpleListenerObservable *listener)
{
	LLControlBase *control = mNameTable[name];
	if (control)
	{
		return control->registerListener(listener);
	}
	return LLSD();
}

BOOL LLControlGroup::getBOOL(const LLString& name)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_BOOLEAN))
		return control->get().asBoolean();
	else
	{
		CONTROL_ERRS << "Invalid BOOL control " << name << llendl;
		return FALSE;
	}
}

S32 LLControlGroup::getS32(const LLString& name)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_S32))
		return control->get().asInteger();
	else
	{
		CONTROL_ERRS << "Invalid S32 control " << name << llendl;
		return 0;
	}
}

U32 LLControlGroup::getU32(const LLString& name)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_U32))		
		return control->get().asInteger();
	else
	{
		CONTROL_ERRS << "Invalid U32 control " << name << llendl;
		return 0;
	}
}

F32 LLControlGroup::getF32(const LLString& name)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_F32))
		return (F32) control->get().asReal();
	else
	{
		CONTROL_ERRS << "Invalid F32 control " << name << llendl;
		return 0.0f;
	}
}

LLString LLControlGroup::findString(const LLString& name)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_STRING))
		return control->get().asString();
	return LLString::null;
}

LLString LLControlGroup::getString(const LLString& name)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_STRING))
		return control->get().asString();
	else
	{
		CONTROL_ERRS << "Invalid string control " << name << llendl;
		return LLString::null;
	}
}

LLWString LLControlGroup::getWString(const LLString& name)
{
	return utf8str_to_wstring(getString(name));
}

LLString LLControlGroup::getText(const LLString& name)
{
	LLString utf8_string = getString(name);
	LLString::replaceChar(utf8_string, '^', '\n');
	LLString::replaceChar(utf8_string, '%', ' ');
	return (utf8_string);
}

LLVector3 LLControlGroup::getVector3(const LLString& name)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_VEC3))
		return control->get();
	else
	{
		CONTROL_ERRS << "Invalid LLVector3 control " << name << llendl;
		return LLVector3::zero;
	}
}

LLVector3d LLControlGroup::getVector3d(const LLString& name)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_VEC3D))
		return control->get();
	else
	{
		CONTROL_ERRS << "Invalid LLVector3d control " << name << llendl;
		return LLVector3d::zero;
	}
}

LLRect LLControlGroup::getRect(const LLString& name)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_RECT))
		return control->get();
	else
	{
		CONTROL_ERRS << "Invalid rect control " << name << llendl;
		return LLRect::null;
	}
}


LLColor4 LLControlGroup::getColor(const LLString& name)
{
	ctrl_name_table_t::const_iterator i = mNameTable.find(name);

	if (i != mNameTable.end())
	{
		LLControlBase* control = i->second;

		switch(control->mType)
		{
		case TYPE_COL4:
			{
				return LLColor4(control->get());
			}
		case TYPE_COL4U:
			{
				return LLColor4(LLColor4U(control->get()));
			}
		default:
			{
				CONTROL_ERRS << "Control " << name << " not a color" << llendl;
				return LLColor4::white;
			}
		}
	}
	else
	{
		CONTROL_ERRS << "Invalid getColor control " << name << llendl;
		return LLColor4::white;
	}
}

LLColor4U LLControlGroup::getColor4U(const LLString& name)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_COL4U))
		return control->get();
	else
	{
		CONTROL_ERRS << "Invalid LLColor4 control " << name << llendl;
		return LLColor4U::white;
	}
}

LLColor4 LLControlGroup::getColor4(const LLString& name)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_COL4))
		return control->get();
	else
	{
		CONTROL_ERRS << "Invalid LLColor4 control " << name << llendl;
		return LLColor4::white;
	}
}

LLColor3 LLControlGroup::getColor3(const LLString& name)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_COL3))
		return control->get();
	else
	{
		CONTROL_ERRS << "Invalid LLColor3 control " << name << llendl;
		return LLColor3::white;
	}
}

BOOL LLControlGroup::controlExists(const LLString& name)
{
	void *control = mNameTable[name];

	return (control != 0);
}

//-------------------------------------------------------------------
// Set functions
//-------------------------------------------------------------------

void LLControlGroup::setBOOL(const LLString& name, BOOL val)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_BOOLEAN))
	{
		control->set(val);
	}
	else
	{
		CONTROL_ERRS << "Invalid control " << name << llendl;
	}
}


void LLControlGroup::setS32(const LLString& name, S32 val)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_S32))
	{
		control->set(val);
	}
	else
	{
		CONTROL_ERRS << "Invalid control " << name << llendl;
	}
}


void LLControlGroup::setF32(const LLString& name, F32 val)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_F32))
	{
		control->set(val);
	}
	else
	{
		CONTROL_ERRS << "Invalid control " << name << llendl;
	}
}


void LLControlGroup::setU32(const LLString& name, U32 val)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_U32))
	{
		control->set((LLSD::Integer) val);
	}
	else
	{
		CONTROL_ERRS << "Invalid control " << name << llendl;
	}
}


void LLControlGroup::setString(const LLString& name, const LLString &val)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_STRING))
	{
		control->set(val);
	}
	else
	{
		CONTROL_ERRS << "Invalid control " << name << llendl;
	}
}


void LLControlGroup::setVector3(const LLString& name, const LLVector3 &val)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_VEC3))
	{
		control->set(val.getValue());
	}
	else
	{
		CONTROL_ERRS << "Invalid control " << name << llendl;
	}
}

void LLControlGroup::setVector3d(const LLString& name, const LLVector3d &val)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_VEC3D))
	{
		control->set(val.getValue());
	}
	else
	{
		CONTROL_ERRS << "Invalid control " << name << llendl;
	}
}

void LLControlGroup::setRect(const LLString& name, const LLRect &val)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_RECT))
	{
		control->set(val.getValue());
	}
	else
	{
		CONTROL_ERRS << "Invalid rect control " << name << llendl;
	}
}

void LLControlGroup::setColor4U(const LLString& name, const LLColor4U &val)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_COL4U))
	{
		control->set(val.getValue());
	}
	else
	{
		CONTROL_ERRS << "Invalid LLColor4 control " << name << llendl;
	}
}

void LLControlGroup::setColor4(const LLString& name, const LLColor4 &val)
{
	LLControlBase* control = mNameTable[name];
	
	if (control && control->isType(TYPE_COL4))
	{
		control->set(val.getValue());
	}
	else
	{
		CONTROL_ERRS << "Invalid LLColor4 control " << name << llendl;
	}
}

void LLControlGroup::setValue(const LLString& name, const LLSD& val)
{
	if (name.empty())
	{
		return;
	}

	LLControlBase* control = mNameTable[name];
	
	if (control)
	{
		control->set(val);
	}
	else
	{
		CONTROL_ERRS << "Invalid control " << name << llendl;
	}
}

//---------------------------------------------------------------
// Load and save
//---------------------------------------------------------------

U32 LLControlGroup::loadFromFileLegacy(const LLString& filename, BOOL require_declaration, eControlType declare_as)
{
	U32		item = 0;
	U32		validitems = 0;
	llifstream file;
	S32 version;
	
	file.open(filename.c_str());		/*Flawfinder: ignore*/ 

	if (!file)
	{
		llinfos << "LLControlGroup::loadFromFile unable to open." << llendl;
		return 0;
	}

	// Check file version
	LLString name;
	file >> name;
	file >> version;
	if (name != "version" || version != CURRENT_VERSION)
	{
		llinfos << filename << " does not appear to be a version " << CURRENT_VERSION << " controls file" << llendl;
		return 0;
	}

	while (!file.eof())
	{
		file >> name;
		
		if (name.empty())
		{
			continue;
		}

		if (name.substr(0,2) == "//")
		{
			// This is a comment.
			char buffer[MAX_STRING];		/*Flawfinder: ignore*/
			file.getline(buffer, MAX_STRING);
			continue;
		}

		BOOL declared = mNameTable.find(name) != mNameTable.end();

		if (require_declaration && !declared)
		{
			// Declaration required, but this name not declared.
			// Complain about non-empty names.
			if (!name.empty())
			{
				//read in to end of line
				char buffer[MAX_STRING];		/*Flawfinder: ignore*/
				file.getline(buffer, MAX_STRING);
				llwarns << "LLControlGroup::loadFromFile() : Trying to set \"" << name << "\", setting doesn't exist." << llendl;
			}
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
				declareColor4(name, LLColor4::white, LLString::null, NO_PERSIST);
				break;
			case TYPE_COL4U:
				declareColor4U(name, LLColor4U::white, LLString::null, NO_PERSIST);
				break;
			case TYPE_STRING:
			default:
				declareString(name, LLString::null, LLString::null, NO_PERSIST);
				break;
			}
		}

		// Control name has been declared in code.
		LLControlBase *control = getControl(name);

		llassert(control);

		switch(control->mType)
		{
		case TYPE_F32:
			{
				F32 initial;

				file >> initial;

				control->set(initial);
				validitems++;
			}
			break;
		case TYPE_S32:
			{
				S32 initial;

				file >> initial;

				control->set(initial);
				validitems++;
			}
			break;
		case TYPE_U32:
			{
				U32 initial;

				file >> initial;
				control->set((LLSD::Integer) initial);
				validitems++;
			}
			break;
		case TYPE_BOOLEAN:
			{
				char boolstring[256];		/*Flawfinder: ignore*/
				BOOL valid = FALSE;
				BOOL initial = FALSE;

				file >> boolstring;
				if (!strcmp("TRUE", boolstring))
				{
					initial = TRUE;
					valid = TRUE;
				}
				else if (!strcmp("FALSE", boolstring))
				{
					initial = FALSE;
					valid = TRUE;
				}

				if (valid)
				{
					control->set(initial);
				}
				else
				{
					llinfos << filename << "Item " << item << ": Invalid BOOL control " << name << ", " << boolstring << llendl; 
				}

				validitems++;
			}
			break;
		case TYPE_STRING:
			{
				LLString string;
				
				file >> string;
				
				control->set(string);
				validitems++;
			}
			break;
		case TYPE_VEC3:
			{
				F32 x, y, z;

				file >> x >> y >> z;

				LLVector3 vector(x, y, z);

				control->set(vector.getValue());
				validitems++;
			}
			break;
		case TYPE_VEC3D:
			{
				F64 x, y, z;

				file >> x >> y >> z;

				LLVector3d vector(x, y, z);

				control->set(vector.getValue());
				validitems++;
			}
			break;
		case TYPE_RECT:
			{
				S32 left, bottom, width, height;

				file >> left >> bottom >> width >> height;

				LLRect rect;
				rect.setOriginAndSize(left, bottom, width, height);

				control->set(rect.getValue());
				validitems++;
			}
			break;
		case TYPE_COL4U:
			{
				S32 red, green, blue, alpha;
				LLColor4U color;
				file >> red >> green >> blue >> alpha;
				color.setVec(red, green, blue, alpha);
				control->set(color.getValue());
				validitems++;
			}
			break;
		case TYPE_COL4:
			{
				LLColor4 color;
				file >> color.mV[VRED] >> color.mV[VGREEN]
					 >> color.mV[VBLUE] >> color.mV[VALPHA];
				control->set(color.getValue());
				validitems++;
			}
			break;
		case TYPE_COL3:
			{
				LLColor3 color;
				file >> color.mV[VRED] >> color.mV[VGREEN]
					 >> color.mV[VBLUE];
				control->set(color.getValue());
				validitems++;
			}
			break;
		}
	}

	file.close();

	return validitems;
}

// Returns number of controls loaded, so 0 if failure
U32 LLControlGroup::loadFromFile(const LLString& filename, BOOL require_declaration, eControlType declare_as)
{
	LLString name;

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
		
		BOOL declared = (mNameTable[name].notNull());

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
				declareColor4(name, LLColor4::white, "", NO_PERSIST);
				break;
			case TYPE_COL4U:
				declareColor4U(name, LLColor4U::white, "", NO_PERSIST);
				break;
			case TYPE_STRING:
			default:
				declareString(name, LLString::null, "", NO_PERSIST);
				break;
			}
		}

		// Control name has been declared in code.
		LLControlBase *control = getControl(name);

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
				LLString string;
				child_nodep->getAttributeString("value", string);
				if (string == LLString::null)
				{
					string = "";
				}
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
				LLString rect_string;

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
		case TYPE_COL4U:
			{
				LLColor4U color;

				child_nodep->getAttributeColor4U("value", color);
				control->set(color.getValue());
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
		}

		child_nodep = rootp->getNextChild();
	}

	return validitems;
}


U32 LLControlGroup::saveToFile(const LLString& filename, BOOL nondefault_only)
{
	const char ENDL = '\n';

	llinfos << "Saving settings to file: " << filename << llendl;

	// place the objects in a temporary container that enforces a sort
	// order to ease manual editing of the file
	LLLinkedList< LLControlBase > controls;
	controls.setInsertBefore( &control_insert_before );
	LLString name;
	for (ctrl_name_table_t::iterator iter = mNameTable.begin();
		 iter != mNameTable.end(); iter++)
	{
		name = iter->first;
		if (name.empty())
		{
			CONTROL_ERRS << "Control with no name found!!!" << llendl;
			break;
		}

		LLControlBase* control = (LLControlBase *)mNameTable[name];

		if (!control)
		{
			llwarns << "Tried to save invalid control: " << name << llendl;
		}

		if( control && control->mPersist )
		{
			if (!(nondefault_only && (control->mIsDefault)))
			{
				controls.addDataSorted( control );
			}
			else
			{
				// Debug spam
				// llinfos << "Skipping " << control->getName() << llendl;
			}
		}
	}

	llofstream file;
	file.open(filename.c_str());		/*Flawfinder: ignore*/

	if (!file.is_open())
	{
		// This is a warning because sometime we want to use settings files which can't be written...
		llwarns << "LLControlGroup::saveToFile unable to open file for writing" << llendl;
		return 0;
	}

	// Write file version
	file << "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>\n";
	file << "<settings version = \"" << CURRENT_VERSION << "\">\n";
    for( LLControlBase* control = controls.getFirstData();
		 control != NULL;
		 control = controls.getNextData() )
	{
		file << "\t<!--" << control->comment() << "-->" << ENDL;
		name = control->name();
		switch (control->type())
		{
			case TYPE_U32:
			{
				file << "\t<" << name << " value=\"" << (U32) control->get().asInteger() << "\"/>\n";
				break;
			}
			case TYPE_S32:
			{
				file << "\t<" << name << " value=\"" << (S32) control->get().asInteger() << "\"/>\n";
				break;
			}
			case TYPE_F32:
			{
				file << "\t<" << name << " value=\"" << (F32) control->get().asReal() << "\"/>\n";
				break;
			}
			case TYPE_VEC3:
			{
				LLVector3 vector(control->get());
				file << "\t<" << name << " value=\"" << vector.mV[VX] << " " << vector.mV[VY] << " " << vector.mV[VZ] << "\"/>\n";
				break;
			}
			case TYPE_VEC3D:
			{
				LLVector3d vector(control->get());
				file << "\t<" << name << " value=\"" << vector.mdV[VX] << " " << vector.mdV[VY] << " " << vector.mdV[VZ] << "\"/>\n";
				break;
			}
			case TYPE_RECT:
			{
				LLRect rect(control->get());
				file << "\t<" << name << " value=\"" << rect.mLeft << " " << rect.mBottom << " " << rect.getWidth() << " " << rect.getHeight() << "\"/>\n";
				break;
			}
			case TYPE_COL4:
			{
				LLColor4 color(control->get());
				file << "\t<" << name << " value=\"" << color.mV[VRED] << ", " << color.mV[VGREEN] << ", " << color.mV[VBLUE] << ", " << color.mV[VALPHA] << "\"/>\n";
				break;
			}
			case TYPE_COL3:
			{
				LLColor3 color(control->get());
				file << "\t<" << name << " value=\"" << color.mV[VRED] << ", " << color.mV[VGREEN] << ", " << color.mV[VBLUE] << "\"/>\n";
				break;
			}
			case TYPE_BOOLEAN:
			{
				file << "\t<" << name << " value=\"" << (control->get().asBoolean() ? "TRUE" : "FALSE") << "\"/>\n";			
				break;
			}
			case TYPE_STRING:
			{
				file << "\t<" << name << " value=\"" << LLSDXMLFormatter::escapeString(control->get().asString()) << "\"/>\n";
				break;
			}
			default:
			{
				CONTROL_ERRS << "LLControlGroup::saveToFile - unknown control type!" << llendl;
				break;
			}
		}

		// Debug spam
		// llinfos << name << " " << control->getValue().asString() << llendl;
	}// next

	file << "</settings>\n";
	file.close();

	return controls.getLength();
}

void LLControlGroup::applyOverrides(const std::map<std::string, std::string>& overrides)
{
	for (std::map<std::string, std::string>::const_iterator iter = overrides.begin();
		 iter != overrides.end(); ++iter)
	{
		const std::string& command = iter->first;
		const std::string& value = iter->second;
		LLControlBase* control = (LLControlBase *)mNameTable[command];
		if (control)
		{
			switch(control->mType)
			{
			case TYPE_U32:
				control->set((LLSD::Integer)atof(value.c_str()));
				break;
			case TYPE_S32:
				control->set((S32)atof(value.c_str()));
				break;
			case TYPE_F32:
				control->set((F32)atof(value.c_str()));
				break;
			case TYPE_BOOLEAN:
			  	if (!LLString::compareInsensitive(value.c_str(), "TRUE"))
				{
					control->set(TRUE);
				} 
				else if (!LLString::compareInsensitive(value.c_str(), "FALSE"))
				{
					control->set(FALSE);
				}
				else
				{
					control->set((BOOL)atof(value.c_str()));
				}
				break;
			case TYPE_STRING:
				control->set(value);
				break;
//			// *FIX: implement this given time and need.
//			case TYPE_UUID:
//				break;
			// we don't support command line overrides of vec3 or col4
			// yet - requires parsing of multiple values
			case TYPE_VEC3:
			case TYPE_VEC3D:
			case TYPE_COL4:
			case TYPE_COL3:
			default:
				break;
			}
		}
		else
		{
			llinfos << "There is no control variable " << command << llendl;
		}
	}
}

void LLControlGroup::resetToDefaults()
{
	ctrl_name_table_t::iterator control_iter;
	for (control_iter = mNameTable.begin();
		control_iter != mNameTable.end();
		++control_iter)
	{
		LLControlBase* control = (*control_iter).second;
		control->resetToDefault();
	}
}

//============================================================================
// FIrst-use


void LLControlGroup::addWarning(const LLString& name)
{
	LLString warnname = "Warn" + name;
	if(!mNameTable[warnname])
	{
		LLString comment = LLString("Enables ") + name + LLString(" warning dialog");
		declareBOOL(warnname, TRUE, comment);
		mWarnings.insert(warnname);
	}
}

BOOL LLControlGroup::getWarning(const LLString& name)
{
	LLString warnname = "Warn" + name;
	return getBOOL(warnname);
}

void LLControlGroup::setWarning(const LLString& name, BOOL val)
{
	LLString warnname = "Warn" + name;
	setBOOL(warnname, val);
}

void LLControlGroup::resetWarnings()
{
	for (std::set<LLString>::iterator iter = mWarnings.begin();
		 iter != mWarnings.end(); ++iter)
	{
		setBOOL(*iter, TRUE);
	}
}



//=============================================================================
// Listener ID generator/management

void	LLControlBase::releaseListenerID(S32	id)
{	
	mFreeIDs.push_back(id);
}

S32	LLControlBase::allocateListenerID()
{	
	if(mFreeIDs.size() == 0)
	{	//Out of IDs so generate some new ones.
		for(int t=0;t<32;t++)
		{
			mFreeIDs.push_back(mTopID++);
		}
	}
	S32	rtn = mFreeIDs.front();
	mFreeIDs.pop_front();
	mUsedIDs.push_back(rtn);
	return rtn;
}

bool LLControlBase::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	if (event->desc() == "value_changed")
	{
		setValue(((LLValueChangedEvent*)(LLEvent*)event)->mValue);
		return TRUE;
	}
	return TRUE;
}

void LLControlBase::firePropertyChanged()
{
	LLValueChangedEvent *evt = new LLValueChangedEvent(this, getValue());
	fireEvent(evt, "");
}

//============================================================================
// Used to add a listener callback that will be called on the frame that the controls value changes

S32 LLControl::addListener(LLControl::tListenerCallback* cbfn)
{
	S32	id = allocateListenerID();
	mListeners.push_back(cbfn);
	mListenerIDs.push_back( id );
	return id;
}

void LLControl::updateListeners() {
	LLControl::tPropertyChangedListIter iter = mChangeEvents.begin();
	while(iter!=mChangeEvents.end()){
		LLControl::tPropertyChangedEvent&	evt = *iter;
		(*evt.mCBFN)(evt.mNewValue,evt.mID,*this);
		iter++;
	}
	mChangeEvents.clear();
}

//static
void	LLControlBase::updateAllListeners()
{
	std::set< LLControlBase* >::iterator iter = mChangedControls.begin();
	while(iter != mChangedControls.end()){
		(*iter)->updateListeners();
		iter++;
	}
	mChangedControls.clear();
}

LLControl::LLControl(
	const LLString& name,
	eControlType type,
	LLSD initial,
	const LLString& comment,
	BOOL persist) :
	LLControlBase(name, type, comment, persist),
	mCurrent(initial),
	mDefault(initial)
{
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
	foo = new LLControl<F32>("gFoo", 5.f, 1.f, 20.f);
	gGlobals.addEntry("gFoo", foo);

	bar = new LLControl<S32>("gBar", 10, 2, 22);
	gGlobals.addEntry("gBar", bar);

	baz = new LLControl<BOOL>("gBaz", FALSE);
	gGlobals.addEntry("gBaz", baz);

	// test retrieval
	getfoo = (LLControl<F32>*) gGlobals.resolveName("gFoo");
	getfoo->dump();

	getbar = (S32_CONTROL) gGlobals.resolveName("gBar");
	getbar->dump();

	// change data
	getfoo->set(10.f);
	getfoo->dump();

	// Failure modes

	// ...min > max
	// badfoo = new LLControl<F32>("gFoo2", 100.f, 20.f, 5.f);

	// ...initial > max
	// badbar = new LLControl<S32>("gBar2", 10, 20, 100000);

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

BOOL control_insert_before( LLControlBase* first, LLControlBase* second )
{
	return ( first->getName().compare(second->getName()) < 0 );
}

