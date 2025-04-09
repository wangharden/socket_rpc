#pragma once
#include <span>
#include <utility>
#include <cassert>
#include <string_view>
#include <unordered_map>
#include <exception>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <iostream>
#include "xtraits.h"
#include "ReflectReg.h"

class ReflectException : public std::exception
{
public:
	ReflectException(const char* msg) {}
};


#define BUFFER_SIZE 256
inline char SR_buffer[BUFFER_SIZE] = { 0 };
#define _THROW_ANYWAY(msg, ...) \
{\
snprintf(SR_buffer, BUFFER_SIZE, msg, ##__VA_ARGS__);\
throw ReflectException{(const char*) SR_buffer};		\
}
#ifdef _DEBUG
#define _THROW_IF_DEBUG _THROW_ANYWAY
#define _DEBUG_MSG(...) printf(##__VA_ARGS__)
#else
#define _THROW_IF_DEBUG(...)
#define _DEBUG_MSG(...)
#endif

namespace SReflect
{
	using namespace std;
	struct Type;
	/*
	* Any 可以代表任意类型的值
	* Any 之间的拷贝构造，会保持行为的一致性。即原先一个Any类型是内存拷贝的，那么赋值后的新的Any类型的变量也是内存拷贝的。
	* 例如：
	*		Any a = 4;						//1:这样构造Any类型的变量默认是拷贝的。
	*		Any b = make_any_copy(3);		//2:通过make_any_copy获得的Any变量b是内存拷贝的。
	*		int i = 5;
	*		Any c = make_any_ref(i);		//3:通过make_any_ref获得的Any变量c是内存引用的。
	*		Any d = i;						//4:同1是内存拷贝的。
	*		Any e = d;						//5:同d一致，是内存拷贝的
	*		Any f = c;						//6:同c一致，是内存引用的
	* 两个Any类型之间的变量的赋值=操作，如m = n，会遵循以下规则：
	*						m 的flag类型
	*			  | NONE | COPY |  REF | CREF|
	*		| NONE|   I  |  II  |  II  |  II |
	*	n	| COPY|  II  |  III |  III |  IV |
	*  类型	| REF |  II  |  III |  III |  IV |
	*		| CREF|  II  |  III |  III |  IV |
	* 规则I:
	*		什么也不做。
	* 规则II:
	*		行为与 Any m = n 构造函数一致。即模仿n的数据。
	* 规则III:
	*		如果m与n的type一致，则进行赋值操作。或者，n的type可以构造m的type的对象，也可以进行赋值操作。否则，应用规则II。
	* 规则IV:
	*		如果m与n的type一致，则抛异常；否则，应用规则II。
	*/
	class Any
	{
		typedef std::function<SReflect::Any(std::span<SReflect::Any>)> Callable;
		friend struct Type;
	public:
		enum class AnyFlag : size_t
		{
			NONE = 0,
			COPY = 1,
			REF = 2,
			CREF = 3
		};
	private:
		Type* type;
		void* data;
		AnyFlag flag;
	protected:
		void clear();
		Any(Type* _type, void* _data, AnyFlag _flag) :type(_type), data(_data), flag(_flag) {}
		void copy_from(const Any& other);
	public:
		template<class T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Any>>>
		Any(T&& value);
		template<class T>
		static Any make_ref(T& value);
		Any() :type(nullptr), data(nullptr), flag(AnyFlag::NONE) {}
		Any(const Any& other);
		template<class T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Any>>>
		Any& operator=(T&& value);
		Any& operator=(const Any& other);
		bool operator==(const Any& other)const;
		bool operator!=(const Any& other)const;
		~Any() { clear(); }
		template<class T>
		T& cast();
		Type* get_type()const { return type; }

		//通过名字索引成员数据或者成员函数，如果存在同名的成员数据和成员函数，优先返回成员数据
		Any operator[](string_view field)const;
		//调用函数
		Any operator()(span<Any> args);
		template<class ...Args>
		Any operator()(Args&&... args);
		//如果成功设置，则返回true；否则，返回false
		template<class T>
		bool set_member_data(string_view fieldname, T&& value);

		//获取成员变量的值。默认以引用的方式返回该成员变量。如果该成员不存在或未注册，返回空Any。
		Any get_member_data(string_view fieldname, bool is_copy = false)const;

		//调用成员函数。会检查函数是否存在（注册），以及参数个数，类型等。
		Any call_member_function(string_view funcname, std::span<Any> args);
		inline Any call_member_function(string_view funcname, std::vector<Any>& args) { return this->call_member_function(funcname, std::span{ args }); }
		inline Any call_member_function(string_view funcname, const std::vector<Any>& args) { return this->call_member_function(funcname, std::span{ args }); }
		template<class ...Args>
		Any call_member_function(string_view funcname, Args&&... args);
		
		//获得成员函数。会返回一个可调用的Any对象。找不到则返回空Any
		Any get_member_function(string_view funcname)const;
			
		//创建与名称对应的类的对象。名称默认与类名一致。
		static Any create_class(string_view class_name);
	private:
		//Any间赋值操作的集合
		Any& assign_I(const Any& other);
		Any& assign_II(const Any& other);
		Any& assign_III(const Any& other);
		Any& assign_IV(const Any& other);
	};

	struct Type
	{
		using CopyConstruct = void* (*)(const void*);
		using MoveConstruct = void* (*)(void*);
		using Destroy = void (*)(void*);
		using CopyAssign = void (*)(void*, const void*);
		using Method = Any(*)(void*, span<Any>);

		//记录类成员数据的信息
		struct FieldInfo 
		{
			string_view field_name;
			Type* type;
			size_t offset;
			bool is_const;
			bool is_static;
			void* data = nullptr;		//只为静态数据使用
		};
		//记录类成员函数的信息
		struct MethodInfo
		{
			string_view method_name;
			Method method;
			Type* ret_type;
			std::vector<Type*> args_type;
			bool is_const;
			bool is_static;
		};

		string_view name;								//name of the type
		size_t size;									//size of the type
		bool ispod;										//is pod class type
		CopyConstruct copy;								//copy function
		MoveConstruct move;								//move function
		CopyAssign assign;								//assign function
		Destroy destroy;								//destroy function
		unordered_map<string_view, FieldInfo> fields;		//field info
		unordered_map<string_view, MethodInfo> methods;		//method info
		unordered_map<string_view, MethodInfo> static_methods;		//static method info
		vector<Type*> parents;								//parent types

		using ConvertMethod = void* (*)(void*);
		struct Convertables
		{
			Type* src;
			Type* dst;
			ConvertMethod method;
		};
		vector<Convertables> from;									//can convert to this type, where this type == dst
		vector<Convertables> to;									//can convert to dst type, where this type == src

		using ClassConstruct = Any(*)();
		static unordered_map<string_view, ClassConstruct> s_create_methods;
		static unordered_map<string_view, Type*> s_type_table;
		//判断是否可以从源类型转化为目标类型
		static bool castable(Type* dst, Type* src, Convertables* ct = nullptr);
		friend ostream& operator<<(ostream& os, const Type& t);

		//返回找到的method的调用方式。因继承结构复杂，找到的函数不一定是该类中的函数，可能是父类中的函数。
		//需要返回函数所在类相对于本类的地址偏移，以方便后续的调用。
		struct CallInfo
		{
			MethodInfo methodInfo;
			size_t offset = 0;
		};
		//返回找到的field的位置。因继承结构复杂，找到的字段不一定是本类中的字段，可能是父类中的字段。
		//返回查找字段在所在类相对于本类的地址偏移，以方便后续访问。
		struct FieldLoc
		{
			FieldInfo fieldInfo;
			size_t offset = 0;
		};
		//查找名为func的methodInfo, 递归查找。找到了返回true.
		bool find_method(string_view func, CallInfo& info)const;
		//查找名为field的fieldInfo, 递归查找。找到了返回true.
		bool find_field(string_view field, FieldLoc& info)const;
		//调用类中静态函数
		template<class ...Args>
		Any call_static_method(string_view funcname, Args&&... args) const {
			std::vector<Any> v1{ args... };
			std::span<Any> v{ v1 };
			return this->call_static_method(funcname, v);
		}
		Any call_static_method(string_view funcname, std::span<Any> args)const;
		//获得类中静态成员变量
		Any get_static_field(string_view fieldname, bool is_copy = false)const;
	};
	inline unordered_map<string_view, Type::ClassConstruct> Type::s_create_methods{};
	inline unordered_map<string_view, Type*> Type::s_type_table{};

	template<class T>
	struct ClassOperation
	{
		static void* copy(const void* obj) {
			return obj != nullptr ? (void*)(new T(*static_cast<const T*>(obj))) : nullptr;
		}
		static void destroy(void* obj) {
			if (obj != nullptr) 
				delete static_cast<T*>(obj);
		}
		static void assign(void* obj, const void* src) {
			*static_cast<T*>(obj) = *static_cast<const T*>(src);
		}
		static void* move(void* obj) {
			return obj != nullptr ? (void*)(new T(std::move(*static_cast<T*>(obj)))) : nullptr;
		}
	};


#define Common_Rigister_Snippets(C) \
	static SReflect::Type type;	\
	using U = std::decay<C>::type; \
	using CO = ClassOperation<C>;	\
	type.size = sizeof(U);	\
	type.ispod = std::is_trivial_v<U> && std::is_standard_layout_v<U>; \
	type.copy = CO::copy; \
	type.move = CO::move; \
	type.destroy = CO::destroy; \
	type.assign = CO::assign;

	template<class T>
	Type* type_of()
	{
		Common_Rigister_Snippets(T)
		type.name = typeid(T).name();
		return &type;
	}

	//用来存放全局函数和变量的类，仅仅是一个载体
	struct Global {};

	template<>
	inline Type* type_of<Global>() {
		Common_Rigister_Snippets(Global)
		type.name = "SReflect::Global";
		return &type;
	}

	template<class T>
	struct Member_Data_Traits : inner::member_data_traits<T> {};

	//静态成员变量
	template<class T>
	struct Member_Data_Traits<T*>
	{
		using data_type = T;
		static constexpr bool is_const = false;
		static constexpr bool is_static = true;
	};


	template<class T>
	struct Member_Fun_Traits : inner::member_fun_traits<T> {};

	//静态成员函数
	template<class R, class ...Args>
	struct Member_Fun_Traits<R(*)(Args...)>
	{
		using class_type = void;
		using return_type = R;
		using args_type = tuple<Args...>;
		static constexpr bool is_const = false;
		static constexpr bool is_static = true;
	};

	template<typename T>
	struct Member_Traits : inner::member_traits<T> {};

	//静态成员函数
	template<class R, class ...Args>
	struct Member_Traits<R(*)(Args...)> : Member_Fun_Traits<R(*)(Args...)>
	{
		static constexpr bool is_func = true;
		static constexpr bool is_data = false;
		
	};

	//静态成员变量
	template<class T>
	struct Member_Traits<T*> : Member_Data_Traits<T*>
	{
		static constexpr bool is_func = false;
		static constexpr bool is_data = true;
	};

	
	template<auto ptr>
	Type* member_data_type_gen()
	{
		using traits = Member_Data_Traits<decltype(ptr)>;
		using data_type = typename traits::data_type;
		return type_of<data_type>();
	}

	template<auto ptr>
	auto* member_fun_gen()
	{
		using traits = Member_Traits<decltype(ptr)>;
		using class_type = typename traits::class_type;
		using return_type = typename traits::return_type;
		using args_type = typename traits::args_type;

		return +[](void* obj, span<Any> args)->Any
		{
			class_type* self = static_cast<class_type*>(obj);
			auto _ptr = ptr;
			if constexpr (traits::is_static)
			{
				return[=]<size_t... Is>(index_sequence<Is...>)
				{
					if constexpr (std::is_void_v<return_type>) {
						(*_ptr)(args[Is].cast<tuple_element_t<Is, args_type>>()...);
						return Any{};
					}
					else
					{
						auto result = (*_ptr)(args[Is].cast<tuple_element_t<Is, args_type>>()...);
						return Any{ result };
					}
				}(make_index_sequence<tuple_size_v<args_type>>{});
			}
			else
			{
				return[=]<size_t... Is>(index_sequence<Is...>)
				{
					if constexpr (std::is_void_v<return_type>) {
						(self->*_ptr)(args[Is].cast<tuple_element_t<Is, args_type>>()...);
						return Any{};
					}
					else
					{
						auto result = (self->*_ptr)(args[Is].cast<tuple_element_t<Is, args_type>>()...);
						return Any{ result };
					}
				}(make_index_sequence<tuple_size_v<args_type>>{});
			}
		};
	}

	template<class MemberFunTraits>
	struct Member_Fun_Arg_Traits 
	{
		using return_type = typename MemberFunTraits::return_type;
		using args_type = typename MemberFunTraits::args_type;
		static Type* get_return_type() {
			if constexpr (std::is_void_v<return_type>)
				return nullptr;
			else
				return type_of<return_type>();
		}
		static std::vector<Type*> get_args_type()
		{
			return []<size_t... Is>(index_sequence<Is...>)
			{
				return std::vector<Type*>{type_of<tuple_element_t<Is, args_type>>()...};
			}(make_index_sequence<tuple_size_v<args_type>>{});
		}
	};
	 
	template<auto ptr>
	Type* Get_Member_Fun_Ret_Type()
	{
		using MemFunTraits = Member_Fun_Traits<decltype(ptr)>;
		return Member_Fun_Arg_Traits<MemFunTraits>::get_return_type();
	}

	template<auto ptr>
	std::vector<Type*> Get_Member_Fun_Args_Type()
	{
		using MemFunTraits = Member_Fun_Traits<decltype(ptr)>;
		return Member_Fun_Arg_Traits<MemFunTraits>::get_args_type();
	}

	template<auto ptr>
	Type::FieldInfo Get_Member_Field_Info(string_view field_name, size_t offset)
	{
		using traits = Member_Traits<decltype(ptr)>;
		Type::FieldInfo info;
		info.field_name = field_name;
		info.offset = offset;
		info.type = member_data_type_gen<ptr>();
		info.is_const = traits::is_const;
		info.is_static = traits::is_static;
		return info;
	}

	template<auto ptr>
	Type::FieldInfo Get_Field_Info(string_view field_name)
	{
		using traits = Member_Traits<decltype(ptr)>;
		Type::FieldInfo info;
		info.field_name = field_name;
		info.offset = 0;
		info.type = member_data_type_gen<ptr>();
		info.is_const = traits::is_const;
		info.is_static = traits::is_static;
		info.data = ptr;
		return info;
	}

	template<auto ptr>
	bool Reg_Member_Method_Info(Type* t, string_view method_name)
	{
		using traits = Member_Traits<decltype(ptr)>;
		Type::MethodInfo info;
		info.method_name = method_name;
		info.method = member_fun_gen<ptr>();
		info.ret_type = Get_Member_Fun_Ret_Type<ptr>();
		info.args_type = Get_Member_Fun_Args_Type<ptr>();
		info.is_const = traits::is_const;
		info.is_static = traits::is_static;
		if constexpr (traits::is_static)
		{
			t->methods.insert({ method_name, info });
			t->static_methods.insert({ method_name, info });
		}
		else
		{
			t->methods.insert({ method_name, info });
		}
		return true;
	}

	template<auto ptr>
	bool Reg_Global_Method_Info(string_view method_name)
	{
		using traits = Member_Traits<decltype(ptr)>;
		Type* global = type_of<Global>();
		Type::MethodInfo info;
		info.method_name = method_name;
		info.method = member_fun_gen<ptr>();
		info.ret_type = Get_Member_Fun_Ret_Type<ptr>();
		info.args_type = Get_Member_Fun_Args_Type<ptr>();
		info.is_const = traits::is_const;
		info.is_static = traits::is_static;
		global->methods.insert({ method_name, info });
		global->static_methods.insert({ method_name, info });
		return true;
	}



	template<typename ...Args>
	std::vector<Type*> Get_Type_List()
	{
		return { type_of<Args>()... };
	}

	

	template<typename Parent, typename Base>
	struct Is_Base_Traits;

	template<typename Parent>
	struct Is_Base_Traits<Parent, tuple<>> {
		static constexpr bool value = true;
	};

	template<typename Parent, typename Base>
	struct Is_Base_Traits<Parent, tuple<Base>> {
		static constexpr bool value = std::is_base_of_v<Base, Parent>;
	};

	template<typename Parent, typename ...Args>
	struct Is_Base_Traits<Parent, tuple<Args...>> {
		using base = tuple<Args...>;
		using head = std::tuple_element_t<0, base>;
		static constexpr bool value = std::is_base_of_v<head, Parent> &&
			Is_Base_Traits<Parent, inner::tail_t<tuple<Args...>>>::value;
	};

}

//注册类型
#define Reflect_Class(ClassName, ...) \
	template<> \
	inline SReflect::Type* SReflect::type_of<ClassName>() \
	{ \
	Common_Rigister_Snippets(ClassName)	\
	type.name = #ClassName;	\
	return &type;	\
	} \
	namespace SReflect{	\
	const bool __reg_class##ClassName = [] { \
	SReflect::Type::s_create_methods.insert({#ClassName, []()->SReflect::Any {return SReflect::Any{ClassName{}};}}); \
	SReflect::Type::s_type_table.insert({#ClassName, type_of<ClassName>()});	\
	Type* t = type_of<ClassName>(); \
	static_assert(Is_Base_Traits<ClassName, std::tuple<##__VA_ARGS__>>::value, "not base of" );	\
	t->parents = Get_Type_List<##__VA_ARGS__>(); \
	return true; }();	\
}
	
//注册类成员变量
#define Reflect_Class_Field(ClassName, FieldName) \
	namespace SReflect{	\
	const bool __reg_data##ClassName##_##FieldName = [] {type_of<ClassName>()->fields.insert({ #FieldName, \
	Get_Member_Field_Info<&ClassName::FieldName>(#FieldName, offsetof(ClassName, FieldName)) });	\
	return true; }();	\
}
//注册类静态成员变量
#define Reflect_Class_Static_Field(ClassName, FieldName) \
	namespace SReflect{	\
	const bool __reg_static_data##ClassName##_##FieldName = [] {type_of<ClassName>()->fields.insert({ #FieldName, \
	Get_Field_Info<&ClassName::FieldName>(#FieldName) });	\
	return true; }();	\
}

//注册类成员函数
#define Reflect_Class_Method(ClassName, MethodName) \
	namespace SReflect{	\
	const bool __reg_func##ClassName##_##MethodName = [] { \
	Reg_Member_Method_Info<&ClassName::MethodName>(type_of<ClassName>(), #MethodName);	\
	return true; }();	\
}
//注册全局函数
#define Reflect_Method(MethodName) \
	namespace SReflect{ \
	const bool __reg_func##MethodName = [] {	\
	Reg_Global_Method_Info<&MethodName>(#MethodName);	\
	return true;	\
	}();	\
}
//注册全局变量
#define Reflect_Field(FieldName) \
	namespace SReflect{	\
	const bool __reg_data##FieldName = []{	\
	type_of<Global>()->fields.insert({	\
	#FieldName,	\
	Get_Field_Info<&FieldName>(#FieldName)});	\
	return true;	\
	}();\
}


//获取类型
template<typename T>
SReflect::Type* GetType()
{
	return SReflect::type_of<T>();
}

//根据名称获取类型
inline SReflect::Type* GetType(std::string_view class_name)
{
	auto& map = SReflect::Type::s_type_table;
	auto it = map.find(class_name);
	return it != map.end() ? (*it).second : nullptr;
}

//获取全部的类型表
inline const std::unordered_map<std::string_view, SReflect::Type*> GetTypeTable()
{
	return SReflect::Type::s_type_table;
}

/*
* 通过make_any_copy获得的Any类型的变量是具有自己的内存，而非引用源变量。
*/
template<typename T>
SReflect::Any make_any_copy(T&& value)
{
	return SReflect::Any{ value };
}


/*
* 通过make_any_ref获得的Any类型的变量不具有自己的内存，引用源变量的内存。
* 当源变量的析构后，对获得的Any类型的变量操作将产生未定义行为。
*/
template<typename T>
SReflect::Any make_any_ref(T&& value)
{
	return SReflect::Any::make_ref(std::forward<T>(value));
}

/*
* 调用全局函数
*/
SReflect::Any call_global_method(std::string_view funcname, std::span<SReflect::Any> args);
template<class ...Args>
SReflect::Any call_global_method(std::string_view funcname, Args&& ...args) {
	std::vector<SReflect::Any> v1{ args... };
	std::span<SReflect::Any> v{ v1 };
	return call_global_method(funcname, v);
}
/*
* 获取全局变量, 默认以引用的方式返回
*/
SReflect::Any get_global_field(std::string_view fieldname, bool is_copy = false);

#include "ReflectStatic.inl"