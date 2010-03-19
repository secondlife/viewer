/**
* @file llnotifications.cpp
* @brief Non-UI queue manager for keeping a prioritized list of notifications
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

#include "linden_common.h"

#include "llnotifications.h"

#include "llxmlnode.h"
#include "lluictrl.h"
#include "lluictrlfactory.h"
#include "lldir.h"
#include "llsdserialize.h"
#include "lltrans.h"
#include "llnotificationslistener.h"

#include <algorithm>
#include <boost/regex.hpp>


const std::string NOTIFICATION_PERSIST_VERSION = "0.93";

// local channel for notification history
class LLNotificationHistoryChannel : public LLNotificationChannel
{
	LOG_CLASS(LLNotificationHistoryChannel);
public:
	LLNotificationHistoryChannel(const std::string& filename) : 
		LLNotificationChannel("History", "Visible", &historyFilter, LLNotificationComparators::orderByUUID()),
		mFileName(filename)
	{
		connectChanged(boost::bind(&LLNotificationHistoryChannel::historyHandler, this, _1));
		loadPersistentNotifications();
	}

private:
	bool historyHandler(const LLSD& payload)
	{
		// we ignore "load" messages, but rewrite the persistence file on any other
		std::string sigtype = payload["sigtype"];
		if (sigtype != "load")
		{
			savePersistentNotifications();
		}
		return false;
	}

	// The history channel gets all notifications except those that have been cancelled
	static bool historyFilter(LLNotificationPtr pNotification)
	{
		return !pNotification->isCancelled();
	}

	void savePersistentNotifications()
	{
		/* NOTE: As of 2009-11-09 the reload of notifications on startup does not
		work, and has not worked for months.  Skip saving notifications until the
		read can be fixed, because this hits the disk once per notification and
		causes log spam.  James

		llinfos << "Saving open notifications to " << mFileName << llendl;

		llofstream notify_file(mFileName.c_str());
		if (!notify_file.is_open()) 
		{
			llwarns << "Failed to open " << mFileName << llendl;
			return;
		}

		LLSD output;
		output["version"] = NOTIFICATION_PERSIST_VERSION;
		LLSD& data = output["data"];

		for (LLNotificationSet::iterator it = mItems.begin(); it != mItems.end(); ++it)
		{
			if (!LLNotifications::instance().templateExists((*it)->getName())) continue;

			// only store notifications flagged as persisting
			LLNotificationTemplatePtr templatep = LLNotifications::instance().getTemplate((*it)->getName());
			if (!templatep->mPersist) continue;

			data.append((*it)->asLLSD());
		}

		LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
		formatter->format(output, notify_file, LLSDFormatter::OPTIONS_PRETTY);
		*/
	}

	void loadPersistentNotifications()
	{
		llinfos << "Loading open notifications from " << mFileName << llendl;

		llifstream notify_file(mFileName.c_str());
		if (!notify_file.is_open()) 
		{
			llwarns << "Failed to open " << mFileName << llendl;
			return;
		}

		LLSD input;
		LLPointer<LLSDParser> parser = new LLSDXMLParser();
		if (parser->parse(notify_file, input, LLSDSerialize::SIZE_UNLIMITED) < 0)
		{
			llwarns << "Failed to parse open notifications" << llendl;
			return;
		}

		if (input.isUndefined()) return;
		std::string version = input["version"];
		if (version != NOTIFICATION_PERSIST_VERSION)
		{
			llwarns << "Bad open notifications version: " << version << llendl;
			return;
		}
		LLSD& data = input["data"];
		if (data.isUndefined()) return;

		LLNotifications& instance = LLNotifications::instance();
		for (LLSD::array_const_iterator notification_it = data.beginArray();
			notification_it != data.endArray();
			++notification_it)
		{
			instance.add(LLNotificationPtr(new LLNotification(*notification_it)));
		}
	}

	//virtual
	void onDelete(LLNotificationPtr pNotification)
	{
		// we want to keep deleted notifications in our log
		mItems.insert(pNotification);
		
		return;
	}
	
private:
	std::string mFileName;
};

bool filterIgnoredNotifications(LLNotificationPtr notification)
{
	// filter everything if we are to ignore ALL
	if(LLNotifications::instance().getIgnoreAllNotifications())
	{
		return false;
	}

	LLNotificationFormPtr form = notification->getForm();
	// Check to see if the user wants to ignore this alert
	if (form->getIgnoreType() != LLNotificationForm::IGNORE_NO)
	{
		return LLUI::sSettingGroups["ignores"]->getBOOL(notification->getName());
	}

	return true;
}

bool handleIgnoredNotification(const LLSD& payload)
{
	if (payload["sigtype"].asString() == "add")
	{
		LLNotificationPtr pNotif = LLNotifications::instance().find(payload["id"].asUUID());
		if (!pNotif) return false;

		LLNotificationFormPtr form = pNotif->getForm();
		LLSD response;
		switch(form->getIgnoreType())
		{
		case LLNotificationForm::IGNORE_WITH_DEFAULT_RESPONSE:
			response = pNotif->getResponseTemplate(LLNotification::WITH_DEFAULT_BUTTON);
			break;
		case LLNotificationForm::IGNORE_WITH_LAST_RESPONSE:
			response = LLUI::sSettingGroups["ignores"]->getLLSD("Default" + pNotif->getName());
			break;
		case LLNotificationForm::IGNORE_SHOW_AGAIN:
			break;
		default:
			return false;
		}
		pNotif->setIgnored(true);
		pNotif->respond(response);
		return true; 	// don't process this item any further
	}
	return false;
}

namespace LLNotificationFilters
{
	// a sample filter
	bool includeEverything(LLNotificationPtr p)
	{
		return true;
	}
};

