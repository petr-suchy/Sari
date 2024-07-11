#pragma once

#include <algorithm>
#include "split_base.h"

namespace Sari { namespace String {

    template<typename Type>
    class StringDelimiter {
    public:

        StringDelimiter(const Type& delimiter) :
            delimiter_(delimiter)
        {}

        void operator()(
            typename Type::const_iterator& start,
            typename Type::const_iterator& end,
            typename Type::const_iterator& next,
            typename Type::const_iterator& last
        ) {
            start = next;
            next = std::search(next, last, delimiter_.begin(), delimiter_.end());
            end = next;

            if (next != last) {
                auto distance = std::distance(delimiter_.begin(), delimiter_.end());
                std::advance(next, distance);
            }
        }

    private:

        Type delimiter_;

    };

    template<typename Type>
    class SplitByString : public SplitBase<Type, StringDelimiter<Type>> {
    public:

        SplitByString(const Type& str, const Type& delimiter) :
            SplitBase<Type, StringDelimiter<Type>>(str, StringDelimiter<Type>{delimiter})
        {}

        SplitByString(const ConstRange<Type>& range, const Type& delimiter) :
            SplitBase<Type, StringDelimiter<Type>>(range, StringDelimiter<Type>{delimiter})
        {}

    };

}}
