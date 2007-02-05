/** 
 * @file llcontrol.h
 * @brief A mechanism for storing "control state" for a program
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCONTROL_H
#define LL_LLCONTROL_H

#include "llevent.h"
#include "llnametable.h"
#include "llmap.h"
#include "llstring.h"
#include "llrect.h"

class LLVector3;
class LLVector3d;
class LLColor4;
class LLColor3;
class LLColor4U;

const BOOL NO_PERSIST = FALSE;

typedef enum e_control_type
{
	TYPE_U32,
	TYPE_S32,
	TYPE_F32,
	TYPE_BOOLEAN,
	TYPE_STRING,
	TYPE_VEC3,
	TYPE_VEC3D,
	TYPE_RECT,
	TYPE_COL4,
	TYPE_COL3,
	TYPE_COL4U
} eControlType;

class LLControlBase : public LLSimpleListenerObservable
{
friend class LLControlGroup;
protected:
	LLString		mName;
	LLString		mComment;
	eControlType	mType;
	BOOL			mHasRange;
	BOOL			mPersist;
	BOOL            mIsDefault;

	static	std::set<LLControlBase*>	mChangedControls;
	static	std::list<S32>				mFreeIDs;//These lists are used to store the ID's of registered event listeners.
	static	std::list<S32>				mUsedIDs;
	static	S32							mTopID;//This is the index of the highest ID event listener ID. When the free pool is exhausted, new IDs are allocated from here.

public:
	static	void						releaseListenerID(S32	id);
	static	S32							allocateListenerID();
	static	void						updateAllListeners();
	virtual void updateListeners() = 0;

	LLControlBase(const LLString& name, eControlType type, const LLString& comment, BOOL persist)
		: mName(name),
		mComment(comment),
		mType(type),
		mHasRange(FALSE),
		mPersist(persist),
		mIsDefault(TRUE)
	{ 
		if (mPersist && mComment.empty())
		{
			llerrs << "Must supply a comment for control " << mName << llendl;
		}
		sMaxControlNameLength = llmax((U32)mName.size(), sMaxControlNameLength);
	}

	virtual ~LLControlBase();

	const LLString& getName() const { return mName; }
	const LLString& getComment() const { return mComment; }

	eControlType type()		{ return mType; }
	BOOL	isType(eControlType tp) { return tp == mType; }

	// Defaults to no-op
	virtual void resetToDefault();

	LLSD registerListener(LLSimpleListenerObservable *listener, LLSD userdata = "");

	virtual LLSD get() const = 0;
	virtual LLSD getValue() const = 0;
	virtual void setValue(LLSD value) = 0;
	virtual void set(LLSD value) = 0;

	// From LLSimpleListener
	virtual bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata);

	void firePropertyChanged();

	static U32	sMaxControlNameLength;

protected:
	const char* name()			{ return mName.c_str(); }
	const char* comment()		{ return mComment.c_str(); }
};

class LLControl
: public LLControlBase
{
friend class LLControlGroup;
protected:
	LLSD mCurrent;
	LLSD mDefault;

public:	

	typedef void	tListenerCallback(const LLSD&	newValue,S32	listenerID, LLControl& control);
	typedef struct{
		S32					mID;
		LLSD			mNewValue;
		tListenerCallback*	mCBFN;
	}tPropertyChangedEvent;

	typedef std::list<tPropertyChangedEvent>::iterator tPropertyChangedListIter;
	std::list<tPropertyChangedEvent>	mChangeEvents;
	std::list< tListenerCallback* >		mListeners;
	std::list< S32 >					mListenerIDs;

	virtual void						updateListeners();
	S32									addListener(tListenerCallback*	cbfn);
	
	LLControl(
		const LLString& name,
		eControlType type,
		LLSD initial, const
		LLString& comment,
		BOOL persist = TRUE);

	void set(LLSD val)			{ setValue(val); }
	LLSD get()			const	{ return getValue(); } 
	LLSD getdefault()	const	{ return mDefault; }
	LLSD getValue()		const	{ return mCurrent; }
	BOOL llsd_compare(const LLSD& a, const LLSD& b);

	void setValue(LLSD value)			
	{ 
		if (llsd_compare(mCurrent, value) == FALSE)
		{
			mCurrent = value; 
			mIsDefault = llsd_compare(mCurrent, mDefault); 
			firePropertyChanged(); 
		}
	}

	/*virtual*/ void resetToDefault() 
	{ 
		setValue(mDefault);
	}

	virtual	~LLControl()
	{
		//Remove and deregister all listeners..
		while(mListenerIDs.size())
		{
			S32	id = mListenerIDs.front();
			mListenerIDs.pop_front();
			releaseListenerID(id);
		}
	}
};

//const U32 STRING_CACHE_SIZE = 10000;
class LLControlGroup
{
public:
	typedef std::map<LLString, LLPointer<LLControlBase> > ctrl_name_table_t;
	ctrl_name_table_t mNameTable;
	std::set<LLString> mWarnings;
	
public:
	LLControlGroup();
	~LLControlGroup();
	void cleanup();
	
	LLControlBase*	getControl(const LLString& name);
	LLSD registerListener(const LLString& name, LLSimpleListenerObservable *listener);
	
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
	LLSD		getValue(const LLString& name);


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
	void	setValue(const LLString& name, const LLSD& val);

	BOOL    controlExists(const LLString& name);

	// Returns number of controls loaded, 0 if failed
	// If require_declaration is false, will auto-declare controls it finds
	// as the given type.
	U32		loadFromFileLegacy(const LLString& filename, BOOL require_declaration = TRUE, eControlType declare_as = TYPE_STRING);
	U32		loadFromFile(const LLString& filename, BOOL require_declaration = TRUE, eControlType declare_as = TYPE_STRING);
	U32		saveToFile(const LLString& filename, BOOL skip_if_default);
	void	applyOverrides(const std::map<std::string, std::string>& overrides);
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
