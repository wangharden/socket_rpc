#pragma once
#include <functional>
#include <tuple>
namespace inner
{
	using namespace std;
	template<typename T>
	struct invocable_traits;

	//普通函数
	template<typename R, typename ...Args>
	struct invocable_traits<R(*)(Args...)> {
		using return_type = R;
		using args_type = std::tuple<Args...>;
		static constexpr size_t args_count = sizeof...(Args);
	};

	//std::function
	template<typename R, typename ...Args>
	struct invocable_traits<std::function<R(Args...)>> : invocable_traits <R(*)(Args...)>{};

	//静态成员函数
	template<typename C, typename R, typename ...Args>
	struct invocable_traits<R(C::*)(Args...)> : invocable_traits<R(*)(Args...)> {

	};

	//lambda函数 待定



	/**************************成员函数萃取**************************/
	template<class T>
	struct member_fun_traits;

	template<class R, class C, class ...Args>
	struct member_fun_traits<R(C::*)(Args...)>
	{
		using return_type = R;
		using class_type = C;
		using args_type = tuple<Args...>;
		static constexpr bool is_const = false;
		static constexpr bool is_static = false;
	};

	template<class R, class C, class ...Args>
	struct member_fun_traits<R(C::*)(Args...)const>
	{
		using return_type = R;
		using class_type = C;
		using args_type = tuple<Args...>;
		static constexpr bool is_const = true;
		static constexpr bool is_static = false;
	};

	/**************************成员函数萃取**************************/

	/**************************成员数据萃取**************************/
	template<class T>
	struct member_data_traits;

	template<class C, class D>
	struct member_data_traits<D(C::*)>
	{
		using class_type = C;
		using data_type = D;
		static constexpr bool is_const = false;
		static constexpr bool is_static = false;
	};

	template<class C, class D>
	struct member_data_traits<const D(C::*)>
	{
		using class_type = C;
		using data_type = D;
		static constexpr bool is_const = true;
		static constexpr bool is_static = false;
	};

	/**************************成员数据萃取**************************/

	/**************************成员萃取**************************/
	template<class T>
	struct member_traits;

	template<class C, class D>
	struct member_traits<D(C::*)> : member_data_traits<D(C::*)> {
		static constexpr bool is_data = true;
		static constexpr bool is_func = false;
	};

	template<class C, class D>
	struct member_traits<const D(C::*)> : member_data_traits<const D(C::*)> {
		static constexpr bool is_data = true;
		static constexpr bool is_func = false;
	};

	template<class R, class C, class ...Args>
	struct member_traits<R(C::*)(Args...)> : member_fun_traits<R(C::*)(Args...)> {
		static constexpr bool is_data = false;
		static constexpr bool is_func = true;
	};

	template<class R, class C, class ...Args>
	struct member_traits<R(C::*)(Args...)const> : member_fun_traits<R(C::*)(Args...)const> {
		static constexpr bool is_data = false;
		static constexpr bool is_func = true;
	};

	/**************************成员萃取**************************/

	/**************************函数萃取**************************/

	template<class T>
	struct function_traits : member_traits<T> {};

	template<class R, class ...Args>
	struct function_traits<R(*)(Args...)> {
		using return_type = R;
		using args_type = tuple<Args...>;
	};

	template<class R, class ...Args>
	struct function_traits<std::function<R(Args...)>> : function_traits<R(*)(Args...)> {};

	/**************************函数萃取**************************/

	/**************************类型列表萃取**************************/
	template<typename T>
	struct head_type_traits;
	template<typename T, typename ...Args>
	struct head_type_traits<tuple<T, Args...>> {
		using type = T;
		using tail = tuple<Args...>;
	};
	template<typename T>
	using head_t = typename head_type_traits<T>::type;
	template<typename T>
	using tail_t = typename head_type_traits<T>::tail;
	/**************************类型列表萃取**************************/
}

template<typename Func>
using invocable_traits_r = typename inner::invocable_traits<Func>::return_type;

template<typename Func>
using invocable_traits_a = typename inner::invocable_traits<Func>::args_type;