#include "ReflectStatic.h"
inline std::ostream& operator<<(std::ostream& os, const SReflect::Type& t) {
	os << "class " << t.name;
	if (t.parents.size() > 0) {
		os << " : ";
		for (size_t i = 0; i < t.parents.size(); ++i) {
			auto pt = t.parents[i];
			os << pt->name;
			if (i + 1 != t.parents.size()) {
				os << ", ";
			}
		}
	}
	os << "\n{\n";
	os << "  " << "field:\n";
	for (auto& item : t.fields)
	{
		os << "\t" << item.second.type->name << " " << item.first << ";\n";
	}
	os << "  " << "method:\n";
	for (auto& item : t.methods)
	{
		os << "\t";
		if (item.second.is_static)
			os << "static ";
		if (item.second.ret_type == nullptr) {
			os << "void ";
		}
		else
			os << item.second.ret_type->name << " ";
		os << item.first << "(";
		size_t arg_count = item.second.args_type.size();
		for (size_t i = 0; i < arg_count; ++i)
		{
			auto& arg_type = item.second.args_type[i];
			os << arg_type->name;
			if (i + 1 != arg_count)
				os << arg_type->name << ", ";
		}
		os << ")";
		if (item.second.is_const)
			os << " const";
		os << ";\n";
	}
	os << "}\n";
	return os;
}

inline bool SReflect::Type::castable(Type* dst, Type* src, Type::Convertables* ct)
{
	if (dst == nullptr || src == nullptr)
		return false;
	for (auto& item : src->to) {
		if (dst == item.dst)
		{
			if (ct != nullptr)
				*ct = item;
			return true;
		}
	}
	return false;
}
inline bool SReflect::Type::find_method(string_view func, CallInfo& info) const
{
	auto it = this->methods.find(func);
	size_t child_size = 0;
	if (it != this->methods.end())
	{
		info.methodInfo = (*it).second;
		return true;
	}
	else if (parents.size() > 0)
	{
		for (auto it = this->parents.begin(); it != this->parents.end(); ++it) {
			if (*it != nullptr) {
				bool finded = (*it)->find_method(func, info);
				if (finded)
				{
					return true;
				}
				child_size += (*it)->size;
			}
		}
	}
	if (this->parents.size() == 0)
	{
		info.offset += this->size;
	}
	else
	{
		info.offset += this->size - child_size;
	}
	return false;
}

inline bool SReflect::Type::find_field(string_view field, FieldLoc& info) const
{
	auto it = this->fields.find(field);
	size_t child_size = 0;
	if (it != this->fields.end())
	{
		info.fieldInfo = (*it).second;
		return true;
	}
	else if (parents.size() > 0)
	{
		for (auto it = parents.begin(); it != parents.end(); ++it)
		{
			assert(*it != nullptr);
			bool finded = (*it)->find_field(field, info);
			if (finded) {
				return true;
			}
			child_size += (*it)->size;
		}
	}
	if (this->parents.size() == 0) {
		info.offset += this->size;
	}
	else {
		info.offset += this->size - child_size;
	}
	return false;
}

inline SReflect::Any SReflect::Type::call_static_method(string_view funcname, std::span<Any> args) const
{
	Type::CallInfo cf;
	bool finded = this->find_method(funcname, cf);
	if (!finded) {
		_DEBUG_MSG("Invalid call: function [%s] of class [%s] is not found.\n", funcname.data(), this->name.data());
		return Any{};
	}
	Type::MethodInfo& f = cf.methodInfo;
	if (args.size() != f.args_type.size()) {
		_THROW_IF_DEBUG("Arguments count error, cannot call function %s::%s. Input arguments' number is %zd, expected arguments's number is %zd.",
			this->name.data(), f.method_name.data(), args.size(), f.args_type.size());
		return Any{};
	}
	return f.method(nullptr, args);
}

