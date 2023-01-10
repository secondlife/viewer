/**
* @file llnotifications.cpp
* @brief Non-UI queue manager for keeping a prioritized list of notifications
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

#include "linden_common.h"

#include "llnotifications.h"
#include "llnotificationtemplate.h"
#include "llnotificationvisibilityrule.h"

#include "llavatarnamecache.h"
#include "llinstantmessage.h"
#include "llcachename.h"
#include "llxmlnode.h"
#include "lluictrl.h"
#include "lluictrlfactory.h"
#include "lldir.h"
#include "llsdserialize.h"
#include "lltrans.h"
#include "llstring.h"
#include "llsdparam.h"
#include "llsdutil.h"

#include <algorithm>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>


const std::string NOTIFICATION_PERSIST_VERSION = "0.93";

void NotificationPriorityValues::declareValues()
{
	declare("low", NOTIFICATION_PRIORITY_LOW);
	declare("normal", NOTIFICATION_PRIORITY_NORMAL);
	declare("high", NOTIFICATION_PRIORITY_HIGH);
	declare("critical", NOTIFICATION_PRIORITY_CRITICAL);
}

LLNotificationForm::FormElementBase::FormElementBase()
:	name("name"),
	enabled("enabled", true)
{}

LLNotificationForm::FormIgnore::FormIgnore()
:	text("text"),
	control("control"),
	invert_control("invert_control", false),
	save_option("save_option", false),
	session_only("session_only", false),
	checkbox_only("checkbox_only", false)
{}

LLNotificationForm::FormButton::FormButton()
:	index("index"),
	text("text"),
	ignore("ignore"),
	is_default("default"),
	width("width", 0),
	type("type")
{
	// set type here so it gets serialized
	type = "button";
}

LLNotificationForm::FormInput::FormInput()
:	type("type"),
	text("text"),
	max_length_chars("max_length_chars"),
	width("width", 0),
	value("value")
{}

LLNotificationForm::FormElement::FormElement()
:	button("button"),
	input("input")
{}

LLNotificationForm::FormElements::FormElements()
:	elements("")
{}

LLNotificationForm::Params::Params()
:	name("name"),
	ignore("ignore"),
	form_elements("")
{}



bool filterIgnoredNotifications(LLNotificationPtr notification)
{
	LLNotificationFormPtr form = notification->getForm();
	// Check to see if the user wants to ignore this alert
	return !notification->getForm()->getIgnored();
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
		case LLNotificationForm::IGNORE_WITH_DEFAULT_RESPONSE_SESSION_ONLY:
			response = pNotif->getResponseTemplate(LLNotification::WITH_DEFAULT_BUTTON);
			break;
		case LLNotificationForm::IGNORE_WITH_LAST_RESPONSE:
			response = LLUI::getInstance()->mSettingGroups["ignores"]->getLLSD("Default" + pNotif->getName());
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

bool defaultResponse(const LLSD& payload)
{
	if (payload["sigtype"].asString() == "add")
	{
		LLNotificationPtr pNotif = LLNotifications::instance().find(payload["id"].asUUID());
		if (pNotif) 
		{
			// supply default response
			pNotif->respond(pNotif->getResponseTemplate(LLNotification::WITH_DEFAULT_BUTTON));
		}
	}
	return false;
}

bool visibilityRuleMached(const LLSD& payload)
{
	// This is needed because LLNotifications::isVisibleByRules may have cancelled the notification.
	// Returning true here makes LLNotificationChannelBase::updateItem do an early out, which prevents things from happening in the wrong order.
	return true;
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
:	mIgnore(IGNORE_NO)
{
}

LLNotificationForm::LLNotificationForm( const LLNotificationForm& other )
{
	mFormData 	   = other.mFormData;
	mIgnore 	   = other.mIgnore;
	mIgnoreMsg 	   = other.mIgnoreMsg;
	mIgnoreSetting = other.mIgnoreSetting;
	mInvertSetting = other.mInvertSetting;
}

LLNotificationForm::LLNotificationForm(const std::string& name, const LLNotificationForm::Params& p) 
:	mIgnore(IGNORE_NO),
	mInvertSetting(false) // ignore settings by default mean true=show, false=ignore
{
	if (p.ignore.isProvided())
	{
		// For all cases but IGNORE_CHECKBOX_ONLY this is name for use in preferences
		mIgnoreMsg = p.ignore.text;

		LLUI *ui_inst = LLUI::getInstance();
		if (p.ignore.checkbox_only)
		{
			mIgnore = IGNORE_CHECKBOX_ONLY;
		}
		else if (!p.ignore.save_option)
		{
			mIgnore = p.ignore.session_only ? IGNORE_WITH_DEFAULT_RESPONSE_SESSION_ONLY : IGNORE_WITH_DEFAULT_RESPONSE;
		}
		else
		{
			// remember last option chosen by user and automatically respond with that in the future
			mIgnore = IGNORE_WITH_LAST_RESPONSE;
			ui_inst->mSettingGroups["ignores"]->declareLLSD(std::string("Default") + name, "", std::string("Default response for notification " + name));
		}

		BOOL show_notification = TRUE;
		if (p.ignore.control.isProvided())
		{
			mIgnoreSetting = ui_inst->mSettingGroups["config"]->getControl(p.ignore.control);
			mInvertSetting = p.ignore.invert_control;
		}
		else if (mIgnore > IGNORE_NO)
		{
			ui_inst->mSettingGroups["ignores"]->declareBOOL(name, show_notification, "Show notification with this name", LLControlVariable::PERSIST_NONDFT);
			mIgnoreSetting = ui_inst->mSettingGroups["ignores"]->getControl(name);
		}
	}

	LLParamSDParser parser;
	parser.writeSD(mFormData, p.form_elements);

	for (LLSD::array_iterator it = mFormData.beginArray(), end_it = mFormData.endArray();
		it != end_it;
		++it)
	{
		// lift contents of form element up a level, since element type is already encoded in "type" param
		if (it->isMap() && it->beginMap() != it->endMap())
		{
			*it = it->beginMap()->second;
		}
	}

	LL_DEBUGS("Notifications") << name << LL_ENDL;
	LL_DEBUGS("Notifications") << ll_pretty_print_sd(mFormData) << LL_ENDL;
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
		LL_WARNS("Notifications") << "Invalid form data " << sd << LL_ENDL;
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


bool LLNotificationForm::hasElement(const std::string& element_name) const
{
	for (LLSD::array_const_iterator it = mFormData.beginArray();
		it != mFormData.endArray();
		++it)
	{
		if ((*it)["name"].asString() == element_name) return true;
	}
	return false;
}

void LLNotificationForm::getElements(LLSD& elements, S32 offset)
{
    //Finds elements that the template did not add
    LLSD::array_const_iterator it = mFormData.beginArray() + offset;

    //Keeps track of only the dynamic elements
    for(; it != mFormData.endArray(); ++it)
    {
        elements.append(*it);
    }
}

bool LLNotificationForm::getElementEnabled(const std::string& element_name) const
{
	for (LLSD::array_const_iterator it = mFormData.beginArray();
		it != mFormData.endArray();
		++it)
	{
		if ((*it)["name"].asString() == element_name)
		{
			return (*it)["enabled"].asBoolean();
		}
	}

	return false;
}

void LLNotificationForm::setElementEnabled(const std::string& element_name, bool enabled)
{
	for (LLSD::array_iterator it = mFormData.beginArray();
		it != mFormData.endArray();
		++it)
	{
		if ((*it)["name"].asString() == element_name)
		{
			(*it)["enabled"] = enabled;
		}
	}
}


void LLNotificationForm::addElement(const std::string& type, const std::string& name, const LLSD& value, bool enabled)
{
	LLSD element;
	element["type"] = type;
	element["name"] = name;
	element["text"] = name;
	element["value"] = value;
	element["index"] = LLSD::Integer(mFormData.size());
	element["enabled"] = enabled;
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

LLControlVariablePtr LLNotificationForm::getIgnoreSetting() 
{ 
	return mIgnoreSetting; 
}

bool LLNotificationForm::getIgnored()
{
	bool show = true;
	if (mIgnore > LLNotificationForm::IGNORE_NO
		&& mIgnoreSetting) 
	{
		show = mIgnoreSetting->getValue().asBoolean();
		if (mInvertSetting) show = !show;
	}
	return !show;
}

void LLNotificationForm::setIgnored(bool ignored)
{
	if (mIgnoreSetting)
	{
		if (mInvertSetting) ignored = !ignored;
		mIgnoreSetting->setValue(!ignored);
	}
}

LLNotificationTemplate::LLNotificationTemplate(const LLNotificationTemplate::Params& p)
:	mName(p.name),
	mType(p.type),
	mMessage(p.value),
	mFooter(p.footer.value),
	mLabel(p.label),
	mIcon(p.icon),
	mURL(p.url.value),
	mExpireSeconds(p.duration),
	mExpireOption(p.expire_option),
	mURLOption(p.url.option),
	mURLTarget(p.url.target),
	mForceUrlsExternal(p.force_urls_external),
	mUnique(p.unique.isProvided()),
	mCombineBehavior(p.unique.combine),
	mPriority(p.priority),
	mPersist(p.persist),
	mDefaultFunctor(p.functor.isProvided() ? p.functor() : p.name()),
	mLogToChat(p.log_to_chat),
	mLogToIM(p.log_to_im),
	mShowToast(p.show_toast),
	mFadeToast(p.fade_toast),
    mSoundName("")
{
	if (p.sound.isProvided()
		&& LLUI::getInstance()->mSettingGroups["config"]->controlExists(p.sound))
	{
		mSoundName = p.sound;
	}

	BOOST_FOREACH(const LLNotificationTemplate::UniquenessContext& context, p.unique.contexts)
	{
		mUniqueContext.push_back(context.value);
	}
	
	LL_DEBUGS("Notifications") << "notification \"" << mName << "\": tag count is " << p.tags.size() << LL_ENDL;
	
	BOOST_FOREACH(const LLNotificationTemplate::Tag& tag, p.tags)
	{
		LL_DEBUGS("Notifications") << "    tag \"" << std::string(tag.value) << "\"" << LL_ENDL;
		mTags.push_back(tag.value);
	}

	mForm = LLNotificationFormPtr(new LLNotificationForm(p.name, p.form_ref.form));
}

LLNotificationVisibilityRule::LLNotificationVisibilityRule(const LLNotificationVisibilityRule::Rule &p)
{
	if (p.show.isChosen())
	{
		mType = p.show.type;
		mTag = p.show.tag;
		mName = p.show.name;
		mVisible = true;
	}
	else if (p.hide.isChosen())
	{
		mType = p.hide.type;
		mTag = p.hide.tag;
		mName = p.hide.name;
		mVisible = false;
	}
	else if (p.respond.isChosen())
	{
		mType = p.respond.type;
		mTag = p.respond.tag;
		mName = p.respond.name;
		mVisible = false;
		mResponse = p.respond.response;
	}
}

LLNotification::LLNotification(const LLSDParamAdapter<Params>& p) : 
	mTimestamp(p.time_stamp), 
	mSubstitutions(p.substitutions),
	mPayload(p.payload),
	mExpiresAt(p.expiry),
	mTemporaryResponder(false),
	mRespondedTo(false),
	mPriority(p.priority),
	mCancelled(false),
	mIgnored(false),
	mResponderObj(NULL),
	mId(p.id.isProvided() ? p.id : LLUUID::generateNewID()),
	mOfferFromAgent(p.offer_from_agent),
    mIsDND(p.is_dnd)
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
	else if(p.functor.responder.isChosen())
	{
		mResponder = p.functor.responder;
	}

	if(p.responder.isProvided())
	{
		mResponderObj = p.responder;
	}

	init(p.name, p.form_elements);
}


LLSD LLNotification::asLLSD(bool excludeTemplateElements)
{ 
	LLParamSDParser parser;

	Params p;
	p.id = mId;
	p.name = mTemplatep->mName;
	p.substitutions = mSubstitutions;
	p.payload = mPayload;
	p.time_stamp = mTimestamp;
	p.expiry = mExpiresAt;
	p.priority = mPriority;

    LLNotificationFormPtr templateForm = mTemplatep->mForm;
    LLSD formElements = mForm->asLLSD();

    //All form elements (dynamic or not)
    if(!excludeTemplateElements)
    {
        p.form_elements = formElements;
    }
    //Only dynamic form elements (exclude template elements)
    else if(templateForm->getNumElements() < formElements.size())
    {
        LLSD dynamicElements;
        //Offset to dynamic elements and store them
        mForm->getElements(dynamicElements, templateForm->getNumElements());
        p.form_elements = dynamicElements;
    }

    if(mResponder)
    {
        p.functor.responder_sd = mResponder->asLLSD();
    }

	if(!mResponseFunctorName.empty())
	{
		p.functor.name = mResponseFunctorName;
	}

	LLSD output;
	parser.writeSD(output, p);
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

	if(mResponder)
	{
		mResponder->handleRespond(asLLSD(), response);
	}
	else if (!mResponseFunctorName.empty())
	{
		// look up the functor
		LLNotificationFunctorRegistry::ResponseFunctor functor =
			LLNotificationFunctorRegistry::instance().getFunctor(mResponseFunctorName);
		// and then call it
		functor(asLLSD(), response);
	}
	else if (mCombinedNotifications.empty())
	{
		// no registered responder
		return;
	}

	if (mTemporaryResponder)
	{
		LLNotificationFunctorRegistry::instance().unregisterFunctor(mResponseFunctorName);
		mResponseFunctorName = "";
		mTemporaryResponder = false;
	}

	if (mForm->getIgnoreType() > LLNotificationForm::IGNORE_NO)
	{
		mForm->setIgnored(mIgnored);
		if (mIgnored && mForm->getIgnoreType() == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE)
		{
			LLUI::getInstance()->mSettingGroups["ignores"]->setLLSD("Default" + getName(), response);
		}
	}

	for (std::vector<LLNotificationPtr>::const_iterator it = mCombinedNotifications.begin(); it != mCombinedNotifications.end(); ++it)
	{
		if ((*it))
		{
			(*it)->respond(response);
		}
	}

	update();
}

void LLNotification::respondWithDefault()
{
	respond(getResponseTemplate(WITH_DEFAULT_BUTTON));
}


const std::string& LLNotification::getName() const
{
	return mTemplatep->mName;
}

const std::string& LLNotification::getIcon() const
{
	return mTemplatep->mIcon;
}


bool LLNotification::isPersistent() const
{
	return mTemplatep->mPersist;
}

std::string LLNotification::getType() const
{
	return (mTemplatep ? mTemplatep->mType : "");
}

S32 LLNotification::getURLOption() const
{
	return (mTemplatep ? mTemplatep->mURLOption : -1);
}

S32 LLNotification::getURLOpenExternally() const
{
	return(mTemplatep? mTemplatep->mURLTarget == "_external": -1);
}

bool LLNotification::getForceUrlsExternal() const
{
    return (mTemplatep ? mTemplatep->mForceUrlsExternal : false);
}

bool LLNotification::hasUniquenessConstraints() const 
{ 
	return (mTemplatep ? mTemplatep->mUnique : false);
}

bool LLNotification::matchesTag(const std::string& tag)
{
	bool result = false;
	
	if(mTemplatep)
	{
		std::list<std::string>::iterator it;
		for(it = mTemplatep->mTags.begin(); it != mTemplatep->mTags.end(); it++)
		{
			if((*it) == tag)
			{
				result = true;
				break;
			}
		}
	}
	
	return result;
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

void LLNotification::setResponseFunctor(const LLNotificationResponderPtr& responder)
{
	mResponder = responder;
}

bool LLNotification::isEquivalentTo(LLNotificationPtr that) const
{
	if (this->mTemplatep->mName != that->mTemplatep->mName) 
	{
		return false; // must have the same template name or forget it
	}
	if (this->mTemplatep->mUnique)
	{
		const LLSD& these_substitutions = this->getSubstitutions();
		const LLSD& those_substitutions = that->getSubstitutions();
		const LLSD& this_payload = this->getPayload();
		const LLSD& that_payload = that->getPayload();

		// highlander bit sez there can only be one of these
		for (std::vector<std::string>::const_iterator it = mTemplatep->mUniqueContext.begin(), end_it = mTemplatep->mUniqueContext.end();
			it != end_it;
			++it)
		{
			// if templates differ in either substitution strings or payload with the given field name
			// then they are considered inequivalent
			// use of get() avoids converting the LLSD value to a map as the [] operator would
			if (these_substitutions.get(*it).asString() != those_substitutions.get(*it).asString()
				|| this_payload.get(*it).asString() != that_payload.get(*it).asString())
			{
				return false;
			}
		}
		return true;
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

	mIgnored = mForm->getIgnored();

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

std::string LLNotification::getFooter() const
{
	if (!mTemplatep)
		return std::string();

	std::string footer = mTemplatep->mFooter;
	LLStringUtil::format(footer, mSubstitutions);
	return footer;
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

bool LLNotification::canLogToChat() const
{
	return mTemplatep->mLogToChat;
}

bool LLNotification::canLogToIM() const
{
	return mTemplatep->mLogToIM;
}

bool LLNotification::canShowToast() const
{
	return mTemplatep->mShowToast;
}

bool LLNotification::canFadeToast() const
{
	return mTemplatep->mFadeToast;
}

bool LLNotification::hasFormElements() const
{
	return mTemplatep->mForm->getNumElements() != 0;
}

void LLNotification::playSound()
{ 
    make_ui_sound(mTemplatep->mSoundName.c_str());
}

LLNotification::ECombineBehavior LLNotification::getCombineBehavior() const
{
	return mTemplatep->mCombineBehavior;
}

void LLNotification::updateForm( const LLNotificationFormPtr& form )
{
	mForm = form;
}

void LLNotification::repost()
{
	mRespondedTo = false;
	LLNotifications::instance().update(shared_from_this());
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
	bool passesFilter = mFilter ? mFilter(pNotification) : true;
	
	// first, we offer the result of the filter test to the simple
	// signals for pass/fail. One of these is guaranteed to be called.
	// If either signal returns true, the change processing is NOT performed
	// (so don't return true unless you know what you're doing!)
	bool abortProcessing = false;
	if (passesFilter)
	{
		onFilterPass(pNotification);
		abortProcessing = mPassedFilter(payload);
	}
	else
	{
		onFilterFail(pNotification);
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
			onLoad(pNotification);
			abortProcessing = mChanged(payload);
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
				onChange(pNotification);
				abortProcessing = mChanged(payload);
			}
			else
			{
				// not in our list, add it and say so
				mItems.insert(pNotification);
				onChange(pNotification);
				// our payload is const, so make a copy before changing it
				LLSD newpayload = payload;
				newpayload["sigtype"] = "add";
				abortProcessing = mChanged(newpayload);
			}
		}
		else
		{
			if (wasFound)
			{
				// it already existed, so this is a delete
				mItems.erase(pNotification);
				onChange(pNotification);
				// our payload is const, so make a copy before changing it
				LLSD newpayload = payload;
				newpayload["sigtype"] = "delete";
				abortProcessing = mChanged(newpayload);
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
			onAdd(pNotification);
			abortProcessing = mChanged(payload);
		}
	}
	else if (cmd == "delete")
	{
		// if we have it in our list, pass on the delete, then delete it, else do nothing
		if (wasFound)
		{
			onDelete(pNotification);
			abortProcessing = mChanged(payload);
			mItems.erase(pNotification);
		}
	}
	return abortProcessing;
}

LLNotificationChannel::LLNotificationChannel(const Params& p)
:	LLNotificationChannelBase(p.filter()),
	LLInstanceTracker<LLNotificationChannel, std::string>(p.name.isProvided() ? p.name : LLUUID::generateNewID().asString()),
	mName(p.name.isProvided() ? p.name : LLUUID::generateNewID().asString())
{
	BOOST_FOREACH(const std::string& source, p.sources)
    {
		connectToChannel(source);
	}
}


LLNotificationChannel::LLNotificationChannel(const std::string& name, 
											 const std::string& parent,
											 LLNotificationFilter filter) 
:	LLNotificationChannelBase(filter),
	LLInstanceTracker<LLNotificationChannel, std::string>(name),
	mName(name)
{
	// bind to notification broadcast
	connectToChannel(parent);
}

bool LLNotificationChannel::isEmpty() const
{
	return mItems.empty();
}

S32 LLNotificationChannel::size() const
{
	return mItems.size();
}

LLNotificationChannel::Iterator LLNotificationChannel::begin()
{
	return mItems.begin();
}

LLNotificationChannel::Iterator LLNotificationChannel::end()
{
	return mItems.end();
}

size_t LLNotificationChannel::size()
{
	return mItems.size();
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

void LLNotificationChannel::connectToChannel( const std::string& channel_name )
{
	if (channel_name.empty())
	{
		LLNotifications::instance().connectChanged(
			boost::bind(&LLNotificationChannelBase::updateItem, this, _1));
	}
	else
	{
		mParents.push_back(channel_name);
		LLNotificationChannelPtr p = LLNotifications::instance().getChannel(channel_name);
		p->connectChanged(boost::bind(&LLNotificationChannelBase::updateItem, this, _1));
	}
}

// ---
// END OF LLNotificationChannel implementation
// =========================================================


// ==============================================	===========
// LLNotifications implementation
// ---
LLNotifications::LLNotifications() 
:	LLNotificationChannelBase(LLNotificationFilters::includeEverything),
	mIgnoreAllNotifications(false)
{
        mListener.reset(new LLNotificationsListener(*this));
	LLUICtrl::CommitCallbackRegistry::currentRegistrar().add("Notification.Show", boost::bind(&LLNotifications::addFromCallback, this, _2));

	// touch the instance tracker for notification channels, so that it will still be around in our destructor
	LLInstanceTracker<LLNotificationChannel, std::string>::instanceCount();
}

void LLNotifications::clear()
{
   mDefaultChannels.clear();
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
			if (pNotif->getCombineBehavior() == LLNotification::CANCEL_OLD)
			{
				cancel(existing_notification);
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}

bool LLNotifications::uniqueHandler(const LLSD& payload)
{
	std::string cmd = payload["sigtype"];

	LLNotificationPtr pNotif = LLNotifications::instance().find(payload["id"].asUUID());
	if (pNotif && pNotif->hasUniquenessConstraints()) 
	{
		if (cmd == "add")
		{
			// not a duplicate according to uniqueness criteria, so we keep it
			// and store it for future uniqueness checks
			mUniqueNotifications.insert(std::make_pair(pNotif->getName(), pNotif));
		}
		else if (cmd == "delete")
		{
			mUniqueNotifications.erase(pNotif->getName());
		}
	}

	return false;
}

bool LLNotifications::failedUniquenessTest(const LLSD& payload)
{
	LLNotificationPtr pNotif = LLNotifications::instance().find(payload["id"].asUUID());
	
	std::string cmd = payload["sigtype"];

	if (!pNotif || cmd != "add")
	{
		return false;
	}

	switch(pNotif->getCombineBehavior())
	{
	case  LLNotification::REPLACE_WITH_NEW:
		// Update the existing unique notification with the data from this particular instance...
		// This guarantees that duplicate notifications will be collapsed to the one
		// most recently triggered
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
				cancel(pNotif);
			}
		}
		break;
	case  LLNotification::COMBINE_WITH_NEW:
		// Add to the existing unique notification with the data from this particular instance...
		// This guarantees that duplicate notifications will be collapsed to the one
		// most recently triggered
		for (LLNotificationMap::iterator existing_it = mUniqueNotifications.find(pNotif->getName());
			existing_it != mUniqueNotifications.end();
			++existing_it)
		{
			LLNotificationPtr existing_notification = existing_it->second;
			if (pNotif != existing_notification 
				&& pNotif->isEquivalentTo(existing_notification))
			{
				// copy the notifications from the newest instance into the oldest
				existing_notification->mCombinedNotifications.push_back(pNotif);
				existing_notification->mCombinedNotifications.insert(existing_notification->mCombinedNotifications.end(),
					pNotif->mCombinedNotifications.begin(), pNotif->mCombinedNotifications.end());

				// pop up again
				existing_notification->update();
			}
		}
		break;
	case LLNotification::KEEP_OLD:
		break;
	case LLNotification::CANCEL_OLD:
		// already handled by filter logic
		break;
	default:
		break;
	}

	return false;
}

LLNotificationChannelPtr LLNotifications::getChannel(const std::string& channelName)
{
	return LLNotificationChannelPtr(LLNotificationChannel::getInstance(channelName).get());
}


// this function is called once at construction time, after the object is constructed.
void LLNotifications::initSingleton()
{
	loadTemplates();
	loadVisibilityRules();
	createDefaultChannels();
}

void LLNotifications::cleanupSingleton()
{
    clear();
}

void LLNotifications::createDefaultChannels()
{
    LL_INFOS("Notifications") << "Generating default notification channels" << LL_ENDL;
	// now construct the various channels AFTER loading the notifications,
	// because the history channel is going to rewrite the stored notifications file
	mDefaultChannels.push_back(new LLNotificationChannel("Enabled", "",
		!boost::bind(&LLNotifications::getIgnoreAllNotifications, this)));
	mDefaultChannels.push_back(new LLNotificationChannel("Expiration", "Enabled",
		boost::bind(&LLNotifications::expirationFilter, this, _1)));
	mDefaultChannels.push_back(new LLNotificationChannel("Unexpired", "Enabled",
		!boost::bind(&LLNotifications::expirationFilter, this, _1))); // use negated bind
	mDefaultChannels.push_back(new LLNotificationChannel("Unique", "Unexpired",
		boost::bind(&LLNotifications::uniqueFilter, this, _1)));
	mDefaultChannels.push_back(new LLNotificationChannel("Ignore", "Unique",
		filterIgnoredNotifications));
	mDefaultChannels.push_back(new LLNotificationChannel("VisibilityRules", "Ignore",
		boost::bind(&LLNotifications::isVisibleByRules, this, _1)));
	mDefaultChannels.push_back(new LLNotificationChannel("Visible", "VisibilityRules",
		&LLNotificationFilters::includeEverything));
	mDefaultChannels.push_back(new LLPersistentNotificationChannel());

	// connect action methods to these channels
	getChannel("Enabled")->connectFailedFilter(&defaultResponse);
	getChannel("Expiration")->connectChanged(boost::bind(&LLNotifications::expirationHandler, this, _1));
	// uniqueHandler slot should be added as first slot of the signal due to
	// usage LLStopWhenHandled combiner in LLStandardSignal
	getChannel("Unique")->connectAtFrontChanged(boost::bind(&LLNotifications::uniqueHandler, this, _1));
	getChannel("Unique")->connectFailedFilter(boost::bind(&LLNotifications::failedUniquenessTest, this, _1));
	getChannel("Ignore")->connectFailedFilter(&handleIgnoredNotification);
	getChannel("VisibilityRules")->connectFailedFilter(&visibilityRuleMached);
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

void LLNotifications::forceResponse(const LLNotification::Params& params, S32 option)
{
	LLNotificationPtr temp_notify(new LLNotification(params));
	LLSD response = temp_notify->getResponseTemplate();
	LLSD selected_item = temp_notify->getForm()->getElement(option);
	
	if (selected_item.isUndefined())
	{
		LL_WARNS("Notifications") << "Invalid option" << option << " for notification " << (std::string)params.name << LL_ENDL;
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
				LL_DEBUGS("Notifications") << "replaceSubstitutionStrings: value: \"" << value << "\" repl: \"" << replacement << "\"." << LL_ENDL;
				it->second->setValue(replacement);
			}
			else
			{
				LL_WARNS("Notifications") << "replaceSubstitutionStrings FAILURE: could not find replacement \"" << value << "\"." << LL_ENDL;
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

void replaceFormText(LLNotificationForm::Params& form, const std::string& pattern, const std::string& replace)
{
	if (form.ignore.isProvided() && form.ignore.text() == pattern)
	{
		form.ignore.text = replace;
	}

	BOOST_FOREACH(LLNotificationForm::FormElement& element, form.form_elements.elements)
	{
		if (element.button.isChosen() && element.button.text() == pattern)
		{
			element.button.text = replace;
		}
	}
}

void addPathIfExists(const std::string& new_path, std::vector<std::string>& paths)
{
	if (gDirUtilp->fileExists(new_path))
	{
		paths.push_back(new_path);
	}
}

bool LLNotifications::loadTemplates()
{
	LL_INFOS("Notifications") << "Reading notifications template" << LL_ENDL;
	// Passing findSkinnedFilenames(constraint=LLDir::ALL_SKINS) makes it
	// output all relevant pathnames instead of just the ones from the most
	// specific skin.
	std::vector<std::string> search_paths =
		gDirUtilp->findSkinnedFilenames(LLDir::XUI, "notifications.xml", LLDir::ALL_SKINS);

	std::string base_filename = search_paths.front();
	LLXMLNodePtr root;
	BOOL success  = LLXMLNode::getLayeredXMLNode(root, search_paths);

	if (!success || root.isNull() || !root->hasName( "notifications" ))
	{
		LL_ERRS() << "Problem reading XML from UI Notifications file: " << base_filename << LL_ENDL;
		return false;
	}

	LLNotificationTemplate::Notifications params;
	LLXUIParser parser;
	parser.readXUI(root, params, base_filename);

	if(!params.validateBlock())
	{
		LL_ERRS() << "Problem reading XUI from UI Notifications file: " << base_filename << LL_ENDL;
		return false;
	}

	mTemplates.clear();

	BOOST_FOREACH(LLNotificationTemplate::GlobalString& string, params.strings)
	{
		mGlobalStrings[string.name] = string.value;
	}

	std::map<std::string, LLNotificationForm::Params> form_templates;

	BOOST_FOREACH(LLNotificationTemplate::Template& notification_template, params.templates)
	{
		form_templates[notification_template.name] = notification_template.form;
	}

	BOOST_FOREACH(LLNotificationTemplate::Params& notification, params.notifications)
	{
		if (notification.form_ref.form_template.isChosen())
		{
			// replace form contents from template
			notification.form_ref.form = form_templates[notification.form_ref.form_template.name];
			if(notification.form_ref.form_template.yes_text.isProvided())
			{
				replaceFormText(notification.form_ref.form, "$yestext", notification.form_ref.form_template.yes_text);
			}
			if(notification.form_ref.form_template.no_text.isProvided())
			{
				replaceFormText(notification.form_ref.form, "$notext", notification.form_ref.form_template.no_text);
			}
			if(notification.form_ref.form_template.cancel_text.isProvided())
			{
				replaceFormText(notification.form_ref.form, "$canceltext", notification.form_ref.form_template.cancel_text);
			}
			if(notification.form_ref.form_template.help_text.isProvided())
			{
				replaceFormText(notification.form_ref.form, "$helptext", notification.form_ref.form_template.help_text);
			}
			if(notification.form_ref.form_template.ignore_text.isProvided())
			{
				replaceFormText(notification.form_ref.form, "$ignoretext", notification.form_ref.form_template.ignore_text);
			}
		}
		mTemplates[notification.name] = LLNotificationTemplatePtr(new LLNotificationTemplate(notification));
	}

	LL_INFOS("Notifications") << "...done" << LL_ENDL;

	return true;
}

bool LLNotifications::loadVisibilityRules()
{
	const std::string xml_filename = "notification_visibility.xml";
	// Note that here we're looking for the "en" version, the default
	// language, rather than the most localized version of this file.
	std::string full_filename = gDirUtilp->findSkinnedFilenameBaseLang(LLDir::XUI, xml_filename);

	LLNotificationVisibilityRule::Rules params;
	LLSimpleXUIParser parser;
	parser.readXUI(full_filename, params);

	if(!params.validateBlock())
	{
		LL_ERRS() << "Problem reading UI Notification Visibility Rules file: " << full_filename << LL_ENDL;
		return false;
	}

	mVisibilityRules.clear();

	BOOST_FOREACH(LLNotificationVisibilityRule::Rule& rule, params.rules)
	{
		mVisibilityRules.push_back(LLNotificationVisibilityRulePtr(new LLNotificationVisibilityRule(rule)));
	}

	return true;
}

// Add a simple notification (from XUI)
void LLNotifications::addFromCallback(const LLSD& name)
{
	add(name.asString(), LLSD(), LLSD());
}

LLNotificationPtr LLNotifications::add(const std::string& name, const LLSD& substitutions, const LLSD& payload)
{
	LLNotification::Params::Functor functor_p;
	functor_p.name = name;
	return add(LLNotification::Params().name(name).substitutions(substitutions).payload(payload).functor(functor_p));	
}

LLNotificationPtr LLNotifications::add(const std::string& name, const LLSD& substitutions, const LLSD& payload, const std::string& functor_name)
{
	LLNotification::Params::Functor functor_p;
	functor_p.name = functor_name;
	return add(LLNotification::Params().name(name)
										.substitutions(substitutions)
										.payload(payload)
										.functor(functor_p));	
}
							  
//virtual
LLNotificationPtr LLNotifications::add(const std::string& name, const LLSD& substitutions, const LLSD& payload, LLNotificationFunctorRegistry::ResponseFunctor functor)
{
	LLNotification::Params::Functor functor_p;
	functor_p.function = functor;
	return add(LLNotification::Params().name(name)
										.substitutions(substitutions)
										.payload(payload)
										.functor(functor_p));	
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
	if (pNotif == NULL) return;

	// first see if we already have it -- if so, that's a problem
	LLNotificationSet::iterator it=mItems.find(pNotif);
	if (it != mItems.end())
	{
		LL_ERRS() << "Notification added a second time to the master notification channel." << LL_ENDL;
	}

	updateItem(LLSD().with("sigtype", "add").with("id", pNotif->id()), pNotif);
}

void LLNotifications::load(const LLNotificationPtr pNotif)
{
	if (pNotif == NULL) return;

	// first see if we already have it -- if so, that's a problem
	LLNotificationSet::iterator it=mItems.find(pNotif);
	if (it != mItems.end())
	{
		LL_ERRS() << "Notification loaded a second time to the master notification channel." << LL_ENDL;
	}

	updateItem(LLSD().with("sigtype", "load").with("id", pNotif->id()), pNotif);
}

void LLNotifications::cancel(LLNotificationPtr pNotif)
{
	if (pNotif == NULL || pNotif->isCancelled()) return;

	LLNotificationSet::iterator it=mItems.find(pNotif);
	if (it != mItems.end())
	{
		pNotif->cancel();
		updateItem(LLSD().with("sigtype", "delete").with("id", pNotif->id()), pNotif);
	}
}

void LLNotifications::cancelByName(const std::string& name)
{
	std::vector<LLNotificationPtr> notifs_to_cancel;
	for (LLNotificationSet::iterator it=mItems.begin(), end_it = mItems.end();
		it != end_it;
		++it)
	{
		LLNotificationPtr pNotif = *it;
		if (pNotif->getName() == name)
		{
			notifs_to_cancel.push_back(pNotif);
		}
	}

	for (std::vector<LLNotificationPtr>::iterator it = notifs_to_cancel.begin(), end_it = notifs_to_cancel.end();
		it != end_it;
		++it)
	{
		LLNotificationPtr pNotif = *it;
		pNotif->cancel();
		updateItem(LLSD().with("sigtype", "delete").with("id", pNotif->id()), pNotif);
	}
}

void LLNotifications::cancelByOwner(const LLUUID ownerId)
{
	std::vector<LLNotificationPtr> notifs_to_cancel;
	for (LLNotificationSet::iterator it = mItems.begin(), end_it = mItems.end();
		 it != end_it;
		 ++it)
	{
		LLNotificationPtr pNotif = *it;
		if (pNotif && pNotif->getPayload().get("owner_id").asUUID() == ownerId)
		{
			notifs_to_cancel.push_back(pNotif);
		}
	}

	for (std::vector<LLNotificationPtr>::iterator it = notifs_to_cancel.begin(), end_it = notifs_to_cancel.end();
		 it != end_it;
		 ++it)
	{
		LLNotificationPtr pNotif = *it;
		pNotif->cancel();
		updateItem(LLSD().with("sigtype", "delete").with("id", pNotif->id()), pNotif);
	}
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
	LLNotificationPtr target = LLNotificationPtr(new LLNotification(LLNotification::Params().id(uuid)));
	LLNotificationSet::iterator it=mItems.find(target);
	if (it == mItems.end())
	{
		LL_DEBUGS("Notifications") << "Tried to dereference uuid '" << uuid << "' as a notification key but didn't find it." << LL_ENDL;
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

void LLNotifications::setIgnored(const std::string& name, bool ignored)
{
	LLNotificationTemplatePtr templatep = getTemplate(name);
	templatep->mForm->setIgnored(ignored);
}

bool LLNotifications::getIgnored(const std::string& name)
{
	LLNotificationTemplatePtr templatep = getTemplate(name);
	return (mIgnoreAllNotifications) || ( (templatep->mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO) && (templatep->mForm->getIgnored()) );
}
													
bool LLNotifications::isVisibleByRules(LLNotificationPtr n)
{
	if(n->isRespondedTo())
	{
		// This avoids infinite recursion in the case where the filter calls respond()
		return true;
	}
	
	VisibilityRuleList::iterator it;
	
	for(it = mVisibilityRules.begin(); it != mVisibilityRules.end(); it++)
	{
		// An empty type/tag/name string will match any notification, so only do the comparison when the string is non-empty in the rule.
		LL_DEBUGS("Notifications")
			<< "notification \"" << n->getName() << "\" " 
			<< "testing against " << ((*it)->mVisible?"show":"hide") << " rule, "
			<< "name = \"" << (*it)->mName << "\" "
			<< "tag = \"" << (*it)->mTag << "\" "
			<< "type = \"" << (*it)->mType << "\" "
			<< LL_ENDL;

		if(!(*it)->mType.empty())
		{
			if((*it)->mType != n->getType())
			{
				// Type doesn't match, so skip this rule.
				continue;
			}
		}
		
		if(!(*it)->mTag.empty())
		{
			// check this notification's tag(s) against it->mTag and continue if no match is found.
			if(!n->matchesTag((*it)->mTag))
			{
				// This rule's non-empty tag didn't match one of the notification's tags.  Skip this rule.
				continue;
			}
		}

		if(!(*it)->mName.empty())
		{
			// check this notification's name against the notification's name and continue if no match is found.
			if((*it)->mName != n->getName())
			{
				// This rule's non-empty name didn't match the notification.  Skip this rule.
				continue;
			}
		}
		
		// If we got here, the rule matches.  Don't evaluate subsequent rules.
		if(!(*it)->mVisible)
		{
			// This notification is being hidden.
			
			if((*it)->mResponse.empty())
			{
				// Response property is empty.  Cancel this notification.
				LL_DEBUGS("Notifications") << "cancelling notification " << n->getName() << LL_ENDL;

				cancel(n);
			}
			else
			{
				// Response property is not empty.  Return the specified response.
				LLSD response = n->getResponseTemplate(LLNotification::WITHOUT_DEFAULT_BUTTON);
				// TODO: verify that the response template has an item with the correct name
				response[(*it)->mResponse] = true;

				LL_DEBUGS("Notifications") << "responding to notification " << n->getName() << " with response = " << response << LL_ENDL;
				
				n->respond(response);
			}

			return false;
		}
		
		// If we got here, exit the loop and return true.
		break;
	}
	
	LL_DEBUGS("Notifications") << "allowing notification " << n->getName() << LL_ENDL;

	return true;
}
			

// ---
// END OF LLNotifications implementation
// =========================================================

std::ostream& operator<<(std::ostream& s, const LLNotification& notification)
{
	s << notification.summarize();
	return s;
}

void LLPostponedNotification::lookupName(const LLUUID& id,
										 bool is_group)
{
	if (is_group)
	{
		gCacheName->getGroup(id,
			boost::bind(&LLPostponedNotification::onGroupNameCache,
				this, _1, _2, _3));
	}
	else
	{
		fetchAvatarName(id);
	}
}

void LLPostponedNotification::onGroupNameCache(const LLUUID& id,
											   const std::string& full_name,
											   bool is_group)
{
	finalizeName(full_name);
}

void LLPostponedNotification::fetchAvatarName(const LLUUID& id)
{
	if (id.notNull())
	{
		if (mAvatarNameCacheConnection.connected())
		{
			mAvatarNameCacheConnection.disconnect();
		}

		mAvatarNameCacheConnection = LLAvatarNameCache::get(id, boost::bind(&LLPostponedNotification::onAvatarNameCache, this, _1, _2));
	}
}

void LLPostponedNotification::onAvatarNameCache(const LLUUID& agent_id,
												const LLAvatarName& av_name)
{
	mAvatarNameCacheConnection.disconnect();

	std::string name = av_name.getCompleteName();

	// from PE merge - we should figure out if this is the right thing to do
	if (name.empty())
	{
		LL_WARNS("Notifications") << "Empty name received for Id: " << agent_id << LL_ENDL;
		name = SYSTEM_FROM;
	}
	
	finalizeName(name);
}

void LLPostponedNotification::finalizeName(const std::string& name)
{
	mName = name;
	modifyNotificationParams();
	LLNotifications::instance().add(mParams);
	cleanup();
}
