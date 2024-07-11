#pragma once

#include "split_base.h"

namespace Sari { namespace String {

    template<typename Type>
    class BlankDelimiter {
    public:

        void operator()(
            typename Type::const_iterator& start,
            typename Type::const_iterator& end,
            typename Type::const_iterator& next,
            typename Type::const_iterator& last
        ) {
            while (next != last && static_cast<unsigned>(*next) <= ' ') {
                next++;
            }

            start = next;

            while (next != last && static_cast<unsigned>(*next) > ' ') {
                next++;
            }

            end = next;
        }

    };

    template<typename Type>
    class SplitByBlank : public SplitBase<Type, BlankDelimiter<Type>> {
    public:

        SplitByBlank(const Type& str) :
            SplitBase<Type, BlankDelimiter<Type>>(str, BlankDelimiter<Type>{})
        {}

    };

}}
