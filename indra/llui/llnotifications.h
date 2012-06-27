/**
* @file llnotifications.h
* @brief Non-UI manager and support for keeping a prioritized list of notifications
* @author Q (with assistance from Richard and Coco)
*
* $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#ifndef LL_LLNOTIFICATIONS_H
#define LL_LLNOTIFICATIONS_H

/**
 * This system is intended to provide a singleton mechanism for adding
 * notifications to one of an arbitrary set of event channels.
 * 
 * Controlling JIRA: DEV-9061
 *
 * Every notification has (see code for full list):
 *  - a textual name, which is used to look up its template in the XML files
 *  - a payload, which is a block of LLSD
 *  - a channel, which is normally extracted from the XML files but
 *	  can be overridden.
 *  - a timestamp, used to order the notifications
 *  - expiration time -- if nonzero, specifies a time after which the
 *    notification will no longer be valid.
 *  - a callback name and a couple of status bits related to callbacks (see below)
 * 
 * There is a management class called LLNotifications, which is an LLSingleton.
 * The class maintains a collection of all of the notifications received
 * or processed during this session, and also manages the persistence
 * of those notifications that must be persisted.
 * 
 * We also have Channels. A channel is a view on a collection of notifications;
 * The collection is defined by a filter function that controls which
 * notifications are in the channel, and its ordering is controlled by 
 * a comparator. 
 *
 * There is a hierarchy of channels; notifications flow down from
 * the management class (LLNotifications, which itself inherits from
 * The channel base class) to the individual channels.
 * Any change to notifications (add, delete, modify) is 
 * automatically propagated through the channel hierarchy.
 * 
 * We provide methods for adding a new notification, for removing
 * one, and for managing channels. Channels are relatively cheap to construct
 * and maintain, so in general, human interfaces should use channels to
 * select and manage their lists of notifications.
 * 
 * We also maintain a collection of templates that are loaded from the 
 * XML file of template translations. The system supports substitution
 * of named variables from the payload into the XML file.
 * 
 * By default, only the "unknown message" template is built into the system.
 * It is not an error to add a notification that's not found in the 
 * template system, but it is logged.
 *
 */

#include <string>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <iomanip>
#include <sstream>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/type_traits.hpp>

// we want to minimize external dependencies, but this one is important
#include "llsd.h"

// and we need this to manage the notification callbacks
#include "llevents.h"
#include "llfunctorregistry.h"
#include "llpointer.h"
#include "llinitparam.h"
#include "llnotificationslistener.h"
#include "llnotificationptr.h"

class LLAvatarName;
typedef enum e_notification_priority
{
	NOTIFICATION_PRIORITY_UNSPECIFIED,
	NOTIFICATION_PRIORITY_LOW,
	NOTIFICATION_PRIORITY_NORMAL,
	NOTIFICATION_PRIORITY_HIGH,
	NOTIFICATION_PRIORITY_CRITICAL
} ENotificationPriority;

struct NotificationPriorityValues : public LLInitParam::TypeValuesHelper<ENotificationPriority, NotificationPriorityValues>
{
	static void declareValues();
};

class LLNotificationResponderInterface
{
public:
	LLNotificationResponderInterface(){};
	virtual ~LLNotificationResponderInterface(){};

	virtual void handleRespond(const LLSD& notification, const LLSD& response) = 0;

	virtual LLSD asLLSD() = 0;

	virtual void fromLLSD(const LLSD& params) = 0;
};

typedef boost::function<void (const LLSD&, const LLSD&)> LLNotificationResponder;

typedef boost::shared_ptr<LLNotificationResponderInterface> LLNotificationResponderPtr;

typedef LLFunctorRegistry<LLNotificationResponder> LLNotificationFunctorRegistry;
typedef LLFunctorRegistration<LLNotificationResponder> LLNotificationFunctorRegistration;

