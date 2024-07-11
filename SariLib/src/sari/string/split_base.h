#pragma once

#include "range.h"

namespace Sari { namespace String {

	template<typename Type, typename Delimiter>
	class SplitBase {
	public:

		using ConstTypeIterator = typename Type::const_iterator;

		class ConstIterator {
		public:

			ConstIterator(ConstTypeIterator first, ConstTypeIterator last, Delimiter delimiter) :
				start_(first),
				end_(first),
				next_(first),
				last_(last),
				delimiter_(delimiter)
			{
				operator++();
			}

			ConstRange<Type> rest() const
			{
				return ConstRange<Type>(next_, last_);
			}

			ConstRange<Type> curr() const
			{
				return ConstRange<Type>(start_, end_);
			}

			void operator++()
			{
				delimiter_(start_, end_, next_, last_);
			}

			void operator++(int)
			{
				operator++();
			}

			Type operator*() const
			{
				return curr().str();
			}

			bool operator==(const ConstIterator& rhs) const
			{
				return start_ == rhs.start_;
			}

			bool operator!=(const ConstIterator& rhs) const
			{
				return !operator==(rhs);
			}

		private:

			ConstTypeIterator start_;
			ConstTypeIterator end_;
			ConstTypeIterator next_;
			ConstTypeIterator last_;
			Delimiter delimiter_;

		};

		SplitBase(const Type& s, Delimiter delimiter) :
			first_(s.begin()),
			last_(s.end()),
			delimiter_(delimiter)
		{}

		SplitBase(const ConstRange<Type>& range, Delimiter delimiter) :
			first_(range.begin()),
			last_(range.end()),
			delimiter_(delimiter)
		{}

		ConstIterator begin() const { return ConstIterator(first_, last_, delimiter_); }
		ConstIterator end() const { return ConstIterator(last_, last_, delimiter_); }

	private:

		ConstTypeIterator first_;
		ConstTypeIterator last_;
		Delimiter delimiter_;

	};

}}