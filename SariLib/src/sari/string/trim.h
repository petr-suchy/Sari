#pragma once

#include "range.h"

namespace Sari { namespace String {

	template<typename T>
	static ConstRange<T> Trim(const ConstRange<T>& range)
	{
		ConstRange<T> result{range.end(), range.end()};

		// find the first non-white character in the input string from the beginning

		auto start = range.begin();

		while (start != range.end() && static_cast<unsigned>(*start) <= ' ') {
			++start;
		}

		if (start != range.end()) {

			// find the last white character from the end

			auto end = range.end();
			auto curr = end;

			do {
				end = curr;
				curr--;
			} while (curr != start && static_cast<unsigned>(*curr) <= ' ');

			// extract string
			result = ConstRange<T>(start, end);
		}

		return result;
	}

	template<typename T>
	static ConstRange<T> Trim(const T& str)
	{
		return Trim(ConstRange<T>{str.begin(), str.end()});
	}

}}