// context data that can be looked up via a notification's payload by the display logic
// derive from this class to implement specific contexts
class LLNotificationContext : public LLInstanceTracker<LLNotificationContext, LLUUID>
{
public:
	LLNotificationContext() : LLInstanceTracker<LLNotificationContext, LLUUID>(LLUUID::generateNewID())
	{
	}

	virtual ~LLNotificationContext() {}

	LLSD asLLSD() const
	{
		return getKey();
	}

private:

};

// Contains notification form data, such as buttons and text fields along with
// manipulator functions
class LLNotificationForm
{
	LOG_CLASS(LLNotificationForm);

public:
	struct FormElementBase : public LLInitParam::Block<FormElementBase>
	{
		Optional<std::string>	name;

		FormElementBase();
	};

	struct FormIgnore : public LLInitParam::Block<FormIgnore, FormElementBase>
	{
		Optional<std::string>	text;
		Optional<bool>			save_option;
		Optional<std::string>	control;
		Optional<bool>			invert_control;

		FormIgnore();
	};

	struct FormButton : public LLInitParam::Block<FormButton, FormElementBase>
	{
		Mandatory<S32>			index;
		Mandatory<std::string>	text;
		Optional<std::string>	ignore;
		Optional<bool>			is_default;

		Mandatory<std::string>	type;

		FormButton();
	};

	struct FormInput : public LLInitParam::Block<FormInput, FormElementBase>
	{
		Mandatory<std::string>	type;
		Optional<S32>			width;
		Optional<S32>			max_length_chars;
		Optional<std::string>	text;

		Optional<std::string>	value;
		FormInput();
	};

	struct FormElement : public LLInitParam::ChoiceBlock<FormElement>
	{
		Alternative<FormButton> button;
		Alternative<FormInput>	input;

		FormElement();
	};

	struct FormElements : public LLInitParam::Block<FormElements>
	{
		Multiple<FormElement> elements;
		FormElements();
	};

	struct Params : public LLInitParam::Block<Params>
	{
		Optional<std::string>	name;
		Optional<FormIgnore>	ignore;
		Optional<FormElements>	form_elements;

		Params();
	};

	typedef enum e_ignore_type
	{ 
		IGNORE_NO,
		IGNORE_WITH_DEFAULT_RESPONSE, 
		IGNORE_WITH_LAST_RESPONSE, 
		IGNORE_SHOW_AGAIN 
	} EIgnoreType;

	LLNotificationForm();
	LLNotificationForm(const LLSD& sd);
	LLNotificationForm(const std::string& name, const Params& p);

	LLSD asLLSD() const;

	S32 getNumElements() { return mFormData.size(); }
	LLSD getElement(S32 index) { return mFormData.get(index); }
	LLSD getElement(const std::string& element_name);
	bool hasElement(const std::string& element_name);
	void addElement(const std::string& type, const std::string& name, const LLSD& value = LLSD());
	void formatElements(const LLSD& substitutions);
	// appends form elements from another form serialized as LLSD
	void append(const LLSD& sub_form);
	std::string getDefaultOption();
	LLPointer<class LLControlVariable> getIgnoreSetting();
	bool getIgnored();
	void setIgnored(bool ignored);

	EIgnoreType getIgnoreType() { return mIgnore; }
	std::string getIgnoreMessage() { return mIgnoreMsg; }

private:
	LLSD								mFormData;
	EIgnoreType							mIgnore;
	std::string							mIgnoreMsg;
	LLPointer<class LLControlVariable>	mIgnoreSetting;
	bool								mInvertSetting;
};

typedef boost::shared_ptr<LLNotificationForm> LLNotificationFormPtr;


struct LLNotificationTemplate;

// we want to keep a map of these by name, and it's best to manage them
// with smart pointers
typedef boost::shared_ptr<LLNotificationTemplate> LLNotificationTemplatePtr;


struct LLNotificationVisibilityRule;

typedef boost::shared_ptr<LLNotificationVisibilityRule> LLNotificationVisibilityRulePtr;

