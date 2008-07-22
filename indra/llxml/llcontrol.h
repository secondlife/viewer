/** 
 * @file llcontrol.h
 * @brief A mechanism for storing "control state" for a program
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLCONTROL_H
#define LL_LLCONTROL_H

#include "llevent.h"
#include "llnametable.h"
#include "llmap.h"
#include "llstring.h"
#include "llrect.h"

#include <vector>

// *NOTE: boost::visit_each<> generates warning 4675 on .net 2003
// Disable the warning for the boost includes.
#if LL_WINDOWS
# if (_MSC_VER >= 1300 && _MSC_VER < 1400)
#   pragma warning(push)
#   pragma warning( disable : 4675 )
# endif
#endif

#include <boost/bind.hpp>
#include <boost/signal.hpp>

#if LL_WINDOWS
# if (_MSC_VER >= 1300 && _MSC_VER < 1400)
#   pragma warning(pop)
# endif
#endif

class LLVector3;
class LLVector3d;
class LLColor4;
class LLColor3;
class LLColor4U;

const BOOL NO_PERSIST = FALSE;

typedef enum e_control_type
{
	TYPE_U32 = 0,
	TYPE_S32,
	TYPE_F32,
	TYPE_BOOLEAN,
	TYPE_STRING,
	TYPE_VEC3,
	TYPE_VEC3D,
	TYPE_RECT,
	TYPE_COL4,
	TYPE_COL3,
	TYPE_COL4U,
	TYPE_LLSD,
	TYPE_COUNT
} eControlType;

class LLControlVariable : public LLRefCount
{
	friend class LLControlGroup;
	typedef boost::signal<void(const LLSD&)> signal_t;

private:
	std::string		mName;
	std::string		mComment;
	eControlType	mType;
	bool mPersist;
	std::vector<LLSD> mValues;
	
	signal_t mSignal;
	
public:
	LLControlVariable(const std::string& name, eControlType type,
					  LLSD initial, const std::string& comment,
					  bool persist = true);

	virtual ~LLControlVariable();
	
	const std::string& getName() const { return mName; }
	const std::string& getComment() const { return mComment; }

	eControlType type()		{ return mType; }
	bool isType(eControlType tp) { return tp == mType; }

	void resetToDefault(bool fire_signal = false);

	signal_t* getSignal() { return &mSignal; }

	bool isDefault() { return (mValues.size() == 1); }
	bool isSaveValueDefault();
	bool isPersisted() { return mPersist; }
	LLSD get()			const	{ return getValue(); }
	LLSD getValue()		const	{ return mValues.back(); }
	LLSD getDefault()	const	{ return mValues.front(); }
	LLSD getSaveValue() const;

	void set(const LLSD& val)	{ setValue(val); }
	void setValue(const LLSD& value, bool saved_value = TRUE);
	void setDefaultValue(const LLSD& value);
	void setPersist(bool state);
	void setComment(const std::string& comment);

	void firePropertyChanged()
	{
		mSignal(mValues.back());
	}
private:
	LLSD getComparableValue(const LLSD& value);
	bool llsd_compare(const LLSD& a, const LLSD & b);

};

//const U32 STRING_CACHE_SIZE = 10000;
class LLControlGroup
{
protected:
	typedef std::map<std::string, LLPointer<LLControlVariable> > ctrl_name_table_t;
	ctrl_name_table_t mNameTable;
	std::set<std::string> mWarnings;
	std::string mTypeString[TYPE_COUNT];

	eControlType typeStringToEnum(const std::string& typestr);
	std::string typeEnumToString(eControlType typeenum);	
public:
	LLControlGroup();
	~LLControlGroup();
	void cleanup();
	
	LLPointer<LLControlVariable> getControl(const std::string& name);

	struct ApplyFunctor
	{
		virtual ~ApplyFunctor() {};
		virtual void apply(const std::string& name, LLControlVariable* control) = 0;
	};
	void applyToAll(ApplyFunctor* func);
	
	BOOL declareControl(const std::string& name, eControlType type, const LLSD initial_val, const std::string& comment, BOOL persist);
	BOOL declareU32(const std::string& name, U32 initial_val, const std::string& comment, BOOL persist = TRUE);
	BOOL declareS32(const std::string& name, S32 initial_val, const std::string& comment, BOOL persist = TRUE);
	BOOL declareF32(const std::string& name, F32 initial_val, const std::string& comment, BOOL persist = TRUE);
	BOOL declareBOOL(const std::string& name, BOOL initial_val, const std::string& comment, BOOL persist = TRUE);
	BOOL declareString(const std::string& name, const std::string &initial_val, const std::string& comment, BOOL persist = TRUE);
	BOOL declareVec3(const std::string& name, const LLVector3 &initial_val,const std::string& comment,  BOOL persist = TRUE);
	BOOL declareVec3d(const std::string& name, const LLVector3d &initial_val, const std::string& comment, BOOL persist = TRUE);
	BOOL declareRect(const std::string& name, const LLRect &initial_val, const std::string& comment, BOOL persist = TRUE);
	BOOL declareColor4U(const std::string& name, const LLColor4U &initial_val, const std::string& comment, BOOL persist = TRUE);
	BOOL declareColor4(const std::string& name, const LLColor4 &initial_val, const std::string& comment, BOOL persist = TRUE);
	BOOL declareColor3(const std::string& name, const LLColor3 &initial_val, const std::string& comment, BOOL persist = TRUE);
	BOOL declareLLSD(const std::string& name, const LLSD &initial_val, const std::string& comment, BOOL persist = TRUE);
	
	std::string 	findString(const std::string& name);

	std::string 	getString(const std::string& name);
	LLWString	getWString(const std::string& name);
	std::string	getText(const std::string& name);
	LLVector3	getVector3(const std::string& name);
	LLVector3d	getVector3d(const std::string& name);
	LLRect		getRect(const std::string& name);
	BOOL		getBOOL(const std::string& name);
	S32			getS32(const std::string& name);
	F32			getF32(const std::string& name);
	U32			getU32(const std::string& name);
	LLSD        getLLSD(const std::string& name);


	// Note: If an LLColor4U control exists, it will cast it to the correct
	// LLColor4 for you.
	LLColor4	getColor(const std::string& name);
	LLColor4U	getColor4U(const std::string& name);
	LLColor4	getColor4(const std::string& name);
	LLColor3	getColor3(const std::string& name);

	void	setBOOL(const std::string& name, BOOL val);
	void	setS32(const std::string& name, S32 val);
	void	setF32(const std::string& name, F32 val);
	void	setU32(const std::string& name, U32 val);
	void	setString(const std::string&  name, const std::string& val);
	void	setVector3(const std::string& name, const LLVector3 &val);
	void	setVector3d(const std::string& name, const LLVector3d &val);
	void	setRect(const std::string& name, const LLRect &val);
	void	setColor4U(const std::string& name, const LLColor4U &val);
	void	setColor4(const std::string& name, const LLColor4 &val);
	void	setColor3(const std::string& name, const LLColor3 &val);
	void    setLLSD(const std::string& name, const LLSD& val);
	void	setValue(const std::string& name, const LLSD& val);
	
	
	BOOL    controlExists(const std::string& name);

	// Returns number of controls loaded, 0 if failed
	// If require_declaration is false, will auto-declare controls it finds
	// as the given type.
	U32	loadFromFileLegacy(const std::string& filename, BOOL require_declaration = TRUE, eControlType declare_as = TYPE_STRING);
 	U32 saveToFile(const std::string& filename, BOOL nondefault_only);
 	U32	loadFromFile(const std::string& filename, bool default_values = false);
	void	resetToDefaults();

	
	// Ignorable Warnings
	
	// Add a config variable to be reset on resetWarnings()
	void addWarning(const std::string& name);
	BOOL getWarning(const std::string& name);
	void setWarning(const std::string& name, BOOL val);
	
	// Resets all ignorables
	void resetWarnings();
};

#endif
