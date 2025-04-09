#pragma once
#include "Utility.h"
#include <map>


class RPCHandler
{
	std::map<std::string, std::vector<SReflect::Any>> Slot;

protected:
	void Clear()
	{
		Slot.clear();
	}
public:
	~RPCHandler()
	{
		Clear();
	}

	void Dispatch(SOCKET, const std::string& data)
	{
		std::vector<SReflect::Any> args;
		bool result = SockProto::Decode(data, args);
		if (!result || args.size() == 0)
		{
			LogWarn("RPCHandler failed, decode error\n");
			return;
		}
		if (args[0].get_type() != GetType<std::string>())
		{
			LogWarn("RPCHandler first parameter should be string, corresponding to function name\n");
			return;
		}
		std::string funcname = args[0].cast<std::string>();
		auto& listeners = Slot[funcname];
		if (!listeners.empty())
		{
			args.erase(args.begin());
			for (auto& obj : listeners)
			{
				obj.call_member_function(funcname, args);
			}
		}
	}

	//subscribe corresponding net Event
	template<class T>
	bool Subscribe(T* ptr, const std::string& funcname)
	{
		using namespace SReflect;
		if (ptr == nullptr)
		{
			return false;
		}
		Any obj = make_any_ref(*ptr);
		auto& listeners = Slot[funcname];
		//check if already subscribed
		for (auto& item : listeners)
		{
			if (item == obj)
			{
				return true;
			}
		}
		listeners.emplace_back(obj);
		return true;
	}

	//subscribe all reflected function of this class, whose function name are registered in netEvent map
	template<class T>
	bool Subscribe(T* ptr)
	{
		using namespace SReflect;
		Type* t = GetType<T>();
		for (auto& item : t->methods)
		{
			Subscribe(ptr, (std::string)item.first);
		}
		return true;
	}

	template<class T>
	bool Unsubscribe(T* ptr, const std::string& funcname)
	{
		using namespace SReflect;
		if (ptr == nullptr)
		{
			return false;
		}
		Any obj = make_any_ref(*ptr);
		auto& listeners = Slot[funcname];
		for (auto it = listeners.begin(); it != listeners.end(); ++it)
		{
			if ((*it) == obj)
			{
				listeners.erase(it);
				return true;
			}
		}
		return true;
	}

	template<class T>
	bool Unsubscribe(T* ptr)
	{
		using namespace SReflect;
		if (ptr == nullptr)
		{
			return false;
		}
		Any obj = make_any_ref(*ptr);
		for (auto& item:Slot)
		{
			auto& listeners = item.second;
			auto pos = std::find(listeners.begin(), listeners.end(), obj);
			if (pos != listeners.end())
			{
				listeners.erase(pos);
			}
		}
		return true;
	}
};