/**
 * @class LLNotification
 * @brief The object that expresses the details of a notification
 * 
 * We make this noncopyable because
 * we want to manage these through LLNotificationPtr, and only
 * ever create one instance of any given notification.
 * 
 * The enable_shared_from_this flag ensures that if we construct
 * a smart pointer from a notification, we'll always get the same
 * shared pointer.
 */
class LLNotification  : 
	boost::noncopyable,
	public boost::enable_shared_from_this<LLNotification>
{
LOG_CLASS(LLNotification);
friend class LLNotifications;

public:
	// parameter object used to instantiate a new notification
	struct Params : public LLInitParam::Block<Params>
	{
		friend class LLNotification;
	
		Mandatory<std::string>					name;

		// optional
		Optional<LLSD>							substitutions;
		Optional<LLSD>							payload;
		Optional<ENotificationPriority, NotificationPriorityValues>	priority;
		Optional<LLSD>							form_elements;
		Optional<LLDate>						time_stamp;
		Optional<LLNotificationContext*>		context;
		Optional<void*>							responder;

		struct Functor : public LLInitParam::ChoiceBlock<Functor>
		{
			Alternative<std::string>										name;
			Alternative<LLNotificationFunctorRegistry::ResponseFunctor>	function;
			Alternative<LLNotificationResponderPtr>						responder;

			Functor()
			:	name("functor_name"),
				function("functor"),
				responder("responder")
			{}
		};
		Optional<Functor>						functor;

		Params()
		:	name("name"),
			priority("priority", NOTIFICATION_PRIORITY_UNSPECIFIED),
			time_stamp("time_stamp"),
			payload("payload"),
			form_elements("form_elements")
		{
			time_stamp = LLDate::now();
			responder = NULL;
		}

		Params(const std::string& _name) 
		:	name("name"),
			priority("priority", NOTIFICATION_PRIORITY_UNSPECIFIED),
			time_stamp("time_stamp"),
			payload("payload"),
			form_elements("form_elements")
		{
			functor.name = _name;
			name = _name;
			time_stamp = LLDate::now();
			responder = NULL;
		}
	};

	LLNotificationResponderPtr getResponderPtr() { return mResponder; }

private:
	
	LLUUID mId;
	LLSD mPayload;
	LLSD mSubstitutions;
	LLDate mTimestamp;
	LLDate mExpiresAt;
	bool mCancelled;
	bool mRespondedTo; 	// once the notification has been responded to, this becomes true
	LLSD mResponse;
	bool mIgnored;
	ENotificationPriority mPriority;
	LLNotificationFormPtr mForm;
	void* mResponderObj; // TODO - refactor/remove this field
	bool mIsReusable;
	LLNotificationResponderPtr mResponder;

	// a reference to the template
	LLNotificationTemplatePtr mTemplatep;

	/*
	 We want to be able to store and reload notifications so that they can survive
	 a shutdown/restart of the client. So we can't simply pass in callbacks;
	 we have to specify a callback mechanism that can be used by name rather than 
	 by some arbitrary pointer -- and then people have to initialize callbacks 
	 in some useful location. So we use LLNotificationFunctorRegistry to manage them.
	 */
	 std::string mResponseFunctorName;
	
	/*
	 In cases where we want to specify an explict, non-persisted callback, 
	 we store that in the callback registry under a dynamically generated
	 key, and store the key in the notification, so we can still look it up
	 using the same mechanism.
	 */
	bool mTemporaryResponder;

	void init(const std::string& template_name, const LLSD& form_elements);

	LLNotification(const Params& p);

	// this is just for making it easy to look things up in a set organized by UUID -- DON'T USE IT
	// for anything real!
 LLNotification(LLUUID uuid) : mId(uuid), mCancelled(false), mRespondedTo(false), mIgnored(false), mPriority(NOTIFICATION_PRIORITY_UNSPECIFIED), mTemporaryResponder(false) {}

	void cancel();

public:

	// constructor from a saved notification
	LLNotification(const LLSD& sd);

	void setResponseFunctor(std::string const &responseFunctorName);

	void setResponseFunctor(const LLNotificationFunctorRegistry::ResponseFunctor& cb);

	void setResponseFunctor(const LLNotificationResponderPtr& responder);

	typedef enum e_response_template_type
	{
		WITHOUT_DEFAULT_BUTTON,
		WITH_DEFAULT_BUTTON
	} EResponseTemplateType;

	// return response LLSD filled in with default form contents and (optionally) the default button selected
	LLSD getResponseTemplate(EResponseTemplateType type = WITHOUT_DEFAULT_BUTTON);

	// returns index of first button with value==TRUE
	// usually this the button the user clicked on
	// returns -1 if no button clicked (e.g. form has not been displayed)
	static S32 getSelectedOption(const LLSD& notification, const LLSD& response);
	// returns name of first button with value==TRUE
	static std::string getSelectedOptionName(const LLSD& notification);

	// after someone responds to a notification (usually by clicking a button,
	// but sometimes by filling out a little form and THEN clicking a button),
    // the result of the response (the name and value of the button clicked,
	// plus any other data) should be packaged up as LLSD, then passed as a
	// parameter to the notification's respond() method here. This will look up
	// and call the appropriate responder.
	//
	// response is notification serialized as LLSD:
	// ["name"] = notification name
	// ["form"] = LLSD tree that includes form description and any prefilled form data
	// ["response"] = form data filled in by user
	// (including, but not limited to which button they clicked on)
	// ["payload"] = transaction specific data, such as ["source_id"] (originator of notification),  
	//				["item_id"] (attached inventory item), etc.
	// ["substitutions"] = string substitutions used to generate notification message
    // from the template
	// ["time"] = time at which notification was generated;
	// ["expiry"] = time at which notification expires;
	// ["responseFunctor"] = name of registered functor that handles responses to notification;
	LLSD asLLSD();

	void respond(const LLSD& sd);
	void respondWithDefault();

	void* getResponder() { return mResponderObj; }

	void setResponder(void* responder) { mResponderObj = responder; }

	void setIgnored(bool ignore);

	bool isCancelled() const
	{
		return mCancelled;
	}

	bool isRespondedTo() const
	{
		return mRespondedTo;
	}

	bool isActive() const
	{
		return !isRespondedTo()
			&& !isCancelled()
			&& !isExpired();
	}

	const LLSD& getResponse() { return mResponse; }

	bool isIgnored() const
	{
		return mIgnored;
	}

	const std::string& getName() const;

	const std::string& getIcon() const;

	bool isPersistent() const;

	const LLUUID& id() const
	{
		return mId;
	}
	
	const LLSD& getPayload() const
	{
		return mPayload;
	}

	const LLSD& getSubstitutions() const
	{
		return mSubstitutions;
	}

	const LLDate& getDate() const
	{
		return mTimestamp;
	}

	std::string getType() const;
	std::string getMessage() const;
	std::string getFooter() const;
	std::string getLabel() const;
	std::string getURL() const;
	S32 getURLOption() const;
    S32 getURLOpenExternally() const;
	
	const LLNotificationFormPtr getForm();

	const LLDate getExpiration() const
	{
		return mExpiresAt;
	}

	ENotificationPriority getPriority() const
	{
		return mPriority;
	}

	const LLUUID getID() const
	{
		return mId;
	}

	bool isReusable() { return mIsReusable; }

	void setReusable(bool reusable) { mIsReusable = reusable; }
	
	// comparing two notifications normally means comparing them by UUID (so we can look them
	// up quickly this way)
	bool operator<(const LLNotification& rhs) const
	{
		return mId < rhs.mId;
	}

	bool operator==(const LLNotification& rhs) const
	{
		return mId == rhs.mId;
	}

	bool operator!=(const LLNotification& rhs) const
	{
		return !operator==(rhs);
	}

	bool isSameObjectAs(const LLNotification* rhs) const
	{
		return this == rhs;
	}
	
	// this object has been updated, so tell all our clients
	void update();

	void updateFrom(LLNotificationPtr other);
	
	// A fuzzy equals comparator.
	// true only if both notifications have the same template and 
	//     1) flagged as unique (there can be only one of these) OR 
	//     2) all required payload fields of each also exist in the other.
	bool isEquivalentTo(LLNotificationPtr that) const;
	
	// if the current time is greater than the expiration, the notification is expired
	bool isExpired() const
	{
		if (mExpiresAt.secondsSinceEpoch() == 0)
		{
			return false;
		}
		
		LLDate rightnow = LLDate::now();
		return rightnow > mExpiresAt;
	}
	
	std::string summarize() const;

	bool hasUniquenessConstraints() const;

	bool matchesTag(const std::string& tag);

	virtual ~LLNotification() {}
};