LLNotificationForm::LLNotificationForm()
:	mFormData(LLSD::emptyArray()),
	mIgnore(IGNORE_NO)
{
}


LLNotificationForm::LLNotificationForm(const std::string& name, const LLXMLNodePtr xml_node) 
:	mFormData(LLSD::emptyArray()),
	mIgnore(IGNORE_NO)
{
	if (!xml_node->hasName("form"))
	{
		llwarns << "Bad xml node for form: " << xml_node->getName() << llendl;
	}
	LLXMLNodePtr child = xml_node->getFirstChild();
	while(child)
	{
		child = LLNotifications::instance().checkForXMLTemplate(child);

		LLSD item_entry;
		std::string element_name = child->getName()->mString;

		if (element_name == "ignore" )
		{
			bool save_option = false;
			child->getAttribute_bool("save_option", save_option);
			if (!save_option)
			{
				mIgnore = IGNORE_WITH_DEFAULT_RESPONSE;
			}
			else
			{
				// remember last option chosen by user and automatically respond with that in the future
				mIgnore = IGNORE_WITH_LAST_RESPONSE;
				LLUI::sSettingGroups["ignores"]->declareLLSD(std::string("Default") + name, "", std::string("Default response for notification " + name));
			}
			child->getAttributeString("text", mIgnoreMsg);
			BOOL show_notification = TRUE;
			LLUI::sSettingGroups["ignores"]->declareBOOL(name, show_notification, "Ignore notification with this name", TRUE);
		}
		else
		{
			// flatten xml form entry into single LLSD map with type==name
			item_entry["type"] = element_name;
			const LLXMLAttribList::iterator attrib_end = child->mAttributes.end();
			for(LLXMLAttribList::iterator attrib_it = child->mAttributes.begin();
				attrib_it != attrib_end;
				++attrib_it)
			{
				item_entry[std::string(attrib_it->second->getName()->mString)] = attrib_it->second->getValue();
			}
			item_entry["value"] = child->getTextContents();
			mFormData.append(item_entry);
		}

		child = child->getNextSibling();
	}
}

LLNotificationForm::LLNotificationForm(const LLSD& sd)
	: mIgnore(IGNORE_NO)
{
	if (sd.isArray())
	{
		mFormData = sd;
	}
	else
	{
		llwarns << "Invalid form data " << sd << llendl;
		mFormData = LLSD::emptyArray();
	}
}

LLSD LLNotificationForm::asLLSD() const
{ 
	return mFormData; 
}

LLSD LLNotificationForm::getElement(const std::string& element_name)
{
	for (LLSD::array_const_iterator it = mFormData.beginArray();
		it != mFormData.endArray();
		++it)
	{
		if ((*it)["name"].asString() == element_name) return (*it);
	}
	return LLSD();
}


bool LLNotificationForm::hasElement(const std::string& element_name)
{
	for (LLSD::array_const_iterator it = mFormData.beginArray();
		it != mFormData.endArray();
		++it)
	{
		if ((*it)["name"].asString() == element_name) return true;
	}
	return false;
}

void LLNotificationForm::addElement(const std::string& type, const std::string& name, const LLSD& value)
{
	LLSD element;
	element["type"] = type;
	element["name"] = name;
	element["text"] = name;
	element["value"] = value;
	element["index"] = mFormData.size();
	mFormData.append(element);
}

void LLNotificationForm::append(const LLSD& sub_form)
{
	if (sub_form.isArray())
	{
		for (LLSD::array_const_iterator it = sub_form.beginArray();
			it != sub_form.endArray();
			++it)
		{
			mFormData.append(*it);
		}
	}
}

void LLNotificationForm::formatElements(const LLSD& substitutions)
{
	for (LLSD::array_iterator it = mFormData.beginArray();
		it != mFormData.endArray();
		++it)
	{
		// format "text" component of each form element
		if ((*it).has("text"))
		{
			std::string text = (*it)["text"].asString();
			LLStringUtil::format(text, substitutions);
			(*it)["text"] = text;
		}
		if ((*it)["type"].asString() == "text" && (*it).has("value"))
		{
			std::string value = (*it)["value"].asString();
			LLStringUtil::format(value, substitutions);
			(*it)["value"] = value;
		}
	}
}

std::string LLNotificationForm::getDefaultOption()
{
	for (LLSD::array_const_iterator it = mFormData.beginArray();
		it != mFormData.endArray();
		++it)
	{
		if ((*it)["default"]) return (*it)["name"].asString();
	}
	return "";
}

LLNotificationTemplate::LLNotificationTemplate() :
	mExpireSeconds(0),
	mExpireOption(-1),
	mURLOption(-1),
	mURLOpenExternally(-1),
	mPersist(false),
	mUnique(false),
	mPriority(NOTIFICATION_PRIORITY_NORMAL)
{
	mForm = LLNotificationFormPtr(new LLNotificationForm()); 
}

LLNotification::LLNotification(const LLNotification::Params& p) : 
	mTimestamp(p.time_stamp), 
	mSubstitutions(p.substitutions),
	mPayload(p.payload),
	mExpiresAt(0),
	mTemporaryResponder(false),
	mRespondedTo(false),
	mPriority(p.priority),
	mCancelled(false),
	mIgnored(false),
	mResponderObj(NULL)
{
	if (p.functor.name.isChosen())
	{
		mResponseFunctorName = p.functor.name;
	}
	else if (p.functor.function.isChosen())
	{
		mResponseFunctorName = LLUUID::generateNewID().asString();
		LLNotificationFunctorRegistry::instance().registerFunctor(mResponseFunctorName, p.functor.function());

		mTemporaryResponder = true;
	}

	if(p.responder.isProvided())
	{
		mResponderObj = p.responder;
	}

	mId.generate();
	init(p.name, p.form_elements);
}


