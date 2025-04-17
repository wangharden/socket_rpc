#pragma once
#include <tuple>
#include <string>
namespace SReflect
{

	struct convertable_pairs
	{
		//from pair::first_type -> pair::second_type
		using pairs = std::tuple<
			std::pair<char*, std::string>,
			std::pair<const char*, std::string>,
			std::pair<std::string, std::string_view>
		>;


		constexpr static size_t value = std::tuple_size_v<pairs>;
	};
}

