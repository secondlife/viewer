/** 
 * @file llconfirmationmanager.h
 * @brief LLConfirmationManager class definition
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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