LLNotification::LLNotification(const LLSD& sd) :
	mTemporaryResponder(false),
	mRespondedTo(false),
	mCancelled(false),
	mIgnored(false),
	mResponderObj(NULL)
{ 
	mId.generate();
	mSubstitutions = sd["substitutions"];
	mPayload = sd["payload"]; 
	mTimestamp = sd["time"]; 
	mExpiresAt = sd["expiry"];
	mPriority = (ENotificationPriority)sd["priority"].asInteger();
	mResponseFunctorName = sd["responseFunctor"].asString();
	std::string templatename = sd["name"].asString();
	init(templatename, LLSD());
	// replace form with serialized version
	mForm = LLNotificationFormPtr(new LLNotificationForm(sd["form"]));
}


LLSD LLNotification::asLLSD()
{
	LLSD output;
	output["name"] = mTemplatep->mName;
	output["form"] = getForm()->asLLSD();
	output["substitutions"] = mSubstitutions;
	output["payload"] = mPayload;
	output["time"] = mTimestamp;
	output["expiry"] = mExpiresAt;
	output["priority"] = (S32)mPriority;
	output["responseFunctor"] = mResponseFunctorName;
	return output;
}

void LLNotification::update()
{
	LLNotifications::instance().update(shared_from_this());
}

void LLNotification::updateFrom(LLNotificationPtr other)
{
	// can only update from the same notification type
	if (mTemplatep != other->mTemplatep) return;

	// NOTE: do NOT change the ID, since it is the key to
	// this given instance, just update all the metadata
	//mId = other->mId;

	mPayload = other->mPayload;
	mSubstitutions = other->mSubstitutions;
	mTimestamp = other->mTimestamp;
	mExpiresAt = other->mExpiresAt;
	mCancelled = other->mCancelled;
	mIgnored = other->mIgnored;
	mPriority = other->mPriority;
	mForm = other->mForm;
	mResponseFunctorName = other->mResponseFunctorName;
	mRespondedTo = other->mRespondedTo;
	mResponse = other->mResponse;
	mTemporaryResponder = other->mTemporaryResponder;

	update();
}

const LLNotificationFormPtr LLNotification::getForm()
{
	return mForm;
}

void LLNotification::cancel()
{
	mCancelled = true;
}

LLSD LLNotification::getResponseTemplate(EResponseTemplateType type)
{
	LLSD response = LLSD::emptyMap();
	for (S32 element_idx = 0;
		element_idx < mForm->getNumElements();
		++element_idx)
	{
		LLSD element = mForm->getElement(element_idx);
		if (element.has("name"))
		{
			response[element["name"].asString()] = element["value"];
		}

		if ((type == WITH_DEFAULT_BUTTON) 
			&& element["default"].asBoolean())
		{
			response[element["name"].asString()] = true;
		}
	}
	return response;
}

//static
S32 LLNotification::getSelectedOption(const LLSD& notification, const LLSD& response)
{
	LLNotificationForm form(notification["form"]);

	for (S32 element_idx = 0;
		element_idx < form.getNumElements();
		++element_idx)
	{
		LLSD element = form.getElement(element_idx);

		// only look at buttons
		if (element["type"].asString() == "button" 
			&& response[element["name"].asString()].asBoolean())
		{
			return element["index"].asInteger();
		}
	}

	return -1;
}

//static
std::string LLNotification::getSelectedOptionName(const LLSD& response)
{
	for (LLSD::map_const_iterator response_it = response.beginMap();
		response_it != response.endMap();
		++response_it)
	{
		if (response_it->second.isBoolean() && response_it->second.asBoolean())
		{
			return response_it->first;
		}
	}
	return "";
}


void LLNotification::respond(const LLSD& response)
{
	// *TODO may remove mRespondedTo and use mResponce.isDefined() in isRespondedTo()
	mRespondedTo = true;
	mResponse = response;
	// look up the functor
	LLNotificationFunctorRegistry::ResponseFunctor functor = 
		LLNotificationFunctorRegistry::instance().getFunctor(mResponseFunctorName);
	// and then call it
	functor(asLLSD(), response);
	
	bool is_resusable = getPayload()["reusable"].asBoolean();

	if (mTemporaryResponder && !is_resusable)
	{
		LLNotificationFunctorRegistry::instance().unregisterFunctor(mResponseFunctorName);
		mResponseFunctorName = "";
		mTemporaryResponder = false;
	}

	if (mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO)
	{
		BOOL show_notification = mIgnored ? FALSE : TRUE;
		LLUI::sSettingGroups["ignores"]->setBOOL(getName(), show_notification);
		if (mIgnored && mForm->getIgnoreType() == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE)
		{
			LLUI::sSettingGroups["ignores"]->setLLSD("Default" + getName(), response);
		}
	}

	update();
}

void LLNotification::setIgnored(bool ignore)
{
	mIgnored = ignore;
}

void LLNotification::setResponseFunctor(std::string const &responseFunctorName)
{
	if (mTemporaryResponder)
		// get rid of the old one
		LLNotificationFunctorRegistry::instance().unregisterFunctor(mResponseFunctorName);
	mResponseFunctorName = responseFunctorName;
	mTemporaryResponder = false;
}

void LLNotification::setResponseFunctor(const LLNotificationFunctorRegistry::ResponseFunctor& cb)
{
	if(mTemporaryResponder)
	{
		LLNotificationFunctorRegistry::instance().unregisterFunctor(mResponseFunctorName);
	}

	LLNotificationFunctorRegistry::instance().registerFunctor(mResponseFunctorName, cb);
}

bool LLNotification::payloadContainsAll(const std::vector<std::string>& required_fields) const
{
	for(std::vector<std::string>::const_iterator required_fields_it = required_fields.begin(); 
		required_fields_it != required_fields.end();
		required_fields_it++)
	{
		std::string required_field_name = *required_fields_it;
		if( ! getPayload().has(required_field_name))
		{
			return false; // a required field was not found
		}
	}
	return true; // all required fields were found
}

