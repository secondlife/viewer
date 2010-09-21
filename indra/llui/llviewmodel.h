/**
 * @file   llviewmodel.h
 * @author Nat Goodspeed
 * @date   2008-08-08
 * @brief  Define "View Model" classes intended to store data values for use
 *         by LLUICtrl subclasses. The phrase is borrowed from Microsoft
 *         terminology, in which "View Model" means the storage object
 *         underlying a specific widget object -- as in our case -- rather
 *         than the business "model" object underlying the overall "view"
 *         presented by the collection of widgets.
 * 
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#if ! defined(LL_LLVIEWMODEL_H)
#define LL_LLVIEWMODEL_H

#include "llpointer.h"
#include "llsd.h"
#include "llrefcount.h"
#include "stdenums.h"
#include "llstring.h"
#include <string>

class LLScrollListItem;

class LLViewModel;
class LLTextViewModel;
class LLListViewModel;
// Because LLViewModel is derived from LLRefCount, always pass, store
// and return LLViewModelPtr rather than plain LLViewModel*.
typedef LLPointer<LLViewModel> LLViewModelPtr;
typedef LLPointer<LLTextViewModel> LLTextViewModelPtr;
typedef LLPointer<LLListViewModel> LLListViewModelPtr;

/**
 * LLViewModel stores a scalar LLSD data item, the current display value of a
 * scalar LLUICtrl widget. LLViewModel subclasses are used to store data
 * collections used for aggregate widgets. LLViewModel is ref-counted because
 * -- for multiple skins -- we may have distinct widgets sharing the same
 * LLViewModel data. This way, the LLViewModel is quietly deleted when the
 * last referencing widget is destroyed.
 */
class LLViewModel: public LLRefCount
{
public:
    LLViewModel();
    /// Instantiate an LLViewModel with an existing data value
    LLViewModel(const LLSD& value);

    /// Update the stored value
    virtual void setValue(const LLSD& value);
    /// Get the stored value, in appropriate type.
    virtual LLSD getValue() const;

    /// Has the value been changed since last time we checked?
    bool isDirty() const { return mDirty; }
    /// Once the value has been saved to a file, or otherwise consumed by the
    /// app, we no longer need to enable the Save button
    void resetDirty() { mDirty = false; }
	// 
    void setDirty() { mDirty = true; }

protected:
    LLSD mValue;
    bool mDirty;
};

/**
 * LLTextViewModel stores a value displayed as text. 
 */
class LLTextViewModel: public LLViewModel
{
public:
    LLTextViewModel();
    /// Instantiate an LLViewModel with an existing data value
    LLTextViewModel(const LLSD& value);
	
	// LLViewModel functions
    virtual void setValue(const LLSD& value);
    virtual LLSD getValue() const;

	// New functions
    /// Get the stored value in string form
    const LLWString& getDisplay() const { return mDisplay; }

    /**
     * Set the display string directly (see LLTextEditor). What the user is
     * editing is actually the LLWString value rather than the underlying
     * UTF-8 value.
     */
    void setDisplay(const LLWString& value);
	
private:
    /// To avoid converting every widget's stored value from LLSD to LLWString
    /// every frame, cache the converted value
    LLWString mDisplay;
    /// As the user edits individual characters (setDisplay()), defer
    /// LLWString-to-UTF8 conversions until s/he's done.
    bool mUpdateFromDisplay;
};

/**
 * LLListViewModel stores a list of data items. The semantics are borrowed
 * from LLScrollListCtrl.
 */
class LLListViewModel: public LLViewModel
{
public:
    LLListViewModel() {}
    LLListViewModel(const LLSD& values);

    virtual void addColumn(const LLSD& column, EAddPosition pos = ADD_BOTTOM);
    virtual void clearColumns();
    virtual void setColumnLabel(const std::string& column, const std::string& label);
    virtual LLScrollListItem* addElement(const LLSD& value, EAddPosition pos = ADD_BOTTOM,
                                         void* userdata = NULL);
    virtual LLScrollListItem* addSimpleElement(const std::string& value, EAddPosition pos,
                                               const LLSD& id);
    virtual void clearRows();
    virtual void sortByColumn(const std::string& name, bool ascending);
};

//namespace LLViewModel
//{
//	class Value
//	{
//	public:
//		Value(const LLSD& value = LLSD());
//
//		LLSD getValue() const { return mValue; }
//		void setValue(const LLSD& value) { mValue = value; }
//
//		bool isAvailable() const { return false; }
//		bool isReadOnly() const { return false; }
//
//		bool undo() { return false; }
//		bool redo() { return false; }
//
//	    /// Has the value been changed since last time we checked?
//		bool isDirty() const { return mDirty; }
//		/// Once the value has been saved to a file, or otherwise consumed by the
//		/// app, we no longer need to enable the Save button
//		void resetDirty() { mDirty = false; }
//		// 
//		void setDirty() { mDirty = true; }
//
//	protected:
//		LLSD	mValue;
//		bool mDirty;
//	};
//
//	class Numeric : public Value
//	{
//	public:
//		Numeric(S32 value = 0);
//		Numeric(F32 value);
//
//		F32 getPrecision();
//		F32 getMin();
//		F32 getMax();
//
//		void increment();
//		void decrement();
//	};
//
//	class MultipleValues : public Value
//	{
//		class Selector
//		{};
//
//		MultipleValues();
//		virtual S32 numElements();
//	};
//
//	class Tuple : public MultipleValues
//	{
//		Tuple(S32 size);
//		LLSD getValue(S32 which) const;
//		void setValue(S32 which, const LLSD& value);
//	};
//
//	class List : public MultipleValues
//	{
//		List();
//
//		void add(const ValueModel& value);
//		bool remove(const Selector& item);
//
//		void setSortElement(const Selector& element);
//		void sort();
//	};
//
//};
#endif /* ! defined(LL_LLVIEWMODEL_H) */