std::ostream& operator<<(std::ostream& s, const LLNotification& notification);

namespace LLNotificationFilters
{
	// a sample filter
	bool includeEverything(LLNotificationPtr p);

	typedef enum e_comparison 
	{ 
		EQUAL, 
		LESS, 
		GREATER, 
		LESS_EQUAL, 
		GREATER_EQUAL 
	} EComparison;

	// generic filter functor that takes method or member variable reference
	template<typename T>
	struct filterBy
	{
		typedef boost::function<T (LLNotificationPtr)>	field_t;
		typedef typename boost::remove_reference<T>::type		value_t;
		
		filterBy(field_t field, value_t value, EComparison comparison = EQUAL) 
			:	mField(field), 
				mFilterValue(value),
				mComparison(comparison)
		{
		}		
		
		bool operator()(LLNotificationPtr p)
		{
			switch(mComparison)
			{
			case EQUAL:
				return mField(p) == mFilterValue;
			case LESS:
				return mField(p) < mFilterValue;
			case GREATER:
				return mField(p) > mFilterValue;
			case LESS_EQUAL:
				return mField(p) <= mFilterValue;
			case GREATER_EQUAL:
				return mField(p) >= mFilterValue;
			default:
				return false;
			}
		}

		field_t mField;
		value_t	mFilterValue;
		EComparison mComparison;
	};
};