inline SReflect::Any SReflect::Type::get_static_field(string_view fieldname, bool is_copy) const
{
	Type::FieldLoc loc;
	bool finded = this->find_field(fieldname, loc);
	if (!finded) {
		_DEBUG_MSG("Any::get_member_data don't have field of %s::%s.\n", this->name.data(), fieldname.data());
		return Any{};
	}
	auto& f = loc.fieldInfo;
	if (finded && f.is_static) {
		if (!is_copy)
		{
			Any::AnyFlag mark = f.is_const ? Any::AnyFlag::CREF : Any::AnyFlag::REF;
			return Any(f.type, f.data, mark);
		}
		else
		{
			Any tmp(f.type, f.data, Any::AnyFlag::COPY);
			Any ret = tmp;
			tmp.flag = Any::AnyFlag::REF;
			return ret;
		}
	}
	return Any{};
}

inline SReflect::Any SReflect::Any::operator[](string_view field) const
{
	Type::FieldLoc loc;
	bool finded = type->find_field(field, loc);
	if (finded)
		return this->get_member_data(field);
	return get_member_function(field);
}

inline SReflect::Any SReflect::Any::operator()(span<Any> args)
{
	if (type == type_of<Callable>()) {
		return this->cast<Callable>()(args);
	}
	_THROW_IF_DEBUG("Invalid calling");
	return {};
}

inline SReflect::Any& SReflect::Any::assign_I(const Any& other)
{
	return *this;
}

inline SReflect::Any& SReflect::Any::assign_II(const Any& other)
{
	//与other保持一致的flag以及数据
	clear();
	copy_from(other);
	return *this;
}

inline SReflect::Any& SReflect::Any::assign_III(const Any& other)
{
	if (type == other.type) {
		type->assign(data, other.data);
		return *this;
	}
	//如果大小一致并且都是pod类型，则直接强转赋值
	bool sizematch = type->ispod && other.type->ispod && type->size == other.type->size;
	if (sizematch) {
		type->assign(data, other.data);
		return *this;
	}
	//如果类型可构造转化
	Type::Convertables out;
	if (Type::castable(type, other.type, &out)) {
		clear();
		void* tmp = out.method((void*)other.data);
		type->assign(data, tmp);
		type->destroy(tmp);
		return *this;
	}
	return assign_II(other);
}

inline SReflect::Any& SReflect::Any::assign_IV(const Any& other)
{
	if (type == other.type) {
		_THROW_ANYWAY("cannot assign value on const value.");
		return *this;
	}
	return assign_II(other);
}

inline SReflect::Any call_global_method(std::string_view funcname, std::span<SReflect::Any> args)
{
	using namespace SReflect;
	Type* global = type_of<Global>();
	return global->call_static_method(funcname, args);
}

inline SReflect::Any get_global_field(std::string_view fieldname, bool is_copy)
{
	using namespace SReflect;
	Type* global = type_of<Global>();
	return global->get_static_field(fieldname, is_copy);
}

