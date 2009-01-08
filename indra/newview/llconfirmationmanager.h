/** 
 * @file llconfirmationmanager.h
 * @brief LLConfirmationManager class definition
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
