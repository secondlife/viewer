/** 
 * @file llconfirmationmanager.h
 * @brief LLConfirmationManager class definition
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLCONFIRMATIONMANAGER_H
#define LL_LLCONFIRMATIONMANAGER_H

class LLConfirmationManager
{
public:
    class ListenerBase
    {
    public:
        virtual ~ListenerBase();
        virtual void confirmed(const std::string& password) = 0;
    };

    enum Type { TYPE_NONE, TYPE_CLICK, TYPE_PASSWORD };
    
    static void confirm(Type type,
        const std::string& purchase, ListenerBase* listener);
    static void confirm(const std::string& type,
        const std::string& purchase, ListenerBase* listener);
        // note: these take control of, and delete the listener when done

    template <class T>
    class Listener : public ListenerBase
    {
    public:
        typedef void (T::*ConfirmationMemberFunction)(const std::string&);
        
        Listener(T& object, ConfirmationMemberFunction function)
            : mObject(object), mFunction(function)
            { }
        
        void confirmed(const std::string& password)
        {
            (mObject.*mFunction)(password);
        }
        
    private:
        T& mObject;
        ConfirmationMemberFunction mFunction;
    };

    template <class T>
    static void confirm(Type type,
        const std::string& action,
        T& obj, void(T::*func)(const std::string&))
    {
        confirm(type, action, new Listener<T>(obj, func));
    }
    
    template <class T>
    static void confirm(const std::string& type,
        const std::string& action,
        T& obj, void(T::*func)(const std::string&))
    {
        confirm(type, action, new Listener<T>(obj, func));
    }       
};

#endif 