bool LLNotification::isEquivalentTo(LLNotificationPtr that) const
{
	if (this->mTemplatep->mName != that->mTemplatep->mName) 
	{
		return false; // must have the same template name or forget it
	}
	if (this->mTemplatep->mUnique)
	{
		// highlander bit sez there can only be one of these
		return
			this->payloadContainsAll(that->mTemplatep->mUniqueContext) &&
			that->payloadContainsAll(this->mTemplatep->mUniqueContext);
	}
	return false; 
}

void LLNotification::init(const std::string& template_name, const LLSD& form_elements)
{
	mTemplatep = LLNotifications::instance().getTemplate(template_name);
	if (!mTemplatep) return;

	// add default substitutions
	const LLStringUtil::format_map_t& default_args = LLTrans::getDefaultArgs();
	for (LLStringUtil::format_map_t::const_iterator iter = default_args.begin();
		 iter != default_args.end(); ++iter)
	{
		mSubstitutions[iter->first] = iter->second;
	}
	mSubstitutions["_URL"] = getURL();
	mSubstitutions["_NAME"] = template_name;
	// TODO: something like this so that a missing alert is sensible:
	//mSubstitutions["_ARGS"] = get_all_arguments_as_text(mSubstitutions);

	mForm = LLNotificationFormPtr(new LLNotificationForm(*mTemplatep->mForm));
	mForm->append(form_elements);

	// apply substitution to form labels
	mForm->formatElements(mSubstitutions);

	LLDate rightnow = LLDate::now();
	if (mTemplatep->mExpireSeconds)
	{
		mExpiresAt = LLDate(rightnow.secondsSinceEpoch() + mTemplatep->mExpireSeconds);
	}

	if (mPriority == NOTIFICATION_PRIORITY_UNSPECIFIED)
	{
		mPriority = mTemplatep->mPriority;
	}
}

std::string LLNotification::summarize() const
{
	std::string s = "Notification(";
	s += getName();
	s += ") : ";
	s += mTemplatep ? mTemplatep->mMessage : "";
	// should also include timestamp and expiration time (but probably not payload)
	return s;
}

std::string LLNotification::getMessage() const
{
	// all our callers cache this result, so it gives us more flexibility
	// to do the substitution at call time rather than attempting to 
	// cache it in the notification
	if (!mTemplatep)
		return std::string();

	std::string message = mTemplatep->mMessage;
	LLStringUtil::format(message, mSubstitutions);
	return message;
}

std::string LLNotification::getLabel() const
{
	std::string label = mTemplatep->mLabel;
	LLStringUtil::format(label, mSubstitutions);
	return (mTemplatep ? label : "");
}

std::string LLNotification::getURL() const
{
	if (!mTemplatep)
		return std::string();
	std::string url = mTemplatep->mURL;
	LLStringUtil::format(url, mSubstitutions);
	return (mTemplatep ? url : "");
}

// =========================================================
// LLNotificationChannel implementation
// ---
LLBoundListener LLNotificationChannelBase::connectChangedImpl(const LLEventListener& slot)
{
	// when someone wants to connect to a channel, we first throw them
	// all of the notifications that are already in the channel
	// we use a special signal called "load" in case the channel wants to care
	// only about new notifications
	for (LLNotificationSet::iterator it = mItems.begin(); it != mItems.end(); ++it)
	{
		slot(LLSD().with("sigtype", "load").with("id", (*it)->id()));
	}
	// and then connect the signal so that all future notifications will also be
	// forwarded.
	return mChanged.connect(slot);
}

LLBoundListener LLNotificationChannelBase::connectAtFrontChangedImpl(const LLEventListener& slot)
{
	for (LLNotificationSet::iterator it = mItems.begin(); it != mItems.end(); ++it)
	{
		slot(LLSD().with("sigtype", "load").with("id", (*it)->id()));
	}
	return mChanged.connect(slot, boost::signals2::at_front);
}

LLBoundListener LLNotificationChannelBase::connectPassedFilterImpl(const LLEventListener& slot)
{
	// these two filters only fire for notifications added after the current one, because
	// they don't participate in the hierarchy.
	return mPassedFilter.connect(slot);
}

LLBoundListener LLNotificationChannelBase::connectFailedFilterImpl(const LLEventListener& slot)
{
	return mFailedFilter.connect(slot);
}

// external call, conforms to our standard signature
bool LLNotificationChannelBase::updateItem(const LLSD& payload)
{	
	// first check to see if it's in the master list
	LLNotificationPtr pNotification	 = LLNotifications::instance().find(payload["id"]);
	if (!pNotification)
		return false;	// not found
	
	return updateItem(payload, pNotification);
}


//FIX QUIT NOT WORKING


