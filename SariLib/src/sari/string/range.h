#pragma once

namespace Sari { namespace String {

	template<typename Type>
	class ConstRange {
	public:

		using ConstTypeIterator = typename Type::const_iterator;

		ConstRange(ConstTypeIterator first, ConstTypeIterator last) :
			first_(first),
			last_(last)
		{}

		ConstTypeIterator begin() const { return  first_; }
		ConstTypeIterator end() const { return last_; }

		Type str() const { return Type(first_, last_); }

	private:

		ConstTypeIterator first_;
		ConstTypeIterator last_;

	};

}}
