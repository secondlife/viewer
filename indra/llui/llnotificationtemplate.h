/**
* @file llnotificationtemplate.h
* @brief Description of notification contents
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

#ifndef LL_LLNOTIFICATION_TEMPLATE_H
#define LL_LLNOTIFICATION_TEMPLATE_H

#include "llinitparam.h"
#include "llnotifications.h"

typedef std::shared_ptr<LLNotificationForm> LLNotificationFormPtr;

// This is the class of object read from the XML file (notifications.xml,
// from the appropriate local language directory).
struct LLNotificationTemplate
{
    struct CombineBehaviorNames
        :   public LLInitParam::TypeValuesHelper<LLNotification::ECombineBehavior, CombineBehaviorNames>
    {
        static void declareValues()
        {
            declare("replace_with_new", LLNotification::REPLACE_WITH_NEW);
            declare("combine_with_new", LLNotification::COMBINE_WITH_NEW);
            declare("keep_old", LLNotification::KEEP_OLD);
            declare("cancel_old", LLNotification::CANCEL_OLD);
        }
    };


    struct GlobalString : public LLInitParam::Block<GlobalString>
    {
        Mandatory<std::string>  name,
                                value;

        GlobalString()
        :   name("name"),
            value("value")
        {}
    };

    struct UniquenessContext : public LLInitParam::Block<UniquenessContext>
    {
        Mandatory<std::string>  value;

        UniquenessContext()
        :   value("value")
        {
            addSynonym(value, "key");
        }

    };

    struct UniquenessConstraint : public LLInitParam::Block<UniquenessConstraint>
    {
    private:
        // this idiom allows
        // <notification> <unique/> </notification>
        // as well as
        // <notification> <unique> <context></context> </unique>...
        Optional<LLInitParam::Flag> dummy_val;
    public:
        Multiple<UniquenessContext> contexts;
        Optional<LLNotification::ECombineBehavior, CombineBehaviorNames> combine;

        UniquenessConstraint()
        :   contexts("context"),
            combine("combine", LLNotification::REPLACE_WITH_NEW),
            dummy_val("")
        {}
    };

    // Templates are used to define common form types, such as OK/Cancel dialogs, etc.

    struct Template : public LLInitParam::Block<Template>
    {
        Mandatory<std::string>                  name;
        Mandatory<LLNotificationForm::Params>   form;

        Template()
        :   name("name"),
            form("form")
        {}
    };

    // Reference a template to use its form elements
    struct TemplateRef : public LLInitParam::Block<TemplateRef>
    {
        Mandatory<std::string>  name;
        Optional<std::string>   yes_text,
                                no_text,
                                cancel_text,
                                help_text,
                                ignore_text;

        TemplateRef()
        :   name("name"),
            yes_text("yestext"),
            no_text("notext"),
            cancel_text("canceltext"),
            help_text("helptext"),
            ignore_text("ignoretext")
        {}
    };

    struct URL : public LLInitParam::Block<URL>
    {
        Mandatory<S32>          option;
        Mandatory<std::string>  value;
        Optional<std::string>   target;
        Ignored                 name;

        URL()
        :   option("option", -1),
            value("value"),
            target("target", "_blank"),
            name("name")
        {}
    };

    struct FormRef : public LLInitParam::ChoiceBlock<FormRef>
    {
        Alternative<LLNotificationForm::Params>     form;
        Alternative<TemplateRef>                    form_template;

        FormRef()
        :   form("form"),
            form_template("usetemplate")
        {}
    };

    struct Tag : public LLInitParam::Block<Tag>
    {
        Mandatory<std::string>  value;

        Tag()
        :   value("value")
        {}
    };

    struct Footer : public LLInitParam::Block<Footer>
    {
        Mandatory<std::string> value;

        Footer()
        :   value("value")
        {
            addSynonym(value, "");
        }
    };

    struct Params : public LLInitParam::Block<Params>
    {
        Mandatory<std::string>          name;
        Optional<bool>                  persist,
                                        log_to_im,
                                        show_toast,
                                        fade_toast,
                                        log_to_chat,
                                        force_urls_external;
        Optional<std::string>           functor,
                                        icon,
                                        label,
                                        sound,
                                        type,
                                        value;
        Optional<U32>                   duration;
        Optional<S32>                   expire_option;
        Optional<URL>                   url;
        Optional<UniquenessConstraint>  unique;
        Optional<FormRef>               form_ref;
        Optional<ENotificationPriority,
            NotificationPriorityValues> priority;
        Multiple<Tag>                   tags;
        Optional<Footer>                footer;


        Params()
        :   name("name"),
            persist("persist", false),
            fade_toast("fade_toast", true),
            log_to_im("log_to_im", false),
            show_toast("show_toast", true),
            log_to_chat("log_to_chat", true),
            force_urls_external("force_urls_external", false),
            functor("functor"),
            icon("icon"),
            label("label"),
            priority("priority"),
            sound("sound"),
            type("type"),
            value("value"),
            duration("duration"),
            expire_option("expireOption", -1),
            url("url"),
            unique("unique"),
            form_ref(""),
            tags("tag"),
            footer("footer")
        {}

    };

    struct Notifications : public LLInitParam::Block<Notifications>
    {
        Multiple<GlobalString>  strings;
        Multiple<Template>      templates;
        Multiple<Params>        notifications;

        Notifications()
        :   strings("global"),
            notifications("notification"),
            templates("template")
        {}
    };

    LLNotificationTemplate(const Params& p);
    // the name of the notification -- the key used to identify it
    // Ideally, the key should follow variable naming rules
    // (no spaces or punctuation).
    std::string mName;
    // The type of the notification
    // used to control which queue it's stored in
    std::string mType;
    // The text used to display the notification. Replaceable parameters
    // are enclosed in square brackets like this [].
    std::string mMessage;
    // The text used to display the notification, but under the form.
    std::string mFooter;
    // The label for the notification; used for
    // certain classes of notification (those with a window and a window title).
    // Also used when a notification pops up underneath the current one.
    // Replaceable parameters can be used in the label.
    std::string mLabel;
    // The name of the icon image. This should include an extension.
    std::string mIcon;
    // This is the Highlander bit -- "There Can Be Only One"
    // An outstanding notification with this bit set
    // is updated by an incoming notification with the same name,
    // rather than creating a new entry in the queue.
    // (used for things like progress indications, or repeating warnings
    // like "the grid is going down in N minutes")
    bool mUnique;
    LLNotification::ECombineBehavior mCombineBehavior;
    // if we want to be unique only if a certain part of the payload or substitutions args
    // are constant specify the field names for the payload. The notification will only be
    // combined if all of the fields named in the context are identical in the
    // new and the old notification; otherwise, the notification will be
    // duplicated. This is to support suppressing duplicate offers from the same
    // sender but still differentiating different offers. Example: Invitation to
    // conference chat.
    std::vector<std::string> mUniqueContext;
    // If this notification expires automatically, this value will be
    // nonzero, and indicates the number of seconds for which the notification
    // will be valid (a teleport offer, for example, might be valid for
    // 300 seconds).
    U32 mExpireSeconds;
    // if the offer expires, one of the options is chosen automatically
    // based on its "value" parameter. This controls which one.
    // If expireSeconds is specified, expireOption should also be specified.
    U32 mExpireOption;
    // if the notification contains a url, it's stored here (and replaced
    // into the message where [_URL] is found)
    std::string mURL;
    // if there's a URL in the message, this controls which option visits
    // that URL. Obsolete this and eliminate the buttons for affected
    // messages when we allow clickable URLs in the UI
    U32 mURLOption;

    //This is a flag that tells if option url needs to open externally dispite
    //what the user setting is.
    std::string mURLTarget;

    // All links clicked inside notification will be opened in external browser
    // Note: Some notifications block and exit viewer, yet they provide a link
    // to click, we should be able to open such links in external browser.
    bool mForceUrlsExternal;

    // does this notification persist across sessions? if so, it will be
    // serialized to disk on first receipt and read on startup
    bool mPersist;
    // This is the name of the default functor, if present, to be
    // used for the notification's callback. It is optional, and used only if
    // the notification is constructed without an identified functor.
    std::string mDefaultFunctor;
    // The form data associated with a given notification (buttons, text boxes, etc)
    LLNotificationFormPtr mForm;
    // default priority for notifications of this type
    ENotificationPriority mPriority;
    // Stores the sound name which can then be used to play the sound using make_ui_sound
    std::string mSoundName;
    // List of tags that rules can match against.
    std::list<std::string> mTags;

    // inject these notifications into chat/IM streams
    bool mLogToChat;
    bool mLogToIM;
    bool mShowToast;
    bool mFadeToast;
};

#endif //LL_LLNOTIFICATION_TEMPLATE_H