template<class T>
inline bool SReflect::Any::set_member_data(string_view fieldname, T&& value)
{
	using U = std::decay_t<T>;
	if (type == nullptr) return false;
	Any f = this->get_member_data(fieldname);
	if (f == Any{}) {
		_DEBUG_MSG("Any::set_member_data failed, because class: [%s] field: [%s] isn't registered or type mismatch\n", type->name.data(), fieldname.data());
		return false;
	}
	f = Any{ value };
	return true;
}
template<class ...Args>
inline SReflect::Any SReflect::Any::call_member_function(string_view funcname, Args&& ...args)
{
	std::vector<Any> v{ args... };
	std::span<Any> v1 = v;
	return this->call_member_function(funcname, v1);
}
inline void SReflect::Any::clear()
{
	if (flag == AnyFlag::COPY) {
		if (data && type)
		{
			type->destroy(data);
			type = nullptr;
			data = nullptr;
			flag = AnyFlag::NONE;
		}
	}
}
inline void SReflect::Any::copy_from(const Any& other)
{
	type = other.type;
	if (other.flag == AnyFlag::COPY)
	{
		data = type->copy(other.data);
	}
	else if (other.flag == AnyFlag::REF || other.flag == AnyFlag::CREF)
	{
		data = other.data;
	}
	else {
		data = nullptr;
	}
	flag = other.flag;
}
inline SReflect::Any::Any(const Any& other)
{
	copy_from(other);
}
inline SReflect::Any& SReflect::Any::operator=(const Any& other)
{
	if (this == &other) return *this;
	if (flag == AnyFlag::NONE && other.flag == AnyFlag::NONE) {
		//op I
		return assign_I(other);
	}
	if (flag == AnyFlag::NONE || other.flag == AnyFlag::NONE) {
		//op II
		return assign_II(other);
	}
	if (flag == AnyFlag::CREF) {
		//op IV
		return assign_IV(other);
	}
	//op III
	return assign_III(other);
}
inline bool SReflect::Any::operator==(const Any& other) const
{
	return (type == other.type) && (data == other.data) && (flag == other.flag);
}
inline bool SReflect::Any::operator!=(const Any& other) const
{
	return !(*this == other);
}
inline SReflect::Any SReflect::Any::get_member_data(string_view fieldname, bool is_copy) const
{
	if (type == nullptr || data == nullptr) return false;
	Type::FieldLoc loc;
	bool finded = type->find_field(fieldname, loc);
	if (!finded) {
		_DEBUG_MSG("Any::get_member_data don't have field of %s::%s.\n", type->name.data(), fieldname.data());
		return Any{};
	}
	auto& f = loc.fieldInfo;
	if (finded) {
		if (!is_copy)
		{
			AnyFlag mark = f.is_const ? AnyFlag::CREF : AnyFlag::REF;
			mark = flag == AnyFlag::CREF ? flag : mark;
			return Any(f.type, (char*)data + loc.offset + f.offset, mark);
		}
		else
		{
			Any tmp(f.type, (char*)data + loc.offset + f.offset, AnyFlag::COPY);
			Any ret = tmp;
			tmp.flag = AnyFlag::REF;
			return ret;
		}
	}
	return Any{};
}
inline SReflect::Any SReflect::Any::call_member_function(string_view funcname, std::span<Any> args)
{
	Type::CallInfo cf;
	bool finded = type->find_method(funcname, cf);
	if (!finded) {
		_DEBUG_MSG("Invalid call: function [%s] of class [%s] is not found.\n", funcname.data(), type->name.data());
		return Any{};
	}
	Type::MethodInfo& f = cf.methodInfo;
	if (args.size() != f.args_type.size()) {
		_THROW_IF_DEBUG("Arguments count error, cannot call function %s::%s. Input arguments' number is %zd, expected arguments's number is %zd.",
			type->name.data(), f.method_name.data(), args.size(), f.args_type.size());
		return Any{};
	}
	if (flag != AnyFlag::NONE)
	{
		if (flag == AnyFlag::CREF && !f.is_const)
		{
			_THROW_IF_DEBUG("cannot call non-const function %s::%s on const value.", type->name.data(), f.method_name.data());
			return Any{};
		}
		return f.method((char*)data + cf.offset, args);
	}
	_DEBUG_MSG("Invalid call: function [%s] cannot executed on an invalid value of flag AnyFlag::NONE.\n", funcname.data());
	return Any{};
}
inline SReflect::Any SReflect::Any::get_member_function(string_view field) const
{
	Type::CallInfo cf;
	bool finded = type->find_method(field, cf);
	if (!finded) {
		_DEBUG_MSG("Invalid call: function [%s] of class [%s] is not found.\n", field.data(), type->name.data());
		return Any{};
	}
	Type::MethodInfo& f = cf.methodInfo;
	if (flag != AnyFlag::NONE)
	{
		Callable g = [=,this](span<Any> args)->Any
		{
			if (flag == AnyFlag::CREF && !f.is_const)
			{
				_THROW_IF_DEBUG("cannot call non-const function %s::%s on const value.", type->name.data(), f.method_name.data());
				return Any{};
			}
			return f.method((char*)data + cf.offset, args);
		};
		return g;
	}
	_DEBUG_MSG("Invalid index: field [%s] is not exist\n", field.data());
	return Any{};
}
inline SReflect::Any SReflect::Any::create_class(string_view class_name)
{
	auto it = Type::s_create_methods.find(class_name);
	if (it != Type::s_create_methods.end()) {
		return (*it).second();
	}
	_DEBUG_MSG("Any::create_class failed. Ensure class name %s is registed.\n", class_name.data());
	return Any{};
}
template<class T, typename>
inline SReflect::Any::Any(T&& value)
{
	T& tmp = value;
	type = type_of<std::decay_t<T>>();
	data = new std::decay_t<T>(std::forward<T>(value));
	flag = AnyFlag::COPY;
}

