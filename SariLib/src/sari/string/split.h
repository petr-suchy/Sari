#pragma once

#include "split_by_char.h"
#include "split_by_string.h"
#include "split_by_blank.h"

namespace Sari { namespace String {

    template<typename T>
    static String::SplitByBlank<T> Tokenize(const T& s)
    {
        return  String::SplitByBlank<T>{s};
    }

	template<typename T>
	static String::SplitByString<T> Split(const T& s, const T& delimiter)
	{
		return  String::SplitByString<T>{s, delimiter};
	}

	template<typename T>
	static String::SplitByString<T> Split(const ConstRange<T>& range, const T& delimiter)
	{
		return  String::SplitByString<T>{range, delimiter};
	}

	template<typename T>
	static String::SplitByString<T> Split(const T& s, const typename T::value_type* delimiter)
	{
		return  String::SplitByString<T>{s, delimiter};
	}

	template<typename T>
	static String::SplitByString<T> Split(const ConstRange<T>& range, const typename T::value_type* delimiter)
	{
		return  String::SplitByString<T>{range, delimiter};
	}

	template<typename T>
	static String::SplitByChar<T, char> Split(const T& s, typename T::value_type delimiter)
	{
		return  String::SplitByChar<T, typename T::value_type>{s, delimiter};
	}

	template<typename T>
	static String::SplitByChar<T, char> Split(const ConstRange<T>& range, typename T::value_type delimiter)
	{
		return  String::SplitByChar<T, typename T::value_type>{range, delimiter};
	}

}}