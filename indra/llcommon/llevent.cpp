/** 
 * @file llevent.cpp
 * @brief LLEvent and LLEventListener base classes.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#include "linden_common.h"

#include "llevent.h"

/************************************************
    Events
************************************************/

// virtual
LLEvent::~LLEvent()
{
}

// virtual
bool LLEvent::accept(LLEventListener* listener)
{
	return true;
}

// virtual
const std::string& LLEvent::desc()
{
	return mDesc;
}

/************************************************
    Observables
************************************************/

LLObservable::LLObservable()
	: mDispatcher(new LLEventDispatcher())
{
}

// virtual
LLObservable::~LLObservable()
{
	if (mDispatcher.notNull())
	{
		mDispatcher->disengage(this);
		mDispatcher = NULL;
	}
}

// virtual
bool LLObservable::setDispatcher(LLPointer<LLEventDispatcher> dispatcher)
{
	if (mDispatcher.notNull())
	{
		mDispatcher->disengage(this);
		mDispatcher = NULL;
	}
	if (dispatcher.notNull() || dispatcher->engage(this))
	{
		mDispatcher = dispatcher;
		return true;
	}
	return false;
}

// Returns the current dispatcher pointer.
// virtual
LLEventDispatcher* LLObservable::getDispatcher()
{
	return mDispatcher;
}

// Notifies the dispatcher of an event being fired.
void LLObservable::fireEvent(LLPointer<LLEvent> event, LLSD filter)
{
	if (mDispatcher.notNull())
	{
		mDispatcher->fireEvent(event, filter);
	}
}

/************************************************
    Dispatchers
************************************************/

class LLEventDispatcher::Impl
{
public:
	virtual ~Impl()												{				}
	virtual bool engage(LLObservable* observable)				{ return true;	}
	virtual void disengage(LLObservable* observable)			{				}

	virtual void addListener(LLEventListener *listener, LLSD filter, const LLSD& userdata) = 0;
	virtual void removeListener(LLEventListener *listener) = 0;
	virtual std::vector<LLListenerEntry> getListeners() const = 0;
	virtual bool fireEvent(LLPointer<LLEvent> event, LLSD filter) = 0;
};

bool LLEventDispatcher::engage(LLObservable* observable)
{
	return impl->engage(observable);
}

void LLEventDispatcher::disengage(LLObservable* observable)
{
	impl->disengage(observable);
}

void LLEventDispatcher::addListener(LLEventListener *listener, LLSD filter, const LLSD& userdata)
{
	impl->addListener(listener, filter, userdata);
}

void LLEventDispatcher::removeListener(LLEventListener *listener)
{
	impl->removeListener(listener);
}

std::vector<LLListenerEntry> LLEventDispatcher::getListeners() const
{
	return impl->getListeners();
}


bool LLEventDispatcher::fireEvent(LLPointer<LLEvent> event, LLSD filter)
{
	return impl->fireEvent(event, filter);
}

class LLSimpleDispatcher : public LLEventDispatcher::Impl
{
public:
	LLSimpleDispatcher(LLEventDispatcher *parent) : mParent(parent) { }
	virtual ~LLSimpleDispatcher();
	virtual void addListener(LLEventListener* listener, LLSD filter, const LLSD& userdata);
	virtual void removeListener(LLEventListener* listener);
	virtual std::vector<LLListenerEntry> getListeners() const;
	virtual bool fireEvent(LLPointer<LLEvent> event, LLSD filter);

protected:
	std::vector<LLListenerEntry> mListeners;
	LLEventDispatcher *mParent;
};

LLSimpleDispatcher::~LLSimpleDispatcher()
{
	while (mListeners.size() > 0)
	{
		removeListener(mListeners.begin()->listener);
	}
}

void LLSimpleDispatcher::addListener(LLEventListener* listener, LLSD filter, const LLSD& userdata)
{
	if (listener == NULL) return;
	removeListener(listener);
	LLListenerEntry new_entry;
	new_entry.listener = listener;
	new_entry.filter = filter;
	new_entry.userdata = userdata;
	mListeners.push_back(new_entry);
	listener->handleAttach(mParent);
}

void LLSimpleDispatcher::removeListener(LLEventListener* listener)
{
	std::vector<LLListenerEntry>::iterator itor = mListeners.begin();
	std::vector<LLListenerEntry>::iterator end = mListeners.end();
	for (; itor != end; ++itor)
	{
		if ((*itor).listener == listener)
		{
			mListeners.erase(itor);
			break;
		}
	}
	listener->handleDetach(mParent);
}

std::vector<LLListenerEntry> LLSimpleDispatcher::getListeners() const
{
	std::vector<LLListenerEntry> ret;
	std::vector<LLListenerEntry>::const_iterator itor;
	for (itor=mListeners.begin(); itor!=mListeners.end(); ++itor)
	{
		ret.push_back(*itor);
	}
	
	return ret;
}

// virtual
bool LLSimpleDispatcher::fireEvent(LLPointer<LLEvent> event, LLSD filter)
{
	std::vector<LLListenerEntry>::iterator itor;
	std::string filter_string = filter.asString();
	for (itor=mListeners.begin(); itor!=mListeners.end(); ++itor)
	{
		LLListenerEntry& entry = *itor;
		if (filter_string == "" || entry.filter.asString() == filter_string)
		{
			(entry.listener)->handleEvent(event, (*itor).userdata);
		}
	}
	return true;
}

LLEventDispatcher::LLEventDispatcher()
{
	impl = new LLSimpleDispatcher(this);
}

LLEventDispatcher::~LLEventDispatcher()
{
	if (impl)
	{
		delete impl;
		impl = NULL;
	}
}

/************************************************
    Listeners
************************************************/

LLEventListener::~LLEventListener()
{
}

LLSimpleListener::~LLSimpleListener()
{
	clearDispatchers();
}

void LLSimpleListener::clearDispatchers()
{
	// Remove myself from all listening dispatchers
	std::vector<LLEventDispatcher *>::iterator itor;
	while (mDispatchers.size() > 0)
	{
		itor = mDispatchers.begin();
		LLEventDispatcher *dispatcher = *itor;
		dispatcher->removeListener(this);
		itor = mDispatchers.begin();
		if (itor != mDispatchers.end() && (*itor) == dispatcher)
		{
			// Somehow, the dispatcher was not removed. Remove it forcibly
			mDispatchers.erase(itor);
		}
	}
}

bool LLSimpleListener::handleAttach(LLEventDispatcher *dispatcher)
{
	// Add dispatcher if it doesn't already exist
	std::vector<LLEventDispatcher *>::iterator itor;
	for (itor = mDispatchers.begin(); itor != mDispatchers.end(); ++itor)
	{
		if ((*itor) == dispatcher) return true;
	}
	mDispatchers.push_back(dispatcher);
	return true;
}

bool LLSimpleListener::handleDetach(LLEventDispatcher *dispatcher)
{
	// Remove dispatcher from list
	std::vector<LLEventDispatcher *>::iterator itor;
	for (itor = mDispatchers.begin(); itor != mDispatchers.end(); )
	{
		if ((*itor) == dispatcher)
		{
			itor = mDispatchers.erase(itor);
		}
		else
		{
			++itor;
		}
	}
	return true;
}
