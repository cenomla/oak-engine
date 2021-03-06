#pragma once

#include <array>
#include <vector>
#include <deque>
#include <string>
#include <unordered_map>
#include <scoped_allocator>

#include "oak_alloc.h"

namespace oak {

	namespace detail {
		template<class T>
		using oalloc = std::scoped_allocator_adaptor<OakAllocator<T>>;
	}

	template<class T>
	using vector = std::vector<T, detail::oalloc<T>>;

	template<class T>
	using deque = std::deque<T, detail::oalloc<T>>;

	using string = std::basic_string<char, std::char_traits<char>, detail::oalloc<char>>;
}


namespace std {

	template<>
	struct hash<oak::string> {
		size_t operator()(const oak::string& value) const {
			return std::hash<std::string>{}(value.c_str());
		}
	};

}


namespace oak {

	template<class U, class T>
	using unordered_map = std::unordered_map<U, T, std::hash<U>, std::equal_to<U>, detail::oalloc<std::pair<const U, T>>>;
}