namespace LLNotificationComparators
{
	typedef enum e_direction { ORDER_DECREASING, ORDER_INCREASING } EDirection;

	// generic order functor that takes method or member variable reference
	template<typename T>
	struct orderBy
	{
		typedef boost::function<T (LLNotificationPtr)> field_t;
        	orderBy(field_t field, EDirection direction = ORDER_INCREASING) : mField(field), mDirection(direction) {}
		bool operator()(LLNotificationPtr lhs, LLNotificationPtr rhs)
		{
			if (mDirection == ORDER_DECREASING)
			{
				return mField(lhs) > mField(rhs);
			}
			else
			{
				return mField(lhs) < mField(rhs);
			}
		}

		field_t mField;
		EDirection mDirection;
	};

	struct orderByUUID : public orderBy<const LLUUID&>
	{
		orderByUUID(EDirection direction = ORDER_INCREASING) : orderBy<const LLUUID&>(&LLNotification::id, direction) {}
	};

	struct orderByDate : public orderBy<const LLDate&>
	{
		orderByDate(EDirection direction = ORDER_INCREASING) : orderBy<const LLDate&>(&LLNotification::getDate, direction) {}
	};
};

typedef boost::function<bool (LLNotificationPtr)> LLNotificationFilter;
typedef boost::function<bool (LLNotificationPtr, LLNotificationPtr)> LLNotificationComparator;
typedef std::set<LLNotificationPtr, LLNotificationComparator> LLNotificationSet;
typedef std::multimap<std::string, LLNotificationPtr> LLNotificationMap;

// ========================================================
// Abstract base class (interface) for a channel; also used for the master container.
// This lets us arrange channels into a call hierarchy.