// internal call, for use in avoiding lookup
bool LLNotificationChannelBase::updateItem(const LLSD& payload, LLNotificationPtr pNotification)
{	
	std::string cmd = payload["sigtype"];
	LLNotificationSet::iterator foundItem = mItems.find(pNotification);
	bool wasFound = (foundItem != mItems.end());
	bool passesFilter = mFilter(pNotification);
	
	// first, we offer the result of the filter test to the simple
	// signals for pass/fail. One of these is guaranteed to be called.
	// If either signal returns true, the change processing is NOT performed
	// (so don't return true unless you know what you're doing!)
	bool abortProcessing = false;
	if (passesFilter)
	{
		abortProcessing = mPassedFilter(payload);
	}
	else
	{
		abortProcessing = mFailedFilter(payload);
	}
	
	if (abortProcessing)
	{
		return true;
	}
	
	if (cmd == "load")
	{
		// should be no reason we'd ever get a load if we already have it
		// if passes filter send a load message, else do nothing
		assert(!wasFound);
		if (passesFilter)
		{
			// not in our list, add it and say so
			mItems.insert(pNotification);
			abortProcessing = mChanged(payload);
			onLoad(pNotification);
		}
	}
	else if (cmd == "change")
	{
		// if it passes filter now and was found, we just send a change message
		// if it passes filter now and wasn't found, we have to add it
		// if it doesn't pass filter and wasn't found, we do nothing
		// if it doesn't pass filter and was found, we need to delete it
		if (passesFilter)
		{
			if (wasFound)
			{
				// it already existed, so this is a change
				// since it changed in place, all we have to do is resend the signal
				abortProcessing = mChanged(payload);
				onChange(pNotification);
			}
			else
			{
				// not in our list, add it and say so
				mItems.insert(pNotification);
				// our payload is const, so make a copy before changing it
				LLSD newpayload = payload;
				newpayload["sigtype"] = "add";
				abortProcessing = mChanged(newpayload);
				onChange(pNotification);
			}
		}
		else
		{
			if (wasFound)
			{
				// it already existed, so this is a delete
				mItems.erase(pNotification);
				// our payload is const, so make a copy before changing it
				LLSD newpayload = payload;
				newpayload["sigtype"] = "delete";
				abortProcessing = mChanged(newpayload);
				onChange(pNotification);
			}
			// didn't pass, not on our list, do nothing
		}
	}
	else if (cmd == "add")
	{
		// should be no reason we'd ever get an add if we already have it
		// if passes filter send an add message, else do nothing
		assert(!wasFound);
		if (passesFilter)
		{
			// not in our list, add it and say so
			mItems.insert(pNotification);
			abortProcessing = mChanged(payload);
			onAdd(pNotification);
		}
	}
	else if (cmd == "delete")
	{
		// if we have it in our list, pass on the delete, then delete it, else do nothing
		if (wasFound)
		{
			abortProcessing = mChanged(payload);
			// do not delete the notification to make LLChatHistory::appendMessage add notification panel to IM window
			if( ! pNotification->getPayload()["reusable"].asBoolean() )
			{
				mItems.erase(pNotification);
			}
			onDelete(pNotification);
		}
	}
	return abortProcessing;
}

/* static */
LLNotificationChannelPtr LLNotificationChannel::buildChannel(const std::string& name, 
															 const std::string& parent,
															 LLNotificationFilter filter, 
															 LLNotificationComparator comparator)
{
	// note: this is not a leak; notifications are self-registering.
	// This factory helps to prevent excess deletions by making sure all smart
	// pointers to notification channels come from the same source
	new LLNotificationChannel(name, parent, filter, comparator);
	return LLNotifications::instance().getChannel(name);
}


LLNotificationChannel::LLNotificationChannel(const std::string& name, 
											 const std::string& parent,
											 LLNotificationFilter filter, 
											 LLNotificationComparator comparator) : 
LLNotificationChannelBase(filter, comparator),
mName(name),
mParent(parent)
{
	// store myself in the channel map
	LLNotifications::instance().addChannel(LLNotificationChannelPtr(this));
	// bind to notification broadcast
	if (parent.empty())
	{
		LLNotifications::instance().connectChanged(
			boost::bind(&LLNotificationChannelBase::updateItem, this, _1));
	}
	else
	{
		LLNotificationChannelPtr p = LLNotifications::instance().getChannel(parent);
		p->connectChanged(boost::bind(&LLNotificationChannelBase::updateItem, this, _1));
	}
}


void LLNotificationChannel::setComparator(LLNotificationComparator comparator) 
{ 
	mComparator = comparator; 
	LLNotificationSet s2(mComparator);
	s2.insert(mItems.begin(), mItems.end());
	mItems.swap(s2);
	
	// notify clients that we've been resorted
	mChanged(LLSD().with("sigtype", "sort")); 
}

bool LLNotificationChannel::isEmpty() const
{
	return mItems.empty();
}

LLNotificationChannel::Iterator LLNotificationChannel::begin()
{
	return mItems.begin();
}

LLNotificationChannel::Iterator LLNotificationChannel::end()
{
	return mItems.end();
}

std::string LLNotificationChannel::summarize()
{
	std::string s("Channel '");
	s += mName;
	s += "'\n  ";
	for (LLNotificationChannel::Iterator it = begin(); it != end(); ++it)
	{
		s += (*it)->summarize();
		s += "\n  ";
	}
	return s;
}


// ---
// END OF LLNotificationChannel implementation
// =========================================================


// =========================================================
// LLNotifications implementation
// ---
LLNotifications::LLNotifications() : LLNotificationChannelBase(LLNotificationFilters::includeEverything,
															   LLNotificationComparators::orderByUUID()),
									mIgnoreAllNotifications(false)
{
	LLUICtrl::CommitCallbackRegistry::currentRegistrar().add("Notification.Show", boost::bind(&LLNotifications::addFromCallback, this, _2));

    mListener.reset(new LLNotificationsListener(*this));
}


// The expiration channel gets all notifications that are cancelled
bool LLNotifications::expirationFilter(LLNotificationPtr pNotification)
{
	return pNotification->isCancelled() || pNotification->isRespondedTo();
}

bool LLNotifications::expirationHandler(const LLSD& payload)
{
	if (payload["sigtype"].asString() != "delete")
	{
		// anything added to this channel actually should be deleted from the master
		cancel(find(payload["id"]));
		return true;	// don't process this item any further
	}
	return false;
}

