#pragma once

#include <tuple>
#include <functional>

namespace Sari { namespace Utils {

    template <typename... Args>
    class FunctionParameters;

    // for a function with parameters
    template <typename T, typename... A>
    class FunctionParameters<T, A...> {
    public:
        using Type = T;
        using Next = FunctionParameters<A...>;
    };

    // for a function with no parameters
    template <>
    class FunctionParameters<> {
    public:
        using Type = void;
    };

    template <typename T>
    struct FunctionSignature;

    template <typename R, typename... P>
    struct FunctionSignature<std::function<R(P...)>> {
        using Type = std::function<R(P...)>;
        using Result = R;
        using Params = FunctionParameters<P...>;
        using ParamsTuple = std::tuple<P...>;
        static constexpr std::size_t NumOfParams = std::tuple_size<ParamsTuple>::value;
    };

}}