// We maintain a hierarchy of notification channels; events are always started at the top
// and propagated through the hierarchy only if they pass a filter.
// Any channel can be created with a parent. A null parent (empty string) means it's
// tied to the root of the tree (the LLNotifications class itself).
// The default hierarchy looks like this:
//
// LLNotifications --+-- Expiration --+-- Mute --+-- Ignore --+-- Visible --+-- History
//                                                                          +-- Alerts
//                                                                          +-- Notifications
//
// In general, new channels that want to only see notifications that pass through 
// all of the built-in tests should attach to the "Visible" channel
//
class LLNotificationChannelBase :
	public LLEventTrackable
{
	LOG_CLASS(LLNotificationChannelBase);
public:
	LLNotificationChannelBase(LLNotificationFilter filter, LLNotificationComparator comp) : 
		mFilter(filter), mItems(comp) 
	{}
	virtual ~LLNotificationChannelBase() {}
	// you can also connect to a Channel, so you can be notified of
	// changes to this channel
	template <typename LISTENER>
    LLBoundListener connectChanged(const LISTENER& slot)
    {
        // Examine slot to see if it binds an LLEventTrackable subclass, or a
        // boost::shared_ptr to something, or a boost::weak_ptr to something.
        // Call this->connectChangedImpl() to actually connect it.
        return LLEventDetail::visit_and_connect(slot,
                                  boost::bind(&LLNotificationChannelBase::connectChangedImpl,
                                              this,
                                              _1));
    }
	template <typename LISTENER>
    LLBoundListener connectAtFrontChanged(const LISTENER& slot)
    {
        return LLEventDetail::visit_and_connect(slot,
                                  boost::bind(&LLNotificationChannelBase::connectAtFrontChangedImpl,
                                              this,
                                              _1));
    }
    template <typename LISTENER>
	LLBoundListener connectPassedFilter(const LISTENER& slot)
    {
        // see comments in connectChanged()
        return LLEventDetail::visit_and_connect(slot,
                                  boost::bind(&LLNotificationChannelBase::connectPassedFilterImpl,
                                              this,
                                              _1));
    }
    template <typename LISTENER>
	LLBoundListener connectFailedFilter(const LISTENER& slot)
    {
        // see comments in connectChanged()
        return LLEventDetail::visit_and_connect(slot,
                                  boost::bind(&LLNotificationChannelBase::connectFailedFilterImpl,
                                              this,
                                              _1));
    }

	// use this when items change or to add a new one
	bool updateItem(const LLSD& payload);
	const LLNotificationFilter& getFilter() { return mFilter; }

protected:
    LLBoundListener connectChangedImpl(const LLEventListener& slot);
    LLBoundListener connectAtFrontChangedImpl(const LLEventListener& slot);
    LLBoundListener connectPassedFilterImpl(const LLEventListener& slot);
    LLBoundListener connectFailedFilterImpl(const LLEventListener& slot);

	LLNotificationSet mItems;
	LLStandardSignal mChanged;
	LLStandardSignal mPassedFilter;
	LLStandardSignal mFailedFilter;
	
	// these are action methods that subclasses can override to take action 
	// on specific types of changes; the management of the mItems list is
	// still handled by the generic handler.
	virtual void onLoad(LLNotificationPtr p) {}
	virtual void onAdd(LLNotificationPtr p) {}
	virtual void onDelete(LLNotificationPtr p) {}
	virtual void onChange(LLNotificationPtr p) {}

	bool updateItem(const LLSD& payload, LLNotificationPtr pNotification);
	LLNotificationFilter mFilter;
};

// The type of the pointers that we're going to manage in the NotificationQueue system
// Because LLNotifications is a singleton, we don't actually expect to ever 
// destroy it, but if it becomes necessary to do so, the shared_ptr model
// will ensure that we don't leak resources.
class LLNotificationChannel;
typedef boost::shared_ptr<LLNotificationChannel> LLNotificationChannelPtr;