bool LLNotifications::uniqueFilter(LLNotificationPtr pNotif)
{
	if (!pNotif->hasUniquenessConstraints())
	{
		return true;
	}

	// checks against existing unique notifications
	for (LLNotificationMap::iterator existing_it = mUniqueNotifications.find(pNotif->getName());
		existing_it != mUniqueNotifications.end();
		++existing_it)
	{
		LLNotificationPtr existing_notification = existing_it->second;
		if (pNotif != existing_notification 
			&& pNotif->isEquivalentTo(existing_notification))
		{
			return false;
		}
	}

	return true;
}

bool LLNotifications::uniqueHandler(const LLSD& payload)
{
	LLNotificationPtr pNotif = LLNotifications::instance().find(payload["id"].asUUID());
	if (pNotif && pNotif->hasUniquenessConstraints()) 
	{
		if (payload["sigtype"].asString() == "add")
		{
			// not a duplicate according to uniqueness criteria, so we keep it
			// and store it for future uniqueness checks
			mUniqueNotifications.insert(std::make_pair(pNotif->getName(), pNotif));
		}
		else if (payload["sigtype"].asString() == "delete")
		{
			mUniqueNotifications.erase(pNotif->getName());
		}
	}

	return false;
}

bool LLNotifications::failedUniquenessTest(const LLSD& payload)
{
	LLNotificationPtr pNotif = LLNotifications::instance().find(payload["id"].asUUID());
	
	if (!pNotif || !pNotif->hasUniquenessConstraints())
	{
		return false;
	}

	// checks against existing unique notifications
	for (LLNotificationMap::iterator existing_it = mUniqueNotifications.find(pNotif->getName());
		existing_it != mUniqueNotifications.end();
		++existing_it)
	{
		LLNotificationPtr existing_notification = existing_it->second;
		if (pNotif != existing_notification 
			&& pNotif->isEquivalentTo(existing_notification))
		{
			// copy notification instance data over to oldest instance
			// of this unique notification and update it
			existing_notification->updateFrom(pNotif);
			// then delete the new one
			pNotif->cancel();
		}
	}

	return false;
}


void LLNotifications::addChannel(LLNotificationChannelPtr pChan)
{
	mChannels[pChan->getName()] = pChan;
}

LLNotificationChannelPtr LLNotifications::getChannel(const std::string& channelName)
{
	ChannelMap::iterator p = mChannels.find(channelName);
	if(p == mChannels.end())
	{
		llerrs << "Did not find channel named " << channelName << llendl;
		return LLNotificationChannelPtr();
	}
	return p->second;
}


// this function is called once at construction time, after the object is constructed.
void LLNotifications::initSingleton()
{
	loadTemplates();
	createDefaultChannels();
}

void LLNotifications::createDefaultChannels()
{
	// now construct the various channels AFTER loading the notifications,
	// because the history channel is going to rewrite the stored notifications file
	LLNotificationChannel::buildChannel("Expiration", "",
		boost::bind(&LLNotifications::expirationFilter, this, _1));
	LLNotificationChannel::buildChannel("Unexpired", "",
		!boost::bind(&LLNotifications::expirationFilter, this, _1)); // use negated bind
	LLNotificationChannel::buildChannel("Unique", "Unexpired",
		boost::bind(&LLNotifications::uniqueFilter, this, _1));
	LLNotificationChannel::buildChannel("Ignore", "Unique",
		filterIgnoredNotifications);
	LLNotificationChannel::buildChannel("Visible", "Ignore",
		&LLNotificationFilters::includeEverything);

	// create special history channel
	//std::string notifications_log_file = gDirUtilp->getExpandedFilename ( LL_PATH_PER_SL_ACCOUNT, "open_notifications.xml" );
	// use ^^^ when done debugging notifications serialization
	std::string notifications_log_file = gDirUtilp->getExpandedFilename ( LL_PATH_USER_SETTINGS, "open_notifications.xml" );
	// this isn't a leak, don't worry about the empty "new"
	new LLNotificationHistoryChannel(notifications_log_file);

	// connect action methods to these channels
	LLNotifications::instance().getChannel("Expiration")->
        connectChanged(boost::bind(&LLNotifications::expirationHandler, this, _1));
	// uniqueHandler slot should be added as first slot of the signal due to
	// usage LLStopWhenHandled combiner in LLStandardSignal
	LLNotifications::instance().getChannel("Unique")->
        connectAtFrontChanged(boost::bind(&LLNotifications::uniqueHandler, this, _1));
// failedUniquenessTest slot isn't necessary
//	LLNotifications::instance().getChannel("Unique")->
//        connectFailedFilter(boost::bind(&LLNotifications::failedUniquenessTest, this, _1));
	LLNotifications::instance().getChannel("Ignore")->
		connectFailedFilter(&handleIgnoredNotification);
}

bool LLNotifications::addTemplate(const std::string &name, 
								  LLNotificationTemplatePtr theTemplate)
{
	if (mTemplates.count(name))
	{
		llwarns << "LLNotifications -- attempted to add template '" << name << "' twice." << llendl;
		return false;
	}
	mTemplates[name] = theTemplate;
	return true;
}

LLNotificationTemplatePtr LLNotifications::getTemplate(const std::string& name)
{
	if (mTemplates.count(name))
	{
		return mTemplates[name];
	}
	else
	{
		return mTemplates["MissingAlert"];
	}
}

bool LLNotifications::templateExists(const std::string& name)
{
	return (mTemplates.count(name) != 0);
}

void LLNotifications::clearTemplates()
{
	mTemplates.clear();
}

void LLNotifications::forceResponse(const LLNotification::Params& params, S32 option)
{
	LLNotificationPtr temp_notify(new LLNotification(params));
	LLSD response = temp_notify->getResponseTemplate();
	LLSD selected_item = temp_notify->getForm()->getElement(option);
	
	if (selected_item.isUndefined())
	{
		llwarns << "Invalid option" << option << " for notification " << (std::string)params.name << llendl;
		return;
	}
	response[selected_item["name"].asString()] = true;

	temp_notify->respond(response);
}

