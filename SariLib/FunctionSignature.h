#pragma once

#include <tuple>
#include <functional>

namespace Sari { namespace Utils {

    template <typename T>
    struct FunctionSignature;

    template <typename R, typename... P>
    struct FunctionSignature<std::function<R(P...)>> {
        using Type = std::function<R(P...)>;
        using Result = R;
        using Params = std::tuple<P...>;
        static constexpr std::size_t NumOfParams = std::tuple_size<Params>::value;
    };

}}