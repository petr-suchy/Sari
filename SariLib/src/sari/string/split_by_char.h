#pragma once

#include <algorithm>
#include "split_base.h"

namespace Sari { namespace String {

    template<typename Type, typename Delimiter>
    class CharDelimiter {
    public:

        CharDelimiter(Delimiter delimiter) :
            delimiter_(delimiter)
        {}

        void operator()(
            typename Type::const_iterator& start,
            typename Type::const_iterator& end,
            typename Type::const_iterator& next,
            typename Type::const_iterator& last
        ) {
            start = next;
            next = std::find(next, last, delimiter_);
            end = next;

            if (next != last) {
                next++;
            }
        }

    private:

        Delimiter delimiter_;

    };

    template<typename Type, typename Delimiter>
    class SplitByChar : public SplitBase<Type, CharDelimiter<Type, Delimiter>> {
    public:

        SplitByChar(const Type& str, Delimiter delimiter) :
            SplitBase<Type, CharDelimiter<Type, Delimiter>>(str, CharDelimiter<Type, Delimiter>{delimiter})
        {}

        SplitByChar(const ConstRange<Type>& range, Delimiter delimiter) :
            SplitBase<Type, CharDelimiter<Type, Delimiter>>(range, CharDelimiter<Type, Delimiter>{delimiter})
        {}

    };

}}