// manages a list of notifications
// Note that if this is ever copied around, we might find ourselves with multiple copies
// of a queue with notifications being added to different nonequivalent copies. So we 
// make it inherit from boost::noncopyable, and then create a map of shared_ptr to manage it.
// 
// NOTE: LLNotificationChannel is self-registering. The *correct* way to create one is to 
// do something like:
//		LLNotificationChannel::buildChannel("name", "parent"...);
// This returns an LLNotificationChannelPtr, which you can store, or
// you can then retrieve the channel by using the registry:
//		LLNotifications::instance().getChannel("name")...
//
class LLNotificationChannel : 
	boost::noncopyable, 
	public LLNotificationChannelBase
{
	LOG_CLASS(LLNotificationChannel);

public:  
	virtual ~LLNotificationChannel() {}
	typedef LLNotificationSet::iterator Iterator;
    
	std::string getName() const { return mName; }
	std::string getParentChannelName() { return mParent; }
    
    bool isEmpty() const;
    
    Iterator begin();
    Iterator end();

    // Channels have a comparator to control sort order;
	// the default sorts by arrival date
    void setComparator(LLNotificationComparator comparator);
	
	std::string summarize();

	// factory method for constructing these channels; since they're self-registering,
	// we want to make sure that you can't use new to make them
	static LLNotificationChannelPtr buildChannel(const std::string& name, const std::string& parent,
						LLNotificationFilter filter=LLNotificationFilters::includeEverything, 
						LLNotificationComparator comparator=LLNotificationComparators::orderByUUID());
	
protected:
    // Notification Channels have a filter, which determines which notifications
	// will be added to this channel. 
	// Channel filters cannot change.
	// Channels have a protected constructor so you can't make smart pointers that don't 
	// come from our internal reference; call NotificationChannel::build(args)
	LLNotificationChannel(const std::string& name, const std::string& parent,
						  LLNotificationFilter filter, LLNotificationComparator comparator);

private:
	std::string mName;
	std::string mParent;
	LLNotificationComparator mComparator;
};

// An interface class to provide a clean linker seam to the LLNotifications class.
// Extend this interface as needed for your use of LLNotifications.
class LLNotificationsInterface
{
public:
	virtual LLNotificationPtr add(const std::string& name, 
						const LLSD& substitutions, 
						const LLSD& payload, 
						LLNotificationFunctorRegistry::ResponseFunctor functor) = 0;
};