LLNotifications::TemplateNames LLNotifications::getTemplateNames() const
{
	TemplateNames names;
	for (TemplateMap::const_iterator it = mTemplates.begin(); it != mTemplates.end(); ++it)
	{
		names.push_back(it->first);
	}
	return names;
}

typedef std::map<std::string, std::string> StringMap;
void replaceSubstitutionStrings(LLXMLNodePtr node, StringMap& replacements)
{
	//llwarns << "replaceSubstitutionStrings" << llendl;
	// walk the list of attributes looking for replacements
	for (LLXMLAttribList::iterator it=node->mAttributes.begin();
		 it != node->mAttributes.end(); ++it)
	{
		std::string value = it->second->getValue();
		if (value[0] == '$')
		{
			value.erase(0, 1);	// trim off the $
			std::string replacement;
			StringMap::const_iterator found = replacements.find(value);
			if (found != replacements.end())
			{
				replacement = found->second;
				//llwarns << "replaceSubstituionStrings: value: " << value << " repl: " << replacement << llendl;

				it->second->setValue(replacement);
			}
			else
			{
				llwarns << "replaceSubstituionStrings FAILURE: value: " << value << " repl: " << replacement << llendl;
			}
		}
	}
	
	// now walk the list of children and call this recursively.
	for (LLXMLNodePtr child = node->getFirstChild(); 
		 child.notNull(); child = child->getNextSibling())
	{
		replaceSubstitutionStrings(child, replacements);
	}
}

// private to this file
// returns true if the template request was invalid and there's nothing else we
// can do with this node, false if you should keep processing (it may have
// replaced the contents of the node referred to)
LLXMLNodePtr LLNotifications::checkForXMLTemplate(LLXMLNodePtr item)
{
	if (item->hasName("usetemplate"))
	{
		std::string replacementName;
		if (item->getAttributeString("name", replacementName))
		{
			StringMap replacements;
			for (LLXMLAttribList::const_iterator it=item->mAttributes.begin(); 
				 it != item->mAttributes.end(); ++it)
			{
				replacements[it->second->getName()->mString] = it->second->getValue();
			}
			if (mXmlTemplates.count(replacementName))
			{
				item=LLXMLNode::replaceNode(item, mXmlTemplates[replacementName]);
				
				// walk the nodes looking for $(substitution) here and replace
				replaceSubstitutionStrings(item, replacements);
			}
			else
			{
				llwarns << "XML template lookup failure on '" << replacementName << "' " << llendl;
			}
		}
	}
	return item;
}

bool LLNotifications::loadTemplates()
{
	const std::string xml_filename = "notifications.xml";
	LLXMLNodePtr root;
	
	BOOL success  = LLUICtrlFactory::getLayeredXMLNode(xml_filename, root);
	
	if (!success || root.isNull() || !root->hasName( "notifications" ))
	{
		llerrs << "Problem reading UI Notifications file: " << xml_filename << llendl;
		return false;
	}
	
	clearTemplates();
	
	for (LLXMLNodePtr item = root->getFirstChild();
		 item.notNull(); item = item->getNextSibling())
	{
		// we do this FIRST so that item can be changed if we 
		// encounter a usetemplate -- we just replace the
		// current xml node and keep processing
		item = checkForXMLTemplate(item);
		
		if (item->hasName("global"))
		{
			std::string global_name;
			if (item->getAttributeString("name", global_name))
			{
				mGlobalStrings[global_name] = item->getTextContents();
			}
			continue;
		}
		
		if (item->hasName("template"))
		{
			// store an xml template; templates must have a single node (can contain
			// other nodes)
			std::string name;
			item->getAttributeString("name", name);
			LLXMLNodePtr ptr = item->getFirstChild();
			mXmlTemplates[name] = ptr;
			continue;
		}
		
		if (!item->hasName("notification"))
		{
            llwarns << "Unexpected entity " << item->getName()->mString << 
                       " found in " << xml_filename << llendl;
			continue;
		}
		
		// now we know we have a notification entry, so let's build it
		LLNotificationTemplatePtr pTemplate(new LLNotificationTemplate());

		if (!item->getAttributeString("name", pTemplate->mName))
		{
			llwarns << "Unable to parse notification with no name" << llendl;
			continue;
		}
		
		//llinfos << "Parsing " << pTemplate->mName << llendl;
		
		pTemplate->mMessage = item->getTextContents();
		pTemplate->mDefaultFunctor = pTemplate->mName;
		item->getAttributeString("type", pTemplate->mType);
		item->getAttributeString("icon", pTemplate->mIcon);
		item->getAttributeString("label", pTemplate->mLabel);
		item->getAttributeU32("duration", pTemplate->mExpireSeconds);
		item->getAttributeU32("expireOption", pTemplate->mExpireOption);

		std::string priority;
		item->getAttributeString("priority", priority);
		pTemplate->mPriority = NOTIFICATION_PRIORITY_NORMAL;
		if (!priority.empty())
		{
			if (priority == "low")      pTemplate->mPriority = NOTIFICATION_PRIORITY_LOW;
			if (priority == "normal")   pTemplate->mPriority = NOTIFICATION_PRIORITY_NORMAL;
			if (priority == "high")     pTemplate->mPriority = NOTIFICATION_PRIORITY_HIGH;
			if (priority == "critical") pTemplate->mPriority = NOTIFICATION_PRIORITY_CRITICAL;
		}
		
		item->getAttributeString("functor", pTemplate->mDefaultFunctor);

		BOOL persist = false;
		item->getAttributeBOOL("persist", persist);
		pTemplate->mPersist = persist;
		
		std::string sound;
		item->getAttributeString("sound", sound);
		if (!sound.empty())
		{
			// test for bad sound effect name / missing effect
			if (LLUI::sSettingGroups["config"]->controlExists(sound))
			{
				pTemplate->mSoundEffect = 
					LLUUID(LLUI::sSettingGroups["config"]->getString(sound));
			}
			else
			{
				llwarns << "Unknown sound effect control name " << sound
					<< llendl;
			}
		}

		for (LLXMLNodePtr child = item->getFirstChild();
			 !child.isNull(); child = child->getNextSibling())
		{
			child = checkForXMLTemplate(child);
			
			// <url>
			if (child->hasName("url"))
			{
				pTemplate->mURL = child->getTextContents();
				child->getAttributeU32("option", pTemplate->mURLOption);
				child->getAttributeU32("openexternally", pTemplate->mURLOpenExternally);
			}
			
            if (child->hasName("unique"))
            {
                pTemplate->mUnique = true;
                for (LLXMLNodePtr formitem = child->getFirstChild();
                     !formitem.isNull(); formitem = formitem->getNextSibling())
                {
                    if (formitem->hasName("context"))
                    {
                        std::string key;
                        formitem->getAttributeString("key", key);
                        pTemplate->mUniqueContext.push_back(key);
                        //llwarns << "adding " << key << " to unique context" << llendl;
                    }
                    else
                    {
                        llwarns << "'unique' has unrecognized subelement " 
                        << formitem->getName()->mString << llendl;
                    }
                }
            }
            
			// <form>
			if (child->hasName("form"))
			{
                pTemplate->mForm = LLNotificationFormPtr(new LLNotificationForm(pTemplate->mName, child));
			}
		}
		addTemplate(pTemplate->mName, pTemplate);
	}
	
	//std::ostringstream ostream;
	//root->writeToOstream(ostream, "\n  ");
	//llwarns << ostream.str() << llendl;
	
	return true;
}

