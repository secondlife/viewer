/** 
 * @file llevent.h
 * @author Tom Yedwab
 * @brief LLEvent and LLEventListener base classes.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_EVENT_H
#define LL_EVENT_H

#include "llsd.h"
#include "llpointer.h"
#include "llthread.h"

namespace LLOldEvents
{

class LLEventListener;
class LLEvent;
class LLEventDispatcher;
class LLObservable;

// Abstract event. All events derive from LLEvent
class LLEvent : public LLThreadSafeRefCount
{
protected:
	virtual ~LLEvent();
	
public:
	LLEvent(LLObservable* source, const std::string& desc = "") : mSource(source), mDesc(desc) { }

	LLObservable* getSource() { return mSource; }
	virtual LLSD		getValue() { return LLSD(); }
	// Determines whether this particular listener
	//   should be notified of this event.
	// If this function returns true, handleEvent is
	//   called on the listener with this event as the
	//   argument.
	// Defaults to handling all events. Override this
	//   if associated with an Observable with many different listeners
	virtual bool accept(LLEventListener* listener);

	// return a string describing the event
	virtual const std::string& desc();

private:
	LLObservable* mSource;
	std::string mDesc;
};

// Abstract listener. All listeners derive from LLEventListener
class LLEventListener : public LLThreadSafeRefCount
{
protected:
	virtual ~LLEventListener();
	
public:

	// Processes the event.
	// TODO: Make the return value less ambiguous?
	virtual bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata) = 0;

	// Called when an dispatcher starts/stops listening
	virtual bool handleAttach(LLEventDispatcher *dispatcher) = 0;
	virtual bool handleDetach(LLEventDispatcher *dispatcher) = 0;
};

// A listener which tracks references to it and cleans up when it's deallocated
class LLSimpleListener : public LLEventListener
{
public:
	void clearDispatchers();
	virtual bool handleAttach(LLEventDispatcher *dispatcher);
	virtual bool handleDetach(LLEventDispatcher *dispatcher);

protected:
	~LLSimpleListener();
	std::vector<LLEventDispatcher *> mDispatchers;
};

class LLObservable; // defined below

// A structure which stores a Listener and its metadata
struct LLListenerEntry
{
	LLEventListener* listener;
	LLSD filter;
	LLSD userdata;
};

// Base class for a dispatcher - an object which listens
// to events being fired and relays them to their
// appropriate destinations.
class LLEventDispatcher : public LLThreadSafeRefCount
{
protected:
	virtual ~LLEventDispatcher();
	
public:
	// The default constructor creates a default simple dispatcher implementation.
	// The simple implementation has an array of listeners and fires every event to
	// all of them.
	LLEventDispatcher();
	
	// This dispatcher is being attached to an observable object.
	// If we return false, the attach fails.
	bool engage(LLObservable* observable);

	// This dispatcher is being detached from an observable object.
	void disengage(LLObservable* observable);

	// Adds a listener to this dispatcher, with a given user data
	// that will be passed to the listener when an event is fired.
	// Duplicate pointers are removed on addtion.
	void addListener(LLEventListener *listener, LLSD filter, const LLSD& userdata);

	// Removes a listener from this dispatcher
	void removeListener(LLEventListener *listener);

	// Gets a list of interested listeners
	std::vector<LLListenerEntry> getListeners() const;

	// Handle an event that has just been fired by communicating it
	// to listeners, passing it across a network, etc.
	bool fireEvent(LLPointer<LLEvent> event, LLSD filter);

public:
	class Impl;
private:
	Impl* impl;
};

// Interface for observable data (data that fires events)
// In order for this class to work properly, it needs
// an instance of an LLEventDispatcher to route events to their
// listeners.
class LLObservable
{
public:
	// Initialize with the default Dispatcher
	LLObservable();
	virtual ~LLObservable();

	// Replaces the existing dispatcher pointer to the new one,
	// informing the dispatcher of the change.
	virtual bool setDispatcher(LLPointer<LLEventDispatcher> dispatcher);

	// Returns the current dispatcher pointer.
	virtual LLEventDispatcher* getDispatcher();

	void addListener(LLEventListener *listener, LLSD filter = "", const LLSD& userdata = "")
	{
		if (mDispatcher.notNull()) mDispatcher->addListener(listener, filter, userdata);
	}
	void removeListener(LLEventListener *listener)
	{
		if (mDispatcher.notNull()) mDispatcher->removeListener(listener);
	}
	// Notifies the dispatcher of an event being fired.
	void fireEvent(LLPointer<LLEvent> event, LLSD filter = LLSD());

protected:
	LLPointer<LLEventDispatcher> mDispatcher;
};

class LLValueChangedEvent : public LLEvent
{
public:
	LLValueChangedEvent(LLObservable* source, LLSD value) : LLEvent(source, "value_changed"), mValue(value) { }
	LLSD getValue() { return mValue; }
	LLSD mValue;
};

} // LLOldEvents

#endif // LL_EVENT_H