class LLNotifications : 
	public LLNotificationsInterface,
	public LLSingleton<LLNotifications>, 
	public LLNotificationChannelBase
{
	LOG_CLASS(LLNotifications);

	friend class LLSingleton<LLNotifications>;
public:
	// load all notification descriptions from file
	// calling more than once will overwrite existing templates
	// but never delete a template
	bool loadTemplates();

	// load visibility rules from file; 
	// OK to call more than once because it will reload
	bool loadVisibilityRules();  
	
	// Add a simple notification (from XUI)
	void addFromCallback(const LLSD& name);
	
	// *NOTE: To add simple notifications, #include "llnotificationsutil.h"
	// and use LLNotificationsUtil::add("MyNote") or add("MyNote", args)
	LLNotificationPtr add(const std::string& name, 
						const LLSD& substitutions,
						const LLSD& payload);
	LLNotificationPtr add(const std::string& name, 
						const LLSD& substitutions, 
						const LLSD& payload, 
						const std::string& functor_name);
	/* virtual */ LLNotificationPtr add(const std::string& name, 
						const LLSD& substitutions, 
						const LLSD& payload, 
						LLNotificationFunctorRegistry::ResponseFunctor functor);
	LLNotificationPtr add(const LLNotification::Params& p);

	void add(const LLNotificationPtr pNotif);
	void cancel(LLNotificationPtr pNotif);
	void cancelByName(const std::string& name);
	void update(const LLNotificationPtr pNotif);

	LLNotificationPtr find(LLUUID uuid);
	
	typedef boost::function<void (LLNotificationPtr)> NotificationProcess;
	
	void forEachNotification(NotificationProcess process);

	// This is all stuff for managing the templates
	// take your template out
	LLNotificationTemplatePtr getTemplate(const std::string& name);
	
	// get the whole collection
	typedef std::vector<std::string> TemplateNames;
	TemplateNames getTemplateNames() const;  // returns a list of notification names
	
	typedef std::map<std::string, LLNotificationTemplatePtr> TemplateMap;

	TemplateMap::const_iterator templatesBegin() { return mTemplates.begin(); }
	TemplateMap::const_iterator templatesEnd() { return mTemplates.end(); }

	// test for existence
	bool templateExists(const std::string& name);

	typedef std::list<LLNotificationVisibilityRulePtr> VisibilityRuleList;
	
	void forceResponse(const LLNotification::Params& params, S32 option);

	void createDefaultChannels();

	typedef std::map<std::string, LLNotificationChannelPtr> ChannelMap;
	ChannelMap mChannels;

	void addChannel(LLNotificationChannelPtr pChan);
	LLNotificationChannelPtr getChannel(const std::string& channelName);
	
	std::string getGlobalString(const std::string& key) const;

	void setIgnoreAllNotifications(bool ignore);
	bool getIgnoreAllNotifications();

	bool isVisibleByRules(LLNotificationPtr pNotification);
	
private:
	// we're a singleton, so we don't have a public constructor
	LLNotifications();
	/*virtual*/ void initSingleton();
	
	void loadPersistentNotifications();

	bool expirationFilter(LLNotificationPtr pNotification);
	bool expirationHandler(const LLSD& payload);
	bool uniqueFilter(LLNotificationPtr pNotification);
	bool uniqueHandler(const LLSD& payload);
	bool failedUniquenessTest(const LLSD& payload);
	LLNotificationChannelPtr pHistoryChannel;
	LLNotificationChannelPtr pExpirationChannel;
	
	TemplateMap mTemplates;

	VisibilityRuleList mVisibilityRules;

	std::string mFileName;
	
	LLNotificationMap mUniqueNotifications;
	
	typedef std::map<std::string, std::string> GlobalStringMap;
	GlobalStringMap mGlobalStrings;

	bool mIgnoreAllNotifications;

    boost::scoped_ptr<LLNotificationsListener> mListener;
};

/**
 * Abstract class for postponed notifications.
 * Provides possibility to add notification after specified by id avatar or group will be
 * received from cache name. The object of this type automatically well be deleted
 * by cleanup method after respond will be received from cache name.
 *
 * To add custom postponed notification to the notification system client should:
 *  1 create class derived from LLPostponedNotification;
 *  2 call LLPostponedNotification::add method;
 */
class LLPostponedNotification
{
public:
	/**
	 * Performs hooking cache name callback which will add notification to notifications system.
	 * Type of added notification should be specified by template parameter T
	 * and non-private derived from LLPostponedNotification class,
	 * otherwise compilation error will occur.
	 */
	template<class T>
	static void add(const LLNotification::Params& params,
			const LLUUID& id, bool is_group)
	{
		// upcast T to the base type to restrict T derivation from LLPostponedNotification
		LLPostponedNotification* thiz = new T();
		thiz->mParams = params;

		// Avoid header file dependency on llcachename.h
		lookupName(thiz, id, is_group);
	}

private:
	static void lookupName(LLPostponedNotification* thiz, const LLUUID& id, bool is_group);
	// only used for groups
	void onGroupNameCache(const LLUUID& id, const std::string& full_name, bool is_group);
	// only used for avatars
	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);
	// used for both group and avatar names
	void finalizeName(const std::string& name);

	void cleanup()
	{
		delete this;
	}

protected:
	LLPostponedNotification() {}
	virtual ~LLPostponedNotification() {}

	/**
	 * Abstract method provides possibility to modify notification parameters and
	 * will be called after cache name retrieve information about avatar or group
	 * and before notification will be added to the notification system.
	 */
	virtual void modifyNotificationParams() = 0;

	LLNotification::Params mParams;
	std::string mName;
};

#endif//LL_LLNOTIFICATIONS_H