// Add a simple notification (from XUI)
void LLNotifications::addFromCallback(const LLSD& name)
{
	add(LLNotification::Params().name(name.asString()));	
}

LLNotificationPtr LLNotifications::add(const std::string& name, 
									   const LLSD& substitutions, 
									   const LLSD& payload)
{
	LLNotification::Params::Functor functor_p;
	functor_p.name = name;
	return add(LLNotification::Params().name(name).substitutions(substitutions).payload(payload).functor(functor_p));	
}

LLNotificationPtr LLNotifications::add(const std::string& name, 
									   const LLSD& substitutions, 
									   const LLSD& payload, 
									   const std::string& functor_name)
{
	LLNotification::Params::Functor functor_p;
	functor_p.name = functor_name;
	return add(LLNotification::Params().name(name).substitutions(substitutions).payload(payload).functor(functor_p));	
}
							  
//virtual
LLNotificationPtr LLNotifications::add(const std::string& name, 
										const LLSD& substitutions, 
										const LLSD& payload, 
										LLNotificationFunctorRegistry::ResponseFunctor functor)
{
	LLNotification::Params::Functor functor_p;
	functor_p.function = functor;
	return add(LLNotification::Params().name(name).substitutions(substitutions).payload(payload).functor(functor_p));	
}

// generalized add function that takes a parameter block object for more complex instantiations
LLNotificationPtr LLNotifications::add(const LLNotification::Params& p)
{
	LLNotificationPtr pNotif(new LLNotification(p));
	add(pNotif);
	return pNotif;
}


void LLNotifications::add(const LLNotificationPtr pNotif)
{
	// first see if we already have it -- if so, that's a problem
	LLNotificationSet::iterator it=mItems.find(pNotif);
	if (it != mItems.end())
	{
		llerrs << "Notification added a second time to the master notification channel." << llendl;
	}

	updateItem(LLSD().with("sigtype", "add").with("id", pNotif->id()), pNotif);
}

void LLNotifications::cancel(LLNotificationPtr pNotif)
{
	LLNotificationSet::iterator it=mItems.find(pNotif);
	if (it == mItems.end())
	{
		llerrs << "Attempted to delete nonexistent notification " << pNotif->getName() << llendl;
	}
	pNotif->cancel();
	updateItem(LLSD().with("sigtype", "delete").with("id", pNotif->id()), pNotif);
}

void LLNotifications::update(const LLNotificationPtr pNotif)
{
	LLNotificationSet::iterator it=mItems.find(pNotif);
	if (it != mItems.end())
	{
		updateItem(LLSD().with("sigtype", "change").with("id", pNotif->id()), pNotif);
	}
}


LLNotificationPtr LLNotifications::find(LLUUID uuid)
{
	LLNotificationPtr target = LLNotificationPtr(new LLNotification(uuid));
	LLNotificationSet::iterator it=mItems.find(target);
	if (it == mItems.end())
	{
		llwarns << "Tried to dereference uuid '" << uuid << "' as a notification key but didn't find it." << llendl;
		return LLNotificationPtr((LLNotification*)NULL);
	}
	else
	{
		return *it;
	}
}

void LLNotifications::forEachNotification(NotificationProcess process)
{
	std::for_each(mItems.begin(), mItems.end(), process);
}

std::string LLNotifications::getGlobalString(const std::string& key) const
{
	GlobalStringMap::const_iterator it = mGlobalStrings.find(key);
	if (it != mGlobalStrings.end())
	{
		return it->second;
	}
	else
	{
		// if we don't have the key as a global, return the key itself so that the error
		// is self-diagnosing.
		return key;
	}
}

void LLNotifications::setIgnoreAllNotifications(bool setting)
{
	mIgnoreAllNotifications = setting; 
}
bool LLNotifications::getIgnoreAllNotifications()
{
	return mIgnoreAllNotifications; 
}
													
// ---
// END OF LLNotifications implementation
// =========================================================

std::ostream& operator<<(std::ostream& s, const LLNotification& notification)
{
	s << notification.summarize();
	return s;
}