template<class T>
inline SReflect::Any SReflect::Any::make_ref(T& value)
{
	Any ret;
	ret.data = const_cast<std::decay_t<T>*>(&value);
	ret.type = type_of<std::decay_t<T>>();
	ret.flag = std::is_const_v<T> ? AnyFlag::CREF : AnyFlag::REF;
	return ret;
}

template<class T, typename>
inline SReflect::Any& SReflect::Any::operator=(T&& value)
{
	using U = std::decay_t<T>;
	Type* InType = type_of<U>();
	if (type == InType && type != nullptr) {
		type->assign(this->data, &value);
		return *this;
	}
	Type::Convertables out;
	if (type == nullptr || !Type::castable(type, InType, &out)) {
		clear();
		type = InType;
		data = new U(std::forward<T>(value));
		flag = AnyFlag::COPY;
		return *this;
	}
	Any tmp(value);
	*this = tmp;
	return *this;
}

template<class T>
inline T& SReflect::Any::cast()
{
	auto* targetType = type_of<T>();
	bool typemismatch = type != targetType;
	bool sizematch = type->ispod && targetType->ispod && type->size == targetType->size;
	if (typemismatch && !sizematch) {
		_THROW_ANYWAY( "type mismath" );
	}
	return *static_cast<T*>(data);
}

template<class ...Args>
inline SReflect::Any SReflect::Any::operator()(Args && ...args)
{
	std::vector<Any> v1{ args... };
	std::span<Any> v = v1;
	return (*this)(v);
}
namespace SReflect
{
	template<size_t N>
	struct convertable_pair_at : convertable_pairs
	{
		using type = std::tuple_element_t<N, pairs>;
		using src = typename type::first_type;
		using dst = typename type::second_type;
		static_assert(std::is_constructible_v<dst, src>, "cannot construct from source to destination");
		static_assert(!std::is_const_v<dst>, "convertable_pairs destination type should be non const.");
	};

	template<typename Pair>
	bool reg_convertable_types()
	{
		using src = typename Pair::first_type;
		using dst = typename Pair::second_type;
		Type* st = type_of<src>();
		Type* dt = type_of<dst>();
		Type::Convertables ci;
		ci.method = [](void* p) -> void*
		{
			return new dst{ *(src*)p };
		};
		ci.src = st;
		ci.dst = dt;
		dt->from.push_back(ci);
		st->to.push_back(ci);
		return true;
	}

	inline void reg_all()
	{
		return[]<size_t... Is>(index_sequence<Is...>)
		{
			std::vector<bool> a{ reg_convertable_types<typename convertable_pair_at<Is>::type>()... };
		}(make_index_sequence<convertable_pairs::value>{});
	}

	const bool __reg_convertable_pairs = []()
	{
		reg_all();
		return true;
	}();
}