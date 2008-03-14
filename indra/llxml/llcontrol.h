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

class LLControlVariable
{
	friend class LLControlGroup;
	typedef boost::signal<void(const LLSD&)> signal_t;

private:
	LLString		mName;
	LLString		mComment;
	eControlType	mType;
	BOOL			mPersist;
	std::vector<LLSD> mValues;
	
	signal_t mSignal;
	
public:
	LLControlVariable(const LLString& name, eControlType type,
					  LLSD initial, const LLString& comment,
					  BOOL persist = TRUE);

	virtual ~LLControlVariable();
	
	const LLString& getName() const { return mName; }
	const LLString& getComment() const { return mComment; }

	eControlType type()		{ return mType; }
	BOOL isType(eControlType tp) { return tp == mType; }

	void resetToDefault(bool fire_signal = TRUE);

	signal_t* getSignal() { return &mSignal; }

	bool isDefault() { return (mValues.size() == 1); }
	bool isSaveValueDefault();
	bool isPersisted() { return mPersist; }
	void set(const LLSD& val)	{ setValue(val); }
	LLSD get()			const	{ return getValue(); }
	LLSD getDefault()	const	{ return mValues.front(); }
	LLSD getValue()		const	{ return mValues.back(); }
	LLSD getSaveValue() const;
	void setValue(const LLSD& value, bool saved_value = TRUE);
	void firePropertyChanged()
	{
		mSignal(mValues.back());
	}
	BOOL llsd_compare(const LLSD& a, const LLSD& b);
};

//const U32 STRING_CACHE_SIZE = 10000;
class LLControlGroup
{
protected:
	typedef std::map<LLString, LLControlVariable* > ctrl_name_table_t;
	ctrl_name_table_t mNameTable;
	std::set<LLString> mWarnings;
	LLString mTypeString[TYPE_COUNT];

	eControlType typeStringToEnum(const LLString& typestr);
	LLString typeEnumToString(eControlType typeenum);	
public:
	LLControlGroup();
	~LLControlGroup();
	void cleanup();
	
	LLControlVariable*	getControl(const LLString& name);

	struct ApplyFunctor
	{
		virtual ~ApplyFunctor() {};
		virtual void apply(const LLString& name, LLControlVariable* control) = 0;
	};
	void applyToAll(ApplyFunctor* func);
	
	BOOL declareControl(const LLString& name, eControlType type, const LLSD initial_val, const LLString& comment, BOOL persist);
	BOOL declareU32(const LLString& name, U32 initial_val, const LLString& comment, BOOL persist = TRUE);
	BOOL declareS32(const LLString& name, S32 initial_val, const LLString& comment, BOOL persist = TRUE);
	BOOL declareF32(const LLString& name, F32 initial_val, const LLString& comment, BOOL persist = TRUE);
	BOOL declareBOOL(const LLString& name, BOOL initial_val, const LLString& comment, BOOL persist = TRUE);
	BOOL declareString(const LLString& name, const LLString &initial_val, const LLString& comment, BOOL persist = TRUE);
	BOOL declareVec3(const LLString& name, const LLVector3 &initial_val,const LLString& comment,  BOOL persist = TRUE);
	BOOL declareVec3d(const LLString& name, const LLVector3d &initial_val, const LLString& comment, BOOL persist = TRUE);
	BOOL declareRect(const LLString& name, const LLRect &initial_val, const LLString& comment, BOOL persist = TRUE);
	BOOL declareColor4U(const LLString& name, const LLColor4U &initial_val, const LLString& comment, BOOL persist = TRUE);
	BOOL declareColor4(const LLString& name, const LLColor4 &initial_val, const LLString& comment, BOOL persist = TRUE);
	BOOL declareColor3(const LLString& name, const LLColor3 &initial_val, const LLString& comment, BOOL persist = TRUE);
	BOOL declareLLSD(const LLString& name, const LLSD &initial_val, const LLString& comment, BOOL persist = TRUE);
	
	LLString 	findString(const LLString& name);

	LLString 	getString(const LLString& name);
	LLWString	getWString(const LLString& name);
	LLString	getText(const LLString& name);
	LLVector3	getVector3(const LLString& name);
	LLVector3d	getVector3d(const LLString& name);
	LLRect		getRect(const LLString& name);
	BOOL		getBOOL(const LLString& name);
	S32			getS32(const LLString& name);
	F32			getF32(const LLString& name);
	U32			getU32(const LLString& name);
	LLSD        getLLSD(const LLString& name);


	// Note: If an LLColor4U control exists, it will cast it to the correct
	// LLColor4 for you.
	LLColor4	getColor(const LLString& name);
	LLColor4U	getColor4U(const LLString& name);
	LLColor4	getColor4(const LLString& name);
	LLColor3	getColor3(const LLString& name);

	void	setBOOL(const LLString& name, BOOL val);
	void	setS32(const LLString& name, S32 val);
	void	setF32(const LLString& name, F32 val);
	void	setU32(const LLString& name, U32 val);
	void	setString(const LLString&  name, const LLString& val);
	void	setVector3(const LLString& name, const LLVector3 &val);
	void	setVector3d(const LLString& name, const LLVector3d &val);
	void	setRect(const LLString& name, const LLRect &val);
	void	setColor4U(const LLString& name, const LLColor4U &val);
	void	setColor4(const LLString& name, const LLColor4 &val);
	void	setColor3(const LLString& name, const LLColor3 &val);
	void    setLLSD(const LLString& name, const LLSD& val);
	void	setValue(const LLString& name, const LLSD& val);
	
	
	BOOL    controlExists(const LLString& name);

	// Returns number of controls loaded, 0 if failed
	// If require_declaration is false, will auto-declare controls it finds
	// as the given type.
	U32	loadFromFileLegacy(const LLString& filename, BOOL require_declaration = TRUE, eControlType declare_as = TYPE_STRING);
 	U32 saveToFile(const LLString& filename, BOOL nondefault_only);
 	U32	loadFromFile(const LLString& filename, BOOL require_declaration = TRUE, eControlType declare_as = TYPE_STRING);
	void	resetToDefaults();

	
	// Ignorable Warnings
	
	// Add a config variable to be reset on resetWarnings()
	void addWarning(const LLString& name);
	BOOL getWarning(const LLString& name);
	void setWarning(const LLString& name, BOOL val);
	
	// Resets all ignorables
	void resetWarnings();
};

#endif
